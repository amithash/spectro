#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <math.h>
#include <fftw3.h>
#include "queue.h"

#include "decoder.h"

#define WINDOW_SIZE 1024
#define STEP_SIZE 512
#define NUMFREQS ((WINDOW_SIZE / 2) + 1)

#define SPECTRUM_BAND_FREQ(band, size, rate) \
      (unsigned int)(((float)(band))*((float)(rate))/((float)(size)))
#define BAND_FREQ(i, sr) SPECTRUM_BAND_FREQ(i, WINDOW_SIZE, sr)


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

int do_fft(float *in, unsigned int len, int fd)
{
	fftwf_complex *tmp_out;
	float *tmp_in;
	float *buf = in;
	int i;
	fftwf_plan plan;

	tmp_out = (fftwf_complex *)malloc(sizeof(fftwf_complex) * (WINDOW_SIZE / 2 + 1));
	if(!tmp_out) {
		goto tmp_out_failed;
	}
	tmp_in = (float *)malloc(sizeof(float) * WINDOW_SIZE);
	if(!tmp_in)
	      goto tmp_in_failed;

	plan = fftwf_plan_dft_r2c_1d(WINDOW_SIZE, tmp_in, tmp_out, FFTW_MEASURE);

	for(i = 0; i < len; i+=STEP_SIZE) {
		if(i + WINDOW_SIZE > len)
		      break;
		memcpy(tmp_in, buf, WINDOW_SIZE * sizeof(float));
		fftwf_execute(plan);
		buf += STEP_SIZE;
		do_band((float *)tmp_out, fd);
	}
	fftwf_destroy_plan(plan);
	free(tmp_in);
tmp_in_failed:
	free(tmp_out);
tmp_out_failed:
	return 0;
}


#define MAX_BUFFER_SIZE (1024 * 1024 * 512)
int main(int argc, char *argv[])
{
	FILE *f;
	decoder_handle handle;
	float *decode_buffer;
	unsigned int decode_len;
	unsigned int frate;
	float *buffer;
	unsigned int buffer_len;

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
	buffer = (float *)malloc(sizeof(float) * MAX_BUFFER_SIZE);
	if(!buffer) {
		printf("Buffer malloc failed\n");
		goto buffer_failed;
	}
	buffer_len = 0;

	while(1) {
		decoder_data_pull(handle, &decode_buffer, &decode_len, &frate);
		if(decode_len == 0)
		      break;
		if(!bark_band_table)
		      setup_bark_band_table(frate);

		memcpy(&buffer[buffer_len], decode_buffer, sizeof(float) * decode_len);
		buffer_len += decode_len;
		free(decode_buffer);
	}
	do_fft(buffer, buffer_len, fileno(f));
	free(bark_band_table);
	free(buffer);
buffer_failed:
init_failed:
	fclose(f);
start_failed:
	if(decoder_close(handle)) {
		printf("Closing decoder handle failed\n");
	}
open_failed:
	decoder_exit(handle);
	return 0;
}
