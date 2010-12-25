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

#include "myfileinfo.h"

MyFileInfo::MyFileInfo(const QFileInfo & fileInfo) :
    QFileInfo(fileInfo),
    privDestinationFilePath("")
{
}

MyFileInfo::MyFileInfo(QString path) :
    QFileInfo(path),
    privDestinationFilePath("")
{
}

QString MyFileInfo::destinationFilePath()
{
    return privDestinationFilePath;
}

void MyFileInfo::setDestinationFilePath(QString filePath)
{
    privDestinationFilePath = filePath;
}

bool MyFileInfo::isFileGreaterThan4GB()
{
    if (this->size() >= 4294967296LL)
        return true;

    return false;
}

// overload
bool MyFileInfo::isFileGreaterThan4GB(QString filePath) const
{
    MyFileInfo fileInfo(filePath);
    return fileInfo.isFileGreaterThan4GB();
}
