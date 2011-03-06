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

#ifndef __BPM_H_
#define __BPM_H_

#include "spect-config.h"

#define WINDOW_LEN 3
int spect2bpm(float bpm[BPM_LEN], spect_t *spect);
int _spect2bpm(float bpm[NBANDS][BPM_LEN], spect_t *spect);
void plot_bpm(float bpm[BPM_LEN]);
void _plot_bpm(float bpm[NBANDS][BPM_LEN]);
void smooth_vec(float *vec, unsigned int len, unsigned int window);

#define smooth_bpm(bpm) smooth_vec(bpm, BPM_LEN, WINDOW_LEN)

#endif
