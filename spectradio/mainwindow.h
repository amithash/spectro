/****************************************************************************
**
** Copyright (C) 2010 Amithash Prasad <amithash@gmail.com>
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the examples of the Qt Toolkit.
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
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
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
****************************************************************************/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <phonon/audiooutput.h>
#include <phonon/seekslider.h>
#include <phonon/mediaobject.h>
#include <phonon/volumeslider.h>
#include <phonon/backendcapabilities.h>
#include <QList>
#include <QLineEdit>

class QAction;
class QTableWidget;
class QLCDNumber;


class MainWindow : public QMainWindow
{
	Q_OBJECT

	public:
	MainWindow();

	QSize sizeHint() const {
         	return QSize(500, 300);
     	}

	private slots:
		void loadDB();
		void searchDB();
		void about();
		void stateChanged(Phonon::State newState, Phonon::State oldState);
		void tick(qint64 time);
		void sourceChanged(const Phonon::MediaSource &source);
		void aboutToFinish();
		void tableClicked(int row, int column);
		void searchTableClicked(int row, int column);

	private:
		void setupActions();
		void setupMenus();
		void setupUi();
		void addEntry(QTableWidget *table, QString title, QString artist, QString album);
		void clearSearchWindow();

		Phonon::SeekSlider *seekSlider;
		Phonon::MediaObject *mediaObject;
		Phonon::AudioOutput *audioOutput;
		Phonon::VolumeSlider *volumeSlider;
		QList<Phonon::MediaSource> sources;
		QList<int> searchMap;
		int stop;

		QAction *playAction;
		QAction *pauseAction;
		QAction *stopAction;
		QAction *nextAction;
		QAction *previousAction;
		QAction *loadDBAction;
		QAction *exitAction;
		QAction *aboutAction;
		QAction *aboutQtAction;

		QLCDNumber *timeLcd;
		QTableWidget *musicTable;
		QTableWidget *searchTable;
		QLineEdit *searchBox;
		QStatusBar *statusBar;
};

#endif

