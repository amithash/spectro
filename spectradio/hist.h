#ifndef _HIST_H_
#define _HIST_H_

 #include "../spect-config.h"

 #include <phonon/audiooutput.h>
 #include <phonon/seekslider.h>
 #include <phonon/mediaobject.h>
 #include <phonon/volumeslider.h>
 #include <phonon/backendcapabilities.h>

class Hist
{
	public:
	char   fname[FNAME_LEN];
	char   title[TITLE_LEN];
	char   artist[ARTIST_LEN];
	char   album[ALBUM_LEN];
	unsigned int track;
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
	HistDB(void);
	HistDB(const char *dbname);
	~HistDB();
	unsigned int length(void);
	double distance(unsigned int e1, unsigned int e2);
	void set_media_source(unsigned int ind, Phonon::MediaSource source);
	void set_playing(unsigned int ind);
	int get_next(int current);
	void LoadDB(const char *dbname);
	char *ind_name(unsigned int ind);
	char *ind_title(unsigned int ind);
	char *ind_artist(unsigned int ind);
	char *ind_album(unsigned int ind);
};

#endif

