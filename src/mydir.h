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

#ifndef MYDIR_H
#define MYDIR_H

#include <QDir>
#include <QFile>
#include <QDebug>

class MyDir : public QDir
{

public:
    MyDir() { }
    MyDir(const QDir & dir);
    QStringList listDirs(QString path);
    QStringList listDirs();
    qint64 getTotalSize(QString path);
    qint64 getTotalSize();

};

#endif // MYDIR_H
