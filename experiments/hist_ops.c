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

void cent_clear(void *h)
{
	int i,j;
	hist_t *hist = (hist_t *)h;
	hist->fname[0] = '\0';
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			hist->spect_hist[i][j] = 0;
		}
	}
}

void cent_accum(void *_out, void *_in)
{
	int i,j;
	hist_t *out = (hist_t *)_out;
	hist_t *in  = (hist_t *)_in;
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			out->spect_hist[i][j] += sqrt(in->spect_hist[i][j]);
		}
	}
}

void cent_final(void *_out, void *_in, unsigned int len)
{
	int i,j;
	hist_t *out = (hist_t *)_out;
	hist_t *in  = (hist_t *)_in;

	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			out->spect_hist[i][j] = pow(in->spect_hist[i][j] / len, 2);
		}
	}
}

void cent_copy(void *_out, void *_in) 
{
	hist_t *out = (hist_t *)_out;
	hist_t *in  = (hist_t *)_in;
	memcpy(out, in, sizeof(hist_t));
}

void *hist_ind(void *_data, int i)
{
	hist_t *data = (hist_t *)_data;
	return (void *)&data[i];
}

void *hist_calloc(int len)
{
	hist_t *ret;

	ret = (hist_t *)calloc(len, sizeof(hist_t));
	return (void *)ret;
}

float hist_dist(void *a, void *b)
{
	return hist_distance((hist_t *)a, (hist_t *)b, KL_DIVERGANCE);
}

