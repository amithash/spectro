#ifndef _HIST_H_
#define _HIST_H_


#ifndef BIN_IND
#define BIN_IND  2
#endif

#define BIN_SIZE (1 << BIN_IND)
#define HIST_LEN (256/BIN_SIZE)

#define NBANDS 24

#define FNAME_LEN 256

 #include <phonon/audiooutput.h>
 #include <phonon/seekslider.h>
 #include <phonon/mediaobject.h>
 #include <phonon/volumeslider.h>
 #include <phonon/backendcapabilities.h>

class Hist
{
	public:
	char   fname[FNAME_LEN];
	double hist[NBANDS][HIST_LEN];
	int played;
	Phonon::MediaSource media_source;

	Hist(void)
	    {
	    	played = 0;
	    }
};

class HistDB
{
	Hist *list;
	unsigned int len;
	unsigned int valid;

	public:
	bool is_valid();
	char *ind_name(unsigned int ind);
	HistDB(void);
	HistDB(const char *dbname);
	~HistDB();
	unsigned int length(void);
	double distance(unsigned int e1, unsigned int e2);
	void set_media_source(unsigned int ind, Phonon::MediaSource source);
	void set_playing(unsigned int ind);
	int get_next(int current);
	void LoadDB(const char *dbname);
};

#endif

