#include "hist.h"
#include <pthread.h>
#include <tag_c.h>
#include <stdint.h>

unsigned int last_samples_per_second = 0;
/* Assumption: hist->fname is valid */
static int set_tags(hist_t *hist)
{
	char *p_title;
	char *p_album;
	char *p_artist;
	unsigned int track;
	TagLib_Tag *tag;
	TagLib_File *f = taglib_file_new(hist->fname);
	TagLib_AudioProperties *aprop;

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

#define ZERO_1D(ptr,len) do {				\
	int __i;					\
	for(__i = 0; __i < len; __i++) {		\
		ptr[__i] = 0;				\
	}						\
}while(0)

#define ZERO_2D(ptr, lenx, leny) do {			\
	int __i, __j;					\
	for(__i = 0; __i < lenx; __i++) {		\
		for(__j = 0; __j < leny; __j++) {	\
			ptr[__i][__j] = 0;		\
		}					\
	}						\
}while(0)

int per_hist(hist_t *hist, spect_t *spect, unsigned int len)
{
	int i,j;
	uint8_t *out[NBANDS] = {NULL};
	double period[NBANDS];
	int min_len = 10; /* Default min len */
	double total = 0;
	int rc = 0;

	for(i = 0; i < NBANDS; i++) {
		out[i] = (uint8_t *)malloc(sizeof(uint8_t) * spect->len);
		if(!out[i])  {
			rc = -1; goto cleanup;
		}
	}

	for(j = 0; j < NBANDS; j++) {
		out[j][0] = 0;
	}
	for(i = 1; i < spect->len; i++) {
		for(j = 0; j < NBANDS; j++) {
			double val = (double)spect->spect[j][i] - (double)spect->spect[j][i-1];
			if(val <= 0) {
				out[j][i] = 0;
			} else {
				out[j][i] = 1;
			}
		}
	}

	ZERO_2D(hist->per_hist, PERIOD_LEN, PHIST_LEN);

	ZERO_1D(period, NBANDS);

	for(i = 1; i < spect->len; i++) {
		for(j = 0; j < NBANDS; j++) {
			/* The beat */
			if(out[j][i] == 1) {
				unsigned int ind = period[j] >= PERIOD_LEN ? PERIOD_LEN - 1 : (unsigned int)period[j];
				unsigned int powind = NUM2PBIN(spect->spect[j][i]);
				period[j] = 0;
				
				hist->per_hist[ind][powind]++;
			} else {
				period[j]++;
			}
		}
	}
	if(len > 0) {
		/* Assume maximum beats per second = 240 bpm = 4 bps */
		min_len = spect->len / (4 * len);
	} else {
		spect_warn("Using default min_len = %d", min_len);
	}


	/* Zero out entries corrosponding to unresonalble beats/minutes
	 * that is greater than 240bpm */
	ZERO_2D(hist->per_hist, min_len, PHIST_LEN);

	/* Convert to probabilities */
	for(i = 0; i < PERIOD_LEN; i++) {
		double total = 0;
		for(j = 0; j < PHIST_LEN; j++) {
			total += hist->per_hist[i][j];
		}
		for(j = 0; j < PHIST_LEN; j++) {
			hist->per_hist[i][j] /= total;
		}
	}
cleanup:
	for(i = 0; i < NBANDS; i++) {
		if(out[i])
			free(out[i]);
	}

	return rc;
}


static void vec2hist(double *hist, unsigned char *vec, unsigned int rlen)
{
	int i;
	int len = 0;
	memset(hist, 0, HIST_LEN * sizeof(double));
	for(i = 0; i < rlen; i++) {
		hist[NUM2BIN(vec[i])]++;
		len++;
	}
	for(i = 0; i < HIST_LEN; i++) {
		hist[i] = hist[i] / len;
	}
}

#if 0
static int is_zero(spect_t *spect, int row)
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		if(spect->spect[i][row] != 0) {
			return 0;
		}
	}
	return 1;
}

void get_avoids(unsigned char *avoid, spect_t *spect)
{
	int i,j;

	for(i = 0; i < spect->len; i++) {
		if(is_zero(spect, i)) {
			avoid[i] = 1;
		} else {
			avoid[i] = 0;
		}
	}
}
#endif

int spect2hist(hist_t *hist, spect_t *spect)
	
{
	int i;
	int start = 0;
	int end = spect->len;
#if 0
	unsigned char *avoid;
	avoid = (unsigned char *)malloc(sizeof(unsigned char) * spect->len);
	if(!avoid)
	      return -1;
	/* Avoid all samples with silence */
	get_avoids(avoid, spect);
#endif

	strcpy(hist->fname, spect->fname);

	for(i = 0; i < NBANDS; i++) {
		vec2hist(hist->spect_hist[i], spect->spect[i], spect->len);
	}
	
	set_tags(hist);

	if(hist->length == 0) {
		/* guess based on last processed track */
		hist->length = spect->len / last_samples_per_second;
	} else {
		last_samples_per_second = spect->len / hist->length;
	}

	per_hist(hist, spect, hist->length);

	return 0;
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
		if(write_double_vec(fd, hist->spect_hist[i], HIST_LEN)) {
			return -1;
		}
	}
	for(i = 0; i < PERIOD_LEN; i++) {
		if(write_double_vec(fd, hist->per_hist[i], PHIST_LEN)) {
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
		if(read_double_vec(fd, hist->spect_hist[i], HIST_LEN)) {
			return -1;
		}
	}
	for(i = 0; i < PERIOD_LEN; i++) {
		if(read_double_vec(fd, hist->per_hist[i], PHIST_LEN)) {
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

