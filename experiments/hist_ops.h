#ifndef __HIST_OPS_H_
#define __HIST_OPS_H_

void cent_clear(void *hist);
void cent_accum(void *out, void *in);
void cent_final(void *out, void *in, unsigned int len);
void cent_copy(void *out, void *in);
void *hist_ind(void *_data, int i);
void *hist_calloc(int len);
float hist_dist(void *a, void *b);

#endif
