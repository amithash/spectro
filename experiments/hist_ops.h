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
#ifndef __HIST_OPS_H_
#define __HIST_OPS_H_

void cent_clear(void *hist);
void cent_accum(void *out, void *in);
void cent_final(void *out, void *in, unsigned int len);
void cent_copy(void *out, void *in);
void *hist_ind(void *_data, int i);
void *hist_calloc(int len);
float hist_dist(void *a, void *b);

#endif
