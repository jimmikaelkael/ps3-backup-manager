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

#ifndef GAMEITEM_H
#define GAMEITEM_H

#include <QString>
#include <QFile>
#include <QDir>
#include <QImage>
#include <QIcon>
#include <QPixmap>
#include <QImage>
#include <QDebug>

#include "sfoparamlist.h"
#include "mydir.h"
#include "fileutils.h"

class GameItem
{

public:
    GameItem();
    GameItem(QString gamePath);
    ~GameItem();
    void setEnabled(bool enabled);
    bool isEnabled();
    bool isValid();
    QString path();
    QString psfPath();
    QString icon0Path();
    QString title();
    QString titleID();
    QString totalSize();
    QString version();
    QString category();
    QIcon icon();
    QImage iconImage();
    void update();
    bool pathExists();
    bool parsePARAMSFO(QString psfPath, SfoParamList *paramList);
    bool savePARAMSFO(QString psfPath, SfoParamList *paramList);

private:
    quint32 read_le_uint32(unsigned char *buf);
    void append_le_uint32(unsigned char *buf, quint32 val);
    void updateGameInfo(QString gamePath, SfoParamList *paramList);

    bool isItemValid;
    bool itemEnabled;
    QString itemPath;
    QString itemPsfPath;
    QString itemIcon0Path;
    QString itemTitle;
    QString itemTitleID;
    QString itemTotalSize;
    QString itemVersion;
    QString itemCategory;
    QImage itemIcon;

};

#endif // GAMEITEM_H
