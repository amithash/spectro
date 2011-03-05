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

#ifndef _HIST_H_
#define _HIST_H_

#include "spect-config.h"

typedef struct {
	char fname[FNAME_LEN];
	char title[TITLE_LEN];
	char artist[ARTIST_LEN];
	char album[ALBUM_LEN];
	unsigned int track;
	unsigned int length;
	float spect_hist[NBANDS][SPECT_HIST_LEN];
} hist_t;

hist_t *gen_hist(char *fname);
int read_histdb(hist_t **hist, unsigned int *len, char *fname);
int write_histdb(hist_t *hist, unsigned int len, char *fname);

#endif
