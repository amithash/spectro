#include "hist.h"
#include <pthread.h>

static void vec2hist(double *hist, unsigned char *vec, int vec_len, unsigned char *avoid)
{
	int i;
	int len = 0;
	memset(hist, 0, HIST_LEN * sizeof(double));
	for(i = 0; i < vec_len; i++) {
		if(avoid[i] == 1)
			continue;
		hist[NUM2BIN(vec[i])]++;
		len++;
	}
	for(i = 0; i < HIST_LEN; i++) {
		hist[i] = hist[i] / len;
	}
}

void spect2hist(hist_t *hist, spect_t *spect)
	
{
	int i;
	unsigned char avoid[SPECTLEN];

	if(spect->valid == 0) {
		spect_error("Invalid spect file");
		return;
	}
	if(spect->len > SPECTLEN) {
		spect_error("Invalid spect file");
		return;
	}

	strcpy(hist->fname, spect->fname);

	for(i = 0; i < spect->len; i++) {
		int j;
		int all_zero = 1;
		for(j = 0; j < NBANDS; j++) {
			if(spect->spect[j][i] != 0) {
				all_zero = 0;
				break;
			}
		}
		if(all_zero == 1) {
			avoid[i] = 1;
		} else {
			avoid[i] = 0;
		}
	}

	for(i = 0; i < NBANDS; i++) {
		vec2hist(hist->spect_hist[i], spect->spect[i], spect->len, avoid);
	}
}

static int read_uint(int fd, unsigned int *val) {
	if(read(fd, val, sizeof(unsigned int)) != sizeof(unsigned int)) {
		return -1;
	}
	return 0;
}

static int write_uint(int fd, unsigned int val) {
	if(write(fd, &val, sizeof(unsigned int)) != sizeof(unsigned int)) {
		return -1;
	}
	return 0;
}
static int read_char_vec(int fd, char *vec, unsigned int len) 
{
	if(read(fd, vec, sizeof(char) * len) != (len * sizeof(char))) {
		return -1;
	}
	return 0;
}
static int write_char_vec(int fd, char *vec, unsigned int len) 
{
	if(write(fd, vec, sizeof(char) * len) != (len * sizeof(char))) {
		return -1;
	}
	return 0;
}

static int read_double_vec(int fd, double *vec, unsigned int len) 
{
	if(read(fd, vec, sizeof(double) * len) != (len * sizeof(double))) {
		return -1;
	}
	return 0;
}
static int write_double_vec(int fd, double *vec, unsigned int len) 
{
	if(write(fd, vec, sizeof(double) * len) != (len * sizeof(double))) {
		return -1;
	}
	return 0;
}

static int write_hist(int fd, hist_t *hist)
{
	int i;
	if(!hist) {
		return -1;
	}
	if(write_char_vec(fd, hist->fname, FNAME_LEN)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(write_double_vec(fd, hist->spect_hist[i], HIST_LEN)) {
			return -1;
		}
	}
	return 0;
}

static int read_hist(int fd, hist_t *hist) 
{
	int i;
	if(!hist) {
		return -1;
	}
	if(read_char_vec(fd, hist->fname, FNAME_LEN)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(read_double_vec(fd, hist->spect_hist[i], HIST_LEN)) {
			return -1;
		}
	}
	return 0;
}

int read_hist_db(hist_t **hist, unsigned int *len, char *fname)
{
	FILE *fp;
	hist_t *this;
	int i;
	fp = fopen(fname, "r");
	if(fp == NULL) {
		return -1;
	}
	if(read_uint(fileno(fp), len)) {
		fclose(fp);
		return -1;
	}
	*hist = this = (hist_t *)calloc(*len, sizeof(hist_t));
	if(*hist == NULL) {
		fclose(fp);
		return -1;
	}
	for(i = 0; i < *len; i++) {
		if(read_hist(fileno(fp), &this[i])) {
			goto err;
		}
	}
	fclose(fp);
	return 0;
err:
	free(this);
	*hist = NULL;
	return -1;
}

int write_hist_db(hist_t *hist, unsigned int len, char *fname)
{
	FILE *fp;
	int i;
	fp = fopen(fname, "r");
	if(fp == NULL) {
		return -1;
	}
	if(write_uint(fileno(fp), len)) {
		fclose(fp);
		return -1;
	}
	if(hist == NULL) {
		fclose(fp);
		return -1;
	}
	for(i = 0; i < len; i++) {
		if(write_hist(fileno(fp), &hist[i])) {
			return -1;
		}
	}
	fclose(fp);
	return 0;
}

int spectdb2histdb(char * mdb, char *hdb)
{
	FILE *ofp, *ifp;
	int i;
	unsigned int len;
	int errno = -1;

	ofp = fopen(hdb, "w");
	if(ofp == NULL) {
		return -1;
	}
	ifp = fopen(mdb, "r");
	if(ifp == NULL) {
		fclose(ofp);
		return -1;
	}
	if(read_uint(fileno(ifp), &len)) {
		goto err;
	}
	if(write_uint(fileno(ofp), len)) {
		goto err;
	}
	for(i = 0; i < len; i++) {
		spect_t md;
		hist_t ht;
		if(read_spect_2(fileno(ifp), &md)) {
			goto err;
		}

		spect2hist(&ht, &md);
		if(write_hist(fileno(ofp), &ht)) {
			goto err;
		}
	}
	errno = 0;
err:
	fclose(ifp);
	fclose(ofp);
	return errno;
}

