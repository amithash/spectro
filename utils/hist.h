#ifndef _HIST_H_
#define _HIST_H_

#include "spect.h"

#ifndef BIN_IND
#define BIN_IND  2
#endif

#define BIN_SIZE (1 << BIN_IND)
#define HIST_LEN (256/BIN_SIZE)
#define NUM2BIN(num) (num / BIN_SIZE)


typedef struct {
	char fname[FNAME_LEN];
	double spect_hist[NBANDS][HIST_LEN];
} hist_t;

void spect2hist(hist_t *hist, spect_t *spect);

int read_hist_db(hist_t **hist, unsigned int *len, char *fname);
int write_hist_db(hist_t *hist, unsigned int len, char *fname);
int spectdb2histdb(char * mdb, char *hdb);

#endif
