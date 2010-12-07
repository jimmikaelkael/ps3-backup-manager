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

#include "adrefresher.h"

AdRefresher::AdRefresher(QUrl url, QObject *parent) :
    QObject(parent),
    http(new QHttp(this)),
    downloadTimer(new QTimer(this)),
    refreshTimer(new QTimer(this)),
    adUrlListIsReady(false)
{
    // quick warn if url invalid (not abortive !!!)
    if (!url.isValid())
        qDebug() << "Invalid url given to ad refresher!";
    adUrlListUrl = url;

    // connect http request finished signal
    this->connect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));

    // connect timer timeout to refresh ad url list / ads
    this->connect(downloadTimer, SIGNAL(timeout()), this, SLOT(downloadAdUrlList_Slot()));
}

AdRefresher::~AdRefresher()
{
    downloadTimer->stop();
    delete downloadTimer;
    refreshTimer->stop();
    delete refreshTimer;
}

void AdRefresher::start()
{
    // start the timer
    downloadTimer->start(AdRefresher::timerList);
}

//
// private slots
//

void AdRefresher::downloadAdUrlList_Slot()
{
    downloadTimer->stop();
    this->disconnect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));
    http->abort();
    http->close();
    http->deleteLater();
    http = new QHttp(this);
    this->connect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));

    // build/send a request for downloading new ad url list
    qDebug() << "Downloading ad url list at " << adUrlListUrl.toString();
    httpBuffer = new QBuffer(&httpBytes);
    httpBuffer->open(QIODevice::WriteOnly);
    http->setHost(adUrlListUrl.host());
    adUrlListHttpRequestID = http->get(adUrlListUrl.path(), httpBuffer);
}

void AdRefresher::downloadAd_Slot()
{
    //qDebug() << "downloadAd_Slot!";

    downloadTimer->stop();
    this->disconnect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));
    http->abort();
    http->close();
    http->deleteLater();

    if (adUrlListIndex >= (adUrlList.count())) {
        //qDebug() << "all Ads downloaded!";
        this->disconnect(downloadTimer, SIGNAL(timeout()), this, SLOT(downloadAd_Slot()));
        return;
    }

    http = new QHttp(this);
    this->connect(http, SIGNAL(requestFinished(int,bool)),this, SLOT(httpRequestFinished_Slot(int,bool)));

    // build/send a request for downloading new ad

    QStringList sl = QString(adUrlList[adUrlListIndex]).split("::");

    QUrl url(sl[0]);
    //qDebug() << "Downloading ad at " << url.toString();
    adTargetUrl.clear();
    adTargetUrl = sl[1];
    //qDebug() << "Target url is " << adTargetUrl;

    httpBuffer = new QBuffer(&httpBytes);
    httpBuffer->open(QIODevice::WriteOnly);
    http->setHost(url.host());
    adHttpRequestID = http->get(url.path(), httpBuffer);

    //adUrlListIndex++;
}

void AdRefresher::refreshAd_Slot()
{
    //qDebug() << "refreshAd_Slot!";

    Ad ad = adList.at(adIndex);
    emit adRefreshed(ad.img(), ad.targetUrl());
    adIndex++;
    if (adIndex >= adList.count())
        adIndex = 0;
}

void AdRefresher::httpRequestFinished_Slot(int id, bool error)
{
    if (id == adUrlListHttpRequestID) {
        //qDebug() << "Got download finished request for ad url list!";

        if (!error) {
            //qDebug() << "Finished download of advertisement url list";

            // get url/target list
            adUrlList.clear();
            adUrlList << QString(httpBytes).split("\n");
            foreach (QString url, adUrlList) {
                if (url.length() > 0)
                    qDebug() << "element from adUrlList =" << url;
            }
            adUrlListIndex = 0;
            qDebug() << "Ad Url list is ready";
            this->disconnect(downloadTimer, SIGNAL(timeout()), this, SLOT(downloadAdUrlList_Slot()));
            this->connect(downloadTimer, SIGNAL(timeout()), this, SLOT(downloadAd_Slot()));
            adUrlListIsReady = true;
        }
        httpBuffer->close();
        delete httpBuffer;

        // restart timer
        downloadTimer->start(AdRefresher::timerList);
    }
    else if (id == adHttpRequestID) {
        //qDebug() << "Got download finished request for ad!";

        if (!error) {
            //qDebug("Finished download of advertisement");

            // acquiring advertisement image
            QImage img;
            img.loadFromData(httpBytes);
            if (!(img.format() == QImage::Format_Invalid)) {
                Ad ad;
                ad.setImg(img);
                ad.setTargetUrl(adTargetUrl);
                adList << ad;

                if (adList.count() == 1) {
                    //qDebug() << "1st ad downloaded!";
                    this->connect(refreshTimer, SIGNAL(timeout()), this, SLOT(refreshAd_Slot()));
                    adIndex = 0;
                    refreshAd_Slot();
                    refreshTimer->start(timerRefresh);
                }

                adUrlListIndex++;
            }
        }
        httpBuffer->close();
        delete httpBuffer;

        // restart timer, ad resfreshing timer is different
        downloadTimer->start(AdRefresher::timerAd);
    }
}
