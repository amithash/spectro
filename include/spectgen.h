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

#ifndef __SPECTGEN_H_
#define __SPECTGEN_H_

#ifdef __cplusplus
	extern "C" {
#endif

typedef enum {
	MEL_SCALE = 0,
	BARK_SCALE,
	SEMITONE_SCALE,
	MAX_SCALE
} scale_t;

typedef enum {
	SPECTOGRAM,
	CEPSTOGRAM
} spect_method_t;


typedef void *spectgen_handle;

int spectgen_open(spectgen_handle *handle, char *fname, unsigned int window_size, unsigned int step_size, scale_t scale, spect_method_t method, unsigned int *nbands);
int spectgen_close(spectgen_handle handle);
int spectgen_read(spectgen_handle handle, float **band_array, unsigned int nbands);
int spectgen_start(spectgen_handle handle);
float *spectgen_pull(spectgen_handle handle);
unsigned int spectgen_frate(spectgen_handle _handle);

#ifdef __cplusplus
	}
#endif 

#endif
