#include "mood.h"
#include <pthread.h>

#ifndef BIN_IND
#define BIN_IND  0
#endif
#define BIN_SIZE (1 << BIN_IND)
#define HIST_LEN (256/BIN_SIZE)
#define NUM2BIN(num) (num / BIN_SIZE)

#define HIST_DEBUG


typedef struct {
	mood_t *mood;
	double mood_hist[NBANDS][HIST_LEN];
	double dist;
} hist_t;

typedef struct {
	char *fname;
	double dist;
} max_type;

#define NMAX_DEFAULT 30

max_type *maxes;


hist_t *all_hist  = NULL;
hist_t ref_hist;

#define INIT_DIST 100000.000
#define NR_THREADS 8
int thread_len = 0;

void normalize(double *out, unsigned char *in, unsigned int len)
{
	double sum = 0;
	int i;
	for(i = 0; i < len; i++) {
		sum += (double)in[i];
	}
	for(i = 0; i < len; i++) {
		out[i] = (double)in[i] / sum;
	}
}

/* Compute the bhattacharya coefficient and from it compute the
 * Hellinger distance as the bhattacharya distance does not obay
 * the triangular rule, and the Hellinger distance implies 0 to 
 * be equal while the bhattacharya distance marks 1 as similar
 */
double bdistance(double *a, double *b, unsigned int len) 
{
	double dist = 0.0;
	int i;
	if(a == NULL || b == NULL) {
		return 0;
	}

	for(i = 0; i < len; i++) {
		dist += sqrt((double)a[i] * (double)b[i]);
	}
	return acos(dist);
}

void init_nmax(int len)
{
	int i;
	maxes = (max_type *)calloc(len, sizeof(max_type));
	if(!maxes) {
		mood_error("Alloc failed!");
		exit(-1);
	}
	for(i = 0; i < len; i++) {
		maxes[i].fname = NULL;
		maxes[i].dist = INIT_DIST;
	}
}

#define PRE_OP(val) (val)
#define POST_OP(val) ((val)/NBANDS)

double vec_distance(double dist[NBANDS]) 
{
	int i;
	double val = 0;

	for(i = 0; i < NBANDS; i++) {
		val += PRE_OP(dist[i]);
	}
	return POST_OP(val);
}

int main(int argc, char *argv[])
{
	unsigned int len;
	int i,j;
	mood_t *mood_list;
	mood_t *ref_mood;
	int maxes_len = NMAX_DEFAULT;
	double nref[NBANDS][MOODLEN];

	if(argc < 3) {
		mood_error("USAGE: MOOD_DB ref_mood_file.mood");
		exit(-1);
	}
	if(argc == 4) {
		maxes_len = atoi(argv[3]) + 1;
	}

	if(read_mood_db(argv[1], &mood_list, &len)) {
		mood_error("Could not read mood db: %s", argv[1]);
		exit(-1);
	}
	ref_mood = (mood_t *)malloc(sizeof(mood_t));
	if(!ref_mood) {
		mood_error("Malloc failed");
		exit(-1);
	}

	strncpy(ref_mood->fname, argv[2], 512);
	if(read_mood(ref_mood) < 0) {
		mood_error("Could not read %s", ref_mood->fname);
		exit(-1);
	}
	ref_mood->valid = 1;
	init_nmax(maxes_len);

	for(i = 0; i < NBANDS; i++) {
		normalize(nref[i], ref_mood->mood[i], MOODLEN);
	}
	for(i = 0; i < len; i++) {
		double idist = 0;
		double dist[NBANDS] = {0,0,0};
		for(j = 0; j < NBANDS; j++) {
			double nvec[MOODLEN];
			normalize(nvec, mood_list[i].mood[j], MOODLEN);
			dist[j] = bdistance(nref[j], nvec, MOODLEN);
		}
		idist = vec_distance(dist);

		for(j = 0; j < maxes_len; j++) {
			if(idist < maxes[j].dist) {
				maxes[j].dist = idist;
				maxes[j].fname = mood_list[i].fname;
				break;
			}
		}
	}
	for(i = 1; i < maxes_len; i++) {
		if(!maxes[i].fname) {
			printf("WAAAAAAAA\n");
		      continue;
		}
		printf("%f\t%s\n", maxes[i].dist, maxes[i].fname);
	}
	free(all_hist);
	free(mood_list);
	free(ref_mood);

	return 0;

}
