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
#include <math.h>
#include "kmeans.h"

typedef struct {
	float x;
	float y;
} point_t;

float distance(void *_a, void *_b)
{
	point_t *a = (point_t *)_a;
	point_t *b = (point_t *)_b;

	return sqrt(pow(a->x - b->x, 2) + pow(a->y - b->y, 2));
}

void zero(void *_a)
{
	point_t *a = (point_t *)_a;
	a->x = 0;
	a->y = 0;
}

void copy(void *_out, void *_in) 
{
	point_t *out = (point_t *)_out;
	point_t *in  = (point_t *)_in;

	out->x = in->x;
	out->y = in->y;
}

void accum(void *_out, void *_in)
{
	point_t *out = (point_t *)_out;
	point_t *in  = (point_t *)_in;

	out->x += in->x;
	out->y += in->y;
}

void final(void *_out, void *_in, unsigned int len)
{
	point_t *out = (point_t *)_out;
	point_t *in  = (point_t *)_in;
	
	out->x = in->x / (float)len;
	out->y = in->y / (float)len;
}

void *index(void *_data, int ind)
{
	point_t *data = (point_t *)_data;
	return (void *)&data[ind];
}

void *_calloc(int len)
{
	point_t *data = calloc(len, sizeof(point_t));
	return (void *)data;
}

kmeans_ops_t kmeans_ops = {
	distance,
	zero,
	copy,
	accum,
	final,
	index,
	_calloc
};

int main(void)
{
	point_t points[8] = {
		{0,0},
		{0.5,0.5},
		{5,0},
		{5.5, 0},
		{0,5},
		{0,5.5},
		{5,5},
		{5.5,5.5}

	};
	clustered_data_t *out;
	int err;
	int i;

	err = cluster(4, &points, 8, kmeans_ops, &out);
	if(err != 0) {
		if(err == -2) {
			fprintf(stderr, "Max iterations reached without convergance\n");
		} else {
			fprintf(stderr, "Cluster returned error code = %d\n", err);
		}
		goto cleanup;
	}
	printf("---------out------------\n");
	for(i = 0; i < 8; i++) {
		point_t *pt = (point_t *)out[i].data;
		printf("(%.2f,%.2f):\t\t%d\n", pt->x, pt->y, out[i].id);
	}

cleanup:
	free(out);

	return 0;
}

