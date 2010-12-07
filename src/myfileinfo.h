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

#ifndef MYFILEINFO_H
#define MYFILEINFO_H

#include <QFileInfo>
#include <QString>

class MyFileInfo : public QFileInfo
{

public:
    MyFileInfo(const QFileInfo & fileInfo);
    MyFileInfo(QString path);
    QString destinationFilePath();
    void setDestinationFilePath(QString filePath);
    bool isFileGreaterThan4GB();
    bool isFileGreaterThan4GB(QString filePath) const; // overload

private:
    QString privDestinationFilePath;

};

typedef QList<MyFileInfo> MyFileInfoList;

#endif // MYFILEINFO_H
