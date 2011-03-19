#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "spect-config.h"
#include "spectgen.h"
#include <fftw3.h>
#include "plot.h"

#define BUFFER_SIZE 1024
//#define BPM_STEP_SIZE 8192
#define BPM_STEP_SIZE (8192/2)
#define BPM_WINDOW_SIZE (BPM_STEP_SIZE * 2)

float *get_x(unsigned int samples_per_min, unsigned int len)
{
	int i;
	float freq_resolution = (float)samples_per_min / (float)len;
	float *x = (float *)calloc((len / 2) + 1, sizeof(float));
	if(!x)
	      return NULL;
	for(i = 0; i < 1+(len/2); i++) {
		x[i] = (float)i * freq_resolution;
	}
	return x;
}

void analyze_freqs(float *freqs, unsigned int samples_per_min, unsigned int len)
{
	int i;
	float *x = get_x(samples_per_min, len);
	if(!x)
	      return;
	freqs[0] = 0;
	for(i = 0; i < 1+(len/2); i++) {
		if(x[i] >= 30)
		      break;
		freqs[i] = 0;
	}
	plot(x, freqs, 1, 1+(len/2), PLOT_LINES, 0);
}

void analyze_freqs2(float *freqs[NBANDS], unsigned int samples_per_min, unsigned int len)
{
	int i,j;
	float *x = get_x(samples_per_min, len);
	unsigned int out_len = 1 + (len / 2);
	float *y = calloc(NBANDS * out_len, sizeof(float));
	if(!x || !y)
	      return;
	for(i = 0; i < out_len; i++) {
		if(x[i] >= 30)
		      break;
		for(j = 0; j < NBANDS; j++)
		      freqs[j][i] = 0;
	}
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < out_len; j++) {
			y[(i * out_len) + j] = freqs[i][j];
		}
	}
	plot(x, y, NBANDS, out_len, PLOT_LINES, 0);
}

#define SQR(val) ((val) * (val))

void normalize(float *vec, unsigned int len)
{
	int i;
	float max = 0;
	float min = FLT_MAX;
	for(i = 0; i < len; i++) {
		if(vec[i] > max)
		      max = vec[i];
		if(vec[i] < min)
		      min = vec[i];
	}
	for(i = 0; i < len; i++) {
		vec[i] = (vec[i] - min) / max;
	}
}

void find_frequencies(float *spect[NBANDS], unsigned int samples_per_min, unsigned int len)
{
	float *fft_in = NULL;
	fftwf_complex *fft_out = NULL;
	fftwf_plan plan;
	float *freqs = NULL;
	unsigned int out_len = (len / 2) + 1;
	int i, j;

	fft_in = (float *)fftwf_malloc(sizeof(float) * len);
	fft_out = (fftwf_complex *)fftwf_malloc(sizeof(fftwf_complex) * out_len);
	freqs = (float *)calloc(out_len, sizeof(float));
	if(!fft_in || !fft_out || !freqs)
	      goto alloc_failed_bailout;
	plan = fftwf_plan_dft_r2c_1d(len, fft_in, fft_out, FFTW_MEASURE | FFTW_DESTROY_INPUT);
	if(!plan)
	      goto alloc_failed_bailout;
	for(i = 0; i < out_len; i++)
	      freqs[i] = 0;

	for(i = 0; i < NBANDS; i++) {
		memcpy(fft_in, spect[i], len * sizeof(float));
		normalize(fft_in, len);
		fftwf_execute(plan);
		for(j = 0; j < out_len; j++) {
			float real = fft_out[j][0];
			float imag = fft_out[j][1];
			float powr = SQR(real * imag);
			freqs[j] += powr;
			spect[i][j] = powr;
		}
	}
	for(i = 0; i < out_len; i++)
	      freqs[i] = sqrt(freqs[i] / (float)len);

	//analyze_freqs(freqs, samples_per_min, len);
	analyze_freqs2(spect, samples_per_min, len);


	fftwf_destroy_plan(plan);
alloc_failed_bailout:
	if(freqs)
	      free(freqs);
	if(fft_in)
	      fftwf_free(fft_in);
	if(fft_out)
	      fftwf_free(fft_out);
}

int main(int argc, char *argv[]) 
{
	char *mp3;
	spectgen_handle handle;
	float *sample;
	unsigned int frate;
	unsigned int len = 0;
	float *spect_raw;
	unsigned int max_len = 0;
	int i,j;
	float *spect[NBANDS];
	
	if(argc < 2) {
		printf("USAGE: %s <MP3>\n", argv[0]);
		return -1;
	}
	mp3 = argv[1];

	if(spectgen_open(&handle, mp3, BPM_WINDOW_SIZE, BPM_STEP_SIZE)) {
		printf("SPECT OPEN of %s failed\n", mp3);
		exit(-1);
	}
	if(spectgen_start(handle)) {
		spectgen_close(handle);
		exit(-1);
	}
	spect_raw = malloc(sizeof(float) * NBANDS * BUFFER_SIZE);
	if(!spect_raw) {
		printf("Malloc failed\n");
		exit(-1);
	}
	max_len = BUFFER_SIZE;

	while(1) {
		sample = spectgen_pull(handle);
		if(!sample)
		      break;
		if(len == max_len) {
			spect_raw = realloc(spect_raw, sizeof(float) * NBANDS * (max_len + BUFFER_SIZE));
			max_len += BUFFER_SIZE;
			if(!spect_raw) {
				printf("Realloc failed\n");
				exit(-1);
			}
		}
		memcpy(&spect_raw[NBANDS * len], sample, sizeof(float) * NBANDS);
		free(sample);
		len++;
	}
	spect_raw = realloc(spect_raw, sizeof(float) * NBANDS * len);
	frate = spectgen_frate(handle);
	if(spectgen_close(handle)) {
		printf("Close failed\n");
		exit(-1);
	}
	if(!frate) {
		printf("Frate was 0!\n");
		exit(-1);
	}
	for(i = 0; i < NBANDS; i++) {
		spect[i] = malloc(sizeof(float) * len);
		if(!spect[i]) {
			printf("Malloc for %d failed\n",i);
			exit(-1);
		}
	}
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < len; j++) {
			spect[i][j] = spect_raw[(NBANDS * j) + i];
		}
	}
	free(spect_raw);

	find_frequencies(spect, 
		60 * (unsigned int)(0.5 + ((float)frate / (float)BPM_STEP_SIZE)),
		len);

#if 0
	printf("Samples: %d\n", len);
	for(i = 0; i < len; i++) {
		for(j = 0; j < NBANDS; j++) {
			printf("%f ", spect[(NBANDS * i) + j]);
		}
		printf("\n");
	}
#endif
	return 0;
}
