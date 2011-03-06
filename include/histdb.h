#ifndef __GENHISTDB_H_
#define __GENHISTDB_H_

#ifdef __cplusplus
	extern "C" {
#endif
#include "spect-config.h"

typedef enum {
	UPDATE_MODE = 0,
	RECREATE_MODE
} generate_mode_t;

typedef enum {
	DISTANCE_START = 0,
	KL_DIVERGANCE = 0,
	JEFFREYS_DIVERGANCE,
	EXPECTED_VALUE_DIFFERENCE,
	EXPECTED_DIFFERENCE,
	K_DIVERGANCE,
	HELLINGER_DIVERGANCE,
	EUCLIDEAN_DISTANCE,
	JENSEN_DIVERGANCE,
	DISTANCE_END
} hist_dist_func_t;

typedef float (*distance_func_t)(float *a, float *b, unsigned int len);

typedef struct {
	hist_dist_func_t 	func_type;
	char             	name[32];
	distance_func_t		func;
} dist_t;

typedef struct {
	char fname[FNAME_LEN];
	char title[TITLE_LEN];
	char artist[ARTIST_LEN];
	char album[ALBUM_LEN];
	unsigned int track;
	unsigned int length;
	float spect_hist[NBANDS][SPECT_HIST_LEN];
} hist_t;

float hist_distance(hist_t *hist1, hist_t *hist2, hist_dist_func_t distance);
void get_supported_distances(dist_t **dist);

hist_t *gen_hist(char *fname);
int read_histdb(hist_t **hist, unsigned int *len, char *fname);
int write_histdb(hist_t *hist, unsigned int len, char *fname);

int generate_histdb(char *dirname, char *dbname, 
		unsigned int nr_threads, generate_mode_t mode);

#ifdef __cplusplus
	}
#endif 

#endif
