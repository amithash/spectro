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
#ifndef _PLOT_H_
#define _PLOT_H_

typedef enum {
	PLOT_POINTS = 0,
	PLOT_LINES,
	PLOT_MAX
} plot_type_t;

typedef enum {
	BACKGROUND_BLACK,
	BACKGROUND_WHITE
} background_t;

typedef enum {
	GREYSCALE,
	COLORED
} foreground_t;

typedef enum {
	NO_NORMALIZATION,
	ROW_NORMALIZATION,
	COL_NORMALIZATION,
	ALL_NORMALIZATION
} norm_t;

int plot(float *x, float *y, unsigned int ncol, unsigned int len, plot_type_t type, int debug);
int plot3d(float *data, unsigned int len, int debug);
int pgm(char *fname, float *_data, unsigned int len_x, unsigned int len_y, 
			background_t bg, foreground_t fg, norm_t norm);

#endif
