#ifndef __HIST_CONFIG_H
#define __HIST_CONFIG_H

#ifndef NBANDS
#define NBANDS 24
#endif

#ifndef BIN_IND
#define BIN_IND  0
#endif

#ifndef PBIN_IND
#define PBIN_IND 3
#endif

#define BIN_SIZE (1 << BIN_IND)
#define HIST_LEN (256/BIN_SIZE)
#define NUM2BIN(num) (num / BIN_SIZE)

#define PBIN_SIZE (1 << PBIN_IND)
#define PHIST_LEN (256/PBIN_SIZE)
#define NUM2PBIN(num) (num / PBIN_SIZE)

#define FNAME_LEN 256
#define TITLE_LEN  64
#define ARTIST_LEN 64
#define ALBUM_LEN  64

#define PERIOD_LEN 100

#endif
