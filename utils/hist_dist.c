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

	for(col = 0; col < NBANDS; col++) {
		dist += sy_kl_distance(hist1->spect_hist[col],
				            hist2->spect_hist[col],
				            SPECT_HIST_LEN);
	}
	return dist;
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

