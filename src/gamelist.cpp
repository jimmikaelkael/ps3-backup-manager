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

#include "gamelist.h"

GameList::GameList()
{
}

GameList::~GameList()
{
}

void GameList::addItem(GameItem *item)
{
    itemList << item;
}

void GameList::removeItem(GameItem *item)
{
    itemList.removeOne(item);
    delete item;
}

void GameList::clear()
{
    itemList.clear();
}

int GameList::count()
{
    return itemList.count();
}

int GameList::countEnabled()
{
    int cnt = 0;

    foreach (GameItem *listedItem, itemList) {
        if (listedItem->isEnabled())
            cnt++;
    }
    return cnt;
}

bool GameList::exists(QString gamePath)
{
    foreach(GameItem *item, itemList) {
        if (item->path() == gamePath) {
            //qDebug() << "game" << gamePath << "already listed!";
            return true;
        }
    }
    //qDebug() << "game" << gamePath << "not listed!";

    return false;
}

void GameList::refresh(QList<GameItem *> items)
{
    foreach(GameItem *listedItem, itemList) {
        listedItem->setEnabled(false);
        if (!(listedItem->pathExists())) {
            removeItem(listedItem);
            continue;
        }
        foreach(GameItem *item, items) {
            if (listedItem->path() == item->path()) {
                listedItem->setEnabled(true);
                break;
            }
        }
    }

    foreach(GameItem *item, items) {
        bool listed = false;
        foreach(GameItem *listedItem, itemList) {
            if (listedItem->path() == item->path()) {
                listed = true;
                break;
            }
        }
        if (!listed) {
            //qDebug() << "adding title=" << item->title();
            itemList << item;
        }
    }
}

GameItem *GameList::itemAt(int index)
{
    GameItem *p = itemList[index];

    return p;
}

GameItem *GameList::itemByPath(QString path)
{
    GameItem *p = NULL;

    for (int i = 0; i < itemList.count(); i++) {
        p = itemList[i];
        if (p->path() == path)
            break;
        else
            p = NULL;
    }

    return p;
}

QList<GameItem *> GameList::list()
{
    QList<GameItem *> list;
    for (int i = 0; i < itemList.count(); i++) {
        GameItem *p = itemList[i];
        if (p->isEnabled())
            list << p;
    }

    return list;
}
