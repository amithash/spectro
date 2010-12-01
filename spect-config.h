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

#ifndef __HIST_CONFIG_H
#define __HIST_CONFIG_H

#define NBANDS 24

#define SPECT_MIN_VAL (0.0)
#define SPECT_MAX_VAL (1.0)
#define SPECT_HIST_LEN      64

#define CEPS_MIN_VAL 0
#define CEPS_MAX_VAL 0.5
#define CEPS_HIST_LEN 64

#define FNAME_LEN 256
#define TITLE_LEN  64
#define ARTIST_LEN 64
#define ALBUM_LEN  64

#define PERIOD_LEN 30

#define BPM_MIN 30
#define BPM_MAX 330
#define BPM_LEN (BPM_MAX - BPM_MIN)

#ifdef PROF
#include <sys/time.h>

#define TIME_DIFF(end, start) (1000000 * (end.tv_sec - start.tv_sec)) + ((long int)end.tv_usec - (long int)start.tv_usec)

#define BEGIN(blk) do{ 					\
	struct timeval __##blk##start,__##blk##end;	\
	unsigned long int __##blk##dur;			\
	gettimeofday(&__##blk##start, NULL);		\
	printf("BEGIN  : " #blk "\n");			\
	fflush(stdout)

#define END(blk)   						\
	gettimeofday(&__##blk##end, NULL);			\
	__##blk##dur = TIME_DIFF(__##blk##end, __##blk##start);	\
	printf("END(%2d): " #blk "\n",__##blk##dur);		\
	fflush(stdout);						\
	fsync(fileno(stdout));					\
}while(0)
#else
#define BEGIN(fmt) 
#define END(fmt)
#endif

#define progress(perc)						\
do {								\
	int ____i;						\
	int ____perc = ((int)perc)/2;				\
	printf("[");						\
	for(____i = 0; ____i < ____perc; ____i++) {		\
		printf("#");					\
	}							\
	for(____i = ____perc; ____i < 50; ____i++) {		\
		printf("-");					\
	}							\
	printf("] %.2f%% Done",perc);				\
	if(____perc == 50) {					\
		printf("\n");					\
	} else {						\
		printf("\r");					\
	}							\
	fflush(stdout);						\
} while(0)




#endif
