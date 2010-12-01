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
#ifndef _HIST_H_
#define _HIST_H_

 #include "../spect-config.h"

 #include <vector>
 #include <phonon/audiooutput.h>
 #include <phonon/seekslider.h>
 #include <phonon/mediaobject.h>
 #include <phonon/volumeslider.h>
 #include <phonon/backendcapabilities.h>

class Hist
{
	public:
	QString fname;
	QString title;
	QString artist;
	QString album;
	unsigned int track;
	float spect_hist[NBANDS][SPECT_HIST_LEN];
	int played;
	Phonon::MediaSource media_source;

	Hist(void)
	{
	    	played = 0;
	}
};

class HistDB
{
	std::vector<Hist> list;
	unsigned int valid;
	float skldistance(float *a, float *b, unsigned int len);

	public:
	bool is_valid();
	HistDB(void);
	HistDB(const char *dbname);
	~HistDB();
	unsigned int length(void);
	float distance(unsigned int e1, unsigned int e2);
	void set_media_source(unsigned int ind, Phonon::MediaSource source);
	void set_playing(unsigned int ind);
	int get_next(int current);
	void LoadDB(const char *dbname);
	QString ind_name(unsigned int ind);
	QString ind_title(unsigned int ind);
	QString ind_artist(unsigned int ind);
	QString ind_album(unsigned int ind);
};

#endif

