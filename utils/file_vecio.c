#include <stdlib.h>
#include <unistd.h>

int write_uint(int fd, unsigned int val)
{
	if(write(fd, &val, sizeof(unsigned int)) != sizeof(unsigned int)) {
		return -1;
	}
	return 0;
}

int read_uint(int fd, unsigned int *val)
{
	if(read(fd, val, sizeof(unsigned int)) != sizeof(unsigned int)) {
		return -1;
	}
	return 0;
}

int write_float_vec(int fd, float *vec, unsigned int len) 
{
	if(write(fd, vec, sizeof(float) * len) != (len * sizeof(float))) {
		return -1;
	}
	return 0;
}

int read_float_vec(int fd, float *vec, unsigned int len) 
{
	if(read(fd, vec, sizeof(float) * len) != (len * sizeof(float))) {
		return -1;
	}
	return 0;
}

int write_char_vec(int fd, char *vec, unsigned int len) 
{
	if(write(fd, vec, sizeof(char) * len) != (len * sizeof(char))) {
		return -1;
	}
	return 0;
}

int read_char_vec(int fd, char *vec, unsigned int len) 
{
	if(read(fd, vec, sizeof(char) * len) != (len * sizeof(char))) {
		return -1;
	}
	return 0;
}

