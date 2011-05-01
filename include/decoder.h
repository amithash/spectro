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

#ifndef _DECODER_H_
#define _DECODER_H_

#ifdef __cplusplus
	extern "C" {
#endif

typedef void *decoder_handle;

int decoder_init(decoder_handle *handle);
int decoder_open(decoder_handle handle, char *fname);
int decoder_start(decoder_handle handle);
int decoder_close(decoder_handle handle);
void decoder_exit(decoder_handle handle);
void decoder_data_pull(decoder_handle handle, float **buffer, unsigned int *len, unsigned int *frate);
void decoder_supported_extensions(char **extensions, unsigned int *out_len);

#ifdef __cplusplus
	}
#endif 

#endif
