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

#define WINDOW_SIZE (2048*64)
#define STEP_SIZE   WINDOW_SIZE

typedef struct {
	float band[3];
} spect_band_t;

#define POSITIVE_SMALL_VAL (0.00001)
#define NEGATIVE_SMALL_VAL (-0.00001)

int is_zero(float *data, unsigned int len)
{
	int i;
	for(i = 0; i < len; i++) {
		if(data[i] > POSITIVE_SMALL_VAL)
		      return 0;
		if(data[i] < NEGATIVE_SMALL_VAL)
		      return 0;
	}
	return 1;
}


#define PROCESSING_WINDOW_SIZE 1024

int main(int argc, char *argv[])
{
	spectgen_handle handle;
	float *band;
	spect_band_t *band_array = NULL;
	unsigned int band_max_len = 0;
	unsigned int band_len = 0;
	unsigned int nbands = 3;

	if(argc < 2) {
		printf("Usage: %s <Input MP3 File>\n", argv[0]);
		exit(-1);
	}
	if(spectgen_open(&handle, argv[1], WINDOW_SIZE, STEP_SIZE, BARK_SCALE, SPECTOGRAM, &nbands)) {
		printf("Spectgen open on %s failed\n", argv[1]);
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
			band_array = realloc(band_array, sizeof(spect_band_t) * (band_max_len + PROCESSING_WINDOW_SIZE));
			if(!band_array) {
				printf("Malloc failed!\n");
				exit(-1);
			}
			band_max_len += PROCESSING_WINDOW_SIZE;
		}
		memcpy(band_array[band_len - 1].band, band, sizeof(float) * 3);
		free(band);
	}
	spectgen_close(handle);
	band_array = realloc(band_array, sizeof(spect_band_t) * band_len);
	plot3d((float *)band_array, band_len, 0, PLOT_POINTS);

	return 0;
}
