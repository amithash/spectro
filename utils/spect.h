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
	char         fname[FNAME_LEN];
	unsigned char spect[NBANDS][SPECTLEN];
	unsigned int valid;
	unsigned int len;
} spect_t;

#define RM_SUCCESS             0
#define RM_INVALID_PTR_E       -1
#define RM_INVALID_FNAME_PTR_E -2
#define RM_INVALID_FNAME_E     -3
#define RM_OPEN_FAILED_E       -4
#define RM_LENGTH_NOT_SPECT_E   -5
#define RM_UNMATCHED_END_E     -6
#define RM_MP3_NOT_FOUND       -7

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


int read_spect(spect_t *spect);
int read_spect_list(char *fname, spect_t **list, unsigned int *len, int nthreads);
int write_spect_db(char *fname, spect_t *list, unsigned int len);
int read_spect_db(char *fname, spect_t **list, unsigned int *len);
int read_spect_2(int fd, spect_t *spect);

#endif
