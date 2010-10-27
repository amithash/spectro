#ifndef __HIST_OPS_H_
#define __HIST_OPS_H_

void cent_clear(hist_t *hist);
void cent_accum(hist_t *out, hist_t *in);
void cent_final(hist_t *out, hist_t *in, unsigned int len);
void cent_copy(hist_t *out, hist_t *in);

#endif
