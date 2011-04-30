#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <fftw3.h>
#include "queue.h"
#include "spectgen.h"
#include "spect-config.h"

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
		pthread_mutex_lock(&session->lock);
		pthread_cond_wait(&session->cond, &session->lock);
		pthread_mutex_unlock(&session->lock);
		if(session->active == 0)
		      break;
		/* To be safe... */
		if(session->user_cb && session->user_handle)
			session->user_cb((void *)session->user_handle);
		else
			printf("WARING: Session improperly setup.\n");

		pthread_mutex_lock(&session->wait_lock);
		session->working = 0;
		pthread_cond_broadcast(&session->wait_cond);
		pthread_mutex_unlock(&session->wait_lock);
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

static inline spectgen_session_t *spectgen_session_create(unsigned int window_size)
{
	spectgen_session_t *session;
	session = (spectgen_session_t *)malloc(sizeof(spectgen_session_t));
	if(!session)
	      return NULL;

	session->window_size = window_size;
	session->numfreqs = (window_size / 2) + 1;

	session->fft_in = (float *)fftwf_malloc(sizeof(float) * window_size);
	if(!session->fft_in)
	      goto fft_in_failed;

	session->fft_out = (fftwf_complex *)
	    fftwf_malloc(sizeof(fftwf_complex) * session->numfreqs);
	if(!session->fft_out)
	      goto fft_out_failed;

	/* Even though we are thread safe, the planner is not. so 
	 * serialize the calls to fftwf plan create */
	pthread_mutex_lock(&planner_lock);
	session->plan = fftwf_plan_dft_r2c_1d(window_size, session->fft_in,
				session->fft_out, FFTW_MEASURE | FFTW_DESTROY_INPUT);
	pthread_mutex_unlock(&planner_lock);
	if(!session->plan)
	      goto plan_creation_failed;

	/* The reason we create is for it to be taken! */
	session->busy = 1;
	session->next = NULL;
	pthread_mutex_init(&session->lock, NULL);
	pthread_cond_init(&session->cond, NULL);
	pthread_mutex_init(&session->wait_lock, NULL);
	pthread_cond_init(&session->wait_cond, NULL);
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
	pthread_mutex_destroy(&session->wait_lock);
	pthread_cond_destroy(&session->wait_cond);
	pthread_mutex_destroy(&session->lock);
	pthread_cond_destroy(&session->cond);
plan_creation_failed:
	fftwf_free(session->fft_out);
fft_out_failed:
	fftwf_free(session->fft_in);
fft_in_failed:
	free(session);
	return NULL;
}

spectgen_session_t *spectgen_session_get(unsigned int window_size,
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
		if(i->window_size == window_size) {
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
		session = spectgen_session_create(window_size);
		if(!session)
		      goto malloc_fail;
		session->next = session_hash_table[ind];
		session_hash_table[ind] = session;
	}
	session->user_cb = user_cb;
	session->user_handle = user_handle;
	session->working = 0;

malloc_fail:
	pthread_mutex_unlock(&session_hash_table_lock);
	return session;
}

void spectgen_session_start(spectgen_session_t *session)
{
	if(!session)
	      return;
	pthread_mutex_lock(&session->lock);
	if(session->busy == 0 || session->active == 0) {
		pthread_mutex_unlock(&session->lock);
		return;
	}
	session->working = 1;
	pthread_cond_broadcast(&session->cond);
	pthread_mutex_unlock(&session->lock);
}

static void spectgen_session_wait(spectgen_session_t *session)
{
	while(session->working == 1) {
		pthread_mutex_lock(&session->wait_lock);
		pthread_cond_wait(&session->wait_cond, &session->wait_lock);
		pthread_mutex_unlock(&session->wait_lock);
	}
}

void spectgen_session_put(spectgen_session_t *session)
{
	if(!session)
	      return;

	spectgen_session_wait(session);

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
	spectgen_session_wait(session);
	pthread_mutex_lock(&session->lock);
	pthread_cond_broadcast(&session->cond);
	pthread_mutex_unlock(&session->lock);
	pthread_join(session->thread, &par);

	if(session->fft_in)
	      fftwf_free(session->fft_in);
	if(session->fft_out)
	      fftwf_free(session->fft_out);
	if(session->plan) {
		pthread_mutex_lock(&planner_lock);
		fftwf_destroy_plan(session->plan);
		pthread_mutex_unlock(&planner_lock);
	}
	pthread_mutex_destroy(&session->lock);
	pthread_cond_destroy(&session->cond);
	pthread_mutex_destroy(&session->wait_lock);
	pthread_cond_destroy(&session->wait_cond);
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

