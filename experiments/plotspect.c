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

#define STEP_SIZE(w) (w / 2)

#define PROCESSING_WINDOW_SIZE 1024

static inline scale_t validate_scale(unsigned int inp)
{
	if(inp >= MAX_SCALE)
	      return BARK_SCALE;
	return (scale_t)inp;
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
	unsigned int window_size = (1024 * 16 * 2);

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

	if(spectgen_open(&handle, infile, window_size, STEP_SIZE(window_size), scale, nbands)) {
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
		memcpy(&band_array[band_len * nbands], band, sizeof(float) * nbands);
		free(band);
	}
	printf("Length = %d width=%d\n", band_len, nbands);
	band_array = realloc(band_array, sizeof(float) * nbands * band_len);
	spectgen_close(handle);
	pgm(outfile, (float *)band_array, nbands, band_len, BACKGROUND_BLACK, GREYSCALE, ALL_NORMALIZATION);

	return 0;
}
