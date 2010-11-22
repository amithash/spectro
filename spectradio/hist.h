 /****************************************************************************
 **
 ** Copyright (C) 2010 Amithash Prasad <amithash@gmail.com>
 ** All rights reserved.
 **
 ** This file is part of spectradio
 **
 ** $QT_BEGIN_LICENSE:BSD$
 ** You may use this file under the terms of the BSD license as follows:
 **
 ** "Redistribution and use in source and binary forms, with or without
 ** modification, are permitted provided that the following conditions are
 ** met:
 **   * Redistributions of source code must retain the above copyright
 **     notice, this list of conditions and the following disclaimer.
 **   * Redistributions in binary form must reproduce the above copyright
 **     notice, this list of conditions and the following disclaimer in
 **     the documentation and/or other materials provided with the
 **     distribution.
 **   * The name of Amithash Prasad may be used to endorse or promote
 **     products derived from this software without specific prior written
 **     permission.
 **
 ** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 ** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 ** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 ** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 ** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 ** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 ** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 ** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 ** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 ** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 ** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
 ** $QT_END_LICENSE$
 **
 ***************************************************************************/

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
	float ceps_hist[NBANDS][CEPS_HIST_LEN];
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
	float bdistance(float *a, float *b, unsigned int len);
	float edistance(float *dist, unsigned int len);

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

