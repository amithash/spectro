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
#include <unistd.h>

#define WINDOW_SIZE 4096
#define STEP_SIZE   WINDOW_SIZE / 2


int main(int argc, char *argv[])
{
	FILE *f;
	spectgen_handle handle;
	float *band;
	if(argc < 3) {
		printf("Usage: %s <Input MP3 File> <Output Spect File>\n", argv[0]);
		exit(-1);
	}
	f = fopen(argv[2], "w");
	if(!f) {
		printf("Failed to create %s\n", argv[2]);
		exit(-1);
	}

	if(spectgen_open(&handle, argv[1], WINDOW_SIZE, STEP_SIZE)) {
		printf("Spectgen open on %s failed\n", argv[1]);
		fclose(f);
		exit(-1);

	}
	if(spectgen_start(handle)) {
		printf("Start failed\n");
		goto start_failed;
	}
	while((band = spectgen_pull(handle)) != NULL) {
		if(write(fileno(f), band, sizeof(float) * 24) != (sizeof(float) * 24)) {
			printf("Write failed\n");
		}
		free(band);
	}
start_failed:
	spectgen_close(handle);
	fclose(f);
	return 0;
}
