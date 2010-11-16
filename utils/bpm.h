#ifndef __BPM_H_
#define __BPM_H_

#include "spect.h"

int spect2bpm(double bpm[BPM_LEN], spect_t *spect);
void plot_bpm(double bpm[BPM_LEN]);
void smooth_bpm(double bpm[BPM_LEN]);

#endif
