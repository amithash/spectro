#ifndef __BSPECT_H_
#define __BSPECT_H_
#include "spect.h"

#define TIME_WIDTH 200
#define TIME_MIN   200
#define TIME_MAX   6000
#define BEAT_LEN   ((TIME_MAX - TIME_MIN) / TIME_WIDTH)

int compute_bspect(double beats[BEAT_LEN], spect_t *spect, char *name);

#endif
