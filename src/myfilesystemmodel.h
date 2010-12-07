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

#ifndef MYFILESYSTEMMODEL_H
#define MYFILESYSTEMMODEL_H

#include <QFileSystemModel>
#include <QTreeView>
#include <QMimeData>
#include <QModelIndex>
#include <QUrl>
#include <QFileInfo>
#include <QDialog>
#include <QDebug>

#include "myfileinfo.h"

class ProgressDialog;

class MyFileSystemModel : public QFileSystemModel
{
    Q_OBJECT

public:
    MyFileSystemModel(QObject *parent = 0);
   QModelIndex createNewDirectory(const QModelIndex &parent);
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    Qt::DropActions supportedDropActions() const;
    int activeDialogsCount();

signals:
    void stateChanged();

private slots:
    void progressDialogDone_Slot(ProgressDialog *dialog, bool aborted, bool error);

private:
    QObject *modelParent;
    QList<QDialog *> activeDialogsList;

    bool dropMimeDataLocal(QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QString getAlternativeFileName(MyFileInfo fileInfo);
    QString getNewDirectoryName(QString path);

};

#endif // MYFILESYSTEMMODEL_H
