#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <fftw3.h>
#include "queue.h"

#include "decoder.h"

#define WINDOW_SIZE 2048
#define STEP_SIZE 1024
#define NUMFREQS ((WINDOW_SIZE / 2) + 1)

#define SPECTRUM_BAND_FREQ(band, size, rate) \
      (unsigned int)(((float)(band))*((float)(rate))/((float)(size)))
#define BAND_FREQ(i, sr) SPECTRUM_BAND_FREQ(i, WINDOW_SIZE, sr)


float *fft_in = NULL;
fftwf_complex *fft_out = NULL;
fftwf_plan plan;
unsigned int *bark_band_table = NULL;
static const unsigned int bark_bands[24] 
  = { 100,  200,  300,  400,  510,  630,  770,   920, 
      1080, 1270, 1480, 1720, 2000, 2320, 2700,  3150, 
      3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500 };

void setup_bark_band_table(unsigned int sampling_rate)
{
	int i;
	unsigned int barkband = 0;
	bark_band_table = (unsigned int *)malloc(sizeof(unsigned int) * NUMFREQS);
	for(i = 0; i < NUMFREQS; i++)
	      bark_band_table[i] = 0;
	for(i = 0; i < NUMFREQS; i++) {
		if(barkband < 23 &&
			BAND_FREQ(i, sampling_rate) >= bark_bands[barkband])
		      barkband++;
		bark_band_table[i] = barkband;
	}
}

int do_band(float *buf, int fn)
{
	int i;
	float band[24] = {0};

	for(i = 0; i < 24; i++)
	      band[i] = 0;

	for(i = 1; i < NUMFREQS; i++) {
		float real = buf[2 * i];
		float imag = buf[2 * i + 1];
		band[bark_band_table[i]] += ((real * real) + (imag * imag)) / (float)WINDOW_SIZE;
	}
	for(i = 0; i < 24; i++) {
		band[i] = sqrt(band[i]);
	}
	if(write(fn, band, 24 * sizeof(float)) != sizeof(float) * 24) {
		printf("Write failed\n");
	}
	return 0;
}


int init_fft(void)
{
	fft_in = malloc(sizeof(float) * WINDOW_SIZE);
	if(!fft_in)
	      goto fft_in_failed;
	fft_out = malloc(sizeof(fftwf_complex) * (WINDOW_SIZE / 2 + 1));
	if(!fft_out)
	      goto fft_out_failed;

	plan = fftwf_plan_dft_r2c_1d(WINDOW_SIZE, fft_in, fft_out, FFTW_MEASURE);
	return 0;
fft_out_failed:
	free(fft_in);
	fft_in = NULL;
fft_in_failed:
	return -1;
}

int do_fft(float *in, int fd)
{
	memcpy(fft_in, in, sizeof(float) * WINDOW_SIZE);
	fftwf_execute(plan);
	do_band((float *)fft_out, fd);
	return 0;
}

int main(int argc, char *argv[])
{
	FILE *f;
	decoder_handle handle;
	float *decode_buffer;
	unsigned int decode_len;
	unsigned int frate;
	float *buf = NULL;
	int i;
	float *leftover;
	unsigned int leftover_len = 0;

	if(argc != 3) {
		printf("USAGE: %s <MP3 File> <Output file>\n", argv[0]);
		exit(-1);
	}

	f = fopen(argv[2], "w");
	if(!f) {
		printf("Cannot create %s\n", argv[2]);
		exit(-1);
	}

	if(decoder_init(&handle)) {
		printf("Cound not initialize decoder\n");
		goto init_failed;
	}

	if(decoder_open(handle, argv[1])) {
		printf("Could not open %s to decode\n", argv[1]);
		goto open_failed;
	}
	if(decoder_start(handle)) {
		printf("Could not start decoding\n");
		goto start_failed;
	}
	if(init_fft()) {
		goto fft_init_failed;
	}
	leftover = malloc(sizeof(float) * WINDOW_SIZE);
	if(!leftover)
	      goto leftover_failed;

	while(1) {
		decode_len = 0;
		decoder_data_pull(handle, &decode_buffer, &decode_len, &frate);
		if(decode_len == 0)
		      break;
		if(!bark_band_table)
		      setup_bark_band_table(frate);

		buf = decode_buffer;
		if(leftover_len > 0) {
			buf = malloc(sizeof(float) * (decode_len + leftover_len));
			if(!buf) {
				goto failed_in_loop;
			}
			memcpy(buf, leftover, sizeof(float) * leftover_len);
			memcpy(&buf[leftover_len], decode_buffer, sizeof(float) * decode_len);
			decode_len += leftover_len;
			leftover_len = 0;
			free(decode_buffer);
		}
		if(decode_len >= WINDOW_SIZE) {
			for(i = 0; i < decode_len - WINDOW_SIZE; i+=STEP_SIZE) {
				do_fft(&buf[i], fileno(f));
			}
			if(i < decode_len) {
				memcpy(leftover, &buf[i], sizeof(float) * (decode_len - i));
				leftover_len = decode_len - i;
			}
		} else {
			memcpy(leftover, buf, decode_len * sizeof(float));
			leftover_len = decode_len;
		}
		free(buf);
	}
failed_in_loop:
	free(bark_band_table);
leftover_failed:
	free(fft_in);
	free(fft_out);
	fftwf_destroy_plan(plan);
fft_init_failed:
start_failed:
	if(decoder_close(handle)) {
		printf("Closing decoder handle failed\n");
	}
open_failed:
	decoder_exit(handle);
init_failed:
	fclose(f);
	return 0;
}
