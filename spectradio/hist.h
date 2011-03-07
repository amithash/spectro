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

 #include "histdb.h"

 #include <vector>
 #include <phonon/audiooutput.h>
 #include <phonon/seekslider.h>
 #include <phonon/mediaobject.h>
 #include <phonon/volumeslider.h>
 #include <phonon/backendcapabilities.h>

// Aux list which has a 1:1 equivalance to hist_list
class HistDB
{
	std::vector<int> list;
	hist_t *hist_list;
	unsigned int hist_len;
	unsigned int valid;
	hist_dist_func_t curDistance;
	dist_t *supportedDistances;
	QList<QString> database;

	public:
	HistDB(void);
	HistDB(const char *dbname);
	~HistDB();

	void addToDB(QString name);
	bool existsInDB(QString name);

	bool is_valid();
	unsigned int length(void);
	float distance(unsigned int e1, unsigned int e2);
	void set_playing(unsigned int ind);
	int get_next(int current);
	void LoadDB(const char *dbname);
	QString name(unsigned int ind);
	QString title(unsigned int ind);
	QString artist(unsigned int ind);
	QString album(unsigned int ind);
	QString track(unsigned int ind);

	QList<QString> getSupportedDistanceFunctions();
	void setDistanceFunction(QString dist);
};

#endif

