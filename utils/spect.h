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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <string.h>


#ifndef __SPECT_H_
#define __SPECT_H_

#include "../spect-config.h"

#define FLOAT_UNEQUAL(num1, num2) ((num1 < (num2 - 0.00001)) || (num1 > (num2 + 0.00001)))

#define spect_error(fmt, par...) fprintf(stderr, "ERROR: " fmt "\n", ##par)
#define spect_warn(fmt, par...) fprintf(stderr, "ERROR: " fmt "\n", ##par)

typedef struct {
	unsigned int len;
	char         fname[FNAME_LEN];
	float        *spect[NBANDS];
} spect_t;

#define RM_SUCCESS             0
#define RM_INVALID_PTR_E       -1
#define RM_INVALID_FNAME_PTR_E -2
#define RM_INVALID_FNAME_E     -3
#define RM_OPEN_FAILED_E       -4
#define RM_LENGTH_NOT_SPECT_E   -5
#define RM_UNMATCHED_END_E     -6
#define RM_MP3_NOT_FOUND       -7
#define RM_MALLOC_FAILED_E     -8
#define RM_READ_FAILED_E       -9

extern const char *RM_RC[];
#define RM_RC_STR(rc) RM_RC[-1 * rc]

#define RMFL_SUCCESS                0
#define RMFL_INVALID_FNAME_PTR_E   -1
#define RMFL_INVALID_LIST_PTR_E    -2
#define RMFL_OPEN_FAILED_E         -3
#define RMFL_MALLOC_FAILED_E       -4
#define RMFL_THREAD_CREATE_E       -5

#define WML_SUCCESS                0
#define WML_INVALID_PTR_E          -1
#define WML_OPEN_FAILED_E          -2
#define WML_WRITE_E                -3

#define RML_SUCCESS                0
#define RML_INVALID_PTR_E          -1
#define RML_OPEN_FAILED_E          -2
#define RML_READ_E                 -3

/* Read a spect file spect is allocated, and its members are allocated.
 * when done, call free_spect(*spect), and free(spect) */
int read_spectf(char *name, spect_t *spect);

/* Read spect entry from db file. spect members are allocated.
 * when done, call free_spect(spect).
 */
int read_spect(int fd, spect_t *spect);

/* Write spect entry to db file.
 */
int write_spect(int fd, spect_t *spect);

/* takes spect file list in ifname, and spect db name in ofname.
 * Generates spectdb (*.sdb) as of name
 */
int combine_spect_list(char *ifname, 
		    char *ofname,
		   int nthreads);

/* Read or write the length from or to a spect db file */
int read_spect_db_len(int fd, unsigned int *len);
int write_spect_db_len(int fd, unsigned int len);

/* Get real start and end of spectrogram (the meat when there is content
 * other than 0's in the spectrogram */
void spect_get_edges(int *_start, int *_end, spect_t *spect);

/* Free members which are allocated */
void free_spect(spect_t *spect);

/* Allocate members with specified len 0 on success */
int alloc_spect(spect_t *spect, int len);

#endif
