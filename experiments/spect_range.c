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
#include <float.h>
#include <math.h>
#include "spectgen.h"
#include "spect-config.h"

#define MAX_RANGE 14
#define INCREMENT 0.25

void chomp_name(char *name)
{
	int len = strlen(name) - 1;
	while(name[len] == '\n') {
		name[len] = '\0';
		len--;
	}
}

float min[NBANDS];
float max[NBANDS];
double nr_greater[NBANDS][MAX_RANGE];
double total = 0;

void init_stats(void)
{
	int i, j;
	for(i = 0; i < NBANDS; i++) {
		min[i] = FLT_MAX;
		max[i] = 0;
		for(j = 0; j < MAX_RANGE; j++) {
			nr_greater[i][j] = 0;
		}
	}
	total = 0;
}

void print_stats(void)
{
	int i, j;
	printf("Band Min Max ");
	for(j = 0; j < MAX_RANGE - 1; j++) {
		float val = (float)j * INCREMENT;
		printf("[%2.2f, %2.2f) ", val, val + INCREMENT);
	}
	printf("[%2.2f, inf)\n", (MAX_RANGE - 1) * INCREMENT);

	for(i = 0; i < NBANDS; i++) {
		printf("%2d   %2.3f   %2.3f", i, min[i], max[i]);
		for(j = 0; j < MAX_RANGE; j++) {
			printf("   %3.4f", (100.0 * nr_greater[i][j]) / total);
		}
		printf("\n");
	}
}

void process_band(float *band)
{
	int i;
	int ind;
	for(i = 0; i < NBANDS; i++) {
		if(min[i] > band[i])
		      min[i] = band[i];
		if(max[i] < band[i])
		      max[i] = band[i];
		ind = (int)(band[i] / INCREMENT);
		if(ind >= MAX_RANGE)
		      ind = MAX_RANGE - 1;
		if(ind < 0)
		      ind = 0;
		nr_greater[i][ind] += 1.0;
	}
	total += 1.0;
}

int get_nr_lines(char *fname)
{
	int ret = 0;
	char line[512];
	FILE *f = fopen(fname, "r");
	while(1) {
		if(!fgets(line, 512, f)) 
		      break;
		ret++;
	}
	fclose(f);
	return ret;
}


int main(int argc, char *argv[])
{
	FILE *f;
	char file_name[512];
	float *band;
	int nr_files = 0;
	int done_files = 0;
	spectgen_handle handle;
	if(argc < 2) {
		printf("Usage: %s <file list>\n", argv[0]);
		exit(-1);
	}

	nr_files = get_nr_lines(argv[1]);
	
	f = fopen(argv[1], "r");
	if(!f) {
		printf("Cannot open %s to read\n", argv[1]);
		exit(-1);
	}

	init_stats();

	while(1) {
		float perc = 100.0 * (float)done_files / (float)nr_files;
		unsigned int nbands = NBANDS;
		done_files++;
		progress(perc, stderr);

		if(!fgets(file_name, 512, f)) {
			break;
		}
		chomp_name(file_name);
		if(spectgen_open(&handle, file_name, SPECT_WINDOW_SIZE, SPECT_STEP_SIZE, BARK_SCALE, CEPSTOGRAM, &nbands)) {
			continue;
		}
		if(spectgen_start(handle)) {
			spectgen_close(handle);
			continue;
		}
		while((band = spectgen_pull(handle)) != NULL) {
			process_band(band);
			free(band);
		}
		spectgen_close(handle);
	}
	fclose(f);

	print_stats();

	return 0;
}
