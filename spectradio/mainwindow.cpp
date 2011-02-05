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
#include <iostream>

#include "mainwindow.h"

#define TITLE_COLOR QBrush(QColor(QRgb(qRgb(0xb2, 0xdb, 0xf7))))
#define ARTIST_COLOR QBrush(QColor(QRgb(qRgb(0xb2, 0xdb, 0xf7))))
#define ALBUM_COLOR QBrush(QColor(QRgb(qRgb(0xb2, 0xdb, 0xf7))))

#define TITLE_COLOR_FOREGROUND QBrush(QColor(QRgb(qRgb(0x60, 0x60, 0x60))))
#define ALBUM_COLOR_FOREGROUND QBrush(QColor(QRgb(qRgb(0x30, 0x30, 0x30))))
#define ARTIST_COLOR_FOREGROUND QBrush(QColor(QRgb(qRgb(0x00, 0x00, 0x00))))

#define ARTIST_FONT QFont("" , 0 , QFont::Normal )
#define ALBUM_FONT QFont("" , 0 , QFont::Normal )
#define TITLE_FONT QFont("" , 0 , QFont::Normal )

#define TITLE_COLUMN  0
#define ARTIST_COLUMN 1
#define ALBUM_COLUMN  2
#define INDEX_COLUMN  3

#define STATUS_BAR_COLOR_BLUE QString("<font color='Blue'>")
#define STATUS_BAR_COLOR_RED QString("<font color='Red'>")
#define STATUS_BAR_COLOR_BLACK QString("<font color='Black'>")
#define STATUS_BAR_TEXT_END QString("</font>")

#define ICON_PAUSE    QIcon::fromTheme("media-playback-pause")
#define ICON_PLAY     QIcon::fromTheme("media-playback-start")
#define ICON_STOP     QIcon::fromTheme("media-playback-stop")
#define ICON_NEXT     QIcon::fromTheme("go-next")
#define ICON_RETRY    QIcon::fromTheme("edit-redo")
#define ICON_ABOUT    QIcon::fromTheme("help-about")
#define ICON_SETTINGS QIcon::fromTheme("preferences-system")
#define ICON_LOAD     QIcon::fromTheme("document-open")
#define ICON_PLAYLIST QIcon::fromTheme("view-media-playlist")
#define ICON_TITLE    ICON_PLAY
#define ICON_ALBUM    QIcon::fromTheme("media-optical")
#define ICON_ARTIST   QIcon::fromTheme("view-media-artist")
#define ICON_SEARCH   QIcon::fromTheme("system-search")


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
	setAcceptDrops(true);
}


void MainWindow::appendPlaylist(QString title, QString artist, QString album, int ind)
{
	QTableWidget *table = playlistTable;
	int currentRow = table->rowCount();
	table->insertRow(currentRow);
	QString s_num;
	s_num.setNum(ind);
	if(title.contains(QRegExp("^\\d\\d - "))) {
		title.replace(QRegExp("^\\d\\d - "), "");
	}
	QTableWidgetItem *titleItem = new QTableWidgetItem(title);
	QTableWidgetItem *artistItem = new QTableWidgetItem(artist);
	QTableWidgetItem *albumItem = new QTableWidgetItem(album);
	QTableWidgetItem *indexItem = new QTableWidgetItem(s_num);

	titleItem->setBackground(TITLE_COLOR);
	artistItem->setBackground(ARTIST_COLOR);
	albumItem->setBackground(ALBUM_COLOR);

        titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
        artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);
        albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);
        indexItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);

	table->setItem(currentRow, TITLE_COLUMN, titleItem);
	table->setItem(currentRow, ARTIST_COLUMN, artistItem);
	table->setItem(currentRow, ALBUM_COLUMN, albumItem);
	table->setItem(currentRow, INDEX_COLUMN, indexItem);

	table->setColumnHidden(INDEX_COLUMN, true);
}


QTreeWidgetItem *MainWindow::addEntry(QTreeWidget *tree, QString title, QString artist, QString album, int ind)
{
	int artistCount = tree->topLevelItemCount();
	QTreeWidgetItem *artistItem = NULL;

	if(artist.isEmpty())
	      artist = "Various";
	if(album.isEmpty())
	      album = "Single";

	for(int i = 0; i < artistCount; i++) {
		QString i_artist = tree->topLevelItem(i)->text(0);
		if(i_artist.compare(artist) == 0) {
			artistItem = tree->topLevelItem(i);
			break;
		}
	}
	if(!artistItem) {
		artistItem = new QTreeWidgetItem;
		artistItem->setText(0, artist);
		artistItem->setFont(0, ARTIST_FONT);
		artistItem->setForeground(0, ARTIST_COLOR_FOREGROUND);
		artistItem->setIcon(0, ICON_ARTIST);
		tree->addTopLevelItem(artistItem);
	}

	QTreeWidgetItem *albumItem = NULL;
	int albumCount = artistItem->childCount();
	for(int i = 0; i < albumCount; i++) {
		QString i_album = artistItem->child(i)->text(0);
		if(i_album.compare(album) == 0) {
			albumItem = artistItem->child(i);
			break;
		}
	}

	if(!albumItem) {
		albumItem = new QTreeWidgetItem;
		albumItem->setText(0, album);
		albumItem->setFont(0, ALBUM_FONT);
		albumItem->setForeground(0, ALBUM_COLOR_FOREGROUND);
		albumItem->setIcon(0, ICON_ALBUM);
		artistItem->addChild(albumItem);
	}

	QTreeWidgetItem *titleItem = new QTreeWidgetItem;
	titleItem->setText(0, title);
	QString s_ind;
	s_ind.setNum(ind);
	titleItem->setText(1, s_ind);
	titleItem->setFont(0, TITLE_FONT);
	titleItem->setForeground(0, TITLE_COLOR_FOREGROUND);
	titleItem->setIcon(0, ICON_TITLE);

	albumItem->addChild(titleItem);

	return titleItem;

}

void MainWindow::playlistTableClicked(int row, int /* column */)
{
	int realRow = playlistTable->item(row, INDEX_COLUMN)->text().toInt();
	QTreeWidgetItem *item = treeItemList[realRow];
	treeClicked(item, 0);
}

void MainWindow::retry(void)
{
	int last = playlistTable->rowCount() - 2;
	if(last < 0)
	      return;
	int playRow = playlistTable->item(last, INDEX_COLUMN)->text().toInt();
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
	int sourcesIndex = playlistTable->item(thisInd, INDEX_COLUMN)->text().toInt();
	if(sourcesIndex >= sources.size()) {
		std::cerr << "sourcesIndex invalid: " << sourcesIndex << std::endl;
		return;
	}
	mediaObject->stop();

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
	searchTree->clear();
}

void MainWindow::searchDB()
{
	QString search = searchBox->text().toLower();
	clearSearchWindow();
	if(search.isEmpty()) {
		searchTree->hide();
		browserTree->show();
		return;
	}
	for(int i = 0; i < treeItemList.length(); i++) {
		bool found = false;
		QString title = treeItemList[i]->text(0).toLower();
		QString album = treeItemList[i]->parent()->text(0).toLower();
		QString artist = treeItemList[i]->parent()->parent()->text(0).toLower();
		if(found == false && (currentSearchOption & SEARCH_TITLE)) {
			if(title.contains(search)) {
				found = true;
			}
		}
		if(found == false && (currentSearchOption & SEARCH_ALBUM)) {
			if(album.contains(search)) {
				found = true;
			}
		}
		if(found == false && (currentSearchOption & SEARCH_ARTIST)) {
			if(artist.contains(search)) {
				found = true;
			}
		}
		if(found == true) {
			(void)addEntry(searchTree, title, artist, album, i);
		}
	}
	searchTree->sortItems(0, Qt::AscendingOrder);
	browserTree->hide();
	searchTree->show();
}

void MainWindow::searchOptionAll(void)
{
	searchOptionButton->setText("All");
	currentSearchOption = SEARCH_ALL;
	searchDB();
}
void MainWindow::searchOptionArtist(void)
{
	searchOptionButton->setText("Artist");
	currentSearchOption = SEARCH_ARTIST;
	searchDB();
}
void MainWindow::searchOptionAlbum(void)
{
	searchOptionButton->setText("Album");
	currentSearchOption = SEARCH_ALBUM;
	searchDB();
}
void MainWindow::searchOptionTitle(void)
{
	searchOptionButton->setText("Title");
	currentSearchOption = SEARCH_TITLE;
	searchDB();
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
		loadDB(string.toAscii().data());
	}
}

void MainWindow::loadDB(char *s_dbfile)
{
	QString dbfile(s_dbfile);
	QString message;

	if(!dbfile.endsWith(".hdb")) {
		message =  "Ignoring " + dbfile + ": Not a hdb file";
		statusBar->showMessage(message,4000);
		std::cerr << message.toAscii().data() << std::endl;
		return;
	}
	if(!QFile::exists(dbfile)) {
		message = "File " + dbfile + ": does not exist";
		statusBar->showMessage(message, 4000);
		std::cerr << message.toAscii().data() << std::endl;
		return;
	}

	int at = htdb.length();

	if(htdb.existsInDB(dbfile)) {
		QString str(s_dbfile);
		str.append(" Already loaded, so skipping it");
		statusBar->showMessage(str, 2000);
		std::cerr << str.toAscii().data() << std::endl;
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
	treeItemList.reserve(htdb.length());
	browserTree->hide();
	for(unsigned int i = at; i < htdb.length(); i++) {
		QString string(htdb.ind_name(i));
		Phonon::MediaSource source(string);
		sources.append(source);
		QString title(htdb.ind_title(i));
		QString track(htdb.ind_track(i));
		QString artist(htdb.ind_artist(i));
		QString album(htdb.ind_album(i));

		if(title.isEmpty()) {
			title = QString(htdb.ind_name(i));
		} else if(!artist.isEmpty() && !album.isEmpty()) {
			title = track + " - " + title;
		}

		QTreeWidgetItem *item = addEntry(browserTree, title, artist, album, i);
		treeItemList.append(item);
	}
	browserTree->sortItems(0, Qt::AscendingOrder);
	browserTree->show();
}

void MainWindow::about()
{
	QMessageBox::information(this, tr("Spectradio"), AboutMessage);
}

void MainWindow::dropEvent(QDropEvent *event)
{
	event->acceptProposedAction();
	QString file = event->mimeData()->text();
	file.replace("file://","");
	loadDB(file.toAscii().data());
}
void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
	event->acceptProposedAction();
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
			playAction->setIcon(ICON_PAUSE);
			stopAction->setEnabled(true);
			retryAction->setEnabled(true);
			nextAction->setEnabled(true);
			break;

		case Phonon::StoppedState:
			stopAction->setEnabled(false);
			playAction->setEnabled(true);
			playAction->setIcon(ICON_PLAY);
			retryAction->setEnabled(false);
			nextAction->setEnabled(false);
			timeLcd->display("00:00");
			break;

		case Phonon::PausedState:
			playAction->setEnabled(true);
			playAction->setIcon(ICON_PLAY);
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

void MainWindow::searchTreeClicked(QTreeWidgetItem *item, int /* column */)
{
	/* Start playing */
	if(item->childCount() > 0) {
		item->setExpanded(!item->isExpanded());
		return;
	}
	stop = 1;
	int sourcesIndex = item->text(1).toInt();
	QTreeWidgetItem *browserItem = treeItemList[sourcesIndex];
	searchTree->hide();
	browserTree->show();
	treeClicked(browserItem, 0);
}

void MainWindow::playSource(int sourceIndex)
{
	mediaObject->stop();
	mediaObject->clearQueue();
	if(sourcesIndex >= sources.size()) {
		return;
	}
	htdb.set_playing(sourcesIndex);
	mediaObject->setCurrentSource(sources[sourcesIndex]);
	mediaObject->play();
}

void MainWindow::treeClicked(QTreeWidgetItem *item, int /* column */)
{
	if(item->childCount() > 0) {
		item->setExpanded(!item->isExpanded());
		return;
	}
	int sourcesIndex = item->text(1).toInt();
	playSource(sourcesIndex);
}

void MainWindow::sourceChanged(const Phonon::MediaSource &source)
{
	int sourcesIndex = sources.indexOf(source);
	QTreeWidgetItem *titleItem = treeItemList[sourcesIndex];
	QTreeWidgetItem *albumItem = titleItem->parent();
	QTreeWidgetItem *artistItem = albumItem->parent();
	browserTree->setCurrentItem(titleItem);
	titleItem->setSelected(true);
	setTitle(titleItem->text(0), artistItem->text(0), albumItem->text(0));
	appendPlaylist( titleItem->text(0),
			artistItem->text(0),
			albumItem->text(0),
			sourcesIndex);

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

void MainWindow::setTitle(QString title, QString artist, QString album)
{
	QString windowTitle;
	QString message;

	if(title.contains(QRegExp("^\\d\\d - "))) {
		title.replace(QRegExp("^\\d\\d - "), "");
	}
				

	message = STATUS_BAR_COLOR_BLUE + title + STATUS_BAR_TEXT_END;
	windowTitle = title;

	if(!artist.isEmpty()) {
		windowTitle += " by " + artist;
		message += STATUS_BAR_COLOR_BLACK + " by " + STATUS_BAR_TEXT_END +
			  STATUS_BAR_COLOR_RED + artist + STATUS_BAR_TEXT_END;
	}

	if(!album.isEmpty()) {
		message = message + 
		    STATUS_BAR_COLOR_BLUE + " (" + album + ")" + STATUS_BAR_TEXT_END;
	}

	setWindowTitle(windowTitle);
	songLabel->setText(message);
}

void MainWindow::setupActions()
{
	playAction = new QAction(ICON_PLAY, tr("Play/Pause"), this);
	playAction->setShortcut(tr("space"));
	playAction->setToolTip("Play (space)");
	playAction->setDisabled(true);

	stopAction = new QAction(ICON_STOP, tr("Stop"), this);
	stopAction->setShortcut(tr("Ctrl+S"));
	stopAction->setToolTip("Stop (Ctrl+S)");
	stopAction->setDisabled(true);

	nextAction = new QAction(ICON_NEXT, tr("Next"), this);
	nextAction->setShortcut(tr("Ctrl+N"));
	nextAction->setToolTip("Skip the current playing track and play the next predicted (Ctrl+N)");
	nextAction->setDisabled(true);

	retryAction = new QAction(ICON_RETRY, tr("Retry"), this);
	retryAction->setShortcut(tr("Ctrl+R"));
	retryAction->setToolTip("Skip the current playing and rerun the prediction from the last played track (Ctrl+R)");
	retryAction->setDisabled(true);

	loadDBAction = new QAction(ICON_LOAD, tr("Load DB"), this);
	loadDBAction->setShortcut(tr("Ctrl+L"));
	loadDBAction->setToolTip("Load the hist DB (Ctrl+L)");
	loadDBAction->setDisabled(false);

	settingsAction = new QAction(ICON_SETTINGS, tr("Distance Function"), this);
	settingsAction->setShortcut(tr("Ctrl+D"));
	settingsAction->setToolTip("Choose the distance function for track prediction (Ctrl+D)");
	settingsAction->setDisabled(false);

	togglePlaylistAction = new QAction(ICON_PLAYLIST, tr("Toggle Playlist"), this);
	togglePlaylistAction->setShortcut(tr("Alt+P"));
	togglePlaylistAction->setToolTip("Toggle playlist (Alt+P)");
	settingsAction->setDisabled(false);

	aboutAction = new QAction(ICON_ABOUT, tr("About"), this);
	aboutAction->setDisabled(false);

	searchAllAction = new QAction(tr("All"), this);
	searchArtistAction = new QAction(tr("Artist"), this);
	searchAlbumAction = new QAction(tr("Album"), this);
	searchTitleAction = new QAction(tr("Title"), this);

	connect(searchAllAction, SIGNAL(triggered()), this, SLOT(searchOptionAll()));
	connect(searchArtistAction, SIGNAL(triggered()), this, SLOT(searchOptionArtist()));
	connect(searchAlbumAction, SIGNAL(triggered()), this, SLOT(searchOptionAlbum()));
	connect(searchTitleAction, SIGNAL(triggered()), this, SLOT(searchOptionTitle()));


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
	// ----------- Hidden artifacts ---------------
	dialogBox = new QDialogButtonBox();
	QList<QString> distFuncs = htdb.getSupportedDistanceFunctions();
	for(int i = 0; i < distFuncs.length(); i++) {
		dialogBox->addButton(distFuncs[i], QDialogButtonBox::ActionRole);
	}
	dialogBox->addButton("Cancel", QDialogButtonBox::ActionRole);
	dialogBox->setOrientation(Qt::Vertical);
	dialogBox->setCenterButtons(true);
	connect(dialogBox, SIGNAL(clicked(QAbstractButton *)), 
				this, SLOT(acceptSettings(QAbstractButton *)));
	dialogBox->hide();

	// --------------- Top Tool bar ----------------
	QToolBar *tbar1 = new QToolBar;		// toolbar 
	tbar1->addAction(loadDBAction);		// load db
	tbar1->addAction(playAction);		// play
	tbar1->addAction(stopAction);		// stop
	tbar1->addAction(retryAction);		// Retry
	tbar1->addAction(nextAction);		// Next

	seekSlider = new Phonon::SeekSlider(this);
	seekSlider->setMediaObject(mediaObject);

	volumeSlider = new Phonon::VolumeSlider(this);
	volumeSlider->setAudioOutput(audioOutput);
	volumeSlider->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Maximum);

	QLabel *volumeLabel = new QLabel;
	volumeLabel->setPixmap(QPixmap("images/volume.png"));

	QToolBar *tbar2 = new QToolBar;
	tbar2->addAction(settingsAction);	// settings
	tbar2->addAction(aboutAction);		// about

	QHBoxLayout *toolBar = new QHBoxLayout;	// the toolbar layout
	toolBar->addWidget(tbar1);		// toolbar (above)
	toolBar->addWidget(seekSlider);		// slider
	toolBar->addWidget(volumeLabel);	// volume label
	toolBar->addWidget(volumeSlider);	// volume slider
	toolBar->addWidget(tbar2);		// toggle Playlist

	// ------------- Browser Layout ------------------------
	//
	// +++++++++++++ Search Bar ++++++++++++++++++++++
	searchBox = new QLineEdit;
	searchBox->setText("");
	connect(searchBox, SIGNAL(editingFinished()), this, SLOT(searchDB()));

	QToolBar *searchBar = new QToolBar;
	searchOptionButton = new QToolButton(searchBar);
	searchOptionButton->setPopupMode( QToolButton::MenuButtonPopup );
	searchOptionButton->setText("All");
	searchOptionButton->setIcon(ICON_SEARCH);
	searchBar->addWidget(searchOptionButton);
	searchBar->addWidget(searchBox);
	searchBar->addAction(togglePlaylistAction); // toggle playlist

	QMenu *searchMenu = new QMenu(searchBar);
	searchOptionButton->setMenu(searchMenu);
	searchMenu->addAction(searchAllAction);
	searchMenu->addAction(searchArtistAction);
	searchMenu->addAction(searchAlbumAction);
	searchMenu->addAction(searchTitleAction);
	searchMenu->setDefaultAction(searchAllAction);
	currentSearchOption = SEARCH_ALL;
	connect(searchOptionButton, SIGNAL(clicked()), searchMenu, SLOT(show()));

	// ++++++++++++++ Search Tree ++++++++++++++++++++
	searchTree = new QTreeWidget;
	searchTree->hide();
	searchTree->header()->hide();
	connect(searchTree, SIGNAL(itemActivated(QTreeWidgetItem *, int)), 
		this, SLOT(searchTreeClicked(QTreeWidgetItem *, int)));

	// +++++++++++++ Browser Tree ++++++++++++++++++++
	browserTree = new QTreeWidget;
	browserTree->header()->hide();
	connect(browserTree, SIGNAL(itemActivated(QTreeWidgetItem *, int)), this, SLOT(treeClicked(QTreeWidgetItem *, int)));


	// +++++++++++++ Playlist Table +++++++++++++++++++
	QStringList headers;
	headers << tr("Title") << tr("Artist") << tr("Album") << tr("Index");

	playlistTable = new QTableWidget(0, 4);
	playlistTable->setHorizontalHeaderLabels(headers);
	playlistTable->setSelectionMode(QAbstractItemView::SingleSelection);
	playlistTable->setSelectionBehavior(QAbstractItemView::SelectRows);
	playlistTable->verticalHeader()->hide();
	playlistTable->setColumnHidden(3, true);
	playlistTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
	playlistVisible = true;
	connect(playlistTable, SIGNAL(cellPressed(int, int)), 
		this, SLOT(playlistTableClicked(int, int)));

	// +++++++++++ layout ++++++++++++++++++++++++++
	QVBoxLayout *DBLayout = new QVBoxLayout;
	DBLayout->addWidget(searchBar);
	DBLayout->addWidget(searchTree);
	DBLayout->addWidget(browserTree);

	QVBoxLayout *PlaylistLayout = new QVBoxLayout;
	PlaylistLayout->addWidget(playlistTable);

	QHBoxLayout *SubMainLayout = new QHBoxLayout;
	SubMainLayout->addLayout(DBLayout);
	SubMainLayout->addLayout(PlaylistLayout);

	// ------------ STATUS BAR ---------------
	songLabel = new QLabel;
	songLabel->setTextFormat(Qt::RichText);
	songLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);

	QPalette palette;
	palette.setBrush(QPalette::Light, Qt::darkGray);
	timeLcd = new QLCDNumber;
	timeLcd->setPalette(palette);

	// status bar
	statusBar = new QStatusBar;
	statusBar->clearMessage();
	statusBar->addPermanentWidget(songLabel);	// Song label
	statusBar->addPermanentWidget(timeLcd);		// time LCD

	// ------------- Main layout ------------a
	QVBoxLayout *mainLayout = new QVBoxLayout;
	mainLayout->addLayout(toolBar);
	mainLayout->addLayout(SubMainLayout);
	mainLayout->addWidget(statusBar);

	QWidget *widget = new QWidget;
	widget->setLayout(mainLayout);

	setCentralWidget(widget);
	setWindowTitle("spectradio");
}

