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
#include "kmeans.h"
#include "histdb.h"
#include "distance_matrix.h"
#include "spect-config.h"
#include "getopt_easy.h"
#include <math.h>
#include <string.h>
#include "quicksort.h"

float matrix_dist_cb(void *_a, void *_b)
{
	hist_t *a = (hist_t *)_a;
	hist_t *b = (hist_t *)_b;
	if(!a || !b)
	      return 0;
	return hist_distance(a, b, HELLINGER_DIVERGANCE);
}

void *matrix_index_cb(void *_hist_list, int ind)
{
	hist_t *hist_list = (hist_t *)_hist_list;
	return &hist_list[ind];
}

void print_perc(float perc)
{
	progress(perc, stdout);
}

typedef struct {
	float val;
	unsigned int i;
	unsigned int j;
} dist_touple_t;

void *touple_index(void *arr, int ind)
{
	dist_touple_t *t = (dist_touple_t *)arr;
	return &t[ind];
}
int touple_compare(void *_a, void *_b)
{
	dist_touple_t *a = (dist_touple_t *)_a;
	dist_touple_t *b = (dist_touple_t *)_b;
	if(a->val < b->val)
	      return 1;
	return 0;
}


int main(int argc, char *argv[])
{
	hist_t *hist_list = NULL;
	unsigned int hist_len = 0;
	char *hdb_file = NULL;
	int rc;
	dist_touple_t *touple = NULL;
	dist_matrix_t *dist;
	int k;
	getopt_easy_opt_t opt[] = {
		{"h:", STRING, (void *)&hdb_file}
	};

	if((rc = getopt_easy(&argc, &argv, opt, 1))) {
		printf("Getopt failed with rc = %d\n", rc);
		exit(-1);
	}
	if(!hdb_file) {
		printf("USAGE: %s -h <Path to hdb file>\n", argv[0]);
		exit(-1);
	}

	if(read_histdb(&hist_list, &hist_len, hdb_file)) {
		printf("Opening hdb:%s failed\n", hdb_file);
		exit(-1);
	}
	printf("Computing distance matrix on %d elements\n", hist_len);

	dist = create_dist_matrix(hist_list, hist_len,
				matrix_dist_cb, 
				matrix_index_cb,
				print_perc, 2); /* TODO: Configure nr threads */

	touple = malloc(sizeof(dist_touple_t) * dist->len);
	if(!touple) {
		printf("Alloc failure\n");
		goto touple_failed;
	}
	for(k = 0; k < dist->len; k++) {
		touple[k].val = dist->data[k];
		touple[k].i = (unsigned int)((1.0 + sqrt(1.0 + 8.0 * (float)k))/2);
		touple[k].j = ((touple[k].i * (touple[k].i - 1)) / 2);
	}
	printf("Sorting %d elements...\n", dist->len);
	touple = quicksort(touple, dist->len, sizeof(dist_touple_t), touple_index, touple_compare);
	printf("Sorted!\n");

	for(k = 0; k < dist->len - 1; k++) {
		if(touple[k].val > touple[k + 1].val) {
			printf("Sorting has failed!\n");
			break;
		}
	}
	free(touple);
touple_failed:
	delete_dist_matrix(dist);

	return 0;
}
	


