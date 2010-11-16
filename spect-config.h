#ifndef __HIST_CONFIG_H
#define __HIST_CONFIG_H

#ifndef NBANDS
#define NBANDS 24
#endif
#if 0
#ifndef BIN_IND
#define BIN_IND  
#endif
#endif

#ifndef PBIN_IND
#define PBIN_IND 6
#endif

#define SPECT_MIN_VAL (0.0)
#define SPECT_MAX_VAL (1.0)
#define SPECT_HIST_LEN      25

#define CEPS_MIN_VAL 0
#define CEPS_MAX_VAL 0.5
#define CEPS_HIST_LEN 50

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

#endif
