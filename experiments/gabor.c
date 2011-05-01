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
#include <unistd.h>
#include <complex.h>
#include <math.h>
#include <string.h>
#include <tag_c.h>
#include <float.h>
#include "plot.h"

#define STARTING_NOTE 2

#ifndef PI
#define PI 3.1415926535897932384626433832
#endif
#define ERROR 0.0001
static float error_margin;

float complex _gabor(float base_freq, float t)
{
	float multi = exp(-0.5 * t * t);
	float real = cos(base_freq * t) * multi;
	float imag = sin(base_freq * t) * multi;

	return real + (imag * I);
}

float complex gabor(float t)
{
	return _gabor(2 * PI * 16.352, t);
}

/* u is the time domain transition.
 * k is the scaling factor
 * k = 0 => C0 note, k = 12 => C1 etc.
 */
float complex gabor_scaled(float k, float u, float t)
{
	float den = pow(2, -1.0 * k/12.0);
	return (1.0 / sqrt(den)) * gabor((t - u) / den);
}

float gabor_pow_kernel(float *data, unsigned int len, unsigned int srate, float k, float u)
{
	int i;
	float complex val = 0;
	float ret;
	float error_f = ((float)pow(2, -1 * k/12.0) * error_margin);
	float from_f = u - error_f;
	float to_f   = u + error_f;
	unsigned int from = (unsigned int)(from_f * (float)srate);
	unsigned int to   = (unsigned int)(to_f * (float)srate);
	to = to <=len ? to : len;
	from = from > 0 ? from : 0;
	for(i = from; i < to; i++) {
		val += gabor_scaled(k, u, (float)i / (float)srate) * data[i];
	}
	ret = cabsf(val);
	if(!finite(ret)) {
		printf("Value is infinite\n");
		ret = 0;
	}
	return ret;
}

int _cwt_gabor(float **_out, float *data, unsigned int len, unsigned int num_octaves, unsigned int srate, unsigned int step)
{
	int u,k;
	float *out;
	unsigned int alloc_len = 12 * (num_octaves - STARTING_NOTE) * len / step;
	printf("Allocating %luKB\n", alloc_len * sizeof(float) / (1024));
	*_out = out = (float *)calloc(alloc_len, sizeof(float));
	if(!out)
	      return -1;

	for(k = 0; k < alloc_len; k++)
	      out[k] = 0;

	error_margin = sqrt(2 * log(1.0 / ERROR));

	/* This is not for cats. Humans cannot hear C0 (~16Hz) Note.
	   Start with C1 */
	for(k = 12 * STARTING_NOTE; k < 12 * num_octaves; k++) {
		printf("Octave=%d, semitone=%d\n",k / 12, k % 12);
		for(u = 0; u < len; u+=step) {
			out[(k-(12 * STARTING_NOTE)) * (len / step) + (u/step)] = 
				gabor_pow_kernel(data, len, srate, k, (float)u/(float)srate);
		}
	}
	return 0;
}

/*
 * num_octaves - out - based on sampling rate and max frequency = 8500Hz
 * out         - out - 2d array allocated by this function, must be freed by user
 * data        - in  - input sample
 * len         - in  - length of input
 * srate       - in  - sampling rate for the input stream.
 * width       - in  - resolution in seconds. Example: 0.1 => the width between output notes is 100ms.
 */
int cwt_gabor(unsigned int *_num_octaves, unsigned int *_out_len, float **out, float *data, unsigned int len, unsigned int srate, float width)
{
	unsigned int num_octaves = 1;
	unsigned int max_freq = srate / 2; /* Nyquist rate */
	unsigned int step;
	float min_width = (1.0/(float)srate) * 10;
	if(max_freq > 8500)
	      max_freq = 8500;
	*_num_octaves = num_octaves = (unsigned int)(log((double)max_freq / 16.352) / log(2.0));

	if(width < min_width) {
		printf("Width %.2f is lower than the minimum width supported=%.2f. Fixing it\n",width, min_width);
		width = min_width;
	}

	step = (unsigned int)((float)srate * width);
	*_out_len = len / step;

	printf("Computing cwt with step=%d, num_octaves=%d\n",step, num_octaves);

	return _cwt_gabor(out, data, len, num_octaves, srate, step);
}

#define WINDOW (1024 * 1024)
int get_file(float **_data, unsigned int *_len, const char *fname)
{
	float *data;
	unsigned int len = 0;
	unsigned int max_len = WINDOW;
	FILE *f = fopen(fname, "r");
	if(!f) {
		printf("Bad file\n");
		return -1;
	}

	data = malloc(sizeof(float) * max_len);
	if(!data) {
		printf("Malloc failed\n");
	      return -1;
	}

	do {
		unsigned int bytes = read(fileno(f), &data[len], WINDOW * sizeof(float));
		if(bytes < 0) {
			printf("Error received from read\n");
			break;
		}
		if(bytes < sizeof(float)) {
			printf("Buffer read which is less than a float!\n");
			break;
		}
		if(bytes % sizeof(float) != 0) {
			printf("Fatal error unaligned read\n\n");
			break;
		}

		len += bytes / sizeof(float);

		max_len += WINDOW;
		data = realloc(data, max_len * sizeof(float));
	} while(1);

	data = realloc(data, len * sizeof(float));

	*_data = data;
	*_len = len;

	fclose(f);
	return 0;
}

int main(int argc, char *argv[])
{
	float *data;
	float *out;
	char *mp3_file;
	unsigned int srate;
	unsigned int len = 0;
	unsigned int num_octaves;
	unsigned int out_len;
	char *cmd;
	TagLib_File *f;
	if(argc <= 1) {
		printf("USAGE: %s <MP3 File>\n", argv[0]);
		return 0;
	}
	mp3_file = argv[1];


	f = taglib_file_new(mp3_file);
	if(f == NULL) {
		printf("Cannot open %s\n",mp3_file);
		exit(-1);
	}
	srate = taglib_audioproperties_samplerate(taglib_file_audioproperties(f));
	taglib_file_free(f);
	if(srate == 0) {
		printf("Sample rate in tag == 0, cannot proceed\n");
		exit(-1);
	}
	printf("Sampling rate = %d\n",srate);

	cmd = calloc(512, sizeof(char));
	sprintf(cmd, "spectgen -r -o ./tmp.raw \"%s\"",mp3_file);
	if(system(cmd) != 0) {
		printf("spectgen failed!\n");
		printf("Cmd = %s\n",cmd);
		exit(-1);
	}
	free(cmd);
	cmd = NULL;

	if(get_file(&data, &len, "./tmp.raw")) {
		printf("Getting file failed!\n");
		printf("Len = %d\n",len);
		exit(-1);
	}
	if(len < srate) {
		printf("File too short!\n");
		exit(-1);
	}

	printf("Number of samples = %d\n", len);

	printf("Doing gabor!\n");

	if(cwt_gabor(&num_octaves, &out_len, &out, data, len, srate, 1.0/16.0)) {
		printf("Cwt failed!\n");
		exit(-1);
	}
	free(data);
	unlink("./tmp.raw");


#if 0
	{
	    int i,j;
	    for(i = 0; i < out_len; i++) {
	    	for(j = 0; j < 12 * (num_octaves - 1); j++) {
			printf("%.3f ", out[(j * (num_octaves - 1)) + i]);
		}
		printf("\n");
	    }
	}
    	exit(-1);
#endif

	pgm("test.pgm", out, out_len, 12 * (num_octaves - 2), 
				BACKGROUND_WHITE, GREYSCALE, COL_NORMALIZATION);

	free(out);

	return 0;
}
