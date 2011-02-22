#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <fftw3.h>
#include "queue.h"
#include "spectgen.h"

#include "decoder.h"

#define SPECTRUM_BAND_FREQ(band, size, rate) \
      (unsigned int)(((float)(band))*((float)(rate))/((float)(size)))


static const unsigned int bark_bands[24] 
  = { 100,  200,  300,  400,  510,  630,  770,   920, 
      1080, 1270, 1480, 1720, 2000, 2320, 2700,  3150, 
      3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500 };

struct spectgen_struct {
	pthread_t      thread;
	char           filename[256];
	q_type         queue;
	unsigned int   *barkband_table;
	int            barkband_table_inited;
	decoder_handle d_handle;
	float          *fft_in;
	fftwf_complex  *fft_out;
	fftwf_plan     plan;
	int            started;
	float          *leftover;
	unsigned int   window_size;
	unsigned int   step_size;
	unsigned int   numfreqs;
};
void *spectgen_thread(void *_handle);

static void setup_barkband_table(struct spectgen_struct *handle, unsigned int sampling_rate)
{
	int i;
	unsigned int barkband = 0;
	memset(handle->barkband_table, 0, sizeof(unsigned int) * handle->numfreqs);
	for(i = 0; i < handle->numfreqs; i++) {
		if(barkband < 23 &&
			SPECTRUM_BAND_FREQ(i, handle->window_size, sampling_rate) >= 
			bark_bands[barkband])
		      barkband++;
		handle->barkband_table[i] = barkband;
	}
	handle->barkband_table_inited = 1;
}

static int do_band(struct spectgen_struct *handle, float *buf)
{
	int i;
	float *band;

	if(!handle->barkband_table_inited)
	      return -1;

	band = (float *)calloc(24, sizeof(float));
	if(!band)
	      return -1;

	for(i = 1; i < handle->numfreqs; i++) {
		float real = buf[2 * i];
		float imag = buf[2 * i + 1];
		band[handle->barkband_table[i]] += ((real * real) + (imag * imag)) / (float)handle->window_size;
	}
	for(i = 0; i < 24; i++) {
		band[i] = sqrt(band[i]);
	}
	q_put(&handle->queue, band);

	return 0;
}

static int do_fft(struct spectgen_struct *handle, float *in)
{
	memcpy(handle->fft_in, in, sizeof(float) * handle->window_size);
	fftwf_execute(handle->plan);
	return do_band(handle, (float *)handle->fft_out);
}

int spectgen_open(spectgen_handle *_handle, char *fname, unsigned int window_size, unsigned int step_size)
{
	struct spectgen_struct *handle;

	*_handle = NULL;
	if(!window_size || !step_size)
	      return -1;
	if(step_size > window_size)
	      return -1;

	handle = (struct spectgen_struct *)
	    malloc(sizeof(struct spectgen_struct));
	if(!handle)
	      return -1;

	handle->window_size = window_size;
	handle->step_size = step_size;
	handle->numfreqs = (window_size / 2) + 1;

	strncpy(handle->filename, fname, 256);
	handle->filename[255] = '\0';
	handle->fft_in = (float *)malloc(sizeof(float) * handle->window_size);
	if(!handle->fft_in)
	      goto fft_in_failed;
	handle->fft_out = (fftwf_complex *)
	    malloc(sizeof(fftwf_complex) * handle->numfreqs);
	if(!handle->fft_out)
	      goto fft_out_failed;
	
	handle->plan = fftwf_plan_dft_r2c_1d(handle->window_size, handle->fft_in, 
				handle->fft_out, FFTW_MEASURE);

	if(q_init(&handle->queue))
	      goto q_init_failed;

	handle->leftover = (float *)malloc(sizeof(float) * handle->window_size);
	if(!handle->leftover)
	      goto leftover_failed;


	handle->barkband_table = (unsigned int *)malloc(sizeof(unsigned int) * handle->numfreqs);
	if(!handle->barkband_table)
	      goto barkband_table_failed;

	if(decoder_init(&handle->d_handle))
	      goto decoder_init_failed;
	if(decoder_open(handle->d_handle, handle->filename)) {
		goto open_failed;
	}
	handle->barkband_table_inited = 0;
	handle->started = 0;

	*_handle = handle;
	return 0;

open_failed:
	decoder_exit(handle->d_handle);
decoder_init_failed:
	free(handle->barkband_table);
barkband_table_failed:
	free(handle->leftover);
leftover_failed:
	q_destroy(&handle->queue);
q_init_failed:
	fftwf_destroy_plan(handle->plan);
	free(handle->fft_out);
fft_out_failed:
	free(handle->fft_in);
fft_in_failed:
	free(handle);
	return -1;
}

int spectgen_close(spectgen_handle _handle)
{
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	void *pars;

	if(!handle)
	      return -1;
	if(handle->started) {
		pthread_join(handle->thread, &pars);
	}
	decoder_close(handle->d_handle);
	decoder_exit(handle->d_handle);
	q_destroy(&handle->queue);
	fftwf_destroy_plan(handle->plan);
	free(handle->fft_out);
	free(handle->fft_in);
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

	if(decoder_start(handle->d_handle))
		return -1;

	handle->started = 1;
	if(pthread_create(&handle->thread, 0, spectgen_thread, handle))
		return -1;
	return 0;
}

void *spectgen_thread(void *_handle)
{
	float *decode_buffer;
	unsigned int decode_len;
	unsigned int frate;
	unsigned int leftover_len = 0;
	float *buf = NULL;
	int i;
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	if(!handle)
	      goto bailout;


	while(1) {
		decode_len = 0;
		decoder_data_pull(handle->d_handle, &decode_buffer, &decode_len, &frate);
		if(decode_len == 0)
		      break;

		if(!handle->barkband_table_inited)
		      setup_barkband_table(handle, frate);

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
		if(decode_len >= handle->window_size) {
			for(i = 0; i < decode_len - handle->window_size; i+=handle->step_size) {
				do_fft(handle, &buf[i]);
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
	pthread_exit(NULL);
}

float *spectgen_pull(spectgen_handle _handle)
{
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	return q_get(&handle->queue);
}
