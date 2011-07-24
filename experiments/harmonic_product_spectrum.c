#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "spectgen.h"
#include "plot.h"
#include "math.h"

#define PROCESSING_WINDOW_SIZE 1024

#define WINDOW_SIZE (1024*4)

#define C1_FREQ (32.703 * 4)

#define SPECTRUM_BAND_FREQ(band, size, rate) \
      (unsigned int)(((float)(band))*((float)(rate))/((float)(size)))

#define SPECTRUM_BAND_IND(freq, size, rate) \
	(unsigned int)((float)(freq) * (float)(size) / ((float)(rate)))

void free_2d(float **in, unsigned int len_y)
{
	int i;
	if(!in)
	      return;
	for(i = 0; i < len_y; i++) {
		if(in[i])
		      free(in[i]);
	}
	free(in);
}

float **allocate_2d(unsigned int len_x, unsigned int len_y)
{
	float **out = NULL;
	int i;
	out = (float **)calloc(len_y, sizeof(float *));
	if(!out)
	      return NULL;
	for(i = 0; i < len_y; i++) {
		out[i] = (float *)calloc(len_x, sizeof(float));
		if(!out[i]) {
			free_2d(out, len_y);
			return NULL;
		}
	}
	return out;
}

void hpp(float *fft, unsigned int len, unsigned int nharm)
{
	float **harm;
	int i,j;
	float max = 0;

	harm = allocate_2d(len, nharm);
	if(!harm)
	      return;
	memcpy(harm[0], fft, sizeof(float) * len);
	for(i = 2; i < (nharm + 1); i++) {
		for(j = 0; j < len; j += i) {
			harm[i-1][j/i] = fft[j];
		}
	}
	for(i = 0; i < len; i++) {
		for(j = 1; j < nharm; j++) {
			fft[i] = fft[i] * harm[j][i];
		}
	}
	free_2d(harm, nharm);
	for(i = 0; i < len; i++) {
		if(max < fft[i])
		      max = fft[i];
	}
	max = 3 * max / 4;
	for(i = 0; i < len; i++) {
		if(fft[i] < max)
		      fft[i] = 0;
		else
		      fft[i] = 1;
	}
}

int main(int argc, char *argv[]) 
{
	spectgen_handle handle;
	float *band;
	unsigned int nbands = 0;
	unsigned int window_size = WINDOW_SIZE;
	char *infile;
	float *band_array = NULL;
	unsigned int band_len = 0;
	unsigned int band_max_len = 0;
	unsigned int frate;
	unsigned int end_freq;
	unsigned int step_freq;
	float base_freq = C1_FREQ;
	unsigned int base_ind;
	unsigned int *indexes;
	unsigned int size;
	float *note_val;
	float sc, ad;
	int i,j;

	if(argc < 2) {
		printf("USAGE: %s <MP3>\n", argv[0]);
		return -1;
	}
	infile = argv[1];

	if(spectgen_open(&handle, infile, window_size, window_size / 2, 0, SPECTOGRAM, &nbands)) {
		printf("Opening %s failed\n", infile);
		return -1;
	}
	indexes = calloc(nbands, sizeof(unsigned int));
	if(!indexes) {
		exit(-1);
	}

	if(spectgen_start(handle)) {
		printf("Start failed\n");
		spectgen_close(handle);
		exit(-1);
	}
	while((band = spectgen_pull(handle)) != NULL) {
		if(band_len >= band_max_len)  {
			band_max_len += PROCESSING_WINDOW_SIZE;
			band_array = realloc(band_array, sizeof(float) * nbands * band_max_len);
			if(!band_array) {
				printf("Malloc failed\n");
				exit(-1);
			}
		}
		hpp(band, nbands, 1);
		memcpy(&band_array[band_len * nbands], band, sizeof(float) * nbands);
		free(band);
		band_len++;
	}
	band_array = realloc(band_array, sizeof(float) * nbands * band_len);
	frate = spectgen_frate(handle);
	spectgen_close(handle);

	end_freq = SPECTRUM_BAND_FREQ(nbands, WINDOW_SIZE, frate);
	step_freq = SPECTRUM_BAND_FREQ(1, WINDOW_SIZE, frate);
	base_ind = SPECTRUM_BAND_IND(C1_FREQ, WINDOW_SIZE, frate);
	sc = 12.0 / log(2);
	ad = log((float)frate / ((float)WINDOW_SIZE * C1_FREQ));
	size = 0;
	for(i = base_ind; i < nbands; i++) {
		indexes[i] = (unsigned int)(sc * (log((float)i) + ad));
		if(i == nbands - 1)
		      size = indexes[i] + 1;
	}

	note_val = (float *)calloc(band_len * size, sizeof(float));
	for(i = 0; i < band_len; i++) {
		for(j = base_ind; j < nbands; j++) {
			note_val[(i * size) + indexes[j]] += band_array[(i * nbands) + j];
		}
		for(j = 0; j < size; j++) {
			if(note_val[(i * size) + j] > 0)
			      note_val[(i * size) + j] = 1;
		}
	}

	free(band_array);

	pgm("a.pgm", (float *)note_val, size, band_len, BACKGROUND_BLACK, GREYSCALE, NO_NORMALIZATION);

	return 0;
}

