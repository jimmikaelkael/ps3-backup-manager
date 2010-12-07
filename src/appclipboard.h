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

#ifndef APPCLIPBOARD_H
#define APPCLIPBOARD_H

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

class AppClipboard
{

public:    
    AppClipboard();
    void setMimeData(QMimeData *mimeData, Qt::DropAction action = Qt::IgnoreAction);
    const QMimeData *mimeData();
    bool hasUrls();
    void clear();
    Qt::DropAction dropAction();

private:
    Qt::DropAction currentDropAction;

};

#endif // APPCLIPBOARD_H
