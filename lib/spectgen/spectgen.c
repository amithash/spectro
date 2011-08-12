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
#include "spectgen_session.h"

#include "decoder.h"

#define SCALING_FACTOR (8.0)

#define SPECTRUM_BAND_FREQ(band, size, rate) \
      (unsigned int)(((float)(band))*((float)(rate))/((float)(size)))

struct spectgen_struct {
	char               filename[256];
	q_type             queue;
	unsigned int       *barkband_table;
	int                barkband_table_inited;
	spectgen_session_t *session;
	float              *leftover;
	unsigned int       step_size;
	double             average_frate;
	double             total_samples;
};

static void spectgen_worker(void *_handle);

static void setup_barkband_table(struct spectgen_struct *handle, unsigned int sampling_rate)
{
	int i;
	unsigned int barkband = 0;
	unsigned int nbands = handle->session->nbands;
	if(handle->session->method == CEPSTOGRAM)
	      nbands = 2 * (nbands - 1);

	memset(handle->barkband_table, 0, sizeof(unsigned int) * handle->session->numfreqs);
	for(i = 0; i < handle->session->numfreqs; i++) {
		if(barkband < (nbands - 1) &&
			SPECTRUM_BAND_FREQ(i, handle->session->window_size, sampling_rate) >= 
			handle->session->scale_table[barkband])
		      barkband++;
		handle->barkband_table[i] = barkband;
	}
	handle->barkband_table_inited = 1;
}

static int do_band(struct spectgen_struct *handle, float *buf)
{
	int i;
	float *band;
	float *out;
	int out_num = handle->session->nbands;
	float * norm_table = handle->session->norm_table;
	float window_size = (float)handle->session->window_size;
	unsigned int *barkband_table = handle->barkband_table;

	if(!handle->barkband_table_inited)
	      return -1;

	out = band = (float *)calloc(out_num, sizeof(float));
	if(!out)
	      return -1;

	if(handle->session->method == CEPSTOGRAM) {
		band = handle->session->fft_in_post;
		out_num = (out_num - 1) * 2;
		memset(band, 0, sizeof(float) * out_num);
	}

	for(i = 1; i < handle->session->numfreqs; i++) {
		float real = buf[2 * i];
		float imag = buf[2 * i + 1];
		band[barkband_table[i]] += ((real * real) + (imag * imag));
	}

	for(i = 0; i < out_num; i++) {
		band[i] = SCALING_FACTOR * norm_table[i] * sqrt(band[i]) / window_size;
	}

	if(handle->session->method == CEPSTOGRAM) {
		for(i = 0; i < out_num; i++) {
			if(band[i] != 0)
				band[i] = log(band[i]);
		}
		fftwf_execute(handle->session->plan_post);
		buf = (float *)handle->session->fft_out_post;
		for(i = 0; i < handle->session->nbands; i++) {
			float real = buf[2 * i];
			float imag = buf[2 * i + 1];
			out[i] = SCALING_FACTOR * sqrt((real * real) + (imag * imag)) / (float)(out_num);
		}
	}

	q_put(&handle->queue, out);

	return 0;
}

static int process_window(struct spectgen_struct *handle, float *in)
{
	unsigned int window_size = handle->session->window_size;
	float *fft_in = handle->session->fft_in_pre;
	float *window = handle->session->window;
	int i;

	memcpy(fft_in, in, sizeof(float) * window_size);
	for(i = 0; i < window_size; i++)
		fft_in[i] *= window[i];
	fftwf_execute(handle->session->plan_pre);
	return do_band(handle, (float *)handle->session->fft_out_pre);
}

int spectgen_open(spectgen_handle *_handle, char *fname, unsigned int window_size, unsigned int step_size, scale_t scale, spect_method_t method, unsigned int *_nbands)
{
	struct spectgen_struct *handle;
	int nbands;

	*_handle = NULL;

	if(!_nbands)
	      return -1;

	if(!window_size || !step_size)
	      return -1;
	if(step_size > window_size)
	      return -1;

	nbands = *_nbands;
	if(nbands == 0)
		*_nbands = nbands = (window_size / 2) + 1;

	handle = (struct spectgen_struct *)
	    malloc(sizeof(struct spectgen_struct));
	if(!handle)
	      return -1;

	handle->session = spectgen_session_get(window_size, scale, nbands, method, handle, spectgen_worker);
	if(!handle->session)
	      goto session_creation_failed;

	handle->step_size = step_size;
	handle->average_frate = 0;
	handle->total_samples = 0;

	strncpy(handle->filename, fname, 256);
	handle->filename[255] = '\0';

	if(q_init(&handle->queue))
	      goto q_init_failed;

	handle->leftover = (float *)malloc(sizeof(float) * handle->session->window_size);
	if(!handle->leftover)
	      goto leftover_failed;


	handle->barkband_table = (unsigned int *)malloc(sizeof(unsigned int) * 
				handle->session->numfreqs);
	if(!handle->barkband_table)
	      goto barkband_table_failed;

	if(decoder_open(handle->session->d_handle, handle->filename))
		goto open_failed;

	handle->barkband_table_inited = 0;
	*_handle = handle;
	return 0;

open_failed:
	printf("Open failed\n");
	free(handle->barkband_table);
barkband_table_failed:
	printf("barkband table failed\n");
	free(handle->leftover);
leftover_failed:
	printf("leftover failed\n");
	q_destroy(&handle->queue);
q_init_failed:
	printf("qinit failed\n");
	spectgen_session_put(handle->session);
session_creation_failed:
	printf("session failed\n");
	free(handle);
	return -1;
}

int spectgen_close(spectgen_handle _handle)
{
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;

	if(!handle)
	      return -1;

	/* Will block till decoder is still active */
	decoder_close(handle->session->d_handle);

	/* Will block if the session is still in working state */
	spectgen_session_put(handle->session);

	q_destroy(&handle->queue);
	free(handle->leftover);
	free(handle->barkband_table);

	free(handle);
	return 0;
}

int spectgen_start(spectgen_handle _handle)
{
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	if(!handle)
	      return -1;

	if(decoder_start(handle->session->d_handle))
		return -1;

	spectgen_session_start(handle->session);

	return 0;
}

static void spectgen_worker(void *_handle)
{
	float *decode_buffer;
	unsigned int decode_len;
	unsigned int frate;
	unsigned int old_frate = 0;
	unsigned int leftover_len = 0;
	float *buf = NULL;
	int i;
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	if(!handle)
	      goto bailout;

	
	while(1) {
		decode_len = 0;
		decoder_data_pull(handle->session->d_handle, 
					&decode_buffer, &decode_len, &frate);
		if(decode_len == 0)
		      break;

		/* Handle the case when the frame rate changes in the middle.
		 * This is an odd case, but we need to handle it */
		if(!handle->barkband_table_inited || old_frate != frate) {
			old_frate = frate;
			setup_barkband_table(handle, frate);
		}
		/* Compute the weighted sum, this is done in order to 
		 * calculate the average frame rate */
		handle->average_frate += (double)(frate * decode_len);
		handle->total_samples += (double)(decode_len);

		buf = decode_buffer;
		if(leftover_len > 0) {
			buf = malloc(sizeof(float) * (decode_len + leftover_len));
			if(!buf) {
				goto failed_in_loop;
			}
			memcpy(buf, handle->leftover, sizeof(float) * leftover_len);
			memcpy(&buf[leftover_len], decode_buffer, sizeof(float) * decode_len);
			decode_len += leftover_len;
			leftover_len = 0;
			free(decode_buffer);
		}
		if(decode_len >= handle->session->window_size) {
			for(i = 0; i < decode_len - handle->session->window_size; 
						i+=handle->step_size) {
				process_window(handle, &buf[i]);
			}
			if(i < decode_len) {
				memcpy(handle->leftover, &buf[i], 
					sizeof(float) * (decode_len - i));
				leftover_len = decode_len - i;
			}
		} else {
			memcpy(handle->leftover, buf, decode_len * sizeof(float));
			leftover_len = decode_len;
		}
		free(buf);
	}
bailout:
failed_in_loop:
	q_put(&handle->queue, NULL);
}

unsigned int spectgen_frate(spectgen_handle _handle)
{
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	if(!handle)
	      return 0;
	return (unsigned int)(handle->average_frate / handle->total_samples);
}

float *spectgen_pull(spectgen_handle _handle)
{
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	return q_get(&handle->queue);
}

int spectgen_read(spectgen_handle handle, float **_band_array, unsigned int nbands)
{
	float *band_array;
	float *band;
	int band_len = 0;
	int max_band_len = 0;
	if(!handle || !_band_array || !nbands)
	      return -1;
	band_array = *_band_array = NULL;
	if(spectgen_start(handle)) {
		return -1;
	}

	while((band = spectgen_pull(handle)) != NULL) {
		band_len++;
		if(band_len > max_band_len) {
			max_band_len += 1024;
			band_array = realloc(band_array, sizeof(float) * nbands * max_band_len);
			if(!band_array) {
				do {
					free(band);
				} while((band = spectgen_pull(handle)) != NULL);
				return -1;
			}
		}
		memcpy(&band_array[(band_len-1) * nbands], band, sizeof(float) * nbands);
		free(band);
	}
	band_array = realloc(band_array, sizeof(float) * nbands * band_len);
	*_band_array = band_array;
	return band_len;
}
