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

#include "mydir.h"

MyDir::MyDir(const QDir & dir) :
    QDir(dir)
{
}

QStringList MyDir::listDirs(QString path)
{
    QStringList list;

    // check source directory existence
    QDir sourceDir(path);
    if (!sourceDir.exists())
        return list;

    list = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    return list;
}

// overload
QStringList MyDir::listDirs()
{
    return listDirs(this->path());
}

qint64 MyDir::getTotalSize(QString path)
{
    // check source directory existence
    QDir sourceDir(path);
    if (!sourceDir.exists())
        return 0;

    qint64 totalSize = 0;

    // get all files size
    QStringList list = sourceDir.entryList(QDir::Files);
    foreach (QString fileName, list) {
        QFile file(path + QDir::separator() + fileName);
        totalSize += file.size();
    }

    // scan all folders
    list.clear();
    list = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot);
    foreach (QString dirName, list) {
        totalSize += getTotalSize(path + QDir::separator() + dirName);
        totalSize += 4096;
    }

    return totalSize;
}

// overload
qint64 MyDir::getTotalSize()
{
    return getTotalSize(this->path());
}
