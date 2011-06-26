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
#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <stdbool.h>
#include "plot.h"

#define error(fmt) fprintf(stderr, fmt)

#define PM3D_EXP

const char *standard_types[PLOT_MAX] = {
	"points",
	"lines"
};

#define IND(arr,i,j,len) arr[(i * len) + j]

int plot(float *x, float *y, unsigned int ncol, unsigned int len, plot_type_t type, int debug)
{
	FILE *f;
	int i,j;

	if(type >= PLOT_MAX) {
		return -1;
	}

	if(!(f = fopen("__data.txt", "w"))) {
		error("Failed to create temp file\n");
		return -1;
	}
	for(i = 0; i < len; i++) {
		if(x)
		      fprintf(f, "%.4f %.4f", x[i], IND(y,0,i,len));
		else
		      fprintf(f, "%.4f", IND(y, 0, i, len));

		for(j = 1; j < ncol; j++)
			fprintf(f, " %.4f", IND(y,j,i,len));
		fprintf(f, "\n");
	}
	fclose(f);

	if(!(f = fopen("__out.gp", "w"))) {
		error("Failed to create gp script\n");
		return -1;
	}
	fprintf(f, "plot \\\n");
	for(i = 0; i < ncol; i++) {
		if(x)
			fprintf(f, "\"__data.txt\" using 1:%d with %s title \"data %d\"", i + 2, standard_types[type], i);
		else
			fprintf(f, "\"__data.txt\" using %d with %s title \"data %d\"", i + 1, standard_types[type], i);
		if(i < (ncol - 1)) {
			fprintf(f, ", \\");
		}
		fprintf(f, "\n");
	}
	fprintf(f, " pause -1\n");
	fclose(f);
	if(system("gnuplot __out.gp"))
	      printf("call to gnuplot failed\n");
	if(!debug) {
		if(system("rm -f __out.gp __data.txt"))
		      printf("removing outs failed\n");
	}
	return 0;
}

int plot3d(float *data, unsigned int len, int debug)
{
	FILE *f;
	int i,j;

	if(!(f = fopen("__data.txt", "w"))) {
		error("Failed to create temp file\n");
		return -1;
	}
	for(i = 0; i < len; i++) {
		for(j = 0; j < 3; j++) {
			fprintf(f, "%.4f ", data[i * 3 + j]);
		}
		fprintf(f, "\n");
#ifdef PM3D_EXP
		if(i % 32 == 0)
		      fprintf(f, "\n");
#endif
	}
	fclose(f);

	if(!(f = fopen("__out.gp", "w"))) {
		error("Failed to create gp script\n");
		return -1;
	}
	fprintf(f, "set hidden3d\n");
#ifdef PM3D_EXP
	fprintf(f, "set pm3d at b\n");
#endif
	fprintf(f, "splot \"__data.txt\" using 1:2:3 with points title \"\"\n");
	fprintf(f, "pause -1\n");
	fclose(f);
	if(system("gnuplot __out.gp"))
	      printf("call to gnuplot failed\n");
	if(!debug) {
		if(system("rm -f __out.gp __data.txt"))
		      printf("removing outs failed\n");
	}
	return 0;
}

static void row_normalize(float *data, unsigned int len_y, unsigned int len_x, float to)
{
	int i,j;
	float val;
	for(i = 0; i < len_y; i++) {
		float min = FLT_MAX, max = 0;
		for(j = 0; j < len_x; j++) {
			val = data[(len_x * i) + j];
			if(val > max)
			      max = val;
			if(val < min)
			      min = val;
		}
		if(max == min)
		      continue;

		for(j = 0; j < len_x; j++) {
			data[(len_x * i) + j] = to * (data[(len_x * i) + j] - min) / (max - min);
		}
	}
}

static void col_normalize(float *data, unsigned int len_y, unsigned int len_x, float to)
{
	int i,j;
	float val;

	for(i = 0; i < len_x; i++) {
		float min = FLT_MAX, max = 0;
		for(j = 0; j < len_y; j++) {
			val = data[(len_x * j) + i];
			if(val > max)
			      max = val;
			if(val < min)
			      min = val;
		}
		if(max == min)
		      continue;
		for(j = 0; j < len_y; j++) {
			data[(len_x * j) + i] = to * (data[(len_x * j) + i] - min) / (max - min);
		}
	}

}

static void range(float *data, unsigned int len, float *_max, float *_min)
{
	int i;
	float max = 0, min = FLT_MAX;
	for(i = 0; i < len; i++) {
		if(data[i] > max)
		      max = data[i];
		if(data[i] < min)
		      min = data[i];
	}
	*_max = max;
	*_min = min;
}
#define STEPS 100
static float quantile(float *data, unsigned int len, float req_perc, bool greater_than, float max, float min)
{
	unsigned int nr_in_range = 0;
	int i;
	float perc = 0;
	float step;
	float quant;

	quant = greater_than ? max : min;
	step = (max - min) / STEPS;
	if(greater_than) {
		for(quant = max; quant >= min; quant -= step) {
			nr_in_range = 0;
			for(i = 0; i < len; i++) {
				if(data[i] > quant)
				      nr_in_range++;
			}
			perc = 100 * (float)nr_in_range / (float)len;
			if(perc > req_perc)
			      break;
		}
		printf("%f quantile greater than: %f range=[%f,%f] settling perc=%f\n", req_perc, quant, min, max, perc);
	} else {
		for(quant = min; quant <= max; quant += step) {
			nr_in_range = 0;
			for(i = 0; i < len; i++) {
				if(data[i] < quant)
				      nr_in_range++;
			}
			perc = 100 * (float)nr_in_range / (float)len;
			if(perc > req_perc)
			      break;
		}
		printf("%f quantile lesser than: %f range=[%f,%f] settling perc=%f\n", req_perc, quant, min, max, perc);
	}

	return quant;
}

static void all_normalize(float *data, unsigned int len_y, unsigned int len_x, float to)
{
	float max, min;
	float r_max, r_min;
	int i;

	range(data, len_x * len_y, &r_max, &r_min);

	min = quantile(data, len_x * len_y, 95.0, true, r_max, r_min);
	max = quantile(data, len_x * len_y, 95.0, false, r_max, r_min);

	if(max == min)
	      return;
	for(i = 0; i < len_x * len_y; i++) {
		if(data[i] > max)
		      data[i] = max;
		if(data[i] < min)
		      data[i] = min;
		data[i] = to * (data[i] - min) / (max - min);
	}
}

static void no_normalize(float *data, unsigned int len_y, unsigned int len_x, float to)
{
	int i;
	for(i = 0; i < len_x * len_y; i++) {
		data[i] = data[i] * to;
	}
}

static void normalize(float *data, unsigned int len_y, unsigned int len_x, norm_t norm, float to)
{
	int i;
	switch(norm)
	{
	    	case ROW_NORMALIZATION:
			row_normalize(data, len_y, len_x, to);
			break;
		case COL_NORMALIZATION:
		      col_normalize(data, len_y, len_x, to);
		      break;
		case ALL_NORMALIZATION:
		      all_normalize(data, len_y, len_x, to);
		      break;
		case NO_NORMALIZATION:
		      no_normalize(data, len_y, len_x, to);
		      break;
		default:
			break;
	}
	for(i = 0; i < len_x * len_y; i++) {
		if(!finite(data[i]))
		      data[i] = 0;
		if(data[i] > to)
		      data[i] = to;
		if(data[i] < 0)
		      data[i] = 0;
	}
}

int pgm(char *fname, float *_data, unsigned int len_x, 
	unsigned int len_y, background_t bg, foreground_t fg,
	norm_t norm)
{
	int i,j;
	char buf[256];
	FILE *f;
	float *data;
	int rc = -4;
	unsigned int ui_val;
	unsigned char val[3];
	unsigned int val_len;
	unsigned int mask = 0xff;
	unsigned int str_len;

	if(!_data)
	      return -1;
	f = fopen(fname, "w");
	if(!f)
	      return -2;

	data = (float *)malloc(len_x * len_y * sizeof(float));
	if(!data)
	      return -3;
	memcpy(data, _data, len_x * len_y * sizeof(float));


	if(fg == COLORED)
	      mask = 0xffffff;

	normalize(data, len_y, len_x, norm, (float)mask);

	if(fg == COLORED) {
		sprintf(buf, "P6\n%d %d\n255\n", len_x, len_y);
		val_len = 3 * sizeof(unsigned char);
	}
	else {
		sprintf(buf, "P5\n%d %d\n255\n", len_x, len_y);
		val_len = sizeof(unsigned char);
	}
	str_len = strlen(buf) * sizeof(char);
	if(write(fileno(f), buf, str_len) != str_len)
	      goto cleanup;

	for(i = 0; i < len_y; i++) {
		for(j = 0; j < len_x; j++) {
			ui_val = (unsigned int)data[(i * len_x) + j];

			val[0] = (unsigned char)((ui_val >> 0) & 0xff);
			if(bg == BACKGROUND_WHITE)
			      val[0] = 255 - val[0];

			if(fg == COLORED) {
				val[1] = (unsigned char)((ui_val >> 8) & 0xff);
				if(bg == BACKGROUND_WHITE)
					val[1] = 255 - val[1];
				val[2] = (unsigned char)((ui_val >> 16) & 0xff);
				if(bg == BACKGROUND_WHITE)
					val[2] = 255 - val[2];
			}
			if(write(fileno(f), val, val_len) != val_len)
			      goto cleanup;
		}
	}
	rc = 0;

cleanup:
	fclose(f);
	free(data);
	return rc;
}

#ifdef PLOT_TEST
int main(void)
{
	float x[4] = {1,2,3,4};
	float y[1][4] = {{1,2,3,4}};

	plot(x, (float *)y, 1, 4, PLOT_LINES);

	return 0;
}
#endif
