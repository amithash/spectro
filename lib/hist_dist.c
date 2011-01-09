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

#include "hist_dist.h"
#include <float.h>

const float mask[NBANDS] = {
    0.823529,
    0.897436,
    0.921053,
    0.921053,
    0.921053,
    0.917464,
    0.900932,
    0.879403,
    0.876772,
    0.880981,
    0.885633,
    0.896203,
    0.909091,
    0.932678,
    0.959176,
    0.983516,
    1.000000,
    0.974123,
    0.927985,
    0.864979,
    0.819767,
    0.818741,
    0.736842,
    0.628727
};

#define ABS(val) ((val) > 0 ? (val) : (-1 * (val)))
#define SQR(val) ((val) * (val))

#define KL_DIVERGANCE
//#define JEFFREYS_DIVERGANCE
//#define JENSONS_DIVERGANCE
//#define EXPECTED_VALUE_DIFFERENCE
//#define EXPECTED_DIFFERENCE
//#define K_DIVERGANCE
//#define HELLINGER_DIVERGANCE
//#define EUCLIDIAN_DISTANCE
//#define MAX_DISTANCE


static float distance(float *a, float *b, unsigned int len)
{
	int i;
	float dist = 0;
#if defined(JEFFREYS_DIVERGANCE)
	float log_2 = log(2);
	for(i = 0; i < len; i++) {
		dist += (a[i] - b[i]) * log(a[i] / b[i]) / log_2;
	}
#elif defined(JENSONS_DIVERGANCE)
	float bits_a, bits_b, bits_avg;
	for(i = 0; i < len; i++) {
		bits_a = a[i] * log(a[i]);
		bits_b = b[i] * log(b[i]);
		bits_avg = ((a[i] + b[i]) / 2) * log((a[i] + b[i]) / 2);
		dist += ((bits_a + bits_b) / 2) - bits_avg;
	}
	return dist;
#elif defined(KL_DIVERGANCE)
	float log_2 = log(2);
	for(i = 0; i < len; i++) {
		dist += a[i] * log(a[i] / b[i]) / log_2;
	}
#elif defined(EXPECTED_DIFFERENCE)
	for(i = 0; i < len; i++) {
		float x = (float)i / (float)(len - 1);
		dist += x * ABS(a[i] - b[i]);
	}
#elif defined(EXPECTED_VALUE_DIFFERENCE)
	float exp_a, exp_b;
	for(i = 0; i < len; i++) {
		float x = (float)i / (float)(len - 1);
		exp_a += x * a[i];
		exp_b += x * b[i];
	}
	dist = ABS(exp_a - exp_b);
#elif defined(K_DIVERGANCE)
	for(i = 0; i < len; i++) {
		dist += a[i] * log(2 * a[i] / (a[i] + b[i]));
	}
#elif defined(HELLINGER_DIVERGANCE)
	for(i = 0; i < len; i++) {
		dist += sqrt(a[i] * b[i]);
	}
	dist = sqrt(1 - dist);
#elif defined(EUCLIDIAN_DISTANCE)
	for(i = 0; i < len; i++) {
		dist += SQR(a[i] - b[i]);
	}
	dist = sqrt(dist);
#elif defined(MAX_DISTANCE)
	float diff;
	for(i = 0; i < len; i++) {
		diff = ABS(a[i] - b[i]);
		if(diff > dist)
		      dist = diff;
	}
#elif defined(MAX_DISTANCE)
	float diff;
	float max = 0, min = FLT_MAX, avg = 0;
	for(i = 0; i < len; i++) {
		diff = ABS(a[i] - b[i]);
		if(diff > max)
		      max = diff;
		if(diff < min)
		      min = diff;
		avg += diff;
	}
	avg /= (float)len;
	dist = avg > 0 ? (max - min) /avg  : FLT_MAX;
#else
#error "Need at least one"
#endif
	return dist;
}

float hist_distance(hist_t *hist1, hist_t *hist2)
{
	int col;
	float dist = 0;

	for(col = 0; col < NBANDS; col++) {
		dist += (distance(hist1->spect_hist[col],
				            hist2->spect_hist[col],
				            SPECT_HIST_LEN));
	}
	return  dist;
}

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
		dlist[i] = hist_distance(&list[this_i], &list[i]);
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

