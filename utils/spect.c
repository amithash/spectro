#include "spect.h"
#include <pthread.h>
#include<sys/stat.h>


const char *RM_RC[] = {
	"RM_SUCCESS",
	"RM_INVALID_PTR_E",
	"RM_INVALID_FNAME_PTR_E",
	"RM_INVALID_FNAME_E",
	"RM_OPEN_FAILED_E",
	"RM_LENGTH_NOT_SPECT_E",
	"RM_UNMATCHED_END_E",
	"RM_MP3_NOT_FOUND",
	"RM_MALLOC_FAILED_E",
	"RM_READ_FAILED_E"
};

/* Types */
typedef struct {
	int tid;
	char **start;
	int    len;
	int fd;
	pthread_t thread;
} mthread_t;

/* Globals */
static char **spect_files;
static unsigned int real_len = 0;

static pthread_mutex_t read_spect_mutex = PTHREAD_MUTEX_INITIALIZER;

/* Functions */

/* Takes the spect file 'in', and returns the mp3 path if exists
 * in 'out'. returns 0 on success.
 */
static int get_mp3_path(char *out, char *in)
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

/* Checks if 'name' is a valid spect file name
 * : checkes extension for .spect
 */
static int file_name_valid(char *name)
{
	int len = strlen(name);
	if(len < 7 || strcmp(name + (len - strlen(".spect")), ".spect") != 0) {
		return -1;
	}
	return 0;
}

/* Removes a newline if it exists at the end of the string */
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

#define SET_MALLOC(size)  (spect_e_type *)malloc(size * sizeof(spect_e_type))
#define SET_REALLOC(ptr, size) (spect_e_type *)realloc(ptr, size * sizeof(spect_e_type))
#define SPECT_WINDOW 1024

/* Allocates spect's members with len elements */
static int alloc_spect(spect_t *spect, int len)
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		if(!(spect->spect[i] = SET_MALLOC(len))) {
			return -1;
		}
	}
	return 0;
}

static int realloc_spect(spect_t *spect, int len) 
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		if(!(spect->spect[i] = SET_REALLOC(spect->spect[i], len))) {
			return -1;
		}
	}
	return 0;
}

static int final_resize_spect(spect_t *spect) 
{
	int i; 
	for(i = 0; i < NBANDS; i++) {
		if(!(spect->spect[i] = SET_REALLOC(spect->spect[i], spect->len))) {
			return -1;
		}
	}
	return 0;
}

static int write_uint(int fd, unsigned int val) {
	if(write(fd, &val, sizeof(unsigned int)) != sizeof(unsigned int)) {
		return -1;
	}
	return 0;
}

static int write_spect_e_type_vec(int fd, spect_e_type *vec, unsigned int len) 
{
	if(write(fd, vec, sizeof(spect_e_type) * len) != (len * sizeof(spect_e_type))) {
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

static int read_spect_e_type_vec(int fd, spect_e_type *vec, unsigned int len) 
{
	if(read(fd, vec, sizeof(spect_e_type) * len) != (len * sizeof(spect_e_type))) {
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

void free_spect(spect_t *spect)
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		if(spect->spect[i])
		      free(spect->spect[i]);
	}
}

int read_spectf(char *name, spect_t *spect)
{
	int i = 0, j = 0, n = 0;
	spect_e_type c;
	FILE *fp;
	int rc = RM_SUCCESS;
	unsigned int max_len;


	if(!name || !spect)
		return RM_INVALID_PTR_E;
	if(file_name_valid(name) < 0)
		return RM_INVALID_FNAME_E;
	if(get_mp3_path(spect->fname, name))
		return RM_MP3_NOT_FOUND;
	if((fp = fopen(name, "r")) == NULL)
		return RM_OPEN_FAILED_E;

	/* Allocate for SPECT_WINDOW num of samples */
	if(alloc_spect(spect, SPECT_WINDOW))
		return RM_MALLOC_FAILED_E;

	max_len = SPECT_WINDOW;

	/* read a char at a time, data is in row order */
	while((n = read(fileno(fp), &c, sizeof(spect_e_type))) > 0) {

		if(n != sizeof(spect_e_type)) {
			rc = RM_READ_FAILED_E;
			goto bail_out;
		}

		if(i >= max_len) {
			/* if we need to realloc, 
			 * realloc for SPECT_WINDOW extra samples */
			max_len += SPECT_WINDOW;
			if(realloc_spect(spect, max_len)) {
				rc = RM_MALLOC_FAILED_E;
				goto bail_out;
			}
		}

		/* populate */
		spect->spect[j][i] = c;

		/* Prepare for next band, if last band, reset band index j and 
		 * prepare for next sample
		 */
		if(++j == NBANDS) {
			j = 0;
			i++;
		}
	}
	fclose(fp);

	spect->len = i; /* There are a total of i samples */

	/* Free up extra space by resizing */
	final_resize_spect(spect);

	/* if this is an unmatched spect file (last sample does not have
	 * all bands present), error condition */
	if(j != 0) {
		rc = RM_UNMATCHED_END_E;
		goto bail_out;
	}

	return RM_SUCCESS;

bail_out:
	free_spect(spect);
	return rc;
}

static void *thread_routine(void *data) 
{
	int rc;
	mthread_t *mthread = (mthread_t *)data;
	char    **spect_list;
	int fd = mthread->fd;
	int       len;
	int       i;
	if(!mthread) {
		goto thread_exit;
	}
	spect_list = mthread->start;
	len        = mthread->len;

	if(!spect_list) {
		goto thread_exit;
	}
	if(len <= 0) {
		goto thread_exit;
	}

	for(i = 0; i < len; i++) {
		spect_t spect;
		if((rc = read_spectf(spect_list[i], &spect)) != RM_SUCCESS) {
			spect_warn("Reading %s resulted in an invalid spect file. err=%s",spect_list[i], RM_RC_STR(rc));
			continue;
		}
		pthread_mutex_lock(&read_spect_mutex);
		if((rc = write_spect(fd, &spect)) != 0) {
			spect_error("Writing %s resulted in rc = %d\n", spect_list[i], rc);
			i = len;
		} else {
			real_len++;
		}
		pthread_mutex_unlock(&read_spect_mutex);
		free_spect(&spect);
		fsync(fileno(stdout));
		fsync(fileno(stderr));
	}

thread_exit:
	pthread_exit(NULL);
}

static int spect_files_init(char *name, unsigned int *out_len)
{
	char tmp[FNAME_LEN];
	FILE *ifp;
	int i;
	int len = 0;

	if((ifp = fopen(name, "r")) == NULL) {
		return RMFL_OPEN_FAILED_E;
	}

	/* Get length of the list */
	while(fgets(tmp, FNAME_LEN, ifp) != NULL) {
		len++;
	}
	fclose(ifp);
	if((ifp = fopen(name, "r")) == NULL) {
		return RMFL_OPEN_FAILED_E;
	}
	*out_len = len;

	if((spect_files = (char **) malloc(len * sizeof(char *))) == NULL) {
		return -1;
	}

	for(i = 0; i < len; i++) {
		if((spect_files[i] = (char *)malloc(FNAME_LEN * sizeof(char))) == NULL) {
			return RMFL_MALLOC_FAILED_E;
		}
	}
	i = 0;
	while(fgets(tmp, FNAME_LEN, ifp) != NULL) {
		strncpy(spect_files[i], tmp, FNAME_LEN);
		remove_nline(spect_files[i++], FNAME_LEN);
	}
	fclose(ifp);
	return 0;
}

static void spect_files_exit(int len)
{
	int i;
	if(!spect_files)
	      return;
	for(i = 0; i < len; i++) {
		if(spect_files[i])
			free(spect_files[i]);
	}
	free(spect_files);
	spect_files = NULL;
}

int combine_spect_list(char *ifname, 
		    char *ofname,
		   int nthreads)
{
	int i = 0;
	FILE *ofp;
	mthread_t *mthread;
	unsigned int len = 0;

	if(!ifname || !ofname) {
		return RMFL_INVALID_FNAME_PTR_E;
	}
	if(spect_files_init(ifname, &len)) {
		return RMFL_OPEN_FAILED_E;
	}
	if((ofp = fopen(ofname, "w")) == NULL) {
		return RMFL_OPEN_FAILED_E;
	}
	if(nthreads < 1) {
		nthreads = 1;
	}

	if(write_uint(fileno(ofp), len)) {
		fclose(ofp);
		return -1;
	}

	mthread = calloc(nthreads, sizeof(mthread_t));
	if(!mthread) {
		return RMFL_MALLOC_FAILED_E;
	}

	for(i = 0; i < nthreads; i++) {
		mthread[i].fd = fileno(ofp);
		mthread[i].tid = i;
		mthread[i].start = &spect_files[i * (len / nthreads)];
		mthread[i].len = i == nthreads - 1 ?
		len - ((nthreads - 1) * (len / nthreads)) :
		len / nthreads;
		if(pthread_create(&(mthread[i].thread), NULL, thread_routine, &mthread[i])) {
			return RMFL_THREAD_CREATE_E;
		}
	}

	for(i = 0; i < nthreads; i++) {
		void *status;
		pthread_join(mthread[i].thread, &status);
	}
	free(mthread);

	spect_files_exit(len);

	fflush(ofp);

	if(real_len != len) {
		spect_warn("Some files were invalid, correcting len...");
		rewind(ofp);
		if(write_uint(fileno(ofp), real_len)) {
			spect_error("Rewind and re-write len failed!");
			spect_error("Output %s is corrupt!",ofname);
		}
	}

	fclose(ofp);

	return RMFL_SUCCESS;
}


int write_spect(int fd, spect_t *spect) {
	int i;
	if(!spect) {
		return -1;
	}
	if(write_uint(fd, spect->len)) {
		return -1;
	}
	if(write_char_vec(fd, spect->fname, FNAME_LEN)) {
		return -1;
	}
	for(i = 0; i < NBANDS; i++) {
		if(write_spect_e_type_vec(fd, spect->spect[i], spect->len)) {
			return -1;
		}
	}
	return 0;
}
int read_spect(int fd, spect_t *spect) 
{
	int i;
	if(!spect) {
		return -1;
	}
	if(read_uint(fd, &(spect->len))) {
		return -1;
	}
	if(read_char_vec(fd, spect->fname, FNAME_LEN)) {
		return -1;
	}
	if(alloc_spect(spect, spect->len)) {
		spect_error("Mem failure in read_spect!");
		return -2;
	}

	for(i = 0; i < NBANDS; i++) {
		if(read_spect_e_type_vec(fd, spect->spect[i], spect->len)) {
			return -1;
		}
	}
	return 0;
}

