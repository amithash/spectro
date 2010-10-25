#ifndef __AUTOCORRELATION_H_
#define __AUTOCORRELATION_H_

void auto_correlation(void *handle, double *in, double *out);
void destroy_fft(void *handle);
void * init_fft(int size);

#endif
