/*******************************************************************************
    This file is part of spectro
    Copyright (C) 2010  Amithash Prasad <amithash@gmail.com>

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
#include "hist.h"

int main(int argc, char *argv[]) {
	hist_t *hist = NULL;
	hist_t *hist_list = NULL;
	int rc;
	FILE *pgf;
	char outf_name[256] = "./tmp.ppm";
	char buf[100];
	int i,j;
	unsigned int len;

	if(argc < 3) {
		printf("USAGE: %s <spectdb> FILE.spect4 (Optional output ppm file default: tmp.ppm)\n", argv[0]);
		exit(-1);
	}
	if((rc = read_hist_db(&hist_list, &len, argv[1])) != 0) {
		printf("Reading spect file %s failed with error=%s\n",argv[1], RM_RC_STR(rc));
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
		spect_error("Could not find %s in db", argv[2]);
		goto cleanup;
	}

	if(argc == 4) {
		strcpy(outf_name, argv[3]);
	}
	pgf = fopen(outf_name, "w");
	if(pgf == NULL) {
		printf("Creating %s failed!\n", outf_name);
		exit(-1);
	}
	sprintf(buf, "P6\n%d %d\n255\n", SPECT_HIST_LEN, NBANDS);
	write(fileno(pgf), buf, strlen(buf) * sizeof(char));
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			unsigned int ui_val = (unsigned int)(hist->spect_hist[i][j] * 0x1000000);
			unsigned char val[3];

			val[0] = (unsigned char)((ui_val >> 0) & 0xff);
			val[1] = (unsigned char)((ui_val >> 8) & 0xff);
			val[2] = (unsigned char)((ui_val >> 16) & 0xff);

			if(write(fileno(pgf), &val, 3 * sizeof(unsigned char)) != 3 * sizeof(unsigned char)) {
				printf("Write error!\n");
			}
		}
	}
	fclose(pgf);
cleanup:
	free(hist_list);

	return 0;
}

