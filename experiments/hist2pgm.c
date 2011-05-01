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
#include <string.h>
#include "histdb.h"
#include "plot.h"

int main(int argc, char *argv[]) {
	hist_t *hist = NULL;
	hist_t *hist_list = NULL;
	int rc;
	char outf_name[256] = "./tmp.ppm";
	int i;
	unsigned int len;

	if(argc < 3) {
		printf("USAGE: %s <spectdb> FILE.spect4 (Optional output ppm file default: tmp.ppm)\n", argv[0]);
		exit(-1);
	}
	if((rc = read_histdb(&hist_list, &len, argv[1])) != 0) {
		printf("Reading spect file %s failed\n",argv[1]);
		exit(-1);
	}

	for(i = 0; i < len; i++) {
		if(strcmp(hist_list[i].fname, argv[2]) == 0) {
			printf("Found at index %d\n", i);
			hist = &hist_list[i];
			break;
		}
	}
	if(!hist) {
		printf("Could not find %s in db\n", argv[2]);
		goto cleanup;
	}

	if(argc == 4) {
		strcpy(outf_name, argv[3]);
	}

	if((rc = pgm(outf_name, (float *)hist->spect_hist, SPECT_HIST_LEN, NBANDS, BACKGROUND_BLACK, COLORED, ROW_NORMALIZATION))){
		printf("Write to %s failed with rc = %d\n",outf_name,rc);
		exit(-1);
	}

cleanup:
	free(hist_list);
	return 0;
}

