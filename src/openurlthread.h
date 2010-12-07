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

#ifndef OPENURLTHREAD_H
#define OPENURLTHREAD_H

#include <QObject>
#include <QThread>
#include <QUrl>
#include <QDesktopServices>

class OpenUrlThread : public QThread
{
    Q_OBJECT

public:
    OpenUrlThread(QObject *parent=0) :
        QThread(parent)
    {
    }
    OpenUrlThread(QUrl url, QObject *parent=0) :
        QThread(parent),
        targetUrl(url)
    {
    }

    void setTargetUrl(QUrl url)
    {
        targetUrl = url;
    }

    // reimp!
    void run()
    {
        QDesktopServices::openUrl(targetUrl);
        this->quit();
    }

private:
    QUrl targetUrl;

};

#endif // OPENURLTHREAD_H
