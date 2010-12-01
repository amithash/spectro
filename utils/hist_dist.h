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

#ifndef _HIST_DIST_H_
#define _HIST_DIST_H_

#include "hist.h"

typedef struct {
	int ind;
	float dist;
} similar_t;

float hist_distance(hist_t *hist1, hist_t *hist2);
int get_most_similar(hist_t *list, unsigned int len, int this_i, int n, similar_t **_out);

#endif
