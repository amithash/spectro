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

#define KL_DIVERGANCE "Kullback-Liebler Divergance (Default)"
#define JEFFREYS_DIVERGANCE "Jeffreys divergance (Symmetric KL Divergance)"
#define JENSON_DIVERGANCE "Jenson Divergance (Information radius)"
#define EUCLIDEAN_DISTANCE "Euclidean distance"
#define HELLINGER_DISTANCE "Hellinger distance"

#define SQR(val) ((val) * (val))

static float kl_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	float log_2 = log(2);
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	for(i = 0; i < len; i++) {
		dist += (a[i]) * log(a[i] / b[i]) / log_2;
	}
	return dist;
}
static float jeffery_distance(float *a, float *b, unsigned int len)
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
static float euclidean_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	for(i = 0; i < len; i++) {
		dist += SQR(a[i] - b[i]);
	}
	return sqrt(dist);
}

static float jensen_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	for(i = 0; i < len; i++) {

		dist +=(((a[i] * log(a[i])) + (b[i] * log(b[i]))) / 2) - 
		    (((a[i] + b[i]) / 2) * log((a[i]+b[i]) / 2));
	}
	return dist;
}
static float hellinger_distance(float *a, float *b, unsigned int len)
{
	float dist = 0;
	unsigned int i;
	if(a == NULL || b == NULL) {
		return 0;
	}
	for(i = 0; i < len; i++) {
		dist += sqrt(a[i] * b[i]);
	}
	return sqrt(1 - dist);
}

HistDB::HistDB(void)
{
	valid = 0;
	supportedDistances.append(KL_DIVERGANCE);
	supportedDistances.append(JEFFREYS_DIVERGANCE);
	supportedDistances.append(JENSON_DIVERGANCE);
	supportedDistances.append(HELLINGER_DISTANCE);
	supportedDistances.append(EUCLIDEAN_DISTANCE);

	pdf_distance = kl_distance;
	curDistance = KL_DIVERGANCE;
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
		if(!ifs.read((char *)&list[i].length, sizeof(unsigned int))) {
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
	HistDB();
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

float HistDB::distance(unsigned int e1, unsigned int e2)
{
	int col;
	float dist = 0;

	if(e1 >= list.size() || e1 >= list.size())
	      return 0;

	if(!pdf_distance) {
		std::cerr << "Warning pdf distance function was not set. setting it to \"KL DIVERGANCE\"" << std::endl;
		pdf_distance = kl_distance;
	}

	for(col = 0; col < NBANDS; col++) {
		dist += pdf_distance(list[e1].spect_hist[col],
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

QString HistDB::ind_track(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	unsigned int track = list[ind].track;
	QString ret;
	if(track < 10)
	      ret = "0";
	QString num;
	num.setNum(track);
	ret += num;
	return ret;
}

QString HistDB::ind_album(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	return list[ind].album;
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

	QString ind_title(list[ind].title);
	QString ind_artist(list[ind].artist);
	unsigned int ind_length = list[ind].length;

	list[ind].played = 1;
	for(unsigned int i = 0; i < list.size(); i++) {
		QString i_title(list[i].title);
		QString i_artist(list[i].artist);
		unsigned int i_length = list[i].length;

		// Find out what you can do with ind_length and i_length.

		if(list[i].played == 1 || i == (unsigned int)ind) {
			continue;
		}

		if(ind_title.toLower().compare(i_title.toLower()) == 0 && ind_artist.toLower().compare(i_artist.toLower()) == 0) {
			std::cout << "Skipping the same song in another album" << std::endl;
			list[i].played = 1;
			continue;
		}

		float t_dist = distance(ind, i);
		if(t_dist < dist) {
			ret = i;
			dist = t_dist;
		}
	}
	list[ret].played = 1;
	return (int)ret;
}

QList<QString> HistDB::getSupportedDistanceFunctions()
{
	return supportedDistances;
}

void HistDB::setDistanceFunction(QString s_func)
{
	if(s_func.compare(KL_DIVERGANCE) == 0) {
		pdf_distance = kl_distance;
	} else if(s_func.compare(JEFFREYS_DIVERGANCE) == 0) {
		pdf_distance = jeffery_distance;
	} else if(s_func.compare(JENSON_DIVERGANCE) == 0) {
		pdf_distance = jensen_distance;
	} else if(s_func.compare(EUCLIDEAN_DISTANCE) == 0) {
		pdf_distance = euclidean_distance;
	} else if(s_func.compare(HELLINGER_DISTANCE) == 0) {
		pdf_distance = hellinger_distance;
	} else {
		std::cout << "Invalid distance provided. Ignoring.." << std::endl;
		return;
	}
	std::cout << "Switching from " << curDistance.toAscii().data() << " to " << s_func.toAscii().data() << std::endl;
	curDistance = s_func;
}

void HistDB::addToDB(QString name)
{
	database.append(name);
}

bool HistDB::existsInDB(QString qname)
{
	for(int i = 0; i < database.length(); i++) {
		if(database[i].compare(qname) == 0) {
			return true;
		}
	}
	return false;
}
