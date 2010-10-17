#include "spect.h"
#include <pthread.h>
#include<sys/stat.h>


typedef struct {
	int tid;
	spect_t *start;
	int    len;
	pthread_t thread;
} mthread_t;

int get_mp3_path(char *out, char *in)
{
	int str_len;
	char tmp[FNAME_LEN];
	const char *exts[4] = {
		"mp3",
		"MP3",
		"mP3",
		"Mp3"
	};
	int i;

	memcpy(out, in, FNAME_LEN * sizeof(char));
	str_len = strlen(out);
	/* Get rid of the . at the start of the file name */
	for(i = str_len + 1; i > 0; i--) {
		if(out[i] == '/') {
			break;
		}
	}
	if(i == 0 || i == str_len + 1)
	      return -1;
	i++;
	for(; i < str_len + 2; i++) {
		out[i] = out[i+1];
	}
	str_len = strlen(out);
	for(i = str_len ; i > 0; i--) {
		if(out[i] == '.')
		      break;
	}
	out[++i] = '\0';
	for(i = 0; i < 4; i++) {
		struct stat buf;
		int exists;
		strcpy(tmp, out);
		strcat(tmp,exts[i]);
		exists = stat ( tmp, &buf );
		if(exists == 0) {
			strcpy(out, tmp);
			break;
		}
	}
	if(i == 4) 
	      return -1;
	return 0;
}

static int file_name_valid(char *name)
{
	int len = strlen(name);
	if(len < 7 || strcmp(name + (len - strlen(".spect")), ".spect") != 0) {
		return -1;
	}
	return 0;
}

static int remove_nline(char *name, int maxlen) {
	int i, len;
	if(!name) {
		return -1;
	}
	len = strlen(name);
	if(len > maxlen) {
		name[maxlen - 1] = '\0';
		return 0;
	}

	for(i = 0; i < len; i++) {
		if(name[i] == '\n') {
			name[i] = '\0';
			break;
		}
	}
	return 0;
}

int read_spect(spect_t *spect)
{
	int i = 0, j = 0, n = 0;
	unsigned char c;
	FILE *fp;
	char tmp[FNAME_LEN];


	if(!spect) {
		return RM_INVALID_PTR_E;
	}
	if(!spect->fname) {
		return RM_INVALID_FNAME_PTR_E;
	}
	if(file_name_valid(spect->fname) < 0) {
		return RM_INVALID_FNAME_E;
	}
	if((fp = fopen(spect->fname, "r")) == NULL) {
		return RM_OPEN_FAILED_E;
	}

	while((n = read(fileno(fp), &c, sizeof(unsigned char))) > 0) {
		if(i >= SPECTLEN) {
			fclose(fp);
			return RM_LENGTH_NOT_SPECT_E;
		}
		spect->spect[j][i] = (unsigned char)c & 0xff;
		if(++j == NBANDS) {
			j = 0;
			i++;
		}
	}
	fclose(fp);
	spect->len = i;
	if(j != 0) {
		return RM_UNMATCHED_END_E;
	}
	strcpy(tmp, spect->fname);
	if(get_mp3_path(spect->fname, tmp)) {
		return RM_MP3_NOT_FOUND;
	}

	/* Always keep this the last */
	spect->valid = 1;
	return RM_SUCCESS;
}
static void *thread_routine(void *data) 
{
	int rc;
	mthread_t *mthread = (mthread_t *)data;
	spect_t    *spect;
	int       len;
	int       i;
	if(!mthread) {
		goto thread_exit;
	}
	spect = mthread->start;
	len  = mthread->len;
	if(!spect) {
		goto thread_exit;
	}
	if(len <= 0) {
		goto thread_exit;
	}

	for(i = 0; i < len; i++) {
		if((rc = read_spect(&spect[i])) != RM_SUCCESS) {
			spect_warn("Reading %s resulted in an invalid spect file. rc=%d",spect[i].fname, rc);
		}
	}

thread_exit:
	pthread_exit(NULL);
}

int read_spect_list(char *fname, 
		   spect_t **list, 
		   unsigned int *len,
		   int nthreads)
{
	char tmp[FNAME_LEN];
	spect_t *spect;
	int i = 0;
	FILE *fp;
	mthread_t *mthread;

	*len = 0;

	if(!fname) {
		return RMFL_INVALID_FNAME_PTR_E;
	}
	if(!list) {
		return RMFL_INVALID_LIST_PTR_E;
	}
	if((fp = fopen(fname, "r")) == NULL) {
		return RMFL_OPEN_FAILED_E;
	}
	while(fgets(tmp, FNAME_LEN, fp) != NULL) {
		(*len)++;
	}
	fclose(fp);

	if((fp = fopen(fname, "r")) == NULL) {
		return RMFL_OPEN_FAILED_E;
	}
	spect = *list = (spect_t *)calloc(*len, sizeof(spect_t));
	if(!spect) {
		return RMFL_MALLOC_FAILED_E;
	}
	while(fgets(tmp, FNAME_LEN, fp) != NULL) {
		strncpy(spect[i++].fname, tmp, FNAME_LEN);
		remove_nline(spect[i-1].fname, FNAME_LEN);
		spect[i-1].valid = 0;
	}
	fclose(fp);

	if(nthreads < 1) {
		nthreads = 1;
	}

	mthread = calloc(nthreads, sizeof(mthread_t));
	if(!mthread) {
		return RMFL_MALLOC_FAILED_E;
	}

	for(i = 0; i < nthreads; i++) {
		mthread[i].tid = i;
		mthread[i].start = &spect[i * (*len / nthreads)];
		mthread[i].len = i == nthreads - 1 ?
		*len - ((nthreads - 1) * ((*len) / nthreads)) :
		*len / nthreads;
		if(pthread_create(&(mthread[i].thread), NULL, thread_routine, &mthread[i])) {
			return RMFL_THREAD_CREATE_E;
		}
	}

	for(i = 0; i < nthreads; i++) {
		void *status;
		pthread_join(mthread[i].thread, &status);
	}

	free(mthread);

	return RMFL_SUCCESS;
}

static int write_uint(int fd, unsigned int val) {
	if(write(fd, &val, sizeof(unsigned int)) != sizeof(unsigned int)) {
		return -1;
	}
	return 0;
}

static int write_uchar_vec(int fd, unsigned char *vec, unsigned int len) 
{
	if(write(fd, vec, sizeof(unsigned char) * len) != (len * sizeof(unsigned char))) {
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

static int read_uint(int fd, unsigned int *val) {
	if(read(fd, val, sizeof(unsigned int)) != sizeof(unsigned int)) {
		return -1;
	}
	return 0;
}

static int read_uchar_vec(int fd, unsigned char *vec, unsigned int len) 
{
	if(read(fd, vec, sizeof(unsigned char) * len) != (len * sizeof(unsigned char))) {
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

static int write_spect(int fd, spect_t *spect) {
	int i;
	if(!spect) {
		return -1;
	}
	if(spect->valid != 1) {
		return 0;
	}
	if(write_char_vec(fd, spect->fname, FNAME_LEN)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(write_uchar_vec(fd, spect->spect[i], SPECTLEN)) {
			return -1;
		}
	}
	if(write_uint(fd, spect->valid)) {
		return -1;
	}
	if(write_uint(fd, spect->len)) {
		return -1;
	}
	return 0;
}
int read_spect_2(int fd, spect_t *spect) 
{
	int i;
	if(!spect) {
		return -1;
	}
	if(read_char_vec(fd, spect->fname, FNAME_LEN)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(read_uchar_vec(fd, spect->spect[i], SPECTLEN)) {
			return -1;
		}
	}
	if(read_uint(fd, &(spect->valid))) {
		return -1;
	}
	if(read_uint(fd, &(spect->len))) {
		return -1;
	}


	return 0;
}


int write_spect_db(char *fname, spect_t *list, unsigned int len)
{
	unsigned int real_len = 0;
	int i;
	FILE *fp;
	int fd;

	if(!list || !fname) {
		return WML_INVALID_PTR_E;
	}

	for(i = 0; i < len; i++) {
		if(list[i].valid == 1) {
			real_len++;
		}
	}
	if((fp = fopen(fname, "w")) == NULL) {
		return WML_OPEN_FAILED_E;
	}
	fd = fileno(fp);
	if(write_uint(fd, real_len)) {
		goto write_failed;
	}
	for(i = 0; i < len; i++) {
		if(list[i].valid != 1)
		      continue;
		if(write_spect(fd, &list[i])) {
			goto write_failed;
		}
	}
	fclose(fp);

	return 0;

write_failed:
	fclose(fp);
	return WML_WRITE_E;
}

int read_spect_db(char *fname, spect_t **list, unsigned int *len)
{
	FILE *fp;
	int fd;
	int i;
	unsigned int rlen;
	spect_t *spect;
	if(!list || !len) {
		return RML_INVALID_PTR_E;
	}
	if((fp = fopen(fname, "r")) == NULL) {
		return RML_OPEN_FAILED_E;
	}
	fd = fileno(fp);

	if(read_uint(fd, &rlen)) {
		goto read_failed;
	}
	*len = rlen;
	spect = *list = calloc(rlen, sizeof(spect_t));
	for(i = 0; i < rlen; i++) {
		if(read_spect_2(fd, &spect[i])) {
			goto read_failed_2;
		}
	}

	fclose(fp);

	return 0;
read_failed_2:
	free(spect);
	*list = NULL;
	*len  = 0;
read_failed:
	fclose(fp);
	return RML_READ_E;
}

