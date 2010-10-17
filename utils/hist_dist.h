#ifndef _HIST_DIST_H_
#define _HIST_DIST_H_

#include "hist.h"


double hist_distance(hist_t *hist1, hist_t *hist2);
void cent_final(hist_t *out, hist_t *in, unsigned int len);
void cent_accum(hist_t *out, hist_t *in);
void cent_clear(hist_t *hist);

#endif
