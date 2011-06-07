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

#define WINDOW_SIZE 2048
#define STEP_SIZE   WINDOW_SIZE

typedef struct {
	float band[NBANDS];
} spect_band_t;


#define PROCESSING_WINDOW_SIZE 1024

int main(int argc, char *argv[])
{
	spectgen_handle handle;
	float *band;
	spect_band_t *band_array = NULL;
	unsigned int band_max_len = 0;
	unsigned int band_len = 0;

	if(argc < 3) {
		printf("Usage: %s <Input MP3 File> <Output Spect File>\n", argv[0]);
		exit(-1);
	}
	if(spectgen_open(&handle, argv[1], WINDOW_SIZE, STEP_SIZE)) {
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
		memcpy(&band_array[band_len - 1], band, sizeof(spect_band_t));
		free(band);
	}
	band_array = realloc(band_array, sizeof(spect_band_t) * band_len);
	spectgen_close(handle);
	pgm(argv[2], (float *)band_array, NBANDS, band_len, BACKGROUND_BLACK, GREYSCALE, COL_NORMALIZATION);

	return 0;
}
