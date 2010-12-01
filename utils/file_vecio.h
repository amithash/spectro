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

#ifndef __FILE_VECIO_H_
#define __FILE_VECIO_H_

int write_uint(int fd, unsigned int val);
int read_uint(int fd, unsigned int *val);
int write_float_vec(int fd, float *vec, unsigned int len);
int read_float_vec(int fd, float *vec, unsigned int len);
int write_char_vec(int fd, char *vec, unsigned int len);
int read_char_vec(int fd, char *vec, unsigned int len);

#endif
