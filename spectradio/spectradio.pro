 QT        += phonon

 HEADERS   += mainwindow.h \
              hist.h
 SOURCES   += main.cpp \
              mainwindow.cpp \
	      hist.cpp

 # install
 target.path = $$[QT_INSTALL_EXAMPLES]/phonon/spectradio
 sources.files = $$SOURCES $$HEADERS $$FORMS $$RESOURCES *.pro *.png images
 sources.path = $$[QT_INSTALL_EXAMPLES]/phonon/spectradio
 INSTALLS += target sources

 wince*{
 DEPLOYMENT_PLUGIN += phonon_ds9 phonon_waveout
 }

 symbian:TARGET.UID3 = 0xA000CF6A

