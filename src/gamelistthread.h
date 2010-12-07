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

#ifndef GAMELISTTHREAD_H
#define GAMELISTTHREAD_H

#include <QObject>
#include <QThread>
#include <QFile>
#include <QImage>
#include <QMutex>
#include <QWaitCondition>
#include <QDebug>

#include "mydir.h"
#include "fileutils.h"
#include "gamelist.h"
#include "gameitem.h"

class GameListThread : public QThread
{
    Q_OBJECT

public:

    GameListThread(GameList *list, QObject *parent=0) :
        QThread(parent),
        gameList(list),
        aborted(false)
    {
        this->connect(this, SIGNAL(finished()), this, SLOT(threadFinished_Slot()));
    }

    void setPathList(QStringList list)
    {
        mutex.lock();
        pathList.clear();
        pathList = list;
        mutex.unlock();
    }

    void abort() {
        mutex.lock();
        aborted = true;
        mutex.unlock();
    }

    // reimp!
    void run()
    {
        mutex.lock();
        aborted = false;
        itemList.clear();
        mutex.unlock();

        foreach (QString path, pathList) {

            if (checkAborted())
                break;

            emit scanning();

            if (!(path.length() > 0))
                continue;

            //qDebug() << "gamelistthread - path=" << path;
            QString ps3GameDir = "/PS3_GAME";
            QString ps3UpdateDir = "/PS3_UPDATE";
            QString paramSFO = ps3GameDir + "/PARAM.SFO";
            if (path.contains(ps3GameDir)) {
                QString gamePath = path.left(path.lastIndexOf(ps3GameDir));
                if (QFile::exists(gamePath + paramSFO))
                    addGame(gamePath);
            }
            else if (path.contains(ps3UpdateDir)) {
                QString gamePath = path.left(path.lastIndexOf(ps3UpdateDir));
                if (QFile::exists(gamePath + paramSFO))
                    addGame(gamePath);
            }
            else if (QFile::exists(path + paramSFO)) {
                addGame(path);
            }
            else {
                MyDir dir(path);
                QStringList list = dir.listDirs();

                foreach(QString dirName, list) {

                    if (checkAborted())
                        break;

                    emit scanning();

                    QString gamePath = path + "/" + dirName;
                    if (QFile::exists(gamePath + paramSFO))
                        addGame(gamePath);
                }
            }
        }

        if (!checkAborted())
            gameList->refresh(itemList);

        this->quit();
    }

signals:
    void scanning();
    void finished(bool aborted);

private slots:

    void threadFinished_Slot()
    {
        emit finished(checkAborted());
    }

private:
    GameList *gameList;
    bool aborted;
    QMutex mutex;
    QList<GameItem *> itemList;
    QStringList pathList;

    bool checkAborted()
    {
        mutex.lock();
        bool res = aborted;
        mutex.unlock();

        return res;
    }

    void addGame(QString gamePath)
    {
        //qDebug() << "addGame =" << gamePath;

        foreach (GameItem *item, itemList) {
            // already in the list...
            if (item->path() == gamePath)
                return;
        }

        GameItem *item;
        if (gameList->exists(gamePath)) {
            item = gameList->itemByPath(gamePath);
            item->update();
        }
        else
            item = new GameItem(gamePath);

        if (item->isValid()) {
            //qDebug() << "appending item name =" << item->title();
            itemList.append(item);
        }
        else {
            //qDebug() << "invalid item!";
            delete item;
        }
    }
};

#endif // GAMELISTTHREAD_H
