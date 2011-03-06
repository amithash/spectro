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
	curDistance = KL_DIVERGANCE;
	hist_list = NULL;
	hist_len = 0;
}

bool HistDB::is_valid()
{
	return valid == 1;
}

void HistDB::LoadDB(const char *dbname)
{
	unsigned int old_len = hist_len;

	if(read_append_histdb(&hist_list, &hist_len, (char *)dbname)) {
		std::cout << "Reading " << dbname << " failed" << std::endl;
		return;
	}
	list.reserve(hist_len);
	for(unsigned int i = old_len; i < hist_len; i++) {
		list[i].played = 0;
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
	free(hist_list);

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
	float dist;

	if(e1 >= list.size() || e1 >= list.size())
	      return 0;

	dist = hist_distance(&hist_list[e1], &hist_list[e2], curDistance);

	return dist;
}

QString HistDB::name(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	return QString(hist_list[ind].fname);
}

QString HistDB::title(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	return QString(hist_list[ind].title);
}

QString HistDB::artist(unsigned int ind)
{
	if(ind >= list.size())
	      return NULL;
	return QString(hist_list[ind].artist);
}

QString HistDB::track(unsigned int ind)
{
	if(ind >= list.size())
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
	if(ind >= list.size())
	      return NULL;
	return QString(hist_list[ind].album);
}

void HistDB::set_playing(unsigned int current)
{
	if(current >= list.size())
	      return;
	list[current].played = 1;
}

int HistDB::get_next(int _ind)
{
	QString ind_title;
	QString ind_artist;
	QString i_title;
	QString i_artist;

	unsigned int ind = _ind;
	if((unsigned int)ind >= list.size())
	      return 0;

	float dist = DBL_MAX;
	unsigned int ret = ind + 1 >= list.size() ? 0 : ind + 1;

	ind_title = title(ind);
	ind_artist = artist(ind);

	list[ind].played = 1;
	for(unsigned int i = 0; i < list.size(); i++) {
		i_title = title(i);
		i_artist = artist(i);

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
