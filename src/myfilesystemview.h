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

#ifndef MYFILESYSTEMVIEW_H
#define MYFILESYSTEMVIEW_H

#include <QTreeView>
#include <QAbstractItemModel>
#include <QMenu>
#include <QAction>
#include <QFileInfo>
#include <QUrl>
#include <QMimeData>
#include <QMessageBox>
#include <QDialog>
#include <QImage>
#include <QIcon>
#include <QPixmap>
#include <QDebug>

class AppClipboard;
class MyFileSystemModel;
class OpenUrlThread;
class ProgressDialog;

class MyFileSystemView : public QTreeView
{
    Q_OBJECT

public:
    MyFileSystemView(AppClipboard *clipboard, QWidget *parent = 0);
    ~MyFileSystemView();
    QAbstractItemModel *fileSystemModel();
    int activeDialogsCount();

signals:
    void selectionChanged(QStringList pathList);

public slots:
    void pupCleanerAllSubfolders_Slot();
    void split_Slot();

private slots:
    void contextMenu_Slot(const QPoint& point);
    void open_Slot();
    void cut_Slot();
    void copy_Slot();
    void paste_Slot();
    void newDir_Slot();
    void syncFoldersAtoB_Slot();
    void syncFoldersBtoA_Slot();
    void pupCleanerOnlyThisFolder_Slot();
    void delete_Slot();
    void rename_Slot();
    void fsModelStateChanged_Slot();
    void progressDialogDone_Slot(ProgressDialog *dialog, bool aborted, bool error);

private:
    void setContextMenu();
    void setFileSystemModel();
    void connectContextMenuSignals();
    int getNumSelectedRows();
    QFileInfo getSelectedRowFileInfo();
    QModelIndex getSelectedRowModelIndex();
    QModelIndexList getSelectedRowsModelIndexes();
    void requestGameListRefresh();

    AppClipboard *appClipboard;
    MyFileSystemModel *fsModel;
    QMenu *contextMenu;
    QAction *contextMenu_openAction;
    QAction *contextMenu_cutAction, *contextMenu_copyAction, *contextMenu_pasteAction;
    QAction *contextMenu_newDirAction;
    QAction *contextMenu_deleteAction, *contextMenu_renameAction;
    QMenu *syncSubMenu;
    QAction *syncSubMenu_aAction, *syncSubMenu_bAction;
    QMenu *pupCleanerSubMenu;
    QAction *pupCleanerSubMenu_onlyThisFolderAction, *pupCleanerSubMenu_allSubfoldersAction;
    QAction *contextMenu_splitAction;
    QFileInfo syncFolderA, syncFolderB;
    QList<QDialog *> activeDialogsList;

    OpenUrlThread *openUrlThread;

protected slots:
    void rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end); // reimp !
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected); // reimp!

};

#endif // MYFILESYSTEMVIEW_H
