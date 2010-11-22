#ifndef __FILE_VECIO_H_
#define __FILE_VECIO_H_

int write_uint(int fd, unsigned int val);
int read_uint(int fd, unsigned int *val);
int write_float_vec(int fd, float *vec, unsigned int len);
int read_float_vec(int fd, float *vec, unsigned int len);
int write_char_vec(int fd, char *vec, unsigned int len);
int read_char_vec(int fd, char *vec, unsigned int len);

#endif
