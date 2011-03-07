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
#include <string.h>
#include <math.h>
#include "histdb.h"
#include <pthread.h>
#include <float.h>

#define NMAX_DEFAULT 10
#define INIT_DIST DBL_MAX

typedef struct {
	int ind;
	float dist;
} similar_t;

int get_most_similar(hist_t *list, unsigned int len, int this_i, int n, similar_t **_out)
{
	similar_t *out = NULL;
	int i,j,k;
	float *dlist;
	*_out = NULL;
	if((out = calloc(n, sizeof(similar_t))) == NULL) {
		return -1;
	}
	if((dlist = (float *)calloc(len, sizeof(float))) == NULL)  {
		free(out);
		return -1;
	}
	for(i = 0; i < len; i ++) {
		dlist[i] = hist_distance(&list[this_i], &list[i], HELLINGER_DIVERGANCE);
	}

	for(i = 0; i < n; i++) {
		out[i].ind = -1;
		out[i].dist = FLT_MAX;
	}
	for(k = 0; k < n; k++) {
		for(i = 0; i < len; i++) {
			if(i == this_i)
				continue;
			float idist = dlist[i];
			for(j = 0; j < n; j++) {
				if(idist <= out[j].dist) {
					out[j].dist = idist;
					out[j].ind  = i;
					break;
				}
			}
		}
	}
	free(dlist);

	*_out = out;
	return 0;
}


int main(int argc, char *argv[])
{
	unsigned int len;
	int i;
	hist_t *hist_list;
	int ref_ind = -1;
	int maxes_len = NMAX_DEFAULT;
	similar_t *similar = NULL;

	if(argc < 3) {
		printf("USAGE: HIST_DB <music file>\n");
		exit(-1);
	}
	if(argc == 4) {
		maxes_len = atoi(argv[3]);
	}

	if(read_histdb(&hist_list, &len, argv[1])) {
		printf("Could not read hist db: %s\n", argv[1]);
		exit(-1);
	}
	for(i = 0; i < len; i++) {
		if(strcmp(hist_list[i].fname, argv[2]) == 0) {
			ref_ind = i;
			break;
		}
	}
	if(ref_ind == -1) {
		printf("Cound not find %s in db\n",argv[2]);
		exit(-1);
	}
	if(get_most_similar(hist_list, len, ref_ind, maxes_len, &similar)) {
		printf("Malloc failed!\n");
		exit(-1);
	}

	printf("Distance\t%-40s%-30s%-30s\n","Title", "Artist", "Album");
	printf("---------------------------------------------------------------------------------------------------------------------------------\n");
	for(i = 0; i < maxes_len; i++) {
		if(similar[i].ind == -1) {
			printf("WAAAAAAAA\n");
		      continue;
		}

		printf("%f\t%-40s%-30s%-30s\n", similar[i].dist, 
					hist_list[similar[i].ind].title, 
					hist_list[similar[i].ind].artist, 
					hist_list[similar[i].ind].album);
	}
	free(hist_list);
	free(similar);

	return 0;
}
