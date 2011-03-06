#ifndef __SPECTGEN_H_
#define __SPECTGEN_H_

#ifdef __cplusplus
	extern "C" {
#endif

typedef void *spectgen_handle;

int spectgen_open(spectgen_handle *handle, char *fname, unsigned int window_size, unsigned int step_size);
int spectgen_close(spectgen_handle handle);
int spectgen_start(spectgen_handle handle);
float *spectgen_pull(spectgen_handle handle);

#ifdef __cplusplus
	}
#endif 

#endif
