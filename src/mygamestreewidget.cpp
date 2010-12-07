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

#include "mygamestreewidget.h"

#include "appclipboard.h"
#include "mydir.h"
#include "gamelist.h"
#include "gameitem.h"
#include "gamelistthread.h"
#include "covercachethread.h"
#include "myfilesystemmodel.h"

#include "progressdialog.h"
#include "fileconflictdialog.h"
#include "sfoeditordialog.h"

MyGamesTreeWidget::MyGamesTreeWidget(AppClipboard *clipboard, GameList* list, \
                                     MyFileSystemModel *model, \
                                     QString coverDownloadBase, QString coverCacheDir, \
                                     QWidget *parent) :
    QTreeWidget(parent),
    appClipboard(clipboard),
    gameList(list),
    fsModel(model),
    gameListThread(new GameListThread(gameList, this)),
    coverCacheThread(NULL),
    useCoverCaching(true)
{
    // setting some object properties
    this->setAcceptDrops(false);
    this->setDefaultDropAction(Qt::CopyAction);
    this->setDragDropMode(QTreeWidget::DragOnly);
    this->setDragEnabled(true);
    this->setHeaderLabels(QStringList() << tr("Name") << tr("Size on disk"));
    this->headerItem()->setTextAlignment(1, Qt::AlignRight);
    this->setDropIndicatorShown(false);
    this->setRootIsDecorated(false);
    this->setSelectionMode(QTreeWidget::ExtendedSelection);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->header()->setClickable(true);
    this->header()->setSortIndicatorShown(true);
    this->header()->setSortIndicator(0, Qt::AscendingOrder);
    this->setColumnWidth(0, 360);

    // connect item selection changed signal
    this->connect(this, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged_Slot()));

    this->connect(this->header(), SIGNAL(sectionClicked(int)), this, SLOT(headerSectionClicked_Slot(int)));

    this->connect(gameListThread, SIGNAL(scanning()), this, SLOT(gameListThreadScanning_Slot()));
    this->connect(gameListThread, SIGNAL(finished(bool)), this, SLOT(gameListThreadFinished_Slot(bool)));

    setContextMenu();
    connectContextMenuSignals();

    // setting coverCacheThread object
    coverCacheThread = new CoverCacheThread(coverDownloadBase, coverCacheDir, this);
}

MyGamesTreeWidget::~MyGamesTreeWidget()
{
    delete contextMenu_editAction;
    delete contextMenu_deleteAction;
    delete exportSubMenu_textAction;
    delete exportSubMenu_csvAction;
    delete contextMenu_copyAction;
    delete contextMenu_cutAction;
    delete exportSubMenu;
    delete contextMenu;
    gameListThread->abort();
    gameListThread->wait();
    coverCacheThread->abort();
    coverCacheThread->wait();
    delete gameListThread;
    delete coverCacheThread;
}

void MyGamesTreeWidget::setCoverCachingEnabled(bool enabled)
{
    useCoverCaching = enabled;
}

void MyGamesTreeWidget::refreshItem(GameItem *gameItem)
{
    //qDebug() << "refresh game request! title=" << gameItem->title();
    //qDebug() << "refresh game request! path=" << gameItem->path();
    //qDebug() << "refresh game request! size=" << gameItem->totalSize();

    for (int i=0; i<this->topLevelItemCount(); i++) {
        QTreeWidgetItem *item = this->topLevelItem(i);
        if (item->data(0, Qt::UserRole).toString() == gameItem->path()) {
            item->setText(0, gameItem->title());
            item->setText(1, gameItem->totalSize());
        }
    }
}

int MyGamesTreeWidget::activeDialogsCount()
{
    return activeDialogsList.count();
}

//
// public slots
//

void MyGamesTreeWidget::startCoverCaching_Slot()
{
    //qDebug() << "startCoverCaching_Slot";
    QStringList idList;
    foreach (GameItem *gameItem, gameList->list()) {
        idList << gameItem->titleID();
    }

    if (useCoverCaching) {
        coverCacheThread->setIDList(idList);
        coverCacheThread->start(CoverCacheThread::IdlePriority);
    }
}

void MyGamesTreeWidget::abortCoverCaching_Slot()
{
    coverCacheThread->abort();
}

void MyGamesTreeWidget::refreshList(QStringList pathList)
{
    //qDebug() << "refresh game list request! pathList=" << pathList;

    gameListThread->abort();
    gameListThread->wait();

    gameListThread->setPathList(pathList);
    gameListThread->start();
}

// overload
void MyGamesTreeWidget::refreshList()
{
    //qDebug() << "refresh game list request!

    gameListThread->abort();
    gameListThread->wait();

    gameListThread->start();
}


//
// private slots
//

void MyGamesTreeWidget::itemSelectionChanged_Slot()
{
    //qDebug() << "GamesTreeWidget: item selection changed!";

    if (this->selectedItems().count() == 1) {
        GameItem *item = gameList->itemByPath(this->selectedItems().at(0)->data(0, Qt::UserRole).toString());
        emit gameListItemUpdated(item);
    }
    else
        emit gameListItemUpdated(NULL);
}

void MyGamesTreeWidget::gameListThreadScanning_Slot()
{
    emit scanningForGames();
}

void MyGamesTreeWidget::gameListThreadFinished_Slot(bool aborted)
{
    //qDebug() << "gameListThread finished! aborted=" << aborted;

    if (!aborted) {

        this->clear();

        QStringList idList;
        foreach (GameItem *gameItem, gameList->list()) {
            //qDebug() << "listing title" << gameItem->title();
            QTreeWidgetItem *item = new QTreeWidgetItem(this);
            item->setText(0, gameItem->title());
            item->setIcon(0, gameItem->icon());
            item->setText(1, gameItem->totalSize());
            item->setTextAlignment(1, Qt::AlignRight);
            item->setData(0, Qt::UserRole, gameItem->path());

            idList << gameItem->titleID();
            this->addTopLevelItem(item);
        }

        int countEnabled = gameList->countEnabled();
        //qDebug() << "countEnabled=" << countEnabled;
        if (countEnabled > 0) {
            this->sortItems(0, this->header()->sortIndicatorOrder());
            emit gameListLoaded();
        }
        else
            emit gameListEmpty();

        if (countEnabled == 1) {
            GameItem *item = gameList->list().at(0);
            //qDebug() << "1 item selected name=" << item->title();
            emit gameListItemUpdated(item);
        }
        else
            emit gameListItemUpdated(NULL);
    }

    startCoverCaching_Slot();
}

void MyGamesTreeWidget::headerSectionClicked_Slot(int)
{
    this->sortItems(0, this->header()->sortIndicatorOrder());
}

void MyGamesTreeWidget::contextMenu_Slot(const QPoint& point)
{
    // draw a Context Menu at coordinates point
    qDebug() << "Context menu requested";

    contextMenu_cutAction->setDisabled(true);
    contextMenu_copyAction->setDisabled(true);
    syncSubMenu->setDisabled(true);
    exportSubMenu->setDisabled(true);
    contextMenu_deleteAction->setDisabled(true);
    contextMenu_editAction->setDisabled(true);

    int numSelectedItems = this->selectedItems().count();
    if (numSelectedItems > 0) {
        contextMenu_cutAction->setEnabled(true);
        contextMenu_copyAction->setEnabled(true);
        contextMenu_deleteAction->setEnabled(true);
    }
    if (numSelectedItems == 1) {
        contextMenu_editAction->setEnabled(true);
    }
    else if (numSelectedItems == 2) {
        // if 2 items selected and they are folder sync submenu is enabled
        QList<QTreeWidgetItem *> list = this->selectedItems();
        QFileInfo fileInfoA(list.at(0)->data(0, Qt::UserRole).toString());
        QFileInfo fileInfoB(list.at(1)->data(0, Qt::UserRole).toString());
        if ((fileInfoA.isDir()) && (fileInfoB.isDir()) \
            && (!(fileInfoA.isSymLink())) && (!(fileInfoB.isSymLink()))) {
            syncFolderA = fileInfoA;
            syncFolderB = fileInfoB;
            syncSubMenu_aAction->setText(fileInfoA.fileName() + tr(" -> ") + fileInfoB.fileName());
            syncSubMenu_bAction->setText(fileInfoB.fileName() + tr(" -> ") + fileInfoA.fileName());
            syncSubMenu->setEnabled(true);
        }
    }

    if (this->topLevelItemCount() > 0)
        exportSubMenu->setEnabled(true);

    if (!(!contextMenu_cutAction->isEnabled() && !contextMenu_copyAction->isEnabled() \
          && !exportSubMenu->isEnabled() && !contextMenu_deleteAction->isEnabled() \
          && !contextMenu_editAction->isEnabled())) {
        // exec Context Menu
        contextMenu->exec(this->mapToGlobal(point));
    }
}

void MyGamesTreeWidget::cut_Slot()
{
    //qDebug() << "requested cut!";

    // fill clipboard mimedata with mimedata for selected rows
    appClipboard->setMimeData(mimeData(this->selectedItems()), Qt::MoveAction);
}

void MyGamesTreeWidget::copy_Slot()
{
    //qDebug() << "requested copy!";

    // fill clipboard mimedata with mimedata for selected rows
    appClipboard->setMimeData(mimeData(this->selectedItems()), Qt::CopyAction);
}

void MyGamesTreeWidget::syncBackupsA_Slot()
{
    qDebug() << "requested sync backups A to B";
    qDebug() << "A=" << syncFolderA.absoluteFilePath();
    qDebug() << "B=" << syncFolderB.absoluteFilePath();

    FileConflictDialog *dialog = new FileConflictDialog(FileConflictDialog::syncConfirmDialog, \
                                                        syncFolderA.absoluteFilePath(), \
                                                        syncFolderB.absoluteFilePath(), \
                                                        this);
    int res = dialog->exec();
    delete dialog;
    if (!(res == FileConflictDialog::cancel)) {
        ProgressDialog *dialog = new ProgressDialog(ProgressDialog::WithProgressBar, this);
        this->connect(dialog, SIGNAL(progressDialogDone(ProgressDialog*,bool,bool)), this, SLOT(progressDialogDone_Slot(ProgressDialog*,bool,bool)));
        activeDialogsList << dialog;
        dialog->sync(fsModel, syncFolderA.absoluteFilePath(), syncFolderB.absoluteFilePath());
    }
}

void MyGamesTreeWidget::syncBackupsB_Slot()
{
    qDebug() << "requested sync folders B to A";
    qDebug() << "B=" << syncFolderB.absoluteFilePath();
    qDebug() << "A=" << syncFolderA.absoluteFilePath();

    FileConflictDialog *dialog = new FileConflictDialog(FileConflictDialog::syncConfirmDialog, \
                                                        syncFolderB.absoluteFilePath(), \
                                                        syncFolderA.absoluteFilePath(), \
                                                        this);
    int res = dialog->exec();
    delete dialog;
    if (!(res == FileConflictDialog::cancel)) {
        ProgressDialog *dialog = new ProgressDialog(ProgressDialog::WithProgressBar, this);
        this->connect(dialog, SIGNAL(progressDialogDone(ProgressDialog*,bool,bool)), this, SLOT(progressDialogDone_Slot(ProgressDialog*,bool,bool)));
        activeDialogsList << dialog;
        dialog->sync(fsModel, syncFolderB.absoluteFilePath(), syncFolderA.absoluteFilePath());
    }
}

void MyGamesTreeWidget::exportText_Slot()
{
    if (!(this->topLevelItemCount() > 0))
        return;

    qDebug() << "requested export text list!";

    // display a save file dialog
    QString filePath = QFileDialog::getSaveFileName(this, tr("Please select a filename"), \
                                        tr("gamelist") + ".txt", tr("Text files") + " (*.txt)", \
                                        NULL, QFileDialog::DontUseNativeDialog);
    qDebug() << "choosen filepath=" << filePath;

    if (!filePath.isEmpty()) {
        QList<QTreeWidgetItem *> items;
        for (int i = 0; i < this->topLevelItemCount(); i++)
            items << this->topLevelItem(i);

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;
        QTextStream out(&file);

        foreach (QTreeWidgetItem *item, items) {
            GameItem *gameItem = gameList->itemByPath(item->data(0, Qt::UserRole).toString());
            out << gameItem->titleID() + " - " + gameItem->title() + " - " + gameItem->totalSize() + "\n";
        }

        file.close();
    }
}

void MyGamesTreeWidget::exportCSV_Slot()
{
    if (!(this->topLevelItemCount() > 0))
        return;

    qDebug() << "requested export csv list!";

    // display a save file dialog
    QString filePath = QFileDialog::getSaveFileName(this, tr("Please select a filename"), \
                                        tr("gamelist") + ".csv", tr("CSV files") + " (*.csv)", \
                                        NULL, QFileDialog::DontUseNativeDialog);

    if (!filePath.isEmpty()) {
        QList<QTreeWidgetItem *> items;
        for (int i = 0; i < this->topLevelItemCount(); i++)
            items << this->topLevelItem(i);

        QFile file(filePath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            return;
        QTextStream out(&file);

        out << "\"" + tr("Title_ID") + "\";\"" + tr("Title") + "\";\"" + tr("Size on Disk") + "\"\n";
        foreach (QTreeWidgetItem *item, items) {
            GameItem *gameItem = gameList->itemByPath(item->data(0, Qt::UserRole).toString());
            out << "\"" + gameItem->titleID() + "\";\"" + gameItem->title() + "\";\"" + gameItem->totalSize() + "\"\n";
        }

        file.close();
    }
}

void MyGamesTreeWidget::delete_Slot()
{
    qDebug() << "requested delete!";

    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Warning);
    mb->setWindowTitle(tr("Warning"));
    mb->setText(tr("You requested PS3 game backup(s) deletion."));
    mb->setInformativeText(tr("You will be unable to recover them, do you want to continue ?"));
    mb->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    mb->setDefaultButton(QMessageBox::Cancel);
    int answer = mb->exec();
    delete mb;
    this->update();

    if (answer == QMessageBox::Yes) {
        // handle multi-selection, delete all games given
        // from selected rows in treewidget
        QFileInfoList fileInfoList;
        foreach (QTreeWidgetItem *item, this->selectedItems())
            fileInfoList << QFileInfo(item->data(0, Qt::UserRole).toString());

        ProgressDialog *dialog = new ProgressDialog(ProgressDialog::WithProgressBar, this);
        this->connect(dialog, SIGNAL(progressDialogDone(ProgressDialog*,bool,bool)), this, SLOT(progressDialogDone_Slot(ProgressDialog*,bool,bool)));
        activeDialogsList << dialog;
        dialog->remove(fsModel, fileInfoList);
    }
}

void MyGamesTreeWidget::edit_Slot()
{
    //qDebug() << "requested edit!";

    QList<QTreeWidgetItem *> list = this->selectedItems();
    QTreeWidgetItem *selectedItem = list.at(0);

    GameItem *gameItem = gameList->itemByPath(selectedItem->data(0, Qt::UserRole).toString());
    if (gameItem != NULL) {

        SfoEditorDialog *dialog = new SfoEditorDialog(gameItem, this);
        dialog->exec();
        delete dialog;

        emit gameListItemUpdated(gameItem);
    }
}

void MyGamesTreeWidget::progressDialogDone_Slot(ProgressDialog *dialog, bool aborted, bool error)
{
    Q_UNUSED(aborted);
    Q_UNUSED(error);

    for (int i=0; i<activeDialogsList.count(); i++) {
        QDialog *pDialog = activeDialogsList.at(i);
        if (pDialog == dialog) {
            activeDialogsList.removeAt(i);
            delete dialog;
            break;
        }
    }

    refreshList();
}

//
// private
//

void MyGamesTreeWidget::setContextMenu()
{
    // build a Context Menu

    contextMenu = new QMenu(this);
    QImage img;
    QIcon icon;

    // create all Context Menu actions
    contextMenu_cutAction = new QAction(tr("Cut"), this);
    contextMenu->addAction(contextMenu_cutAction);
    contextMenu_copyAction = new QAction(tr("Copy"), this);
    contextMenu->addAction(contextMenu_copyAction);
    contextMenu->addSeparator();

    syncSubMenu = new QMenu(contextMenu);
    syncSubMenu->setTitle(tr("Sync backups folders"));
    syncSubMenu_aAction = new QAction(this);
    syncSubMenu->addAction(syncSubMenu_aAction);
    syncSubMenu_bAction = new QAction(this);
    syncSubMenu->addAction(syncSubMenu_bAction);
    contextMenu->addAction(syncSubMenu->menuAction());
    exportSubMenu = new QMenu(contextMenu);
    exportSubMenu->setTitle(tr("Export Games List"));
    exportSubMenu_textAction = new QAction(tr("text format"), this);
    exportSubMenu->addAction(exportSubMenu_textAction);
    img.load("images/txt.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    exportSubMenu_textAction->setIcon(icon);
    exportSubMenu_textAction->setIconVisibleInMenu(true);
    exportSubMenu_csvAction = new QAction(tr("csv format"), this);
    exportSubMenu->addAction(exportSubMenu_csvAction);
    contextMenu->addAction(exportSubMenu->menuAction());
    img.load("images/csv.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    exportSubMenu_csvAction->setIcon(icon);
    exportSubMenu_csvAction->setIconVisibleInMenu(true);
    contextMenu->addSeparator();

    contextMenu_deleteAction = new QAction(tr("Delete"), this);
    contextMenu->addAction(contextMenu_deleteAction);
    contextMenu_editAction = new QAction(tr("Edit PARAM.SFO"), this);
    contextMenu->addAction(contextMenu_editAction);
}

void MyGamesTreeWidget::connectContextMenuSignals()
{
    this->connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(contextMenu_Slot(const QPoint&)));

    this->connect(contextMenu_cutAction, SIGNAL(triggered()), this, SLOT(cut_Slot()));
    this->connect(contextMenu_copyAction, SIGNAL(triggered()), this, SLOT(copy_Slot()));
    this->connect(syncSubMenu_aAction, SIGNAL(triggered()), this, SLOT(syncBackupsA_Slot()));
    this->connect(syncSubMenu_bAction, SIGNAL(triggered()), this, SLOT(syncBackupsB_Slot()));
    this->connect(exportSubMenu_textAction, SIGNAL(triggered()), this, SLOT(exportText_Slot()));
    this->connect(exportSubMenu_csvAction, SIGNAL(triggered()), this, SLOT(exportCSV_Slot()));
    this->connect(contextMenu_deleteAction, SIGNAL(triggered()), this, SLOT(delete_Slot()));
    this->connect(contextMenu_editAction, SIGNAL(triggered()), this, SLOT(edit_Slot()));
}


//
// protected
//

// reimp!
QMimeData* MyGamesTreeWidget::mimeData(const QList<QTreeWidgetItem *> items) const
{
    qDebug() << "GamesTreeWidget: mimeData()";

    QList<QUrl> urls;
    QList<QTreeWidgetItem *>::const_iterator it = items.begin();

    for (; it != items.end(); ++it) {
        qDebug() << "url=" << (*it)->data(0, Qt::UserRole).toString();
        urls << QUrl::fromLocalFile((*it)->data(0, Qt::UserRole).toString());
    }

    QMimeData *data = new QMimeData();
    data->setUrls(urls);

    return data;
}

// reimp!
QStringList MyGamesTreeWidget::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}
