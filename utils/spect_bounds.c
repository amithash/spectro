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

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <pthread.h>

#include "spect.h"
#include "spect_bounds.h"

#ifndef MEMORY_SIZE
/* Default to 100MB */
#define MEMORY_SIZE (1024 * 100)
#endif

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
	
	find_max_min(&max, &min, band, len);

	find_quantile(above, band, len, ABOVE_QUANTILE, max, min);
	find_quantile(below, band, len, BELOW_QUANTILE, max, min);
}

static void process_band(int i, float bounds[2], int in_mem) {
	float *band;
	/* usable mem is 3/4th of total memory */
	if(in_mem) {
		band = (float *)malloc(sizeof(float) * bands_len[i]);
		if(band) {
			memcpy(band, bands[i], sizeof(float) * bands_len[i]);
		} else {
			in_mem = 0;
		}
	}
	/* in_mem might be changed in the above block */
	if(!in_mem)
		band = bands[i];
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

typedef struct {
	int tid;
	int end_band;
	int start_band;
	pthread_t thread;
	int in_mem;
	float *spect_bands[NBANDS];
} mthread_t;

static void *thread_routine(void *data)
{
	mthread_t *mthread = (mthread_t *)data;
	int i;
	for(i = mthread->start_band; i < mthread->end_band; i++) {
		process_band(i, mthread->spect_bands[i], mthread->in_mem);
	}
	pthread_exit(NULL);
}

static void process_bands(float spect_bands[NBANDS][2])
{
	int i;
	int rc;
	unsigned int kb_per_band;
	unsigned int usable_mem = (3 * MEMORY_SIZE) / 4;
	int in_mem = 0;
	int nr_threads;
	int t;
	mthread_t *mthread;

	if((rc = mmap_bands())) {
		spect_error("Mapping bands failed. rc=%d",rc);
		return;
	}
	kb_per_band = bands_len[0] * sizeof(float) / 1024;
	if(kb_per_band < usable_mem) {
		in_mem = 1;
	}
	nr_threads = usable_mem / kb_per_band;
	if(!nr_threads)
	      nr_threads = 1;
	if(nr_threads > NBANDS)
	      nr_threads = NBANDS;

	printf("Spawning %d threads\n", nr_threads);

	mthread = (mthread_t *)calloc(nr_threads, sizeof(mthread_t));

	for(t = 0; t < nr_threads; t++) {
		int start = t * (NBANDS / nr_threads);
		int end   = (t + 1) * (NBANDS / nr_threads);
		if(t == nr_threads - 1)
		      end = NBANDS;
		mthread[t].tid = t;
		mthread[t].start_band = start;
		mthread[t].end_band   = end;
		for(i = start; i < end; i++) {
			mthread[t].spect_bands[i] = (float *)&spect_bands[i][0];
		}
		mthread[t].in_mem = in_mem;
		if(pthread_create(&(mthread[t].thread), NULL, thread_routine, &mthread[t])) {
			spect_error("Thread creation failed!!!!");
			exit(-1);
		}
	}
	for(t = 0; t < nr_threads; t++) {
		void *status;
		pthread_join(mthread[t].thread, &status);
	}
	free(mthread);

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
	int bytes_wrote = 0;
	if(len == 0) {
		spect_warn("%s resulted in an empty file",spect->fname);
		return 0;
	}
	nbytes = sizeof(float) * len;
	for(i = 0; i < NBANDS; i++) {
		if((bytes_wrote = write(ofd[i], &(spect->spect[i][start]), nbytes)) != nbytes) {
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
		progress(100.0 * (float)i / (float)len);
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
	progress(100.0);
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

