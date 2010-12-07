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

#ifndef COVERCACHETHREAD_H
#define COVERCACHETHREAD_H

#include <QObject>
#include <QThread>
#include <QString>
#include <QStringList>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QWaitCondition>
#include <QHttp>
#include <QImage>
#include <QUrl>
#include <QBuffer>
#include <QDebug>

class CoverCacheThread : public QThread
{
    Q_OBJECT

public:

    CoverCacheThread(QString base, QString dir, QObject *parent=0) :
        QThread(parent),
        aborted(false),
        http(new QHttp),
        coverBase(base),
        cacheDir(dir)
    {
        http->moveToThread(parent->thread());
        this->connect(this, SIGNAL(finished()), this, SLOT(threadFinished_Slot()));
    }

    void setIDList(QStringList list)
    {
        mutex.lock();
        IDList.clear();
        IDList = list;
        mutex.unlock();
    }

    void abort() {
        mutex.lock();
        aborted = true;
        mutex.unlock();
        coverCachedCondition.wakeAll();
    }

    // reimp!
    void run()
    {
        mutex.lock();
        aborted = false;
        mutex.unlock();

        //qDebug() << "CoverCacheThread start!";
        //qDebug() << "coverBase=" << coverBase;
        //qDebug() << "cacheDir=" << cacheDir;

        foreach (QString gameID, IDList) {
            if (checkAborted()) {
                http->abort();
                break;
            }
            download(gameID);
        }

        this->quit();
    }

signals:
    void finished(bool aborted);

private slots:

    void threadFinished_Slot()
    {
        emit finished(checkAborted());
    }

    void httpRequestFinished_Slot(int id, bool error)
    {
        if (id == httpRequestID) {
            if (!checkAborted()) {
                if (error) {
                    //qDebug() << "failed to download" << coverID;
                }
                else {
                    //qDebug() << "cover download OK" << coverID;
                    QImage img;
                    img.loadFromData(httpBytes);
                    if (!(img.format() == QImage::Format_Invalid)) {
                        QDir().mkpath(cacheDir);
                        //qDebug() << "saving cover to" << cacheDir + coverID + ".PNG";
                        QFile file(cacheDir + coverID + ".PNG");
                        file.open(QIODevice::WriteOnly | QIODevice::Truncate);
                        file.write(httpBytes);
                        file.close();
                    }
                }
            }

            httpBuffer->close();
            delete httpBuffer;

            coverCachedCondition.wakeAll();
        }
    }

private:
    bool aborted;
    QHttp *http;
    QString coverBase;
    QString cacheDir;
    QString coverID;
    QMutex mutex;
    QWaitCondition coverCachedCondition;
    QStringList IDList;
    QBuffer *httpBuffer;
    QByteArray httpBytes;
    int httpRequestID;

    bool checkAborted()
    {
        mutex.lock();
        bool res = aborted;
        mutex.unlock();

        return res;
    }

    void download(QString gameID)
    {
        //qDebug() << "download request! gameID=" << gameID;

        this->disconnect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));
        http->abort();
        http->close();
        http->deleteLater();
        http = new QHttp;
        http->moveToThread(this->parent()->thread());
        this->connect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));

        coverID = gameID;

        // check if cover is in cache
        bool isCoverCached = isCoverInCache(gameID);
        //qDebug() << "isCoverCached=" << isCoverCached;
        if (!isCoverCached) {
            QUrl url(coverBase + gameID + ".PNG");
            // quick warn if url invalid (not abortive !!!)
            if (!url.isValid())
                qDebug() << "Invalid url given to cover downloader!";

            //qDebug() << "Downloading cover at" << url.toString();
            httpBuffer = new QBuffer(&httpBytes);
            httpBuffer->open(QIODevice::WriteOnly);
            http->setHost(url.host());
            httpRequestID = http->get(url.path(), httpBuffer);

            //qDebug() << "download req for" << url.toString() << "sent!";
            coverCachedCondition.wait(&mutex);
        }
    }

    bool isCoverInCache(QString gameID)
    {
        return QFile::exists(cacheDir + gameID + ".PNG");
    }

};

#endif // COVERCACHETHREAD_H
