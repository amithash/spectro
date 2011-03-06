#ifndef __GENHISTDB_H_
#define __GENHISTDB_H_

typedef enum {
	UPDATE_MODE = 0,
	RECREATE_MODE
} generate_mode_t;

#include "spect-config.h"

typedef struct {
	char fname[FNAME_LEN];
	char title[TITLE_LEN];
	char artist[ARTIST_LEN];
	char album[ALBUM_LEN];
	unsigned int track;
	unsigned int length;
	float spect_hist[NBANDS][SPECT_HIST_LEN];
} hist_t;

hist_t *gen_hist(char *fname);
int read_histdb(hist_t **hist, unsigned int *len, char *fname);
int write_histdb(hist_t *hist, unsigned int len, char *fname);

int generate_histdb(char *dirname, char *dbname, 
		unsigned int nr_threads, generate_mode_t mode);

#endif
