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
	list = NULL;
	len = 0;
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
     if(!ifs.read((char *)&len, sizeof(unsigned int))) {
     	std::cout << "Reading length from " << dbname << std::endl;
	return;
     }
     if(len <= 0)
	   return;

     list = new Hist[len];
     if(!list)
	   return;

     for(unsigned int i = 0; i < len; i++) {
	     if(!ifs.read((char *)list[i].fname, sizeof(char) * FNAME_LEN)) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }
	     if(!ifs.read((char *)list[i].title, sizeof(char) * TITLE_LEN)) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }
	     if(!ifs.read((char *)list[i].artist, sizeof(char) * ARTIST_LEN)) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }
	     if(!ifs.read((char *)list[i].album, sizeof(char) * ALBUM_LEN)) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	     }
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
	     if(!ifs.read((char *)list[i].beats, (sizeof(double) * BEAT_LEN) / sizeof(char))) {
	     	std::cerr << "Error reading element number " << i << std::endl;
		goto err;
	    }
     }
     valid = 1;
     return;
err:
     delete [] list;
}

HistDB::HistDB(const char *dbname)
{
	LoadDB(dbname);
}

HistDB::~HistDB()
{
	if(list)
		delete [] list;
}

unsigned int HistDB::length()
{
	return len;
}

/* Compute the hellinger distance of two pdfs a and b */
static double hdistance(double *a, double *b, unsigned int len) 
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
static double edistance(double *dist, unsigned int len) 
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
	if(e1 >= len || e1 >= len)
	      return 0;

	for(col = 0; col < NBANDS; col++) {
		dist[col] = hdistance(list[e1].hist[col],
				      list[e2].hist[col],
				      HIST_LEN);
	}
	return edistance(dist, NBANDS);
}
char *HistDB::ind_name(unsigned int ind)
{
	if(ind >= len)
	      return NULL;
	return list[ind].fname;
}

char*HistDB::ind_title(unsigned int ind)
{
	if(ind >= len)
	      return NULL;
	return list[ind].title;
}

char *HistDB::ind_artist(unsigned int ind)
{
	if(ind >= len)
	      return NULL;
	return list[ind].artist;
}

char *HistDB::ind_album(unsigned int ind)
{
	if(ind >= len)
	      return NULL;
	return list[ind].album;
}

void HistDB::set_media_source(unsigned int ind, Phonon::MediaSource source)
{
	if(ind >= len)
	      return;
	list[ind].media_source = source;
}
void HistDB::set_playing(unsigned int current)
{
	if(current >= len)
	      return;
	list[current].played = 1;
}

int HistDB::get_next(int ind)
{
	if((unsigned int)ind >= len)
	      return 0;

	double dist = DBL_MAX;
	int ret = ind + 1 >= (int)len ? 0 : ind + 1;

	QString ind_title(list[ind].title);
	QString ind_artist(list[ind].artist);

	list[ind].played = 1;
	for(unsigned int i = 0; i < len; i++) {
		if(list[i].played == 1 || i == (unsigned int)ind)
		      continue;
		QString i_title(list[ind].title);
		QString i_artist(list[ind].artist);

		if(ind_title.toLower().compare(i_title.toLower()) == 0 && ind_artist.toLower().compare(i_artist.toLower()) == 0)
		      continue;

		      continue;
		double t_dist = distance(ind, i);
		if(t_dist < dist) {
			ret = i;
			dist = t_dist;
		}
	}
	return (int)ret;

}
