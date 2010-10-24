#include "bspect.h"
#include "tag_c.h"
#include <float.h>

static double *absolutes;
static double **sim;

static double distance(spect_t *spect, int i, int j)
{
	int k;
	double abs_i = absolutes[i];
	double abs_j = absolutes[j];
	double s_prod = 0;

	for(k = 0; k < NBANDS; k++) {
		double a = ((double)spect->spect[k][i]) / 255.0;
		double b = ((double)spect->spect[k][j]) / 255.0;
		s_prod += a * b;
	}
	if(s_prod == 0) {
		return 0;
	}

	/* Largest possible cosine value */
	if(abs_i == 0 || abs_j == 0)
	      return 1.0;

	return s_prod / (abs_i * abs_j);
}

static double get_sim(int i, int j)
{
	if(i == j)
	      return sim[i][i];

	if(j > i)
	      return sim[j][i];

	return sim[i][j];
}

static double bspect_val(spect_t *spect, int l)
{
	int i,j;

	double dist = 0;

	for(i = 0; i < spect->len; i++) {
		for(j = 0; j < spect->len - l; j++) {
			dist += get_sim(i,j) * get_sim(i, j+l);
		}
	}
	return dist;
}

static void compute_absolutes(spect_t *spect) 
{
	int i;
	int j;
	for(i = 0; i < spect->len; i++) {
		double _abs = 0;
		for(j = 0; j < NBANDS; j++) {
			double val = (double)spect->spect[j][i] / 255.0;
			_abs += val * val;
		}
		absolutes[i] = sqrt(_abs);
	}
}

static int bspect_get(double beats[BEAT_LEN], spect_t *spect, int song_len)
{
	int i, j;
	int widths_in_song = (song_len * 1000 / TIME_WIDTH);
	int samples_per_width = spect->len / widths_in_song;
	double total = 0;

	if(samples_per_width == 0) {
		spect_error("Invalid sample");
		return -1;
	}
	absolutes = (double *)malloc(spect->len * sizeof(double));
	if(!absolutes)
	      return -1;

	compute_absolutes(spect);

	sim = (double **)malloc(sizeof(double) * spect->len);
	if(!sim)
	      goto err1;

	for(i = 0; i < spect->len; i++) {
		int len = i + 1;
		sim[i] = (double *)malloc(len * sizeof(double));
		if(!sim[i])
		      goto err2;
		for(j = 0; j < len; j++) {
			sim[i][j] = distance(spect, i, j);
		}
	}

	for(i = 0; i < BEAT_LEN; i++) {
		beats[i] = 0;
		for(j = ((i + 1) * samples_per_width); j < ((i + 2) * samples_per_width); j++) {
			beats[i] += bspect_val(spect, j);
		}
		total += beats[i] /= (double)samples_per_width;
	}
	for(i = 0; i < BEAT_LEN; i++) {
		beats[i] = beats[i] / total;
	}
err2:
	for(i = 0; i < spect->len;i++)
	      if(sim[i])
		    free(sim[i]);
	free(sim);
err1:
	free(absolutes);
	return 0;
}

#define SAMPLE_WINDOW 4000

static double energy(spect_t *spect, int row)
{
	int i;
	double ret = 0;
	for(i = 0; i < NBANDS; i++) {
		double val = (double)spect->spect[i][row] / 255.0;
		ret = val * val;
	}
	return sqrt(ret);
}

static void ind_max_energy_window(spect_t *spect, int *start, int *end)
{
	int i,j;
	int max_e_row = 0;
	double max_e = 0;
	if(spect->len <= SAMPLE_WINDOW) {
		*start = 0;
		*end   = spect->len;
	}

	      
	for(i = 0; i < spect->len - SAMPLE_WINDOW; i++) {
		double tot_energy = 0;
		for(j = i; j < i + SAMPLE_WINDOW; j++) {
			tot_energy += energy(spect, j);
		}
		if(tot_energy > max_e) {
			max_e = tot_energy;
			max_e_row = i;
		}
	}
	*start = max_e_row;
	*end = max_e_row + SAMPLE_WINDOW;
}

static void create_spect(spect_t *new, spect_t *old)
{
	int start;
	int end;
	int i,j;
	ind_max_energy_window(old, &start, &end);
	new->len = end - start;
	for(i = 0; i < NBANDS; i++) {
		new->spect[i] = (unsigned char *)malloc(sizeof(unsigned char) * new->len);
		if(!new->spect[i])
		      return;
	}
	for(i = start; i < end; i++) {
		for(j = 0; j < NBANDS; j++) {
			new->spect[j][i-start] = old->spect[j][i];
		}
	}
}



int compute_bspect(double beats[BEAT_LEN], spect_t *spect, char *name)
{
	unsigned int len;
	TagLib_File *f;
	int i;
	int start, end;
	spect_t new_spect;

	f = taglib_file_new(spect->fname);
	if(f == NULL) {
		spect_error("TAGLIB: Could not open %s",spect->fname);
		return -1;
	}
	len = taglib_audioproperties_length(taglib_file_audioproperties(f));
	taglib_file_free(f);

	create_spect(&new_spect, spect);

	if(bspect_get(beats, &new_spect, len)) {
		spect_error("could not process");
		return -1;
	}
	free_spect(&new_spect);
	return 0;
}
