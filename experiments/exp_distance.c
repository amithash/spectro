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
#include <math.h>
#include "histdb.h"
#include <pthread.h>
#include <dirent.h>
#include <sys/stat.h>
#include <float.h>

#define NMAX_DEFAULT 10
#define SMALL_VAL 0.00001

#define ABS(a) ((a) > 0 ? (a) : (-1 * (a)))
float experimental_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	int i;

	return dist;
}

int main(int argc, char *argv[])
{
	unsigned int len;
	int i;
	hist_t *hist_list;
	int ref_ind = -1;
	int maxes_len = NMAX_DEFAULT;
	char mp3_path[PATH_MAX] = "";
	int *ind_p;
	float *dist_p;

	if(argc < 3) {
		printf("USAGE: HIST_DB <music file>\n");
		exit(-1);
	}
	if(argc >= 4) {
		maxes_len = atoi(argv[3]);
	}

	ind_p = calloc(maxes_len, sizeof(int));
	dist_p = calloc(maxes_len, sizeof(float));
	if(!ind_p || !dist_p) {
		printf("Malloc failure!\n");
		exit(-1);
	}

	if(read_histdb(&hist_list, &len, argv[1])) {
		printf("Could not read hist db: %s\n", argv[1]);
		exit(-1);
	}
	if(!realpath(argv[2], mp3_path)) {
		printf("Could not get real path\n");
		exit(-1);
	}

	for(i = 0; i < len; i++) {
		if(strcmp(hist_list[i].fname, mp3_path) == 0) {
			ref_ind = i;
			break;
		}
	}
	if(ref_ind == -1) {
		printf("Cound not find %s in db\n",argv[2]);
		exit(-1);
	}
	if(hist_get_similar_ext(hist_list, len, &ref_ind, 1, maxes_len, 
			ind_p, dist_p, experimental_distance)) {
		printf("Malloc failed!\n");
		exit(-1);
	}

	printf("Distance\t%-40s%-30s%-30s\n","Title", "Artist", "Album");
	printf("---------------------------------------------------------------------------------------------------------------------------------\n");
	for(i = 0; i < maxes_len; i++) {
		if(ind_p[i] == -1) {
			printf("WAAAAAAAA\n");
		      continue;
		}

		printf("%f\t%-40s%-30s%-30s\n", dist_p[i], 
					hist_list[ind_p[i]].title, 
					hist_list[ind_p[i]].artist, 
					hist_list[ind_p[i]].album);
	}
	free(hist_list);
	free(ind_p);
	free(dist_p);

	return 0;
}
