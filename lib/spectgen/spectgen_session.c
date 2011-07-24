/*******************************************************************************
    This file is part of spectro
    Copyright (C) 2011  Amithash Prasad <amithash@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <fftw3.h>
#include "queue.h"
#include "spectgen.h"
#include "scale.h"

#include "spectgen_session.h"

#define SPECTGEN_SESSION_TABLE_SIZE 32

static spectgen_session_t *session_hash_table[SPECTGEN_SESSION_TABLE_SIZE] = {NULL};
static pthread_mutex_t     session_hash_table_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t     planner_lock = PTHREAD_MUTEX_INITIALIZER;

__attribute__((constructor))
static void spectgen_session_init(void)
{
	int i;
	for(i = 0; i < SPECTGEN_SESSION_TABLE_SIZE; i++) {
		session_hash_table[i] = NULL;
	}
}

static void *spectgen_session_thread(void *data)
{
	spectgen_session_t *session = (spectgen_session_t *)data;
	while(session->active) {
		SIGNAL_WAIT(&session->start_signal);
		if(session->active == 0)
		      break;

		/* To be safe... */
		if(session->user_cb && session->user_handle)
			session->user_cb((void *)session->user_handle);
		else
			printf("WARING: Session improperly setup.\n");

		SIGNAL_SET(&session->finished_signal);
	}
	pthread_exit(NULL);
}

static inline int session_index(unsigned int w)
{
	float val = (float)w;
	if(w == 0)
	      return 0;
	val = log(val) / log(2);
	w = (int) val;
	return w % SPECTGEN_SESSION_TABLE_SIZE;
}

static inline spectgen_session_t *spectgen_session_create(unsigned int window_size,
							 scale_t scale,
							 spect_method_t method,
							 unsigned int nbands)
{
	spectgen_session_t *session;
	int i;
	unsigned int num_bands = nbands;

	if(method == CEPSTOGRAM)
		num_bands = (nbands - 1) * 2;

	if(num_bands > (1 + (window_size / 2)))
	      return NULL;

	session = (spectgen_session_t *)malloc(sizeof(spectgen_session_t));
	if(!session)
	      return NULL;

	session->window_size = window_size;
	session->numfreqs = (window_size / 2) + 1;
	session->nbands = nbands;
	session->scale = scale;
	session->method = method;

	session->window = (float *)malloc(sizeof(float) * window_size);
	if(!session->window)
	      goto window_creation_failed;
	for(i = 0; i < window_size; i++) {
		session->window[i] = 0.5 * (1 - cos(2 * M_PI * (float)i / (float)(window_size - 1)));
	}


	session->scale_table = generate_scale_table(num_bands, scale);
	if(!session->scale_table)
	      goto scale_generation_failed;

	session->norm_table = generate_scale_norm_table(session->scale_table, num_bands);
	if(!session->norm_table)
	      goto norm_generation_failed;

	session->fft_in_pre = (float *)fftwf_malloc(sizeof(float) * window_size);
	if(!session->fft_in_pre)
	      goto fft_in_pre_failed;

	session->fft_out_pre = (fftwf_complex *)
	    fftwf_malloc(sizeof(fftwf_complex) * session->numfreqs);
	if(!session->fft_out_pre)
	      goto fft_out_pre_failed;

	/* Even though we are thread safe, the planner is not. so 
	 * serialize the calls to fftwf plan create */
	pthread_mutex_lock(&planner_lock);
	session->plan_pre  = fftwf_plan_dft_r2c_1d(window_size, session->fft_in_pre,
				session->fft_out_pre, FFTW_MEASURE | FFTW_DESTROY_INPUT);
	pthread_mutex_unlock(&planner_lock);
	if(!session->plan_pre)
	      goto plan_pre_creation_failed;

	session->fft_in_post = NULL;
	session->fft_out_post = NULL;
	session->plan_post = 0;

	if(method == CEPSTOGRAM) {
		unsigned int len_out_post = nbands;
		unsigned int len_in_post = num_bands;
		session->fft_in_post = (float *)fftwf_malloc(sizeof(float) * len_in_post);
		if(!session->fft_in_post)
			goto fft_in_post_failed;

		session->fft_out_post = (fftwf_complex *)
		    fftwf_malloc(sizeof(fftwf_complex) * len_out_post);
		if(!session->fft_out_post)
			goto fft_out_post_failed;

		pthread_mutex_lock(&planner_lock);
		session->plan_post = fftwf_plan_dft_r2c_1d(len_in_post, session->fft_in_post,
				session->fft_out_post, FFTW_MEASURE | FFTW_DESTROY_INPUT);
		pthread_mutex_unlock(&planner_lock);
		if(!session->plan_post)
			goto plan_post_creation_failed;
	}

	/* The reason we create is for it to be taken! */
	session->busy = 1;
	session->next = NULL;
	pthread_mutex_init(&session->lock, NULL);
	SIGNAL_INIT(&session->start_signal);
	SIGNAL_INIT(&session->finished_signal);
	session->active = 1;

	if(decoder_init(&session->d_handle))
	      goto decoder_init_failed;

	/* The thread will wait on the start signal */
	pthread_create(&session->thread, 0, spectgen_session_thread, session);
	if(!session->thread)
	      goto thread_creation_failed;

	return session;

thread_creation_failed:
	decoder_exit(session->d_handle);
decoder_init_failed:
	pthread_mutex_destroy(&session->lock);
	SIGNAL_DEINIT(&session->start_signal);
	SIGNAL_DEINIT(&session->finished_signal);
	if(session->plan_post)
		fftwf_destroy_plan(session->plan_post);
plan_post_creation_failed:
	if(session->fft_out_post)
	      fftwf_free(session->fft_out_post);
fft_out_post_failed:
	if(session->fft_in_post)
	      fftwf_free(session->fft_in_post);
fft_in_post_failed:
	fftwf_destroy_plan(session->plan_pre);
plan_pre_creation_failed:
	fftwf_free(session->fft_out_pre);
fft_out_pre_failed:
	fftwf_free(session->fft_in_pre);
fft_in_pre_failed:
	free(session->norm_table);
norm_generation_failed:
	free(session->scale_table);
scale_generation_failed:
	free(session->window);
window_creation_failed:
	free(session);
	return NULL;
}

spectgen_session_t *spectgen_session_get(unsigned int window_size,
					 scale_t scale,
					 unsigned int nbands,
					 spect_method_t method,
					 void *user_handle,
					 user_session_cb_t user_cb
			)
{
	spectgen_session_t *session = NULL;
	int ind = session_index(window_size);
	spectgen_session_t *i = NULL;
	pthread_mutex_lock(&session_hash_table_lock);
	i = session_hash_table[ind];
	while(i) {
		if(i->window_size == window_size &&
	           i->nbands == nbands &&
		   i->scale == scale &&
		   i->method == method) {
			pthread_mutex_lock(&i->lock);
			if(i->busy == 0) {
				i->busy = 1;
				session = i;
				pthread_mutex_unlock(&i->lock);
				break;
			}
			pthread_mutex_unlock(&i->lock);
		}
		i = i->next;
	}
	if(!session) {
		session = spectgen_session_create(window_size, scale, method, nbands);
		if(!session)
		      goto malloc_fail;
		session->next = session_hash_table[ind];
		session_hash_table[ind] = session;
	}
	session->user_cb = user_cb;
	session->user_handle = user_handle;
malloc_fail:
	pthread_mutex_unlock(&session_hash_table_lock);
	return session;
}

void spectgen_session_start(spectgen_session_t *session)
{
	if(!session)
	      return;
	if(session->busy == 0 || session->active == 0)
	      return;
	SIGNAL_SET(&session->start_signal);
}

void spectgen_session_put(spectgen_session_t *session)
{
	if(!session)
	      return;

	SIGNAL_WAIT(&session->finished_signal);

	pthread_mutex_lock(&session->lock);
	session->busy = 0;
	session->user_handle = NULL;
	session->user_cb = NULL;
	pthread_mutex_unlock(&session->lock);
}

void static inline spectgen_session_destroy(spectgen_session_t *session)
{
	void *par;
	if(!session)
	      return;

	session->active = 0;
	SIGNAL_SET(&session->start_signal);
	pthread_join(session->thread, &par);

	if(session->fft_in_pre)
	      fftwf_free(session->fft_in_pre);
	if(session->fft_out_pre)
	      fftwf_free(session->fft_out_pre);
	if(session->fft_in_post)
	      fftwf_free(session->fft_in_post);
	if(session->fft_out_post)
	      fftwf_free(session->fft_out_post);
	if(session->plan_pre) {
		pthread_mutex_lock(&planner_lock);
		fftwf_destroy_plan(session->plan_pre);
		pthread_mutex_unlock(&planner_lock);
	}
	if(session->plan_post) {
		pthread_mutex_lock(&planner_lock);
		fftwf_destroy_plan(session->plan_post);
		pthread_mutex_unlock(&planner_lock);
	}
	if(session->window)
	      free(session->window);
	if(session->scale_table)
	      free(session->scale_table);
	if(session->norm_table)
	      free(session->norm_table);

	pthread_mutex_destroy(&session->lock);
	SIGNAL_DEINIT(&session->start_signal);
	SIGNAL_DEINIT(&session->finished_signal);
	decoder_exit(session->d_handle);
	free(session);
}

__attribute__((destructor))
static void spectgen_session_exit(void)
{
	int i;
	spectgen_session_t *session;
	pthread_mutex_lock(&session_hash_table_lock);
	for(i = 0; i < SPECTGEN_SESSION_TABLE_SIZE; i++) {
		session = session_hash_table[i];
		while(session) {
			spectgen_session_t *to_free = session;
			session = session->next;
			spectgen_session_destroy(to_free);
		}
		session_hash_table[i] = NULL;
	}
	pthread_mutex_unlock(&session_hash_table_lock);
}

