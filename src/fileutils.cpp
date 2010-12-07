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

#include "fileutils.h"

FileUtils::FileUtils(QObject *parent) :
    QObject(parent)
{
}

QString FileUtils::humanReadableFilesize(qint64 size) const
{
    QString sizeStr = QString::number(size);
    int level = 0;
    while (size >= 1024) {
        if (level == 4)
                break;
        sizeStr = QString::number(size/1024);
        if (((size/1024) < 1024) && ((size % 1024) > 100)) {
            sizeStr.append(tr("."));
            sizeStr.append(QString::number((size % 1024)+50).left(1));
        }
        size = size / 1024;
        level++;
    }
    sizeStr.append(" ");
    switch (level) {
        case 0:
            sizeStr.append(tr("B"));
            break;
        case 1:
            sizeStr.append(tr("KB"));
            break;
        case 2:
            sizeStr.append(tr("MB"));
            break;
        case 3:
            sizeStr.append(tr("GB"));
            break;
        case 4:
            sizeStr.append(tr("TB"));
            break;
    }

    return sizeStr;
}

void FileUtils::setFileWritable(QString filePath) const
{
    QFile file(filePath);
    if (!file.isWritable()) {
        QFile::Permissions currentPermissions = file.permissions();
        file.setPermissions(currentPermissions | QFile::WriteOwner);
    }
}

#if defined (Q_WS_WIN) // Windows system
#include <windows.h>
#else
#if defined (Q_WS_X11) // Linux system
#include <sys/vfs.h>
#include <sys/stat.h>
#endif
#endif

bool FileUtils::getFreeTotalSpace(const QString dirPath, qint64 *total, qint64 *free) const
{
#if defined (Q_WS_WIN) // Windows system

        QString curDir = QDir::current().absolutePath();
        QDir::setCurrent(dirPath);

        ULARGE_INTEGER lFree,lTotal;
        bool res = ::GetDiskFreeSpaceExA(0, &lFree, &lTotal, NULL);
        if (!res)
            return false;

        QDir::setCurrent(curDir);

        *free = static_cast<qint64>(static_cast<__int64>(lFree.QuadPart)) / 1024;
        *total = static_cast<qint64>(static_cast<__int64>(lTotal.QuadPart)) / 1024;

#else
#if defined (Q_WS_X11) // Linux system

        struct stat stst;
        struct statfs stfs;

        if (::stat(dirPath.toLocal8Bit(), &stst) == -1)
            return false;
        if (::statfs(dirPath.toLocal8Bit(), &stfs) == -1)
            return false;

        *free = stfs.f_bavail * (stst.st_blksize / 1024);
        *total = stfs.f_blocks * (stst.st_blksize / 1024);
#endif
#endif

        return true;
}
