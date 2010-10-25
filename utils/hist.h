#ifndef _HIST_H_
#define _HIST_H_

#include "spect.h"

#include "../spect-config.h"

typedef struct {
	char fname[FNAME_LEN];
	char title[TITLE_LEN];
	char artist[ARTIST_LEN];
	char album[ALBUM_LEN];
	unsigned int track;
	unsigned int length;
	double spect_hist[NBANDS][HIST_LEN];
	double beats[NBANDS][BEAT_LEN];
} hist_t;

int spect2hist(hist_t *hist, spect_t *spect);

int read_hist_db(hist_t **hist, unsigned int *len, char *fname);
int write_hist_db(hist_t *hist, unsigned int len, char *fname);
int spectdb2histdb(char * mdb, char *hdb);

#endif
