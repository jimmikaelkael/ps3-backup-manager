// PS3-Backup-Manager
// Copyright (C) 2010 - Metagames
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <QtGui/QApplication>
#include <QLocale>
#include <QTranslator>
#include <QLibraryInfo>

#if defined(_USE_STATIC_BUILDS_)
  #include <QtPlugin>
  #if defined (Q_WS_X11) // Linux system
    #if defined(_USE_qsvgicon)
    Q_IMPORT_PLUGIN(qsvgicon)
    #endif
  #endif
  #if defined(_USE_qico)
  Q_IMPORT_PLUGIN(qico)
  #endif
  #if defined(_USE_qjpeg)
  Q_IMPORT_PLUGIN(qjpeg)
  #endif
  #if defined(_USE_qgif)
  Q_IMPORT_PLUGIN(qgif)
  #endif
#endif

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // qt translation for default dialogs (QFileDialog/QMessageBox) and so on
    QTranslator qtTranslator;
    qtTranslator.load("translations/qt_" + QLocale::system().name());
    a.installTranslator(&qtTranslator);

    // load translation if exist
    QTranslator appTranslator;
    appTranslator.load("translations/app_" + QLocale::system().name());
    a.installTranslator(&appTranslator);

    MainWindow w;
    // check the license is accepted by the user
    if (w.isLicenseAccepted())
        w.show();
    else
        return 0;

    // exec the app
    return a.exec();
}
