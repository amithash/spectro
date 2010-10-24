#include "hist.h"
#include <pthread.h>
#include <tag_c.h>

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

static void vec2hist(double *hist, unsigned char *vec, int start, int end)
{
	int i;
	int len = 0;
	memset(hist, 0, HIST_LEN * sizeof(double));
	for(i = start; i < end; i++) {
		hist[NUM2BIN(vec[i])]++;
		len++;
	}
	for(i = 0; i < HIST_LEN; i++) {
		hist[i] = hist[i] / len;
	}
}

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

void spect2hist(hist_t *hist, spect_t *spect)
	
{
	int i;
	int start = 0;
	int end = spect->len;

	/* Cut out silence at start */
	for(i = 0; i < spect->len; i++)
		if(!is_zero(spect, i))
			break;
	start = i;

	/* Cut out silence at end */
	for(i = spect->len; i >= 0; i--)
		if(!is_zero(spect, i))
			break;
	end = i;

	strcpy(hist->fname, spect->fname);

	for(i = 0; i < NBANDS; i++) {
		vec2hist(hist->spect_hist[i], spect->spect[i], start, end);
	}
	
	set_tags(hist);
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
		if(read_spect(fileno(ifp), &md)) {
			goto err;
		}

		spect2hist(&ht, &md);
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

