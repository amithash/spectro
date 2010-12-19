 /****************************************************************************
 **
 ** Copyright (C) 2010 Amithash Prasad <amithash@gmail.com>
 ** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 ** All rights reserved.
 ** Contact: Nokia Corporation (qt-info@nokia.com)
 **
 ** This file was part of the examples of the Qt Toolkit.
 ** It is now modified and part of spectradio
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
 ***************************************************************************/
#include <QtGui>
#include <iostream>

#include "mainwindow.h"
#include "hist.h"

HistDB htdb;

MainWindow::MainWindow()
{
	audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
	mediaObject = new Phonon::MediaObject(this);
	mediaObject->setTickInterval(1000);
	connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
	connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
		this, SLOT(stateChanged(Phonon::State,Phonon::State)));
	connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
		this, SLOT(sourceChanged(Phonon::MediaSource)));
	connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));

	Phonon::createPath(mediaObject, audioOutput);

	setupActions();
	setupUi();
	timeLcd->display("00:00");
}

void MainWindow::addEntry(QTableWidget *table, QString title, QString artist, QString album)
{
	int currentRow = table->rowCount();
	table->insertRow(currentRow);
	QTableWidgetItem *titleItem = new QTableWidgetItem(title);
	QTableWidgetItem *artistItem = new QTableWidgetItem(artist);
	QTableWidgetItem *albumItem = new QTableWidgetItem(album);

        titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
        artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);
        albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);

	table->setItem(currentRow, 0, titleItem);
	table->setItem(currentRow, 1, artistItem);
	table->setItem(currentRow, 2, albumItem);
}

void MainWindow::clearSearchWindow()
{
	for (int i = searchTable->rowCount() - 1; i >= 0; --i)
		searchTable->removeRow(i);
}

void MainWindow::searchDB()
{
	QString search = searchBox->text().toLower();
	clearSearchWindow();
	searchMap.clear();
	if(searchBox->text().isEmpty()) {
		searchTable->hide();
		musicTable->show();
		return;
	}
	int tblCount = musicTable->rowCount();
	int colCount = musicTable->columnCount();
	stop = 0;
	for(int i = 0; i < tblCount; i++) {
		int found = 0;
		for(int j = 0; j < colCount; j++) {
			QString str = musicTable->item(i, j)->text().toLower();
			if(str.contains(search)) {
				found = 1;
				break;
			}
		}
		if(found == 1) {
			addEntry(searchTable, musicTable->item(i, 0)->text(), musicTable->item(i, 1)->text(), musicTable->item(i, 2)->text());
			searchMap.append(i);
		}
		if(stop == 1)
			break;
	}
	musicTable->hide();
	searchTable->show();
}

void MainWindow::acceptSettings(QAbstractButton * button)
{
	if(button->text().compare("Cancel") != 0) {
		htdb.setDistanceFunction(button->text());
	}
	closeSettings();
}
void MainWindow::closeSettings()
{
	dialogBox->hide();
}


void MainWindow::settings()
{
	dialogBox->show();
}

void MainWindow::loadDB()
{
	QFileDialog dialog(this);
	dialog.setNameFilter("*.hdb");
	dialog.setNameFilterDetailsVisible(true);

	QStringList files = dialog.getOpenFileNames(this, tr("Select Hist DB File generated by spect2hist"),
		QDesktopServices::storageLocation(QDesktopServices::HomeLocation));

	foreach (QString string, files) {
		if(string.endsWith(".hdb")) {
			loadDB(string.toAscii().data());
		} else {
			std::cout << "Ignoring " << string.toAscii().data() << std::endl;
		}
	}
}

void MainWindow::loadDB(char *s_dbfile)
{
	QString dbfile(s_dbfile);
	int at = htdb.length();

	statusBar->showMessage("Loading DB...", 2000);
	htdb.LoadDB(dbfile.toAscii().data());
	statusBar->showMessage("Done loading DB", 2000);

	if(htdb.is_valid() != true) {
		statusBar->showMessage("Error! Db not read!");
		exit(-1);
	}

	sources.reserve(htdb.length());
	musicTable->hide();
	for(unsigned int i = at; i < htdb.length(); i++) {
		QString string(htdb.ind_name(i));
		Phonon::MediaSource source(string);
		htdb.set_media_source(i, source);
		sources.append(source);
		if(htdb.ind_title(i).isEmpty()) {
			addEntry(musicTable, htdb.ind_name(i), htdb.ind_artist(i), htdb.ind_album(i));
		} else {
			addEntry(musicTable, htdb.ind_title(i), htdb.ind_artist(i), htdb.ind_album(i));
		}
	}
	musicTable->show();
	musicTable->resizeColumnsToContents();
	if (musicTable->columnWidth(0) > 300)
		musicTable->setColumnWidth(0, 300);
}

void MainWindow::about()
{
	QMessageBox::information(this, tr("Specradio"),
		tr("Play a song and the player will pick the next one for you... kinda like pandora, but with your own music"));
}

void MainWindow::stateChanged(Phonon::State newState, Phonon::State /* oldState */)
{
	switch (newState) {
		case Phonon::ErrorState:
			if (mediaObject->errorType() == Phonon::FatalError) {
				QMessageBox::warning(this, tr("Fatal Error"),
				mediaObject->errorString());
			} else {
				QMessageBox::warning(this, tr("Error"),
				mediaObject->errorString());
			}
			break;
		
		case Phonon::PlayingState:
			playAction->setEnabled(false);
			pauseAction->setEnabled(true);
			stopAction->setEnabled(true);
			break;

		case Phonon::StoppedState:
			stopAction->setEnabled(false);
			playAction->setEnabled(true);
			pauseAction->setEnabled(false);
			timeLcd->display("00:00");
			break;

		case Phonon::PausedState:
			pauseAction->setEnabled(false);
			stopAction->setEnabled(true);
			playAction->setEnabled(true);
			break;

		case Phonon::BufferingState:
			break;
		default:
			;
	}
}

void MainWindow::tick(qint64 time)
{
	QTime displayTime(0, (time / 60000) % 60, (time / 1000) % 60);
	timeLcd->display(displayTime.toString("mm:ss"));
}

void MainWindow::searchTableClicked(int row, int /* column */)
{
	/* Start playing */
	stop = 1;
	int mt_row = searchMap.at(row);
	searchTable->hide();
	musicTable->show();
	tableClicked(mt_row, 0);
}

void MainWindow::tableClicked(int row, int /* column */)
{
	mediaObject->stop();
	mediaObject->clearQueue();

	if (row >= sources.size())
		return;

	htdb.set_playing(row);
	mediaObject->setCurrentSource(sources[row]);
	mediaObject->play();
}

void MainWindow::sourceChanged(const Phonon::MediaSource &source)
{
	int row = sources.indexOf(source);
	musicTable->selectRow(row);
	setTitle(row);

	timeLcd->display("00:00");
}

void MainWindow::aboutToFinish()
{
	int index = sources.indexOf(mediaObject->currentSource());
	index = htdb.get_next(index);
	if (sources.size() > index) {
		mediaObject->enqueue(sources.at(index));
	}
}

void MainWindow::setTitle(int index)
{
	QString windowTitle;
	windowTitle = musicTable->item(index, 0)->text() +
		      "\tby\t" + musicTable->item(index,1)->text();

	setWindowTitle(windowTitle);
	statusBar->clearMessage();
	statusBar->showMessage(windowTitle);
}

void MainWindow::setupActions()
{
	playAction = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("Play"), this);
	playAction->setShortcut(tr("Ctrl+P"));
	playAction->setDisabled(true);

	pauseAction = new QAction(style()->standardIcon(QStyle::SP_MediaPause), tr("Pause"), this);
	pauseAction->setShortcut(tr("Ctrl+A"));
	pauseAction->setDisabled(true);

	stopAction = new QAction(style()->standardIcon(QStyle::SP_MediaStop), tr("Stop"), this);
	stopAction->setShortcut(tr("Ctrl+S"));
	stopAction->setDisabled(true);

	nextAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("Next"), this);
	nextAction->setShortcut(tr("Ctrl+N"));

	previousAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipBackward), tr("Previous"), this);
	previousAction->setShortcut(tr("Ctrl+R"));

	loadDBAction = new QAction(style()->standardIcon(QStyle::SP_DirOpenIcon), tr("Load DB (Ctrl+F)"), this);
	loadDBAction->setShortcut(tr("Ctrl+F"));
	loadDBAction->setDisabled(false);

	settingsAction = new QAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Choose Distance Function (Ctrl+D)"), this);
	settingsAction->setShortcut(tr("Ctrl+D"));
	settingsAction->setDisabled(false);

	exitAction = new QAction(style()->standardIcon(QStyle::SP_TitleBarCloseButton), tr("Exit"), this);
	exitAction->setShortcuts(QKeySequence::Quit);
	exitAction->setDisabled(false);

	aboutAction = new QAction(style()->standardIcon(QStyle::SP_DialogHelpButton), tr("About (Ctrl+B)"), this);
	aboutAction->setShortcut(tr("Ctrl+B"));
	aboutAction->setDisabled(false);

	connect(playAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
	connect(pauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
	connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));
	connect(loadDBAction, SIGNAL(triggered()), this, SLOT(loadDB()));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(settings()));
	connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
}

void MainWindow::setupUi()
{
	seekSlider = new Phonon::SeekSlider(this);
	seekSlider->setMediaObject(mediaObject);

	volumeSlider = new Phonon::VolumeSlider(this);
	volumeSlider->setAudioOutput(audioOutput);
	volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	QLabel *volumeLabel = new QLabel;
	volumeLabel->setPixmap(QPixmap("images/volume.png"));

	QPalette palette;
	palette.setBrush(QPalette::Light, Qt::darkGray);

	timeLcd = new QLCDNumber;
	timeLcd->setPalette(palette);

	// Music table
	QStringList headers;
	headers << tr("Title") << tr("Artist") << tr("Album");

	musicTable = new QTableWidget(0, 3);
	musicTable->setHorizontalHeaderLabels(headers);
	musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
	musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	connect(musicTable, SIGNAL(cellPressed(int,int)),
		this, SLOT(tableClicked(int,int)));

	// Search Table
	searchTable = new QTableWidget(0, 3);
	searchTable->setHorizontalHeaderLabels(headers);
	searchTable->setSelectionMode(QAbstractItemView::SingleSelection);
	searchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	searchTable->hide();
	connect(searchTable, SIGNAL(cellPressed(int, int)), 
		this, SLOT(searchTableClicked(int, int)));


	QToolBar *tbar = new QToolBar;		// toolbar 
	tbar->addAction(loadDBAction);		// load db
	tbar->addAction(settingsAction);	// settings
	tbar->addAction(aboutAction);		// about
	tbar->addAction(playAction);		// play
	tbar->addAction(pauseAction);		// pause
	tbar->addAction(stopAction);		// stop
	QHBoxLayout *toolBar = new QHBoxLayout;	// the toolbar layout
	toolBar->addWidget(tbar);		// toolbar (above)
	toolBar->addWidget(seekSlider);		// slider
	toolBar->addWidget(timeLcd);		// time LCD
	toolBar->addWidget(volumeLabel);	// volume label
	toolBar->addWidget(volumeSlider);	// volume slider

	// Settings dialog box
	dialogBox = new QDialogButtonBox();
	QList<QString> distFuncs = htdb.getSupportedDistanceFunctions();
	for(int i = 0; i < distFuncs.length(); i++) {
		dialogBox->addButton(distFuncs[i], QDialogButtonBox::ActionRole);
	}
	dialogBox->addButton("Cancel", QDialogButtonBox::ActionRole);
	dialogBox->setOrientation(Qt::Vertical);
	dialogBox->setCenterButtons(true);
	connect(dialogBox, SIGNAL(clicked(QAbstractButton *)), this, SLOT(acceptSettings(QAbstractButton *)));
	dialogBox->hide();

	// search bar
	searchBox = new QLineEdit;
	searchBox->setText("");
	connect(searchBox, SIGNAL(editingFinished()), this, SLOT(searchDB()));

	// status bar
	statusBar = new QStatusBar;
	statusBar->clearMessage();

	// Main layout
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(toolBar);
	mainLayout->addWidget(searchBox);
	mainLayout->addWidget(searchTable);
	mainLayout->addWidget(musicTable);
	mainLayout->addWidget(statusBar);

	QWidget *widget = new QWidget;
	widget->setLayout(mainLayout);

	setCentralWidget(widget);
	setWindowTitle("spectradio");
}

