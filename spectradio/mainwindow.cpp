 /****************************************************************************
 **
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
     metaInformationResolver = new Phonon::MediaObject(this);

     mediaObject->setTickInterval(1000);
     connect(mediaObject, SIGNAL(tick(qint64)), this, SLOT(tick(qint64)));
     connect(mediaObject, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
             this, SLOT(stateChanged(Phonon::State,Phonon::State)));
     connect(metaInformationResolver, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
             this, SLOT(metaStateChanged(Phonon::State,Phonon::State)));
     connect(mediaObject, SIGNAL(currentSourceChanged(Phonon::MediaSource)),
             this, SLOT(sourceChanged(Phonon::MediaSource)));
     connect(mediaObject, SIGNAL(aboutToFinish()), this, SLOT(aboutToFinish()));

     Phonon::createPath(mediaObject, audioOutput);

     setupActions();
     setupMenus();
     setupUi();
     timeLcd->display("00:00");
 }

 void MainWindow::loadDB()
 {
     htdb.LoadDB("/home/aeprasad/mood2pt/db.hdb");

     if(htdb.is_valid() != true) {
        std::cout << "Error db not read!\n" << std::endl;
     	exit(-1);
     }

     sources.reserve(htdb.length());
     for(unsigned int i = 0; i < htdb.length(); i++) {
     	QString string(htdb.ind_name(i));
	Phonon::MediaSource source(string);
	htdb.set_media_source(i, source);
	sources.append(source);
     }
     if (!sources.isEmpty())
         metaInformationResolver->setCurrentSource(sources.at(0));
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

 void MainWindow::tableClicked(int row, int /* column */)
 {
     bool wasPlaying = mediaObject->state() == Phonon::PlayingState;

     mediaObject->stop();
     mediaObject->clearQueue();

     if (row >= sources.size())
         return;

     htdb.set_playing(row);
     mediaObject->setCurrentSource(sources[row]);
     setWindowTitle(htdb.ind_name(row));

     if (wasPlaying)
         mediaObject->play();
     else
         mediaObject->stop();
 }

 void MainWindow::sourceChanged(const Phonon::MediaSource &source)
 {
     musicTable->selectRow(sources.indexOf(source));
     timeLcd->display("00:00");
 }

 void MainWindow::metaStateChanged(Phonon::State newState, Phonon::State /* oldState */)
 {
     if (newState == Phonon::ErrorState) {
         QMessageBox::warning(this, tr("Error opening files"),
             metaInformationResolver->errorString());
         while (!sources.isEmpty() &&
                !(sources.takeLast() == metaInformationResolver->currentSource())) {}  /* loop */;
         return;
     }
     if (newState != Phonon::StoppedState && newState != Phonon::PausedState)
         return;

     if (metaInformationResolver->currentSource().type() == Phonon::MediaSource::Invalid)
             return;

     QMap<QString, QString> metaData = metaInformationResolver->metaData();

     QString title = metaData.value("TITLE");
     if (title == "")
         title = metaInformationResolver->currentSource().fileName();

     QTableWidgetItem *titleItem = new QTableWidgetItem(title);
     titleItem->setFlags(titleItem->flags() ^ Qt::ItemIsEditable);
     QTableWidgetItem *artistItem = new QTableWidgetItem(metaData.value("ARTIST"));
     artistItem->setFlags(artistItem->flags() ^ Qt::ItemIsEditable);
     QTableWidgetItem *albumItem = new QTableWidgetItem(metaData.value("ALBUM"));
     albumItem->setFlags(albumItem->flags() ^ Qt::ItemIsEditable);

     int currentRow = musicTable->rowCount();
     musicTable->insertRow(currentRow);
     musicTable->setItem(currentRow, 0, titleItem);
     musicTable->setItem(currentRow, 1, artistItem);
     musicTable->setItem(currentRow, 2, albumItem);

     if (musicTable->selectedItems().isEmpty()) {
         musicTable->selectRow(0);
         mediaObject->setCurrentSource(metaInformationResolver->currentSource());
     }

     Phonon::MediaSource source = metaInformationResolver->currentSource();
     int index = sources.indexOf(metaInformationResolver->currentSource()) + 1;
     if (sources.size() > index) {
         metaInformationResolver->setCurrentSource(sources.at(index));
     }
     else {
         musicTable->resizeColumnsToContents();
         if (musicTable->columnWidth(0) > 300)
             musicTable->setColumnWidth(0, 300);
	 std::cout << "Done preparing tracks..." << std::endl;
     }
 }

 void MainWindow::aboutToFinish()
 {
     int index = sources.indexOf(mediaObject->currentSource());
     index = htdb.get_next(index);
     if (sources.size() > index) {
	 setWindowTitle(htdb.ind_name(index));
         mediaObject->enqueue(sources.at(index));
     }
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
     loadDBAction = new QAction(tr("Load &DB"), this);
     loadDBAction->setShortcut(tr("Ctrl+F"));
     exitAction = new QAction(tr("E&xit"), this);
     exitAction->setShortcuts(QKeySequence::Quit);
     aboutAction = new QAction(tr("A&bout"), this);
     aboutAction->setShortcut(tr("Ctrl+B"));
     aboutQtAction = new QAction(tr("About &Qt"), this);
     aboutQtAction->setShortcut(tr("Ctrl+Q"));

     connect(playAction, SIGNAL(triggered()), mediaObject, SLOT(play()));
     connect(pauseAction, SIGNAL(triggered()), mediaObject, SLOT(pause()) );
     connect(stopAction, SIGNAL(triggered()), mediaObject, SLOT(stop()));
     connect(loadDBAction, SIGNAL(triggered()), this, SLOT(loadDB()));
     connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
     connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
     connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
 }

 void MainWindow::setupMenus()
 {
     QMenu *fileMenu = menuBar()->addMenu(tr("&File"));
     fileMenu->addAction(loadDBAction);
     fileMenu->addSeparator();
     fileMenu->addAction(exitAction);

     QMenu *aboutMenu = menuBar()->addMenu(tr("&Help"));
     aboutMenu->addAction(aboutAction);
     aboutMenu->addAction(aboutQtAction);
 }

 void MainWindow::setupUi()
 {
     QToolBar *bar = new QToolBar;

     bar->addAction(playAction);
     bar->addAction(pauseAction);
     bar->addAction(stopAction);

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

     QStringList headers;
     headers << tr("Title") << tr("Artist") << tr("Album");

     musicTable = new QTableWidget(0, 3);
     musicTable->setHorizontalHeaderLabels(headers);
     musicTable->setSelectionMode(QAbstractItemView::SingleSelection);
     musicTable->setSelectionBehavior(QAbstractItemView::SelectRows);
     connect(musicTable, SIGNAL(cellPressed(int,int)),
             this, SLOT(tableClicked(int,int)));

     QHBoxLayout *seekerLayout = new QHBoxLayout;
     seekerLayout->addWidget(seekSlider);
     seekerLayout->addWidget(timeLcd);

     QHBoxLayout *playbackLayout = new QHBoxLayout;
     playbackLayout->addWidget(bar);
     playbackLayout->addStretch();
     playbackLayout->addWidget(volumeLabel);
     playbackLayout->addWidget(volumeSlider);

     QVBoxLayout *mainLayout = new QVBoxLayout;
     mainLayout->addWidget(musicTable);
     mainLayout->addLayout(seekerLayout);
     mainLayout->addLayout(playbackLayout);

     QWidget *widget = new QWidget;
     widget->setLayout(mainLayout);

     setCentralWidget(widget);
     setWindowTitle("");
 }

