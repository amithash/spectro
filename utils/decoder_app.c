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
#include <unistd.h>

#include "decoder.h"

int main(int argc, char *argv[])
{
	float *buffer;
	unsigned int len;
	unsigned int frate;
	FILE *f;
	decoder_handle handle;
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
	while(1) {
		decoder_data_pull(handle, &buffer, &len, &frate);
		if(len == 0)
			break;
		if(write(fileno(f), buffer, len * sizeof(float)) != len * sizeof(float)) {
			printf("Write failed\n");
		}

		free(buffer);
	}
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

