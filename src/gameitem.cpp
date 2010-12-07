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

#include "gameitem.h"

GameItem::GameItem()
{
}

GameItem::GameItem(QString gamePath) :
    itemEnabled(true)
{
    QString psfPath = gamePath + "/PS3_GAME/PARAM.SFO";
    SfoParamList sfoParamList;

    isItemValid = parsePARAMSFO(psfPath, &sfoParamList);
    if (isItemValid)
        updateGameInfo(gamePath, &sfoParamList);

    //qDebug() << "item created! path=" << gamePath;
}

GameItem::~GameItem()
{
    //qDebug() << "item deletion!";
}

void GameItem::setEnabled(bool enabled)
{
    itemEnabled = enabled;
}

bool GameItem::isEnabled()
{
    return itemEnabled;
}

bool GameItem::isValid()
{
    return isItemValid;
}

QString GameItem::path()
{
    return itemPath;
}

QString GameItem::psfPath()
{
    return itemPsfPath;
}

QString GameItem::icon0Path()
{
    return itemIcon0Path;
}

QString GameItem::title()
{
    return itemTitle;
}

QString GameItem::titleID()
{
    return itemTitleID;
}

QString GameItem::totalSize()
{
    return itemTotalSize;
}

QString GameItem::version()
{
    return itemVersion;
}

QString GameItem::category()
{
    return itemCategory;
}

QIcon GameItem::icon()
{
    return QIcon(QPixmap::fromImage(itemIcon));
}

QImage GameItem::iconImage()
{
    return itemIcon;
}

void GameItem::update()
{
    QString psfPath = itemPath + "/PS3_GAME/PARAM.SFO";
    SfoParamList sfoParamList;

    isItemValid = parsePARAMSFO(psfPath, &sfoParamList);
    if (isItemValid)
        updateGameInfo(itemPath, &sfoParamList);
}

bool GameItem::pathExists()
{
    return QDir().exists(itemPath);
}

bool GameItem::parsePARAMSFO(QString psfPath, SfoParamList *paramList)
{
    QFile file(psfPath);
    if (!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.size();
    unsigned char *data = new unsigned char[fileSize];
    file.read((char *)data, fileSize);
    file.close();

    unsigned char *hdr = (unsigned char *)data;
    if (!((hdr[0] == 0) && (hdr[1] == 'P') && (hdr[2] == 'S') && (hdr[3] == 'F')))
        return false;

    quint32 dirOffset = read_le_uint32(&hdr[8]);
    quint32 contentOffset = read_le_uint32(&hdr[12]);
    int dirSize = contentOffset - dirOffset;

    paramList->clear();

    unsigned char *dir = (unsigned char *)&data[dirOffset];
    unsigned char *content = (unsigned char *)&data[contentOffset];

    QString dirName = QString((char *)dir);
    int offset = 0;
    while ((offset < dirSize) && (!dirName.isEmpty())) {
        hdr += 16;

        int contentLen = read_le_uint32(&hdr[8]);
        int contentMaxLen = read_le_uint32(&hdr[12]);

        //qDebug() << dirName << "=" << QString().fromUtf8((char *)content);

        SfoParamItem item(dirName, content, contentLen, contentMaxLen);
        paramList->addItem(item);

        dir += dirName.length()+1;
        offset += dirName.length()+1;
        content += contentMaxLen;
        dirName.clear();
        dirName.append((char *)dir);
    }

    delete[] data;

    return true;
}

bool GameItem::savePARAMSFO(QString psfPath, SfoParamList *paramList)
{
    QFile file(psfPath);
    if (!file.open(QIODevice::ReadWrite))
        return false;

    qint64 fileSize = file.size();
    unsigned char *data = new unsigned char[fileSize];
    file.read((char *)data, fileSize);

    unsigned char *hdr = (unsigned char *)data;
    if (!((hdr[0] == 0) && (hdr[1] == 'P') && (hdr[2] == 'S') && (hdr[3] == 'F'))) {
        file.close();
        return false;
    }

    quint32 dirOffset = read_le_uint32(&hdr[8]);
    quint32 contentOffset = read_le_uint32(&hdr[12]);
    qint64 fileOffset = contentOffset;
    qint64 hdrOffset = 0;
    int dirSize = contentOffset - dirOffset;

    unsigned char *dir = (unsigned char *)&data[dirOffset];
    unsigned char *content = (unsigned char *)&data[contentOffset];

    QString dirName = QString((char *)dir);
    int offset = 0;
    while ((offset < dirSize) && (!dirName.isEmpty())) {
        hdr += 16;
        hdrOffset += 16;

        int contentMaxLen = read_le_uint32(&hdr[12]);

        //qDebug() << dirName << "=" << QString().fromUtf8((char *)content);
        for (int i=0; i<paramList->count(); i++) {
            SfoParamItem *item = paramList->at(i);
            if (item->name() == dirName) {
                file.seek(fileOffset);
                file.write((char *)item->value(), item->valueLength());
                unsigned char data[4];
                append_le_uint32(data, item->valueLength());
                file.seek(hdrOffset+8);
                file.write((char *)data, sizeof(data));
                //qDebug() << "should write" << item->name() << "at file offet=" << fileOffset;
            }
        }

        dir += dirName.length()+1;
        offset += dirName.length()+1;
        content += contentMaxLen;
        fileOffset += contentMaxLen;
        dirName.clear();
        dirName.append((char *)dir);
    }

    delete[] data;

    file.close();

    updateGameInfo(itemPath, paramList);

    return true;
}

//
// private
//

quint32 GameItem::read_le_uint32(unsigned char *buf)
{
    quint32 val;

    val = buf[0];
    val |= (buf[1] << 8);
    val |= (buf[2] << 16);
    val |= (buf[3] << 24);

    return val;
}

void GameItem::append_le_uint32(unsigned char *buf, quint32 val)
{
    buf[0] = val & 0xff;
    buf[1] = (val >> 8) & 0xff;
    buf[2] = (val >> 16) & 0xff;
    buf[3] = (val >> 24) & 0xff;
}

void GameItem::updateGameInfo(QString gamePath, SfoParamList *paramList)
{
    QString psfPath = gamePath + "/PS3_GAME/PARAM.SFO";
    QString icon0Path = gamePath + "/PS3_GAME/ICON0.PNG";

    for (int i=0; i<paramList->count(); i++) {
        SfoParamItem *item = paramList->at(i);
        if (item->name() == "TITLE")
            itemTitle = QString().fromUtf8((char *)item->value());
        else if (item->name() == "TITLE_ID")
            itemTitleID = QString().fromUtf8((char *)item->value());
        else if (item->name() == "VERSION")
            itemVersion = QString().fromUtf8((char *)item->value());
        else if (item->name() == "CATEGORY") {
            itemCategory = QString().fromUtf8((char *)item->value());
        }
    }

    itemPath = gamePath;
    itemPsfPath = psfPath;
    itemIcon0Path = icon0Path;
    itemIcon.load(icon0Path);
    itemIcon = itemIcon.scaledToHeight(32);

    MyDir gameDir(gamePath);
    itemTotalSize = FileUtils().humanReadableFilesize(gameDir.getTotalSize());
}
