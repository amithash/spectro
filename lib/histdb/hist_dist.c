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
#include <float.h>
#include "histdb.h"

#define ABS(val) ((val) > 0 ? (val) : (-1 * (val)))
#define SQR(val) ((val) * (val))

static float kl_distance(float *a, float *b, unsigned int len);
static float jeffery_distance(float *a, float *b, unsigned int len);
static float expected_difference(float *a, float *b, unsigned int len);
static float expected_value_difference(float *a, float *b, unsigned int len);
static float k_divergance(float *a, float *b, unsigned int len);
static float hellinger_distance(float *a, float *b, unsigned int len);
static float euclidean_distance(float *a, float *b, unsigned int len);
static float jensen_distance(float *a, float *b, unsigned int len);

static const dist_t supported_distances[] = {
	{KL_DIVERGANCE, "KL Divergance", kl_distance},
	{JEFFREYS_DIVERGANCE, "Jeffreys Divergance", jeffery_distance},
	{EXPECTED_VALUE_DIFFERENCE, "Expected value difference", expected_value_difference},
	{EXPECTED_DIFFERENCE, "Expected difference", expected_difference},
	{K_DIVERGANCE, "K Divergance", k_divergance},
	{HELLINGER_DIVERGANCE, "Hellinger divergance", hellinger_distance},
	{EUCLIDEAN_DISTANCE, "Euclidean distance", euclidean_distance},
	{JENSEN_DIVERGANCE, "Jenson divergance", jensen_distance},
};

void get_supported_distances(dist_t **dist)
{
	*dist = (dist_t *)supported_distances;
}

float hist_distance_ext(hist_t *hist1, hist_t *hist2, distance_func_t func)
{
	int i;
	float dist = 0;

	if(!func)
	      return 0;

	for(i = 0; i < NBANDS; i++) {
		dist += func(
				hist1->spect_hist[i],
				hist2->spect_hist[i],
				SPECT_HIST_LEN
			);
	}
	return dist;
}

float hist_distance(hist_t *hist1, hist_t *hist2, hist_dist_func_t dist_type)
{
	if(dist_type < DISTANCE_START || dist_type >= DISTANCE_END)
	      return 0;

	return hist_distance_ext(hist1, hist2, supported_distances[dist_type].func);
}

#define ALMOST_ZERO 0.0001
static void hist_raise(float *a, unsigned int len)
{
	int i;
	float div_by = 1.0;
	int touched = 0;
	for(i = 0; i < len; i++) {
		if(a[i] < ALMOST_ZERO) {
			div_by += (ALMOST_ZERO - a[i]);
			a[i] = ALMOST_ZERO;
			touched = 1;
		}
	}
	if(!touched)
	      return;

	for(i = 0; i < len; i++) {
		a[i] = a[i] / div_by;
	}
}

static float k_divergance(float *a, float *b, unsigned int len)
{
	int i;
	float dist = 0;
	hist_raise(a,len);
	hist_raise(b,len);
	for(i = 0; i < len; i++) {
		dist += a[i] * log(2 * a[i] / (a[i] + b[i]));
	}
	return dist;
}

static float expected_difference(float *a, float *b, unsigned int len)
{
	float dist = 0;
	int i;
	for(i = 0; i < len; i++) {
		float x = (float)i / (float)(len - 1);
		dist += x * ABS(a[i] - b[i]);
	}
	return dist;
}

static float expected_value_difference(float *a, float *b, unsigned int len)
{
	float exp_a = 0;
	float exp_b = 0;
	int i;
	for(i = 0; i < len; i++) {
		float x = (float)i / (float)(len - 1);
		exp_a += x * a[i];
		exp_b += x * b[i];
	}
	return ABS(exp_a - exp_b);
}

static float kl_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	float log_2 = log(2);
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	hist_raise(a, len);
	hist_raise(b, len);
	for(i = 0; i < len; i++) {
		dist += (a[i]) * log(a[i] / b[i]) / log_2;
	}
	return dist;
}
static float jeffery_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	float log_2 = log(2);
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	hist_raise(a, len);
	hist_raise(b, len);
	for(i = 0; i < len; i++) {
		dist += (a[i] - b[i]) * log(a[i] / b[i]) / log_2;
	}
	return dist;
}
static float euclidean_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	for(i = 0; i < len; i++) {
		dist += SQR(a[i] - b[i]);
	}
	return sqrt(dist);
}

static float jensen_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	hist_raise(a, len);
	hist_raise(b, len);
	for(i = 0; i < len; i++) {

		dist +=(((a[i] * log(a[i])) + (b[i] * log(b[i]))) / 2) - 
		    (((a[i] + b[i]) / 2) * log((a[i]+b[i]) / 2));
	}
	return dist;
}
static float hellinger_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	for(i = 0; i < len; i++) {
		dist += sqrt(a[i] * b[i]);
	}
	return dist >= 1.0 ? 0 : sqrt(1.0 - dist);
}

int hist_get_similar(
	hist_t *list, unsigned int len, int this_i, /* Input */
	int n, int *ind, float *dist,              /* Output */
	hist_dist_func_t dist_type)
{
	if(dist_type < DISTANCE_START || dist_type >= DISTANCE_END)
	      return -1;
	return hist_get_similar_ext(list, len, this_i, n, ind, dist, 
				supported_distances[dist_type].func);

}


int hist_get_similar_ext(
	hist_t *list, unsigned int len, int this_i, /* Input */
	int n, int *ind, float *dist,              /* Output */
	distance_func_t func
)
{
	int i,j,k;
	float *dlist;
	for(i = 0; i < n; i++) {
		ind[i] = -1;
		dist[i] = FLT_MAX;
	}
	dlist = (float *)calloc(len, sizeof(float));
	if(!dist)
	      return -1;
	for(i = 0; i < len; i++) {
		dlist[i] = hist_distance_ext(&list[this_i], &list[i], func);
	}
	for(k = 0; k < n; k++) {
		for(i = 0; i < len; i++) {
			float idist = dlist[i];
			if(i == this_i)
				continue;
			if(list[i].exclude != 0)
			      continue;
			for(j = 0; j < n; j++) {
				if(idist <= dist[j]) {
					ind[j] = i;
					dist[j] = idist;
					break;
				}
			}
		}
	}
	free(dlist);
	return 0;
}

