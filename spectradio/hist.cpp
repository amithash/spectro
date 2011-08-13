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

#include <fstream>
#include <iostream>
#include <string>
#include <math.h>
#include <float.h>
#include <sys/stat.h>
#include "hist.h"

HistDB::HistDB(void)
{
	valid = 0;
	supportedDistances = NULL;
	get_supported_distances(&supportedDistances);
	curDistance = HELLINGER_DIVERGANCE;
	hist_list = NULL;
	hist_len = 0;
}

bool HistDB::is_valid()
{
	return valid == 1;
}

void HistDB::LoadDB(const char *dbname)
{
	valid = 0;
	if(!dbname) {
		std::cout << "Invalid (NULL) pointer passed as DB name" << std::endl;
		return;
	}

	if(read_append_histdb(&hist_list, &hist_len, (char *)dbname)) {
		std::cout << "Reading " << dbname << " failed" << std::endl;
		return;
	}
	valid = 1;
	return;
}

HistDB::HistDB(const char *dbname)
{
	HistDB();
	LoadDB(dbname);
}

HistDB::~HistDB()
{
	if(hist_list)
		free(hist_list);
}

unsigned int HistDB::length()
{
	return hist_len;
}

float HistDB::distance(unsigned int e1, unsigned int e2)
{
	if(e1 >= hist_len || e2 >= hist_len)
	      return 0;

	return hist_distance(&hist_list[e1], &hist_list[e2], curDistance);
}

QString HistDB::name(unsigned int ind)
{
	if(ind >= hist_len)
	      return NULL;
	return QString(hist_list[ind].fname);
}

QString HistDB::title(unsigned int ind)
{
	if(ind >= hist_len)
	      return NULL;
	return QString(hist_list[ind].title);
}

QString HistDB::artist(unsigned int ind)
{
	if(ind >= hist_len)
	      return NULL;
	return QString(hist_list[ind].artist);
}

QString HistDB::track(unsigned int ind)
{
	if(ind >= hist_len)
	      return NULL;
	unsigned int track = hist_list[ind].track;
	QString ret;
	if(track < 10)
	      ret = "0";
	QString num;
	num.setNum(track);
	ret += num;
	return ret;
}

QString HistDB::album(unsigned int ind)
{
	if(ind >= hist_len)
	      return NULL;
	return QString(hist_list[ind].album);
}

void HistDB::set_playing(unsigned int current)
{
	if(current >= hist_len)
	      return;
	hist_list[current].exclude = 1;
}

int HistDB::get_next(int _ind)
{
	QString ind_title;
	QString ind_artist;
	QString i_title;
	QString i_artist;

	int next_ind;
	float next_dist;
	int rc;

	unsigned int ind = _ind;
	if((unsigned int)ind >= hist_len)
	      return 0;

	unsigned int ret = ind + 1 >= hist_len ? 0 : ind + 1;

	ind_title = title(ind);
	ind_artist = artist(ind);

	while(1) {
		rc = hist_get_similar(hist_list, hist_len, &ind, 1, 
			1, &next_ind, &next_dist, curDistance);
		if(rc) {
			std::cout << "Getting similar failed" << std::endl;
			return ret;
		}
		hist_list[next_ind].exclude = 1;
		i_title  = title(next_ind);
		i_artist = artist(next_ind);
		if(i_title.toLower().compare(ind_title.toLower()) != 0)
		      break;
		if(i_artist.toLower().compare(ind_artist.toLower()) != 0)
		      break;
	}
	return next_ind;
}

QList<QString> HistDB::getSupportedDistanceFunctions()
{
	QList<QString> sup_list;
	for(int i = DISTANCE_START; i < DISTANCE_END; i++) {
		sup_list.append(QString(supportedDistances[i].name));
	}
	return sup_list;
}

void HistDB::setDistanceFunction(QString s_func)
{
	char *name = s_func.toAscii().data();
	int i;
	for(i = DISTANCE_START; i < DISTANCE_END; i++) {
		if(strcmp(name, supportedDistances[i].name) == 0)
			break;
	}
	if(i < DISTANCE_END)
	      curDistance = (hist_dist_func_t)i;
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
