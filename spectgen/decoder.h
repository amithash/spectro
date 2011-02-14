#ifndef _DECODER_H_
#define _DECODER_H_

typedef void *decoder_handle;

int decoder_init(decoder_handle *handle);
int decoder_open(decoder_handle handle, char *fname);
int decoder_start(decoder_handle handle);
int decoder_close(decoder_handle handle);
void decoder_exit(decoder_handle handle);
void decoder_data_pull(decoder_handle handle, float **buffer, unsigned int *len, unsigned int *frate);

#endif
