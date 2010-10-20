#ifndef __HIST_CONFIG_H
#define __HIST_CONFIG_H

#ifndef NBANDS
#define NBANDS 24
#endif

#ifndef SPECTLEN
#define SPECTLEN 8192
#endif

#define FNAME_LEN 256

#ifndef BIN_IND
#define BIN_IND  2
#endif


#define BIN_SIZE (1 << BIN_IND)
#define HIST_LEN (256/BIN_SIZE)
#define NUM2BIN(num) (num / HIST_LEN)

#define FNAME_LEN 256
#define TITLE_LEN  64
#define ARTIST_LEN 64
#define ALBUM_LEN  64


#endif
