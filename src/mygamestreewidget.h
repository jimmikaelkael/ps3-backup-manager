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

#ifndef MYGAMESTREEWIDGET_H
#define MYGAMESTREEWIDGET_H

#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMimeData>
#include <QUrl>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QIcon>
#include <QPixmap>
#include <QHeaderView>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QDialog>
#include <QFileDialog>
#include <QDebug>

class AppClipboard;
class GameList;
class GameItem;
class GameListThread;
class CoverCacheThread;
class MyFileSystemModel;
class ProgressDialog;

class MyGamesTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    MyGamesTreeWidget(AppClipboard *clipboard, GameList* list, \
                      MyFileSystemModel *model, \
                      QString coverDownloadBase, QString coverCacheDir, \
                      QWidget *parent = 0);
    ~MyGamesTreeWidget();
    void setCoverCachingEnabled(bool enabled);
    void refreshItem(GameItem *gameItem);
    int activeDialogsCount();

public slots:
    void startCoverCaching_Slot();
    void abortCoverCaching_Slot();
    void refreshList(QStringList pathList);
    void refreshList(); // overload
    void exportText_Slot();
    void exportCSV_Slot();

signals:
    void scanningForGames();
    void gameListLoaded();
    void gameListEmpty();
    void gameListItemUpdated(GameItem *item);

private slots:
    void itemSelectionChanged_Slot();
    void gameListThreadScanning_Slot();
    void gameListThreadFinished_Slot(bool aborted);
    void headerSectionClicked_Slot(int logicalIndex);
    void contextMenu_Slot(const QPoint& point);
    void cut_Slot();
    void copy_Slot();
    void syncBackupsA_Slot();
    void syncBackupsB_Slot();
    void delete_Slot();
    void edit_Slot();
    void progressDialogDone_Slot(ProgressDialog *dialog, bool aborted, bool error);

private:
    AppClipboard *appClipboard;
    GameList *gameList;
    MyFileSystemModel *fsModel;
    GameListThread *gameListThread;
    CoverCacheThread *coverCacheThread;
    bool useCoverCaching;
    QMenu *contextMenu;
    QAction *contextMenu_cutAction, *contextMenu_copyAction;
    QAction *contextMenu_deleteAction, *contextMenu_editAction;
    QMenu *syncSubMenu;
    QAction *syncSubMenu_aAction, *syncSubMenu_bAction;
    QMenu *exportSubMenu;
    QAction *exportSubMenu_textAction, *exportSubMenu_csvAction;
    QFileInfo syncFolderA, syncFolderB;
    QList<QDialog *> activeDialogsList;

    void setContextMenu();
    void connectContextMenuSignals();

protected:
    // reimp!
    QMimeData *mimeData(const QList<QTreeWidgetItem *> items) const;
    QStringList mimeTypes() const;

};

#endif // MYGAMESTREEWIDGET_H
