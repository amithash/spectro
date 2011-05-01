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
#ifndef __FDWT_H_
#define __FDWT_H_
typedef enum
{
	DWT_FORWARD = 0,
	DWT_REVERSE
} dwt_dir_t;

typedef void * dwt_plan_t;

void dwt_destroy_plan(dwt_plan_t plan);


int dwt_plan_size(dwt_plan_t _plan);
/* Note:
 * if in_vec == out_vec, then an in-place transform is performed. During the transform's
 * execution, the contents of in_vec is undefined.
 * Else, extra buffers are created in order not to touch in_vec. This is a bit extra
 * processing in terms of a memcpy before the transform starts and after the transform
 * ends. Also extra memory is used to create the temp bufs.
 */
dwt_plan_t dwt_create_plan(int size, float *in_vec, float *out_vec, dwt_dir_t dir);



void dwt_execute(dwt_plan_t plan);

#endif

