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
#include "getopt_easy.h"

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
	float *band_array = NULL;
	int band_len = 0;
	unsigned int nbands = 24;
	scale_t scale = BARK_SCALE;
	int opt_scale = (int)BARK_SCALE;
	char *outfile = NULL;
	char *infile = NULL;
	int help = 0;
	char default_outfile[100] = "a.pgm";
	unsigned int window_size = (1024 * 16 * 2);
	getopt_easy_opt_t opt[] = {
		{"o:", STRING, (void *)&outfile},
		{"n:", UNSIGNED_INT, &nbands},
		{"s:", INT, &opt_scale},
		{"w:", UNSIGNED_INT, &window_size},
		{"h",  FLAG, &help}
	};

	if(getopt_easy(&argc, &argv, opt, 5)) {
		printf("Options parsing failed\n");
		exit(-1);
	}

	if(nbands < 1 || nbands > 20000) {
		printf("Nbands option (-n) has an invalid value:%d\n", nbands);
		exit(-1);
	}
	scale = validate_scale(opt_scale);
	if(help) {
		printf("USAGE: %s [OPTIONS] InMusicFile\n", argv[0]);
		printf("OPTIONS: \n");
		printf("-o OUTFILE specify out file name. default: a.pgm\n");
		printf("-n nbands  specify number of bands. default: 24\n");
		printf("-s scale specify scale to use to band the music. Options:0 - BARK_SCALE, 1 - MEL_SCALE, 2 - SEMITONE_SCALE, default: BARK_SCALE\n");
		printf("-w window size while performing the fft. default: %d\n", window_size);
		printf("-h - Print this message\n");
		exit(0);
	}

	if(argc <= 1) {
		printf("Required argument File name. Try %s -h for help\n", argv[0]);
		exit(-1);
	}
	infile = argv[1];

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

	if((band_len = spectgen_read(handle, &band_array, nbands)) <= 0) {
		printf("Spectgen read failed!\n");
		spectgen_close(handle);
		exit(-1);
	}
	spectgen_close(handle);

	pgm(outfile, (float *)band_array, nbands, band_len, BACKGROUND_BLACK, GREYSCALE, ALL_NORMALIZATION);

	return 0;
}
