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
	1.05,	/* [100,   200)   */
	1.1,	/* [200,   300)   */
	1.14,	/* [300,   400)   */
	1.16,	/* [400,   510)   */
	1.2,	/* [510,   630)   */
	1.15,	/* [630,   770)   */
	1.11,	/* [770,   920)   */
	1.11,	/* [920,   1080)  */
	1.11,	/* [1080,  1270)  */
	1.11,	/* [1270,  1480)  */
	1.11,	/* [1480,  1720)  */
	1.11,	/* [1720,  2000)  */
	1.16,	/* [2000,  2320)  */
	1.16,	/* [2320,  2700)  */
	1.16,	/* [2700,  3150)  */
	1.14,	/* [3150,  3700)  */
	1.14,	/* [3700,  4400)  */
	2.0,	/* [4400,  5300)  */
	1.3,	/* [5300,  6400)  */
	1.14,	/* [6400,  7700)  */
	1.07,	/* [7700,  9500)  */
	1.05,	/* [9500,  12000) */
	1.07,	/* [12000, 15500) */
	1.11	/* [15500, inf)   */
};

#define ABS(val) ((val) > 0 ? (val) : (-1 * (val)))

/* Return the expected difference in loudness between
 * the two bands */
float loudness_diff(float *a, float *b, unsigned int len)
{
	int i;
	float diff = 0;
	for(i = 0; i < len; i++) {
		float loud = (float)i / (float)(len - 1);
		diff += (a[i] - b[i]) * loud;
	}
	return diff;
}

static float sy_kl_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	int i;
	float log_2 = log(2);
	for(i = 0; i < len; i++) {
		dist += (a[i] - b[i]) * log(a[i] / b[i]) / log_2;
	}
	return dist;
}

float hist_distance(hist_t *hist1, hist_t *hist2)
{
	int col;
	float dist = 0;
	float loudness = 0;

	for(col = 0; col < NBANDS; col++) {
		float loud;
		dist += sy_kl_distance(hist1->spect_hist[col],
				            hist2->spect_hist[col],
				            SPECT_HIST_LEN);
		loud = loudness_diff(hist1->spect_hist[col], 
					  hist2->spect_hist[col],
					  SPECT_HIST_LEN) * mask[col];
		loudness += loud * loud;
	}
	return  loudness;
}

int get_most_similar(hist_t *list, unsigned int len, int this_i, int n, similar_t **_out)
{
	similar_t *out = NULL;
	int i,j;
	*_out = NULL;
	if((out = calloc(n, sizeof(similar_t))) == NULL) {
		return -1;
	}
	for(i = 0; i < n; i++) {
		out[i].ind = -1;
		out[i].dist = FLT_MAX;
	}

	for(i = 0; i < len; i++) {
		if(i == this_i)
		      continue;
		if(
			strcmp(list[i].artist, list[this_i].artist) == 0 &&
			strcmp(list[i].title, list[this_i].title) == 0)
		      continue;
		float idist = hist_distance(&list[this_i], &list[i]);
		for(j = 0; j < n; j++) {
			if(idist < out[j].dist) {
				out[j].dist = idist;
				out[j].ind  = i;
				break;
			}
		}
	}

	*_out = out;
	return 0;
}

