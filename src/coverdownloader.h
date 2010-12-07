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

#ifndef COVERDOWNLOADER_H
#define COVERDOWNLOADER_H

#include <QObject>
#include <QHttp>
#include <QUrl>
#include <QBuffer>
#include <QImage>
#include <QDir>
#include <QFile>
#include <QDebug>

class CoverDownloader : public QObject
{
    Q_OBJECT

public:
    CoverDownloader(QString coverDownloadBase, QString coverCacheDir, QObject *parent = 0);
    ~CoverDownloader();
    void abort();
    void download(QString gameID, bool saveInCache);
    QString gameID(void);
    QImage image(void);

signals:
    void coverDownloaded(bool error);

private slots:
    void httpRequestFinished_Slot(int id, bool error);

private:
    bool isCoverInCache(QString gameID);

    QHttp *http;
    QString coverBase;
    QString cacheDir;
    QString tryID, coverID;
    QImage coverImg;
    bool coverMustBeCached;
    QBuffer *httpBuffer;
    QByteArray httpBytes;
    int httpRequestID;

};

#endif // COVERDOWNLOADER_H
