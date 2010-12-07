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

#include "appclipboard.h"

AppClipboard::AppClipboard() :
    currentDropAction(Qt::IgnoreAction)
{
}

void AppClipboard::setMimeData(QMimeData *mimeData, Qt::DropAction action)
{   // set clipboard's current mimeData
    QClipboard *cb = QApplication::clipboard();
    cb->clear(QClipboard::Clipboard);
    cb->setMimeData(mimeData, QClipboard::Clipboard);
    currentDropAction = action;
}

const QMimeData *AppClipboard::mimeData()
{   // return clipboard's current mimeData
    QClipboard *cb = QApplication::clipboard();
    return cb->mimeData(QClipboard::Clipboard);
}

bool AppClipboard::hasUrls()
{ // check clipboard mimeData have urls
    QClipboard *cb = QApplication::clipboard();
    return cb->mimeData(QClipboard::Clipboard)->hasUrls();
}

void AppClipboard::clear()
{   // clears clipboard's mimeData
    QClipboard *cb = QApplication::clipboard();
    cb->clear(QClipboard::Clipboard);
    currentDropAction = Qt::IgnoreAction;
}

Qt::DropAction AppClipboard::dropAction()
{   // returns clipboard's drop action
    return currentDropAction;
}
