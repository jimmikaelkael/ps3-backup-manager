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

#ifndef ADREFRESHER_H
#define ADREFRESHER_H

#include <QObject>
#include <QTimer>
#include <QHttp>
#include <QUrl>
#include <QBuffer>
#include <QImage>
#include <QDebug>

#include "ad.h"

class AdRefresher : public QObject
{
    Q_OBJECT

public:
    AdRefresher(QUrl url, QObject *parent = 0);
    ~AdRefresher();
    void start();

signals:
    void adRefreshed(QImage image, QUrl url);

private slots:
    void downloadAdUrlList_Slot();
    void downloadAd_Slot();
    void refreshAd_Slot();
    void httpRequestFinished_Slot(int id, bool error);

private:
    QUrl adUrlListUrl;
    QHttp *http;
    QTimer *downloadTimer, *refreshTimer;
    QBuffer *httpBuffer;
    QByteArray httpBytes;
    int adUrlListHttpRequestID, adHttpRequestID;
    QStringList adUrlList;
    QString adTargetUrl;
    int adUrlListIndex;
    bool adUrlListIsReady;
    QList<Ad> adList;
    int adIndex;

    enum timerTick {
        timerList = 2000,       // delay(in ms) before to download ad Url list
        timerAd = 7000,         // delay(in ms) before to download another ad
        timerRefresh = 10000    // delay(in ms) before to refresh ad
    };

};

#endif // ADREFRESHER_H
