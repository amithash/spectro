#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>

#include "spect.h"
#include "spect_bounds.h"

int ofd[NBANDS];
float *bands[NBANDS];
int bands_len[NBANDS];

#define PREC 0.00000001

#define ABOVE_QUANTILE 0.99 /* 99% quantile */
#define BELOW_QUANTILE (1.0 - ABOVE_QUANTILE) /* The other end */
#define ALLOWED_ERROR 0.001 /* Allow (+/-)0.1% error */

static void find_max_min(float *_max, float *_min, float *band, unsigned int len)
{
	float max,min;
	int i;
	max = min = band[0];
	for(i = 1; i < len; i++) {
		if(band[i] > max)
		      max = band[i];
		if(band[i] < min)
		      min = band[i];
	}
	*_max = max;
	*_min = min;
}

#define APPROX_EQUAL(a,b,err)  (  ( (a) > ( (b) - (err) ) ) && ( (a) < ( (b) + (err) ) ) )

/*-----------------------------------------------------------------------------------
 * FUNCTION: find_quantile
 *
 * DESCRIPTION:
 * Find the approximate quantile value of a given array. A binary search type 
 * algorithm is used to, thus this algorithm has a worst case complexity of
 * Q(nlogn). But the average is much less than that. The function continnues
 * till it reaches its worst case behavior, or the error is below ALLOWED_ERROR.
 *
 * Setting ALLOWED_ERROR to 0, will not gaurantee that the returned quantile will
 * be exactly the quantile got by the actual procedure: sorting and indexing.
 *
 * PARAMETERS:
 *
 * _quantile	out	Address in which the quantile value must be returned
 * band		in	array of elements
 * len		in	length of array band
 * req_quantile in	Required quantile as a fraction. Example, 90% quantile
 * 			will be 0.9.
 * max		in	maximum value of the elements in 'band'
 * min		in	minimum value of the elements in 'band'
 *
 * ASSUMPTIONS:
 * band, _quantile are valid pointers. No checks are made.
 *
 * SIDE EFFECTS:
 * None
 *----------------------------------------------------------------------------------*/

static void find_quantile(float *_quantile, float *band, unsigned int len, 
		   float req_quantile, float max, float min)
{
	unsigned int nerror = (unsigned int)((float)len * ALLOWED_ERROR);
	float quantile = (max - min) / 2;
	int found = 0;
	float step;
	unsigned int req_nabove = len - (req_quantile * (float)len);
	unsigned int nabove = 0;
	int i;
	int iter = 0;
	do {
		if(iter == len/2) {
			spect_error("Max iterations reached....");
			break;
		}
		nabove = 0;
		for(i = 0; i < len; i++) {
			if(band[i] > quantile)
			      nabove++;
		}
		if(APPROX_EQUAL(nabove, req_nabove, nerror))
		      break;
		
		/* Too many above the value quantile, increase value */
		if(nabove > req_nabove) {
			min = quantile; /* Quantile can never be below the current value */
			step = (max - quantile) / 2.0; /* Binary like search */
			quantile = quantile + step;
		} else { /* Too many below the value quantile, decrease value */
			max = quantile; /* Quantile can never be above the current value */
			step = (quantile - min) / 2.0; /* Binary like search */
			quantile = quantile - step;
		}
		iter++;
	} while(found == 0);

	*_quantile = quantile;

#ifdef DEBUG
	printf("Quantile=%.6f, %%above=%.2f, %%below=%.2f\n", quantile, 
				(float)nabove * 100.0 / (float)len, 
				((float)len - (float)nabove) * 100.0 / (float)len);

	printf("Iterations = %d\n",iter);
	printf("Took %.2f%% of time compared to O(nlogn)\n", 100.0 * (float)iter / (log(len) / log(2)));
#endif

}

static void _process_band(float *band, unsigned int len, float *above, float *below)
{
	float max,min;
	float median;
	
	find_max_min(&max, &min, band, len);

	find_quantile(above, band, len, ABOVE_QUANTILE, max, min);
	find_quantile(below, band, len, BELOW_QUANTILE, max, min);
	find_quantile(&median, band, len, 0.5, max, min);
}

static void process_band(int i, float bounds[2]) {
	float *band;
	unsigned int kb = bands_len[i] * sizeof(float) / 1024;
	/* usable mem is 3/4th of total memory */
	unsigned int usable_mem = (3 * MEMORY_SIZE) / 4;
	int in_mem = 0;
	if(kb > usable_mem) {
		band = bands[i]; /* Work with the file */
	} else {
		band = (float *)malloc(sizeof(float) * bands_len[i]);
		if(!band) {
			band = bands[i]; /* Malloc failed, work with file */
		} else {
			memcpy(band, bands[i], sizeof(float) * bands_len[i]);
			in_mem = 1;
		}
	}
#ifdef DEBUG
	if(in_mem == 1) {
		printf("Processing in memory\n");
	} else {
		printf("Memory not avaliable, processing in i/o\n");
	}
#endif
	
	_process_band(band, bands_len[i], &bounds[1], &bounds[0]);

	if(in_mem)
	      free(band);
}

static int mmap_bands(void)
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		struct stat sb;
		int fd;
		char fname[100];
		sprintf(fname, "band_%d.dat",i);
		fd = open (fname, O_RDONLY);
		if(fd == -1) {
			return -1 *__LINE__;
		}
		if (fstat (fd, &sb) == -1) {
			return -1 * __LINE__;
		}
		if (!S_ISREG (sb.st_mode)) {
			return -1 * __LINE__;
		}
		bands_len[i] = sb.st_size / sizeof(float);

		bands[i] = (float *)mmap(0, sb.st_size, PROT_READ, MAP_SHARED, fd, 0);
		if (bands[i] == MAP_FAILED) {
			 return -1 * __LINE__;
		}
		if (close (fd) == -1) {
			return -1 * __LINE__;
		}
	}
	return 0;
}

static int munmap_bands(void)
{
	int i;
	int rc = 0;
	for(i = 0; i < NBANDS; i++) {
		if (munmap (bands[i], bands_len[i] * sizeof(float)) == -1) {
				rc = -1;
		}		
	}
	return rc;
}


static void process_bands(float spect_bands[NBANDS][2])
{
	/*Either create 24 threads, each processing a single band.. or do all here */
	int i;
	int rc;

	if((rc = mmap_bands())) {
		spect_error("Mapping bands failed. rc=%d",rc);
		return;
	}
	for(i = 0; i < NBANDS; i++) {
#ifdef DEBUG
		printf("Band:%d\n", i);
#endif
		fsync(fileno(stdout));
		process_band(i, spect_bands[i]);
	}
	printf("\n");

	if((rc = munmap_bands())) {
		spect_error("Un-Mapping bands failed. rc=%d",rc);
		return;
	}
}

static int files_exists(void)
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		char name[100];
		struct stat buf;
		int err;
		sprintf(name, "band_%d.dat", i);
		err = stat ( name, &buf );
		if(err != 0) {
			return 0;
		}
	}
	return 1;
}

static int open_band_files(void)
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		char outn[100];
		FILE *fp;
		sprintf(outn, "band_%d.dat",i);
		fp = fopen(outn, "w");
		if(fp == NULL) {
			return -1;
		}
		ofd[i] = fileno(fp);
	}
	return 0;
}

static void close_bands(void)
{
	int i;
	for(i = 0; i < NBANDS; i++) {
		close(ofd[i]);
	}
}



static int populate_band_files(spect_t *spect)
{
	int start, end;
	int len;
	int nbytes;
	int i;
	spect_get_edges(&start, &end, spect);
	len = end - start;
	if(len == 0) {
		spect_warn("%s resulted in an empty file",spect->fname);
		return 0;
	}
	nbytes = sizeof(float) * len;
	for(i = 0; i < NBANDS; i++) {
		if(write(ofd[i], &(spect->spect[i][start]), nbytes) != nbytes) {
			return -1;
		}
	}
	return 0;
}

static int create_band_files(char *spect_db_name)
{
	FILE *ifp;
	int	ifd;
	spect_t spect;
	unsigned int len;
	int i;

	ifp = fopen(spect_db_name, "r");
	if(ifp == NULL) {
		spect_error("Cannot find/open %s",spect_db_name);
		return -1;
	}

	ifd = fileno(ifp);

	if(open_band_files()) {
		spect_error("Could not create temp files");
		return -1;
	}

	if(read(ifd, &len, sizeof(unsigned int)) != sizeof(unsigned int)) {
		spect_error("Read for file len failed!");
		fclose(ifp);
		return -1;
	}
	for(i = 0; i < len; i++) {
		if(read_spect(ifd, &spect)) {
			spect_error("Read of %dth record failed!\n",i);
			return -1;
		}

		if(populate_band_files(&spect)) {
			spect_error("Could not populate band files for record %d",i);
			return -1;
		}
		free_spect(&spect);
	}
	close_bands();

	return 0;
}

void spect_bounds_exit(void)
{
	int i;
	char name[100];
	for(i = 0; i < NBANDS; i++) {
		sprintf(name, "rm -f band_%d.dat",i);
		system(name);
	}
}

int spect_bounds(char *db_name, float spect_bounds[NBANDS][2])
{
	if(!files_exists()) {
		printf("Creating band files\n");
		if(create_band_files(db_name)) {
			spect_error("Creating band files failed\n");
			exit(-1);
		}
	}

	printf("Processing band files\n");
	process_bands(spect_bounds);

	return 0;
}

