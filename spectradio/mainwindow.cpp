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

#define TITLE_COLOR QRgb(qRgb(0x00, 0x00, 0xFF))
#define ARTIST_COLOR QRgb(qRgb(0x00, 0xFF, 0x00))
#define ALBUM_COLOR QRgb(qRgb(0xFF, 0x00, 0x00))


const QString AboutMessage =
"\
Author: Amithash Prasad <amithash@gmail.com>	\n\
						\n\
spectradio is a personal auto-playlist which 	\n\
automatically plays songs similar to each other.\n\
Please refer to the README file which comes	\n\
with the source for more information on creating\n\
the hist DB file.				\n\
";
			
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

void MainWindow::addEntry(QTableWidget *table, QString title, QString artist, QString album, int ind)
{
	int currentRow = table->rowCount();
	table->insertRow(currentRow);
	QString s_num;
	s_num.setNum(ind);
	QTableWidgetItem *titleItem = new QTableWidgetItem(title);
	QTableWidgetItem *artistItem = new QTableWidgetItem(artist);
	QTableWidgetItem *albumItem = new QTableWidgetItem(album);
	QTableWidgetItem *indexItem = new QTableWidgetItem(s_num);

	titleItem->setBackground(QBrush(QColor(TITLE_COLOR)));
	artistItem->setBackground(QBrush(QColor(ARTIST_COLOR)));
	albumItem->setBackground(QBrush(QColor(ALBUM_COLOR)));

        titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
        artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);
        albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);
        indexItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);

	table->setItem(currentRow, 0, titleItem);
	table->setItem(currentRow, 1, artistItem);
	table->setItem(currentRow, 2, albumItem);
	table->setItem(currentRow, 3, indexItem);

	table->setColumnHidden(3, true);
}

void MainWindow::repopulateReverseIndex()
{
	int tableLen = musicTable->rowCount();
	int reverseIndexLen = reverseIndex.length();

	if(reverseIndexLen < tableLen) {
		reverseIndex.reserve(tableLen);
	} else if(reverseIndexLen > tableLen) {
		if(sources.length() != tableLen) {
			std::cerr << "Unrecoverable error. exiting" << std::endl;
			exit(-1);
		}
		reverseIndex.clear();
		reverseIndex.reserve(tableLen);
	}

	// Invalidate all elements
	for(int i = 0; i < reverseIndex.length(); i++) {
		reverseIndex[i] = -1;
	}

	for(int i = 0; i < tableLen; i++) {
		int sourcesInd = musicTable->item(i, 3)->text().toInt();
		reverseIndex[sourcesInd] = i;
	}
}

void MainWindow::playlistTableClicked(int row, int /* column */)
{
	int realRow = playlistTable->item(row, 3)->text().toInt();
	tableClicked(realRow, 0);
}

void MainWindow::retry(void)
{
	int last = playlistTable->rowCount() - 2;
	if(last < 0)
	      return;
	int playRow = playlistTable->item(last, 3)->text().toInt();
	if(playRow >= sources.size()) {
		std::cerr << "playRow invalid: " << playRow << std::endl; 
		return;
	}
	mediaObject->stop();
	mediaObject->clearQueue();
	int index = htdb.get_next(playRow);
	if(index >= sources.size()) {
		std::cerr << "Bad index: " << index << std::endl;
		return;
	}
	mediaObject->setCurrentSource(sources[index]);
	mediaObject->play();
}

void MainWindow::next(void)
{
	int thisInd = playlistTable->rowCount() - 1;
	if(thisInd < 0)
	      return;
	int playingRow = playlistTable->item(thisInd, 3)->text().toInt();
	if(playingRow >= musicTable->rowCount()) {
		std::cerr << "playRow invalid: " << playingRow << std::endl;
		return;
	}
	mediaObject->stop();

	int sourcesIndex = musicTable->item(playingRow, 3)->text().toInt();
	int nextSourcesIndex = htdb.get_next(sourcesIndex);
	if(nextSourcesIndex >= sources.size()) {
		std::cerr << "Bad index: " << nextSourcesIndex << std::endl;
		mediaObject->play();
		return;
	}

	mediaObject->clearQueue();
	mediaObject->setCurrentSource(sources[nextSourcesIndex]);
	mediaObject->play();
}

void MainWindow::togglePlaylist()
{
	if(playlistVisible) {
		playlistVisible = false;
		playlistTable->hide();
	} else {
		playlistVisible = true;
		playlistTable->show();
	}
}

void MainWindow::togglePlay()
{
  switch(mediaObject->state())
  {
	case Phonon::PlayingState:
      mediaObject->pause();
      break;
	case Phonon::StoppedState:
	case Phonon::PausedState:
      mediaObject->play();
      break;
	case Phonon::ErrorState:
	case Phonon::BufferingState:
	default:
      break;
  }
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
	if(search.isEmpty()) {
		searchTable->hide();
		musicTable->show();
		return;
	}
	int tblCount = musicTable->rowCount();
	int colCount = 3;
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
			addEntry(searchTable, 
			musicTable->item(i, 0)->text(), 
			musicTable->item(i, 1)->text(), 
			musicTable->item(i, 2)->text(), i);
		}
		if(stop == 1)
			break;
	}
	musicTable->hide();
	searchTable->sortItems(1, Qt::AscendingOrder);
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

	if(htdb.existsInDB(dbfile)) {
		std::cerr << s_dbfile << " Already exists, so skipping it" << std::endl;
		return;
	}
	htdb.addToDB(dbfile);

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
			addEntry(musicTable, htdb.ind_name(i), htdb.ind_artist(i), htdb.ind_album(i), i);
		} else {
			addEntry(musicTable, htdb.ind_title(i), htdb.ind_artist(i), htdb.ind_album(i), i);
		}
	}
	musicTable->show();
	musicTable->resizeColumnsToContents();
	musicTable->sortItems(1, Qt::AscendingOrder);
	repopulateReverseIndex();
	if (musicTable->columnWidth(0) > 300)
		musicTable->setColumnWidth(0, 300);
}

void MainWindow::about()
{
	QMessageBox::information(this, tr("Spectradio"), AboutMessage);
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
			playAction->setEnabled(true);
      playAction->setIcon(style()->standardIcon(QStyle::SP_MediaPause));
			stopAction->setEnabled(true);
			retryAction->setEnabled(true);
			nextAction->setEnabled(true);
			break;

		case Phonon::StoppedState:
			stopAction->setEnabled(false);
			playAction->setEnabled(true);
      playAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
			retryAction->setEnabled(false);
			nextAction->setEnabled(false);
			timeLcd->display("00:00");
			break;

		case Phonon::PausedState:
			playAction->setEnabled(true);
      playAction->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
			stopAction->setEnabled(true);
			playAction->setEnabled(true);
			retryAction->setEnabled(true);
			nextAction->setEnabled(true);
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
	QString tableIndex_s = searchTable->item(row, 3)->text();
	int tableIndex = tableIndex_s.toInt();
	searchTable->hide();
	musicTable->show();
	tableClicked(tableIndex, 0);
}

void MainWindow::tableClicked(int row, int /* column */)
{
	mediaObject->stop();
	mediaObject->clearQueue();

	if(row >= musicTable->rowCount())
	      return;
	int sourcesIndex = musicTable->item(row, 3)->text().toInt();

	if (sourcesIndex >= sources.size())
		return;

	htdb.set_playing(sourcesIndex);
	mediaObject->setCurrentSource(sources[sourcesIndex]);
	mediaObject->play();
}

void MainWindow::sourceChanged(const Phonon::MediaSource &source)
{
	int sourcesIndex = sources.indexOf(source);
	int musicTableIndex = reverseIndex[sourcesIndex];
	musicTable->selectRow(musicTableIndex);
	setTitle(musicTableIndex);
	addEntry(playlistTable, 
			musicTable->item(musicTableIndex, 0)->text(),
			musicTable->item(musicTableIndex, 1)->text(),
			musicTable->item(musicTableIndex, 2)->text(),
			musicTableIndex);

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
		      " by " + musicTable->item(index,1)->text();

	setWindowTitle(windowTitle);
	statusBar->clearMessage();
	statusBar->showMessage(windowTitle);
}

void MainWindow::setupActions()
{
	playAction = new QAction(style()->standardIcon(QStyle::SP_MediaPlay), tr("Play/Pause"), this);
	playAction->setShortcut(tr("space"));
	playAction->setToolTip("Play (space)");
	playAction->setDisabled(true);

	stopAction = new QAction(style()->standardIcon(QStyle::SP_MediaStop), tr("Stop"), this);
	stopAction->setShortcut(tr("Ctrl+S"));
	stopAction->setToolTip("Stop (Ctrl+S)");
	stopAction->setDisabled(true);

	nextAction = new QAction(style()->standardIcon(QStyle::SP_MediaSkipForward), tr("Next"), this);
	nextAction->setShortcut(tr("Ctrl+N"));
	nextAction->setToolTip("Skip the current playing track and play the next predicted (Ctrl+N)");
	nextAction->setDisabled(true);

	retryAction = new QAction(style()->standardIcon(QStyle::SP_DialogResetButton), tr("Retry"), this);
	retryAction->setShortcut(tr("Ctrl+R"));
	retryAction->setToolTip("Skip the current playing and rerun the prediction from the last played track (Ctrl+R)");
	retryAction->setDisabled(true);

	loadDBAction = new QAction(style()->standardIcon(QStyle::SP_DirOpenIcon), tr("Load DB"), this);
	loadDBAction->setShortcut(tr("Ctrl+L"));
	loadDBAction->setToolTip("Load the hist DB (Ctrl+L)");
	loadDBAction->setDisabled(false);

	settingsAction = new QAction(style()->standardIcon(QStyle::SP_FileDialogInfoView), tr("Distance Function"), this);
	settingsAction->setShortcut(tr("Ctrl+D"));
	settingsAction->setToolTip("Choose the distance function for track prediction (Ctrl+D)");
	settingsAction->setDisabled(false);

	togglePlaylistAction = new QAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), tr("Distance Function"), this);
	togglePlaylistAction->setShortcut(tr("Alt+P"));
	togglePlaylistAction->setToolTip("Toggle playlist (Alt+P)");
	settingsAction->setDisabled(false);

	aboutAction = new QAction(style()->standardIcon(QStyle::SP_MessageBoxQuestion), tr("About"), this);
	aboutAction->setDisabled(false);

	connect(playAction, SIGNAL(triggered()), this, SLOT(togglePlay()));
	connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));
	connect(loadDBAction, SIGNAL(triggered()), this, SLOT(loadDB()));
	connect(settingsAction, SIGNAL(triggered()), this, SLOT(settings()));
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
	connect(retryAction, SIGNAL(triggered()), this, SLOT(retry()));
	connect(nextAction, SIGNAL(triggered()), this, SLOT(next()));
	connect(togglePlaylistAction, SIGNAL(triggered()), this, SLOT(togglePlaylist()));
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
	headers << tr("Title") << tr("Artist") << tr("Album") << tr("Index");

	musicTable = new QTableWidget(0, 4);
	musicTable->setHorizontalHeaderLabels(headers);
	musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
	musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	musicTable->verticalHeader()->hide();
	musicTable->setColumnHidden(3, true);
	connect(musicTable, SIGNAL(cellPressed(int,int)),
		this, SLOT(tableClicked(int,int)));

	// Search Table
	searchTable = new QTableWidget(0, 4);
	searchTable->setHorizontalHeaderLabels(headers);
	searchTable->setSelectionMode(QAbstractItemView::SingleSelection);
	searchTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	searchTable->hide();
	searchTable->setColumnHidden(3, true);
	searchTable->verticalHeader()->hide();
	connect(searchTable, SIGNAL(cellPressed(int, int)), 
		this, SLOT(searchTableClicked(int, int)));

	playlistTable = new QTableWidget(0,4);
	playlistTable->setHorizontalHeaderLabels(headers);
	playlistTable->setSelectionMode(QAbstractItemView::SingleSelection);
	playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	playlistTable->setColumnHidden(3, true);
	playlistTable->verticalHeader()->hide();
	connect(playlistTable, SIGNAL(cellPressed(int, int)), 
		this, SLOT(playlistTableClicked(int, int)));
	playlistTable->hide();
	playlistVisible = false;


	QToolBar *tbar = new QToolBar;		// toolbar 
	tbar->addAction(loadDBAction);		// load db
	tbar->addAction(playAction);		// play
	tbar->addAction(stopAction);		// stop
	tbar->addAction(retryAction);		// Retry
	tbar->addAction(nextAction);		// Next
	tbar->addAction(settingsAction);	// settings
	tbar->addAction(aboutAction);		// about

	QToolBar *tpl_toolbar = new QToolBar;
	tpl_toolbar->addAction(togglePlaylistAction);

	QHBoxLayout *toolBar = new QHBoxLayout;	// the toolbar layout
	toolBar->addWidget(tbar);		// toolbar (above)
	toolBar->addWidget(seekSlider);		// slider
	toolBar->addWidget(timeLcd);		// time LCD
	toolBar->addWidget(volumeLabel);	// volume label
	toolBar->addWidget(volumeSlider);	// volume slider
	toolBar->addWidget(tpl_toolbar);	// toggle Playlist

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

	QVBoxLayout *DBLayout = new QVBoxLayout;
	DBLayout->addWidget(searchBox);
	DBLayout->addWidget(searchTable);
	DBLayout->addWidget(musicTable);

	QVBoxLayout *PlaylistLayout = new QVBoxLayout;
	PlaylistLayout->addWidget(playlistTable);

	QHBoxLayout *SubMainLayout = new QHBoxLayout;
	SubMainLayout->addLayout(DBLayout);
	SubMainLayout->addLayout(PlaylistLayout);

	// Main layout
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(toolBar);
	mainLayout->addLayout(SubMainLayout);
	mainLayout->addWidget(statusBar);

	QWidget *widget = new QWidget;
	widget->setLayout(mainLayout);

	setCentralWidget(widget);
	setWindowTitle("spectradio");
}

