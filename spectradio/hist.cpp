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

#include "hist.h"
#include <fstream>
#include <iostream>
#include <string>
#include <math.h>
#include <float.h>
#include <sys/stat.h>

HistDB::HistDB(void)
{
	valid = 0;
}

bool HistDB::is_valid()
{
	return valid == 1;
}

void HistDB::LoadDB(const char *dbname)
{
     std::ifstream ifs(dbname, std::ios::in | std::ios::binary);
     valid = 0;
     unsigned int len = 0;

     if(!ifs.read((char *)&len, sizeof(unsigned int))) {
     	std::cout << "Reading length from " << dbname << std::endl;
	return;
     }
     if(len <= 0)
	   return;

     int ext_len = list.size();
     len += ext_len;
     list.resize(len);

     for(unsigned int i = ext_len; i < len; i++) {
	     char fname[FNAME_LEN];
	     char title[TITLE_LEN];
	     char artist[ARTIST_LEN];
	     char album[ALBUM_LEN];

	     if(!ifs.read((char *)fname, sizeof(char) * FNAME_LEN)) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }
	     if(!ifs.read((char *)title, sizeof(char) * TITLE_LEN)) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }
	     if(title[0] == '\0') {
		int len = strlen(fname);
		if(len < TITLE_LEN) {
			strcpy(title, fname);
		} else {
			int ind = len - TITLE_LEN + 1;
			strcpy(title, &(fname[ind]));
			len = strlen(title);
			title[len - 4] = '\0';
		}
	     }

	     if(!ifs.read((char *)artist, sizeof(char) * ARTIST_LEN)) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }
	     if(!ifs.read((char *)album, sizeof(char) * ALBUM_LEN)) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }

	     list[i].album = album;
	     list[i].artist = artist;
	     list[i].title = title;
	     list[i].fname = fname;


	     if(!ifs.read((char *)&list[i].track, sizeof(unsigned int))) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }
	     for(int j = 0; j < NBANDS; j++) {
	     	if(!ifs.read((char *)list[i].hist[j], (sizeof(double) * HIST_LEN) / sizeof(char))) {
	     		std::cerr << "Error reading element number " << i << "Band: " << j << std::endl;
			goto err;
	     	}
	     }
     }
     valid = 1;
     return;
err:
     list.clear();
}

HistDB::HistDB(const char *dbname)
{
	LoadDB(dbname);
}

HistDB::~HistDB()
{
	if(list.size() > 0) {
		list.clear();
	}
}

unsigned int HistDB::length()
{
	return list.size();
}

/* Compute the hellinger distance of two pdfs a and b */
double HistDB::hdistance(double *a, double *b, unsigned int len) 
{
	double dist = 0.0;
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}

	for(i = 0; i < len; i++) {
		dist += sqrt(a[i] * b[i]);
	}
	return sqrt(1 - dist);
}

/* Compute the euclidian distance */
double HistDB::edistance(double *dist, unsigned int len) 
{
	unsigned int i;
	double val = 0;
	if(dist == NULL) {
		return 0;
	}

	for(i = 0; i < len; i++) {
		val += pow(dist[i], 2);
	}
	return sqrt(val / len);
}

double HistDB::distance(unsigned int e1, unsigned int e2)
{
	int col;
	double dist[NBANDS];
	if(e1 >= list.size() || e1 >= list.size())
	      return 0;

	for(col = 0; col < NBANDS; col++) {
		dist[col] = hdistance(list[e1].hist[col],
				      list[e2].hist[col],
				      HIST_LEN);
	}
	return edistance(dist, NBANDS);
}
QString HistDB::ind_name(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	return list[ind].fname;
}

QString HistDB::ind_title(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	return list[ind].title;
}

QString HistDB::ind_artist(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	return list[ind].artist;
}

QString HistDB::ind_album(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	return list[ind].album;
}

void HistDB::set_media_source(unsigned int ind, Phonon::MediaSource source)
{
	if(ind >= list.size())
	      return;
	list[ind].media_source = source;
}
void HistDB::set_playing(unsigned int current)
{
	if(current >= list.size())
	      return;
	list[current].played = 1;
}

int HistDB::get_next(int ind)
{
	if((unsigned int)ind >= list.size())
	      return 0;

	double dist = DBL_MAX;
	int ret = ind + 1 >= (int)list.size() ? 0 : ind + 1;

	QString ind_title(list[ind].title);
	QString ind_artist(list[ind].artist);

	list[ind].played = 1;
	for(unsigned int i = 0; i < list.size(); i++) {
		QString i_title(list[i].title);
		QString i_artist(list[i].artist);

		if(list[i].played == 1 || i == (unsigned int)ind) {
		      std::cout << "Skipping same: " << i_title.toLower().toAscii().data() << std::endl;
		      continue;
		}

		if(ind_title.toLower().compare(i_title.toLower()) == 0 && ind_artist.toLower().compare(i_artist.toLower()) == 0) {
		      std::cout << "SKipping same track " << i_title.toLower().toAscii().data() << "Artist: " <<  i_artist.toLower().toAscii().data() << std::endl;
		      continue;
		}

		double t_dist = distance(ind, i);
		if(t_dist < dist) {
			ret = i;
			dist = t_dist;
		}
	}
	return (int)ret;

}
