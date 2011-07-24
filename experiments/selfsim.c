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
#include "spect-config.h"
#include "spectgen.h"
#include <unistd.h>
#include "plot.h"
#include <math.h>

#define STEP_SIZE(w) (w / 2)

#define DEFAULT_WINDOW_SIZE (1024*16)

#define PROCESSING_WINDOW_SIZE 1024

static void histogram(float *hist, unsigned int hist_len,
			float *vals, unsigned int len,
			float min, float max)
{
	int i;
	float step = (max - min) / (float)hist_len;
	unsigned int tot = 0;
	float *x;
	memset(hist, 0, hist_len * sizeof(float));
	for(i = 0; i < len; i++) {
		int ind;
		if(vals[i] > max)
		      ind = step * (float)hist_len;
		else
		      ind = vals[i] / step;
		hist[ind]++;
		tot++;
	}
	x = malloc(sizeof(float) * hist_len);
	if(!x) {
		printf("Alloc failure\n");
		exit(-1);
	}
	for(i = 0; i < hist_len; i++) {
	      hist[i] /= (float)tot;
	      x[i] = step * (float)i;
	}
	plot(x, hist, 1, hist_len, PLOT_LINES, 0);

}

static void sort(float *a, unsigned int len)
{
	int i,j;
	float temp;
	for(i = 0; i < len; i++) {
		for(j = 0; j < len - 1; j++) {
			if(a[j] > a[j + 1]) {
				temp = a[j];
				a[j] = a[j+1];
				a[j+1] = temp;
			}
		}
	}
}

static void moving_window_smoothing(float *a, unsigned int len, unsigned win)
{
	int i,j;
	for(i = 0; i < len; i++) {
		unsigned int to = i + win;
		unsigned int cnt;
		float  val = 0;
		if(to > len)
		      to = len;
		cnt = to - i;
		for(j = i; j < to; j++) {
			val += a[j];
		}
		if(cnt)
			a[i] = val / (float)cnt;
		else
		      a[i] = 0;
	}
}

static void sum_column(float *out, float *in, unsigned int sq_len)
{
	int i,j;
	for(i = 0; i < sq_len; i++) {
		float val = 0;
		for(j = 0; j < sq_len; j++) {
			val += in[(i * sq_len) + j];
		}
		out[i] = val / (float)sq_len;
	}
}

static inline scale_t validate_scale(unsigned int inp)
{
	if(inp >= MAX_SCALE)
	      return BARK_SCALE;
	return (scale_t)inp;
}

static float distance(float *a, float *b, unsigned int dim)
{
	int i;
	float ret = 0;
	for(i = 0; i < dim; i++) {
		float val = a[i] - b[i];
		ret += val * val;
	}
	return sqrt(ret);
}

static void self_similarity(float *out, float *in, unsigned int dim, unsigned int len)
{
	int i,j;
	for(i = 0; i < len; i++) {
		for(j = 0; j < len; j++) {
			float *a = &in[(i * dim)];
			float *b = &in[(j * dim)];
			out[(i * len) + j] = distance(a,b,dim);
		}
	}
}

int main(int argc, char *argv[])
{
	spectgen_handle handle;
	float *band;
	float *band_array = NULL;
	unsigned int band_max_len = 0;
	unsigned int band_len = 0;
	unsigned int nbands = 24;
	scale_t scale = BARK_SCALE;
	char *outfile = NULL;
	char *infile = NULL;
	int opt;
	char default_outfile[100] = "a.pgm";
	unsigned int window_size = DEFAULT_WINDOW_SIZE;
	float *selfsim;

	while((opt = getopt(argc, argv, "o:n:s:hw:")) != -1) {
		switch(opt) {
			case 'o':
				outfile = optarg;
				break;
			case 'n':
				nbands = atoi(optarg);
				if(nbands < 1 || nbands > 20000) {
					printf("Nbands option (-n) has an invalid value:%d\n", nbands);
					exit(-1);
				}
				break;
			case 's':
				scale = validate_scale(atoi(optarg));
				break;
			case 'w':
				window_size = atoi(optarg);
				break;
			case 'h':
				printf("USAGE: %s [OPTIONS] InMusicFile\n", argv[0]);
				printf("OPTIONS: \n");
				printf("-o OUTFILE specify out file name. default: a.pgm\n");
				printf("-n nbands  specify number of bands. default: 24\n");
				printf("-s scale specify scale to use to band the music. Options:0 - BARK_SCALE, 1 - MEL_SCALE, 2 - SEMITONE_SCALE, default: BARK_SCALE\n");
				printf("-w window size while performing the fft. default: %d\n", window_size);
				printf("-h - Print this message\n");
				exit(0);
			case '?':
				fprintf(stderr, "Option: %c requires an argument\n", optopt);
				exit(-1);
			default:
				printf("Unknown option: %c\n", opt);
				exit(-1);
		}
	}
	if(argc == optind) {
		printf("Required argument File name. Try %s -h for help\n", argv[0]);
		exit(-1);
	}
	infile = argv[optind];

	if(outfile == NULL) {
		outfile = default_outfile;
	}
	if(nbands > (window_size / 4)) {
		printf("Window size should be at lease: %d (4 times that of -n argument)\n", nbands * 4);
		exit(-1);
	}

	if(spectgen_open(&handle, infile, window_size, STEP_SIZE(window_size), scale, SPECTOGRAM, &nbands)) {
		printf("Spectgen open on %s failed\n", infile);
		exit(-1);
	}
	if(spectgen_start(handle)) {
		printf("Start failed\n");
		spectgen_close(handle);
		exit(-1);
	}

	while((band = spectgen_pull(handle)) != NULL) {
		band_len++;
		if(band_len > band_max_len) {
			band_array = realloc(band_array, sizeof(float) * nbands * (band_max_len + PROCESSING_WINDOW_SIZE));
			if(!band_array) {
				printf("Malloc failed!\n");
				exit(-1);
			}
			band_max_len += PROCESSING_WINDOW_SIZE;
		}
		memcpy(&band_array[(band_len - 1) * nbands], band, sizeof(float) * nbands);
		free(band);
	}
	printf("Length = %d width=%d\n", band_len, nbands);
	band_array = realloc(band_array, sizeof(float) * nbands * band_len);

	spectgen_close(handle);

	selfsim = (float *)malloc(sizeof(float) * band_len * band_len);
	if(!selfsim) {
		printf("Alloc failure!\n");
		exit(-1);
	}

	self_similarity(selfsim, band_array, nbands, band_len);

	pgm(outfile, (float *)selfsim, band_len, band_len, BACKGROUND_BLACK, GREYSCALE, ALL_NORMALIZATION);

	{
	    	int i;
		float mean = 0;
		float *hist = NULL;
		float *most_similar = calloc(band_len, sizeof(float));
		
		if(!most_similar) {
			printf("Alloc failure\n");
			exit(-1);
		}
		hist = calloc(SPECT_HIST_LEN, sizeof(float));
		if(!hist) {
			printf("Alloc failure\n");
			exit(-1);
		}
		sum_column(most_similar, selfsim, band_len);
		moving_window_smoothing(most_similar, band_len, band_len / 100);
		sort(most_similar, band_len);
		printf("Dramatization: %1.4f\n", most_similar[band_len / 2]);
		histogram(hist, SPECT_HIST_LEN, most_similar, band_len, SPECT_MIN_VAL, SPECT_MAX_VAL);
	}

	return 0;
}
