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

#include "coverdownloader.h"

CoverDownloader::CoverDownloader(QString coverDownloadBase, QString coverCacheDir, QObject *parent) :
    QObject(parent),
    http(new QHttp(this)),
    coverBase(coverDownloadBase),
    cacheDir(coverCacheDir)
{
}

CoverDownloader::~CoverDownloader()
{
    http->deleteLater();
}

void CoverDownloader::abort()
{
    this->disconnect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));
    http->abort();
}

void CoverDownloader::download(QString gameID, bool saveInCache)
{
    //qDebug() << "download request! gameID=" << gameID;

    this->disconnect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));
    http->abort();
    http->close();
    http->deleteLater();
    http = new QHttp(this);
    this->connect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));

    tryID = gameID;

    // check if cover is in cache
    bool isCoverCached = isCoverInCache(gameID);
    //qDebug() << "isCoverCached=" << isCoverCached;
    if (!isCoverCached) {
        QUrl url(coverBase + gameID + ".PNG");
        // quick warn if url invalid (not abortive !!!)
        if (!url.isValid())
            qDebug() << "Invalid url given to cover downloader!";

        coverMustBeCached = saveInCache;

        //qDebug() << "Downloading cover at" << url.toString();
        httpBuffer = new QBuffer(&httpBytes);
        httpBuffer->open(QIODevice::WriteOnly);
        http->setHost(url.host());
        httpRequestID = http->get(url.path(), httpBuffer);

        //qDebug() << "download req for" << url.toString() << "sent!";
    }
    else {
        bool error = false;
        QImage img;
        img.load(cacheDir + gameID + ".PNG");
        if (img.format() == QImage::Format_Invalid)
            error = true;
        else {
            coverID = tryID;
            coverImg = img;
        }
        emit coverDownloaded(error);
    }
}

QString CoverDownloader::gameID(void)
{
    return coverID;
}

QImage CoverDownloader::image(void)
{
    return coverImg;
}


//
// private slots
//

void CoverDownloader::httpRequestFinished_Slot(int id, bool error)
{
    if (id == httpRequestID) {
        if (error) {
            //qDebug() << "failed to download" << tryUrl;
        }
        else {
            //qDebug() << "cover download OK" << tryUrl;
            QImage img;
            img.loadFromData(httpBytes);
            if (img.format() == QImage::Format_Invalid) {
                //qDebug() << "invalid cover!";
                error = true;
            }
            else {
                coverID = tryID;
                coverImg = img;
            }
        }

        emit coverDownloaded(error);

        if (coverMustBeCached) {
            if ((!error) && !(isCoverInCache(coverID))) { // if no error we save cover in local disk cache
                QDir().mkpath(cacheDir);
                QFile file(cacheDir + coverID + ".PNG");
                file.open(QIODevice::WriteOnly | QIODevice::Truncate);
                file.write(httpBytes);
                file.close();
            }
        }

        httpBuffer->close();
        delete httpBuffer;
    }
}

//
// private
//

bool CoverDownloader::isCoverInCache(QString gameID)
{
    return QFile::exists(cacheDir + gameID + ".PNG");
}
