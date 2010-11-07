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
#define SPECT_MAX_VAL (1.4)

#define HIST_LEN      25

#define BIN_WIDTH     ((SPECT_MAX_VAL - SPECT_MIN_VAL) / HIST_LEN)

#define FLOOR_MIN(val) ((val) < SPECT_MIN_VAL ? SPECT_MIN_VAL : (val))
#define CEIL_MAX(val)  ((val) >= SPECT_MAX_VAL ? (SPECT_MAX_VAL - BIN_WIDTH) : (val))

#define NUM2BIN(val)    (unsigned int)(FLOOR_MIN(CEIL_MAX(val)) / BIN_WIDTH)


#define PBIN_SIZE (1 << PBIN_IND)
#define PHIST_LEN (256/PBIN_SIZE)
#define NUM2PBIN(num) (num / PBIN_SIZE)

#define FNAME_LEN 256
#define TITLE_LEN  64
#define ARTIST_LEN 64
#define ALBUM_LEN  64

#define PERIOD_LEN 30

#define BPM_MIN 30
#define BPM_MAX 330
#define BPM_LEN (BPM_MAX - BPM_MIN)

#endif
