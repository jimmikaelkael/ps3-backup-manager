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

#ifndef FILEUTILS_H
#define FILEUTILS_H

#include <QObject>
#include <QFile>
#include <QDir>
#include <QString>

class FileUtils : public QObject
{
    Q_OBJECT

public:
    FileUtils(QObject *parent = 0);
    QString humanReadableFilesize(qint64 size) const;
    void setFileWritable(QString filePath) const;
    bool getFreeTotalSpace(const QString dirPath, qint64 *total, qint64 *free) const;
};

#endif // FILEUTILS_H
