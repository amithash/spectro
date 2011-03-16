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

#include "decoder.h"

#define SPECTRUM_BAND_FREQ(band, size, rate) \
      (unsigned int)(((float)(band))*((float)(rate))/((float)(size)))

typedef struct {
	unsigned int 	freq;
	float		coef;
} coef_t;

static const coef_t inv_equal_loudness_coef[34] = {
	    { 0,     0.5833333333333334 },
	    { 20,    0.6194690265486725 },
	    { 30,    0.6796116504854368 },
	    { 40,    0.7216494845360825 },
	    { 50,    0.7526881720430109 },
	    { 60,    0.7692307692307693 },
	    { 70,    0.7865168539325843 },
	    { 80,    0.8045977011494253 },
	    { 90,    0.813953488372093  },
	    { 100,   0.8235294117647058 },
	    { 200,   0.8974358974358975 },
	    { 300,   0.9210526315789473 },
	    { 400,   0.9210526315789473 },
	    { 500,   0.9210526315789473 },
	    { 600,   0.9210526315789473 },
	    { 700,   0.9090909090909092 },
	    { 800,   0.8974358974358975 },
	    { 900,   0.8805031446540881 },
	    { 1000,  0.8750000000000001 },
	    { 1500,  0.8860759493670887 },
	    { 2000,  0.9090909090909092 },
	    { 2500,  0.9459459459459461 },
	    { 3000,  0.9790209790209791 },
	    { 3700,  1.0                },
	    { 4000,  0.9929078014184397 },
	    { 5000,  0.9459459459459461 },
	    { 6000,  0.8860759493670887 },
	    { 7000,  0.8333333333333333 },
	    { 8000,  0.813953488372093  },
	    { 9000,  0.813953488372093  },
	    { 10000, 0.8235294117647058 },
	    { 12000, 0.7368421052631579 },
	    { 15000, 0.6363636363636364 },
	    { 20000, 0.5600000000000001 }
};

static const unsigned int bark_bands[NBANDS] 
  = { 100,  200,  300,  400,  510,  630,  770,   920, 
      1080, 1270, 1480, 1720, 2000, 2320, 2700,  3150, 
      3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500 };

static float normalization_coef[NBANDS];

/* fftwf create plan is not thread safe */
pthread_mutex_t planner_lock = PTHREAD_MUTEX_INITIALIZER;

/* TODO: Consider creating wisdom files fft might get a lot faster */

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
	unsigned int   average_frate;
};
void *spectgen_thread(void *_handle);

__attribute__((constructor))
static void spectgen_init(void)
{
	int i, j = 1;
	for(i = 0; i < NBANDS; i++) {
		float x = (float)bark_bands[i];
		float y;
		float x1, x2, y1, y2;
		int found = 0;
		for(; j < 34; j++) {
			x1 = inv_equal_loudness_coef[j-1].freq;
			x2 = inv_equal_loudness_coef[j].freq;
			y1 = inv_equal_loudness_coef[j-1].coef;
			y2 = inv_equal_loudness_coef[j].coef;
			if(x >= x1 && x <= x2) {
				found = 1;
				break;
			}
		}
		if(found == 0) {
			printf("Could not find X!\n");
			exit(-1);
		}
		/* Linear interpolation */
		y = ((x - x1) * (y2 - y1) / (x2 - x1)) + y1;
		normalization_coef[i] = y;
	}
}

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

	band = (float *)calloc(NBANDS, sizeof(float));
	if(!band)
	      return -1;

	for(i = 1; i < handle->numfreqs; i++) {
		float real = buf[2 * i];
		float imag = buf[2 * i + 1];
		band[handle->barkband_table[i]] += ((real * real) + (imag * imag)) / (float)handle->window_size;
	}
	for(i = 0; i < NBANDS; i++) {
		band[i] = sqrt(band[i]) * normalization_coef[i];
	}
	q_put(&handle->queue, band);

	return 0;
}

static int do_fft(struct spectgen_struct *handle, float *in)
{
	/* Copy is bad, but in might be unaligned, and fft computaion
	 * might eat more than the time saved by not copying */
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
	handle->fft_in = (float *)fftwf_malloc(sizeof(float) * handle->window_size);
	if(!handle->fft_in)
	      goto fft_in_failed;
	handle->fft_out = (fftwf_complex *)
	    fftwf_malloc(handle->numfreqs * sizeof(fftwf_complex));
	if(!handle->fft_out)
	      goto fft_out_failed;
	
	/* Even though we are thread safe, the planner is not. so 
	 * serialize the calls to fftwf plan create */
	pthread_mutex_lock(&planner_lock);
	handle->plan = fftwf_plan_dft_r2c_1d(handle->window_size, handle->fft_in, 
				handle->fft_out, FFTW_MEASURE | FFTW_DESTROY_INPUT);

	pthread_mutex_unlock(&planner_lock);

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
	fftwf_free(handle->fft_out);
fft_out_failed:
	fftwf_free(handle->fft_in);
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
	fftwf_free(handle->fft_out);
	fftwf_free(handle->fft_in);
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
	unsigned int old_frate = 0;
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

		/* Handle the case when the frame rate changes in the middle.
		 * This is an odd case, but we need to handle it */
		if(!handle->barkband_table_inited || old_frate != frate) {
			old_frate = frate;
			setup_barkband_table(handle, frate);
			handle->average_frate = frate;
		}

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

unsigned int spectgen_frate(spectgen_handle _handle)
{
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	if(!handle)
	      return 0;
	return handle->average_frate;
}

float *spectgen_pull(spectgen_handle _handle)
{
	struct spectgen_struct *handle = (struct spectgen_struct *)_handle;
	return q_get(&handle->queue);
}
