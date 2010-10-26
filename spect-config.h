#ifndef __HIST_CONFIG_H
#define __HIST_CONFIG_H

#ifndef NBANDS
#define NBANDS 24
#endif

#ifndef BIN_IND
#define BIN_IND  0
#endif

#define BIN_SIZE (1 << BIN_IND)
#define HIST_LEN (256/BIN_SIZE)
#define NUM2BIN(num) (num / BIN_SIZE)

#define FNAME_LEN 256
#define TITLE_LEN  64
#define ARTIST_LEN 64
#define ALBUM_LEN  64

/* All in milli seconds */
#define BEAT_STEP 100
#define BEAT_MIN  200
#define BEAT_MAX  (4000 + BEAT_STEP)
#define BEAT_LEN  ((BEAT_MAX - BEAT_MIN) / BEAT_STEP)

#endif