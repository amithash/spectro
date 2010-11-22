#include "hist.h"
#include "bpm.h"
#include <pthread.h>
#include <tag_c.h>
#include <stdint.h>
#include "ceps.h"
#include "file_vecio.h"

#define BIN_WIDTH     ((SPECT_MAX_VAL - SPECT_MIN_VAL) / SPECT_HIST_LEN)

#define FLOOR_MIN(val) ((val) < SPECT_MIN_VAL ? SPECT_MIN_VAL : (val))
#define CEIL_MAX(val)  ((val) >= SPECT_MAX_VAL ? (SPECT_MAX_VAL - BIN_WIDTH) : (val))

#define NUM2BIN(val)    (unsigned int)(FLOOR_MIN(CEIL_MAX(val)) / BIN_WIDTH)

unsigned int last_samples_per_second = 0;
/* Assumption: hist->fname is valid */
static int set_tags(hist_t *hist)
{
	char *p_title;
	char *p_album;
	char *p_artist;
	TagLib_Tag *tag;
	TagLib_File *f = taglib_file_new(hist->fname);

	if(f == NULL) {
		printf("Cannot open %s\n",hist->fname);
                return -1;
        }
        tag = taglib_file_tag(f);

        p_title = taglib_tag_title(tag);
        strcpy(hist->title, p_title);

        p_artist = taglib_tag_artist(tag);
        strcpy(hist->artist, p_artist);

        p_album = taglib_tag_album(tag);
        strcpy(hist->album, p_album);

        hist->track = taglib_tag_track(tag);

        taglib_tag_free_strings();

	hist->length = taglib_audioproperties_length(taglib_file_audioproperties(f));

        taglib_file_free(f);

	return 0;

}

static void vec2hist(float *hist, float *vec, unsigned int rlen)
{
	int i;
	int len = 0;
	memset(hist, 0, SPECT_HIST_LEN * sizeof(float));
	for(i = 0; i < rlen; i++) {
		hist[NUM2BIN(vec[i])]++;
		len++;
	}
	for(i = 0; i < SPECT_HIST_LEN; i++) {
		hist[i] = hist[i] / len;
	}
}

int spect2hist(hist_t *hist, spect_t *spect)
	
{
	int i;
	int start, end;

	strcpy(hist->fname, spect->fname);

	spect_get_edges(&start, &end, spect);

	for(i = 0; i < NBANDS; i++) {
		vec2hist(hist->spect_hist[i], &spect->spect[i][start], end - start);
	}
	
	set_tags(hist);

	if(hist->length == 0) {
		/* guess based on last processed track */
		hist->length = spect->len / last_samples_per_second;
	} else {
		last_samples_per_second = spect->len / hist->length;
	}

	if(spect2chist(hist->ceps_hist, spect)) {
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
	if(write_char_vec(fd, hist->title, TITLE_LEN)) {
		return -1;
	}
	if(write_char_vec(fd, hist->artist, ARTIST_LEN)) {
		return -1;
	}
	if(write_char_vec(fd, hist->album, ALBUM_LEN)) {
		return -1;
	}
	if(write_uint(fd, hist->track)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(write_float_vec(fd, hist->spect_hist[i], SPECT_HIST_LEN)) {
			return -1;
		}
	}
	for(i = 0; i < NBANDS/2; i++) {
		if(write_float_vec(fd, hist->ceps_hist[i], CEPS_HIST_LEN)) {
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
	if(read_char_vec(fd, hist->title, TITLE_LEN)) {
		return -1;
	}
	if(read_char_vec(fd, hist->artist, ARTIST_LEN)) {
		return -1;
	}
	if(read_char_vec(fd, hist->album, ALBUM_LEN)) {
		return -1;
	}
	if(read_uint(fd, &hist->track)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(read_float_vec(fd, hist->spect_hist[i], SPECT_HIST_LEN)) {
			return -1;
		}
	}
	for(i = 0; i < NBANDS/2; i++) {
		if(read_float_vec(fd, hist->ceps_hist[i], CEPS_HIST_LEN)) {
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
		spect_error("Could not read len");
		return -1;
	}
	*hist = this = (hist_t *)calloc(*len, sizeof(hist_t));
	if(*hist == NULL) {
		spect_error("Malloc failed!");
		fclose(fp);
		return -1;
	}
	for(i = 0; i < *len; i++) {
		if(read_hist(fileno(fp), &this[i])) {
			spect_error("Read %d resulted in error!",i);
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
		if(read_spect(fileno(ifp), &md)) {
			goto err;
		}

		if(spect2hist(&ht, &md)) {
			spect_error("Conversition failed!");
			exit(-1);
		}
		if(write_hist(fileno(ofp), &ht)) {
			free_spect(&md);
			goto err;
		}
		free_spect(&md);
	}
	errno = 0;
err:
	fclose(ifp);
	fclose(ofp);
	return errno;
}

void plot_hist(hist_t *hist)
{
	int i,j;
	FILE *f;
	f = fopen("__out.txt", "w");
	for(i = 0; i < SPECT_HIST_LEN; i++) {
		for(j = 0; j < NBANDS; j++) {
			fprintf(f,"%f ",hist->spect_hist[j][i]);
		}
		fprintf(f,"\n");
	}
	fclose(f);
	f = fopen("__out.gp", "w");
	fprintf(f, "plot \\\n");
	for(i = 0; i < NBANDS; i++) {
		fprintf(f, "\"__out.txt\" using %d with lines title 'Band %d'", i + 1, i);
		if(i < (NBANDS - 1)) {
		      fprintf(f, ", \\");
		}
		fprintf(f, "\n");
	}
	fprintf(f,"pause -1\n");
	fclose(f);
	
	system("gnuplot __out.gp");

	system("rm -f __out.gp __out.txt");
}

