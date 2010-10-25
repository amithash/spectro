#include "hist.h"
#include "auto.h"
#include <pthread.h>
#include <tag_c.h>

#define SPECT_VAL(spect,i,j) (((double)(spect)->spect[i][j]) / 255.0)
#define SQR(val) ((val) * (val))

#define SAMPLES_PER_STEP(per_sec) (per_sec * BEAT_STEP / 1000)

static last_samples_per_second = 0;

int get_beat(spect_t *spect, double beats[NBANDS][BEAT_LEN], int song_len)
{
	void *fft_hdl;
	double *tmp;
	double *band;
	double *autoc;
	int i,j,k;
	int rc = 0;
	int step;
	int samples_per_sec;
	TagLib_File *f;
	double total = 0;

	samples_per_sec = spect->len / song_len;

	fft_hdl = init_fft(spect->len);


	if(!fft_hdl) {
	      rc = -1; goto err1;
	}

	autoc  = (double *)malloc(sizeof(double) * spect->len);
	if(!autoc) {
		rc = -1; goto err2;
	}
	
	step = SAMPLES_PER_STEP(samples_per_sec);

  for(k = 0; k < NBANDS; k++) {
    auto_correlation(fft_hdl, spect->spect[i], autoc);

	  for(i = 0; i < BEAT_LEN; i++) {
		  double avg = 0;
		  for(j = step + (i * step); j < step + ((i + 1) * step); j++) {
			  avg += autoc[j];
		  }
		  total += beats[k][i] = avg / step;
	  }
	  for(i = 0; i < BEAT_LEN; i++) {
		  beats[k][i] /= total;
	  }
  }

	free(autoc);
err2:
	destroy_fft(fft_hdl);
err1:
	return rc;
	
}


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

int spect2hist(hist_t *hist, spect_t *spect)
	
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

	if(hist->length == 0) {
		/* guess based on last processed track */
		hist->length = spect->len / last_samples_per_second;
	} else {
		last_samples_per_second = spect->len / hist->length;
	}

	if(get_beat(spect, hist->beats, hist->length)) {
		return -1;
	}

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
	if(write_double_vec(fd, hist->beats, BEAT_LEN)) {
		return -1;
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
	if(read_double_vec(fd, hist->beats, BEAT_LEN)) {
		return -1;
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

