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
#include "spectgen.h"
#include "spect-config.h"
#include <unistd.h>

#define WINDOW_SIZE 4096
#define STEP_SIZE   WINDOW_SIZE / 2


int main(int argc, char *argv[])
{
	spectgen_handle handle;
	unsigned int nbands = NBANDS;
	float *band = NULL;
	int len;
	int i,j;
	if(argc < 2) {
		printf("Usage: %s <Input MP3 File>\n", argv[0]);
		exit(-1);
	}
	if(spectgen_open(&handle, argv[1], WINDOW_SIZE, STEP_SIZE, BARK_SCALE, SPECTOGRAM, &nbands)) {
		printf("Spectgen open on %s failed\n", argv[1]);
		exit(-1);

	}
	if((len = spectgen_read(handle, &band, nbands)) <= 0) {
		printf("Spectgen read failed!\n");
		spectgen_close(handle);
		exit(-1);
	}
	spectgen_close(handle);

	for(i = 0; i < len; i++) {
		for(j = 0; j < nbands; j++) {
			printf("%.4f ", band[(i * nbands) + j]);
		}
		printf("\n");
	}
	free(band);

	return 0;
}
