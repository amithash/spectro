#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "histdb.h"
#include "spectgen.h"
#include "spect-config.h"
#include <tag_c.h>

#define BIN_WIDTH     ((SPECT_MAX_VAL - SPECT_MIN_VAL) / SPECT_HIST_LEN)

#define FLOOR_MIN(val) ((val) < SPECT_MIN_VAL ? SPECT_MIN_VAL : (val))
#define CEIL_MAX(val)  ((val) >= SPECT_MAX_VAL ? (SPECT_MAX_VAL - BIN_WIDTH) : (val))

#define NUM2BIN(val)    (unsigned int)(FLOOR_MIN(CEIL_MAX(val)) / BIN_WIDTH)

#define HIST_MAGIC 0xdeadbeef

typedef struct {
	unsigned int hist_magic_start;	/* Always the first */
	unsigned int len; /* db len */
	/* Spect configuration */
	unsigned int fname_len;
	unsigned int title_len;
	unsigned int artist_len;
	unsigned int album_len;
	unsigned int nbands;
	unsigned int spect_hist_len;
	unsigned int window_size;
	unsigned int step_size;
	float min_val;
	float max_val;
	unsigned int hist_magic_end;	/* Always the last */
} hist_info_t;

static hist_info_t current_config_info = {
	HIST_MAGIC,
	0,
	FNAME_LEN,
	TITLE_LEN,
	ARTIST_LEN,
	ALBUM_LEN,
	NBANDS,
	SPECT_HIST_LEN,
	SPECT_WINDOW_SIZE,
	SPECT_STEP_SIZE,
	SPECT_MIN_VAL,
	SPECT_MAX_VAL,
	HIST_MAGIC
};

void int_strncpy(char *out, char *in, int len)
{
	strncpy(out, in, len);
	out[len - 1] = '\0';
}

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
	int_strncpy(hist->title, p_title, TITLE_LEN);

        p_artist = taglib_tag_artist(tag);
	int_strncpy(hist->artist, p_artist, ARTIST_LEN);

        p_album = taglib_tag_album(tag);
	int_strncpy(hist->album, p_album, ALBUM_LEN);

        hist->track = taglib_tag_track(tag);

        taglib_tag_free_strings();

	hist->length = taglib_audioproperties_length(taglib_file_audioproperties(f));

        taglib_file_free(f);

	return 0;

}

#define print(fmt, a...) printf(fmt,##a); fflush(stdout)
hist_t *gen_hist(char *fname)
{
	hist_t *hist;
	spectgen_handle spect_handle;
	float *spect;
	unsigned int num_samples = 0;
	int i,j;
	
	hist = malloc(sizeof(hist_t));
	if(!hist)
	      return NULL;

	if(spectgen_open(&spect_handle, fname, SPECT_WINDOW_SIZE, SPECT_STEP_SIZE)) {
		printf("Spectgen open failed\n");
		free(hist);
		return NULL;
	}
	if(spectgen_start(spect_handle)) {
		printf("Speecgen cannot be started\n");
		free(hist);
		hist = NULL;
		goto bailout;
	}
	strncpy(hist->fname, fname, FNAME_LEN);
	hist->fname[FNAME_LEN - 1] = '\0';
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			hist->spect_hist[i][j] = 0;
		}
	}
	while((spect = spectgen_pull(spect_handle)) != NULL) {
		for(i = 0; i < NBANDS; i++) {
		      if(spect[i] != 0)
			    break;
		}
		if(i == NBANDS) {
			free(spect);
			continue;
		}

		for(i = 0; i < NBANDS; i++) {
			unsigned int ind = NUM2BIN(spect[i]);
			if(ind >= SPECT_HIST_LEN) {
				print("Macro is wrong\n"); fflush(stdout);
				continue;
			}
			hist->spect_hist[i][ind]++;
		}
		num_samples++;
		free(spect);
	}
	if(num_samples == 0) {
		free(hist);
		hist = NULL;
		goto bailout;
	}
	for(i = 0; i < NBANDS; i++) {
		for(j = 0; j < SPECT_HIST_LEN; j++) {
			hist->spect_hist[i][j] /= (float)num_samples;
		}
	}
	set_tags(hist);
bailout:
	spectgen_close(spect_handle);

	return hist;
}

static int write_hist(int fd, hist_t *hist)
{
	if(!hist)
		return -1;
	if(write(fd, hist, sizeof(hist_t)) != sizeof(hist_t))
		return -1;
	return 0;
}

static int read_hist(int fd, hist_t *hist) 
{
		return -1;
	if(read(fd, hist, sizeof(hist_t)) != sizeof(hist_t))
		return -1;
	return 0;
}

int read_hist_info(int fd, unsigned int *_len)
{
	hist_info_t info;
	if(read(fd, &info, sizeof(hist_info_t)) != sizeof(hist_info_t))
		return -1;
	if(info.hist_magic_start != HIST_MAGIC)
	      return -1;
	if(info.hist_magic_end != HIST_MAGIC)
	      return -1;
	if(info.fname_len != FNAME_LEN)
	      return -1;
	if(info.title_len != TITLE_LEN)
	      return -1;
	if(info.artist_len != ARTIST_LEN)
	      return -1;
	if(info.album_len != ALBUM_LEN)
	      return -1;
	if(info.nbands != NBANDS)
	      return -1;
	if(info.spect_hist_len != SPECT_HIST_LEN)
	      return -1;
	if(info.window_size != SPECT_WINDOW_SIZE)
	      return -1;
	if(info.step_size != SPECT_STEP_SIZE)
	      return -1;
	if(info.min_val != SPECT_MIN_VAL)
	      return -1;
	if(info.max_val != SPECT_MAX_VAL)
	      return -1;
	if(info.len == 0)
	      return -1;
	*_len = info.len;
	return 0;
}

int write_hist_info(int fd, unsigned int len)
{
	current_config_info.len = len;
	if(write(fd, &current_config_info, sizeof(hist_info_t)) != sizeof(hist_info_t)) {
		return -1;
	}
	return 0;
}

int read_histdb(hist_t **hist, unsigned int *len, char *fname)
{
	FILE *fp;
	hist_t *this;
	int i;
	if(hist == NULL || fname == NULL)
		return -1;

	fp = fopen(fname, "r");
	if(fp == NULL) {
		return -1;
	}
	if(read_hist_info(fileno(fp), len))
		goto bailout;

	*hist = this = (hist_t *)calloc(*len, sizeof(hist_t));
	if(*hist == NULL)
		goto bailout;

	for(i = 0; i < *len; i++) {
		if(read_hist(fileno(fp), &this[i])) {
			printf("Read %d resulted in error!\n",i);
			goto free_bailout;
		}
	}
	fclose(fp);
	return 0;
free_bailout:
	free(this);
bailout:
	*hist = NULL;
	fclose(fp);
	return -1;
}

int read_append_histdb(hist_t **out_hist, unsigned int *len, char *fname)
{
	hist_t *hist_list = NULL;
	hist_t *old_hist_list;
	unsigned int new_len;
	if(*out_hist == NULL) {
		return read_histdb(out_hist, len, fname);
	}
	if(read_histdb(&hist_list, &new_len, fname))
	      return -1;
	old_hist_list = *out_hist;
	old_hist_list = realloc(old_hist_list, sizeof(hist_t) * (*len + new_len));
	if(!old_hist_list)
	      return -1;
	memcpy(&old_hist_list[*len], hist_list, new_len);
	*len = *len + new_len;
	*out_hist = old_hist_list;
	return 0;
}

int write_histdb(hist_t *hist, unsigned int len, char *fname)
{
	FILE *fp;
	int i;
	int rc = -1;
	if(hist == NULL || fname == NULL)
	      return -1;
	fp = fopen(fname, "r");
	if(fp == NULL) {
		return -1;
	}

	if(write_hist_info(fileno(fp), len))
		goto bailout;

	for(i = 0; i < len; i++) {
		if(write_hist(fileno(fp), &hist[i]))
		      goto bailout;
	}
	rc = 0;
bailout:
	fclose(fp);
	return rc;
}

