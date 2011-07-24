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
	float *band_array = NULL;
	int band_len = 0;
	unsigned int nbands = 3;

	if(argc < 2) {
		printf("Usage: %s <Input MP3 File>\n", argv[0]);
		exit(-1);
	}
	if(spectgen_open(&handle, argv[1], WINDOW_SIZE, STEP_SIZE, BARK_SCALE, SPECTOGRAM, &nbands)) {
		printf("Spectgen open on %s failed\n", argv[1]);
		exit(-1);
	}

	if((band_len = spectgen_read(handle, &band_array, nbands)) <= 0) {
		printf("Spectgen Read failed!\n");
		spectgen_close(handle);
		exit(-1);
	}
	spectgen_close(handle);
	plot3d(band_array, band_len, 0, PLOT_POINTS);

	return 0;
}
