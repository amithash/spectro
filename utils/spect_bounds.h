#ifndef __SPECT_BOUNDS_H_
#define __SPECT_BOUNDS_H_

#include "spect.h"

int spect_bounds(char *db_name, float spect_bounds[NBANDS][2]);
void spect_get_edges(int *_start, int *_end, spect_t *spect);
void spect_bounds_exit(void);

#endif
