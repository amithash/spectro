#ifndef _DECODER_H_
#define _DECODER_H_

#ifdef __cplusplus
	extern "C" {
#endif

typedef void *decoder_handle;

int decoder_init(decoder_handle *handle);
int decoder_open(decoder_handle handle, char *fname);
int decoder_start(decoder_handle handle);
int decoder_close(decoder_handle handle);
void decoder_exit(decoder_handle handle);
void decoder_data_pull(decoder_handle handle, float **buffer, unsigned int *len, unsigned int *frate);
void decoder_supported_extensions(char **extensions, unsigned int *out_len);

#ifdef __cplusplus
	}
#endif 

#endif
