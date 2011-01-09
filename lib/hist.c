/*******************************************************************************
    This file is part of spectro
    Copyright (C) 2010  Amithash Prasad <amithash@gmail.com>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*******************************************************************************/

#include "hist.h"
#include <pthread.h>

#define NDEBUG

#include <tag_c.h>
#include <stdint.h>
#include "file_vecio.h"
#include "plot.h"

#define BIN_WIDTH     ((SPECT_MAX_VAL - SPECT_MIN_VAL) / SPECT_HIST_LEN)

#define FLOOR_MIN(val) ((val) < SPECT_MIN_VAL ? SPECT_MIN_VAL : (val))
#define CEIL_MAX(val)  ((val) >= SPECT_MAX_VAL ? (SPECT_MAX_VAL - BIN_WIDTH) : (val))

#define NUM2BIN(val)    (unsigned int)(FLOOR_MIN(CEIL_MAX(val)) / BIN_WIDTH)

#define PREC 0.00001

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
static void remove_zero(float *v, unsigned int len)
{
	int i;
	for(i = 0; i < len; i++) {
		v[i] = (v[i] + PREC) / (1 + ((float)len * PREC));
	}
}


int spect2hist(hist_t *hist, spect_t *spect)
	
{
	int i;
	int start, end;
	int len;

	strcpy(hist->fname, spect->fname);

	spect_get_edges(&start, &end, spect);

	len = end - start;
	if(len <= 0) {
		start = 0;
		len = end = spect->len;
	}
	for(i = 0; i < NBANDS; i++) {
		vec2hist(hist->spect_hist[i], &spect->spect[i][start], len);
		remove_zero(hist->spect_hist[i], SPECT_HIST_LEN);
	}
	
	set_tags(hist);

	if(hist->length == 0) {
		/* guess based on last processed track */
		hist->length = spect->len / last_samples_per_second;
	} else {
		last_samples_per_second = spect->len / hist->length;
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
	if(write_uint(fd, hist->length)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(write_float_vec(fd, hist->spect_hist[i], SPECT_HIST_LEN)) {
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
	if(read_uint(fd, &hist->length)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(read_float_vec(fd, hist->spect_hist[i], SPECT_HIST_LEN)) {
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
		progress(100.0 * (float)i / (float)len);
		if(read_spect(fileno(ifp), &md)) {
			goto err;
		}

		if(spect2hist(&ht, &md)) {
			spect_error("Conversition failed!");
			fflush(stdout);
			exit(-1);
		}
		if(write_hist(fileno(ofp), &ht)) {
			free_spect(&md);
			goto err;
		}
		free_spect(&md);
	}
	progress(100.0);
	errno = 0;
err:
	fclose(ifp);
	fclose(ofp);
	return errno;
}

void plot_hist(hist_t *hist)
{
	plot(NULL, (float *)hist->spect_hist, NBANDS, SPECT_HIST_LEN, PLOT_LINES, 0);
}

