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
#include <pthread.h>
#include "decoder_backend.h"
#ifndef _DECODER_BACKEND_H_
#define _DECODER_BACKEND_H_

#include "decoder_i.h"


struct decoder_buffer_type {
	unsigned int frate;
	float        *buffer;
	unsigned int len;
};

struct decoder_backend_ops {
	int (*open)(void **handle, void *client_handle, char *fname);
	int (*close)(void *handle);
	void (*decode)(void *handle);
};

struct decoder_backend_generic_handle
{
	void *client_handle;
};

/* Decoder visible interface */
int decoder_backend_open(struct decoder_handle_struct *handle, char *fname);
void decoder_backend_decode(struct decoder_handle_struct *handle);
int decoder_backend_close(struct decoder_handle_struct *handle);
void decoder_backend_supported_extensions(char **extensions, unsigned int *out_len);


/* Backend visible interface */
int decoder_backend_push(void *handle, float *data, unsigned int len, unsigned int frate);
void decoder_backend_register(struct decoder_backend_ops *ops, char *extension);

#endif
