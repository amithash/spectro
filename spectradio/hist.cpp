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
		std::cout << "Reading length from " << dbname << " Failed" << std::endl;
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
			if(!ifs.read((char *)list[i].spect_hist[j], (sizeof(float) * SPECT_HIST_LEN) / sizeof(char))) {
				std::cerr << "Error reading sh element number " << i << "Band: " << j << std::endl;
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

float HistDB::skldistance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	float log_2 = log(2);
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	for(i = 0; i < len; i++) {
		dist += (a[i] - b[i]) * log(a[i] / b[i]) / log_2;
	}
	return dist;
}

float HistDB::distance(unsigned int e1, unsigned int e2)
{
	int col;
	float dist = 0;
	if(e1 >= list.size() || e1 >= list.size())
	      return 0;

	for(col = 0; col < NBANDS; col++) {
		dist += skldistance(list[e1].spect_hist[col],
				      list[e2].spect_hist[col],
				      SPECT_HIST_LEN);
	}
	return dist;
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

	float dist = DBL_MAX;
	int ret = ind + 1 >= (int)list.size() ? 0 : ind + 1;
	int found = 0;

	QString ind_title(list[ind].title);
	QString ind_artist(list[ind].artist);

	list[ind].played = 1;
	for(unsigned int i = 0; i < list.size(); i++) {
		QString i_title(list[i].title);
		QString i_artist(list[i].artist);

		if(list[i].played == 1 || i == (unsigned int)ind) {
			std::cout << "Skipping already played: " << i_title.toAscii().data() << std::endl;
			continue;
		}

		if(ind_title.toLower().compare(i_title.toLower()) == 0 && ind_artist.toLower().compare(i_artist.toLower()) == 0) {
			continue;
		}

		float t_dist = distance(ind, i);
		if(t_dist < dist) {
			found = 1;
			ret = i;
			dist = t_dist;
		}
	}
	if(!found)
		std::cout << "Did not find anything.... probable bug...." << std::endl;
	if(dist > 8) {
		std::cout << "Min distance of " << dist << "is too large, Expect deveation" << std::endl;
	}
	return (int)ret;
}
