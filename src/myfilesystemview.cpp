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

#include "myfilesystemview.h"

#include "fileconflictdialog.h"

#include "appclipboard.h"
#include "myfilesystemmodel.h"
#include "progressdialog.h"
#include "openurlthread.h"

MyFileSystemView::MyFileSystemView(AppClipboard *clipboard, QWidget *parent) :
    QTreeView(parent),
    appClipboard(clipboard),
    fsModel(new MyFileSystemModel(this)),
    openUrlThread(new OpenUrlThread(this))
{
    // set some default for our widget
    this->setAnimated(true);
    this->setAcceptDrops(true);
    this->setDefaultDropAction(Qt::MoveAction);
    this->setDragDropMode(QTreeView::DragDrop);
    this->setDragEnabled(true);
    this->setHeaderHidden(true);
    this->setDropIndicatorShown(true);
    this->setRootIsDecorated(true);
    this->setSelectionMode(QTreeView::ExtendedSelection);
    this->setContextMenuPolicy(Qt::CustomContextMenu);

    // set context menu
    setContextMenu();
    connectContextMenuSignals();

    // set FileSystem model
    setFileSystemModel();
}

MyFileSystemView::~MyFileSystemView()
{
    openUrlThread->wait();
    delete openUrlThread;
    delete fsModel;
    delete contextMenu_renameAction;
    delete contextMenu_deleteAction;
    delete pupCleanerSubMenu_allSubfoldersAction;
    delete pupCleanerSubMenu_onlyThisFolderAction;
    delete contextMenu_newDirAction;
    delete contextMenu_pasteAction;
    delete contextMenu_copyAction;
    delete contextMenu_cutAction;
    delete contextMenu_openAction;
    delete pupCleanerSubMenu;
    delete contextMenu;
}

QAbstractItemModel *MyFileSystemView::fileSystemModel()
{   // return fileSystemModel pointer

    return this->model();
}

int MyFileSystemView::activeDialogsCount()
{
    return activeDialogsList.count();
}

//
// private slots
//
void MyFileSystemView::contextMenu_Slot(const QPoint& point)
{
    // draw a Context Menu at coordinates point
    qDebug() << "Context menu requested";

    // check we got at least 1 row selected to exec Context Menu
    int numSelectedItems = getNumSelectedRows();
    if (numSelectedItems > 0) {

        // for all menu item below, we want them activated
        // only if there's only 1 row selected
        contextMenu_openAction->setDisabled(true);
        contextMenu_pasteAction->setDisabled(true);
        contextMenu_newDirAction->setDisabled(true);
        syncSubMenu->setDisabled(true);
        pupCleanerSubMenu->setDisabled(true);
        contextMenu_splitAction->setDisabled(true);
        contextMenu_renameAction->setDisabled(true);
        contextMenu_cutAction->setEnabled(true);
        contextMenu_copyAction->setEnabled(true);
        contextMenu_deleteAction->setEnabled(true);

        if (numSelectedItems == 1) {
            contextMenu_openAction->setEnabled(true);

            // check mimedata has urls & DropAction is engaged by our app (we don't
            // want the context menu to allow to paste item from other apps)
            if ((appClipboard->hasUrls()) && (appClipboard->dropAction() != Qt::IgnoreAction))
                contextMenu_pasteAction->setEnabled(true);

            // if target selected item is a file we take his parent as we want to
            // enable the dir create for this parent folder if writable
            QFileInfo fileInfo = getSelectedRowFileInfo();
            QModelIndex modelIndex = getSelectedRowModelIndex();
            if (fileInfo.isFile()) {
                modelIndex = modelIndex.parent();
                QFileInfo parentInfo = fsModel->fileInfo(modelIndex);
                if (parentInfo.isWritable())
                    contextMenu_newDirAction->setEnabled(true);
            }
            else {
                if (fileInfo.isWritable()) {
                    contextMenu_newDirAction->setEnabled(true);
                    pupCleanerSubMenu->setEnabled(true);
                    contextMenu_splitAction->setEnabled(true);
                }
            }

            // check file/dir is writable to enable 'rename' item
            if (fileInfo.isWritable())
                contextMenu_renameAction->setEnabled(true);

            if (!fileInfo.isRoot())
                contextMenu_deleteAction->setEnabled(true);
        }
        else if (numSelectedItems == 2) {
            // if 2 items selected and they are folder sync submenu is enabled
            QModelIndexList modelIndexList = getSelectedRowsModelIndexes();
            QModelIndexList selectedIndexList;
            int row = -1;
            foreach (QModelIndex index, modelIndexList) {
                if (index.row()!=row && index.column()==0) {
                    row = index.row();
                    selectedIndexList << index;
                }
            }
            QFileInfo fileInfoA(fsModel->fileInfo(selectedIndexList.at(0)));
            QFileInfo fileInfoB(fsModel->fileInfo(selectedIndexList.at(1)));
            if ((fileInfoA.isDir()) && (fileInfoB.isDir()) \
                && (!(fileInfoA.isSymLink())) && (!(fileInfoB.isSymLink()))) {
                syncFolderA = fileInfoA;
                syncFolderB = fileInfoB;
                syncSubMenu_aAction->setText(fileInfoA.fileName() + tr(" -> ") + fileInfoB.fileName());
                syncSubMenu_bAction->setText(fileInfoB.fileName() + tr(" -> ") + fileInfoA.fileName());
                syncSubMenu->setEnabled(true);
            }
        }

        // exec Context Menu
        contextMenu->exec(this->mapToGlobal(point));
    }
}

void MyFileSystemView::open_Slot()
{
    qDebug() << "Requested open";
    QFileInfo fileInfo = getSelectedRowFileInfo();

    // adjust the filePath
    QString filePath = fileInfo.absoluteFilePath();
    if (!filePath.startsWith('/'))
        filePath = "/" + filePath;
    filePath = "file://" + filePath;

    // give target url the OpenUrlThread
    openUrlThread->setTargetUrl(QUrl(filePath));

    // exec OpenUrlThread only if there's no one already running
    if (!openUrlThread->isRunning())
        openUrlThread->start();
}

void MyFileSystemView::cut_Slot()
{
    qDebug() << "Requested cut";

    // fill clipboard mimedata with mimedata for selected rows
    appClipboard->setMimeData(fsModel->mimeData(this->selectionModel()->selectedIndexes()), Qt::MoveAction);
}

void MyFileSystemView::copy_Slot()
{
    qDebug() << "Requested copy";

    // fill clipboard mimedata with mimedata for selected rows
    appClipboard->setMimeData(fsModel->mimeData(this->selectionModel()->selectedIndexes()), Qt::CopyAction);
}

void MyFileSystemView::paste_Slot()
{
    // if drop action is set to something other than 'ignore' it means
    // our app requested a drop action
    if (appClipboard->dropAction() != Qt::IgnoreAction) {
        qDebug() << "Requested paste";

        // if target selected item at drop time is a file we take his
        // parent as we want to do the drop in this parent folder
        QFileInfo fileInfo = getSelectedRowFileInfo();
        QModelIndex modelIndex = getSelectedRowModelIndex();
        if (fileInfo.isFile())
            modelIndex = modelIndex.parent();

        // drop clipboard mimedata to FileSystemModel
        bool res = fsModel->dropMimeData(appClipboard->mimeData(), appClipboard->dropAction(), -1, -1, modelIndex);
        qDebug() << "mimedata drop result:" << res;

        // if we 'cutted', clears clipboard
        if (appClipboard->dropAction() == Qt::MoveAction)
            appClipboard->clear();
    }
}

void MyFileSystemView::syncFoldersAtoB_Slot()
{
    qDebug() << "requested sync folders A to B";
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

void MyFileSystemView::syncFoldersBtoA_Slot()
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

void MyFileSystemView::pupCleanerOnlyThisFolder_Slot()
{
    qDebug() << "PS3UPDAT.PUP cleaner (this folder only) requested";

    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Warning);
    mb->setWindowTitle(tr("Warning"));
    mb->setText(tr("You're about to delete all PS3UPDAT.PUP in this folder."));
    mb->setInformativeText(tr("You will be unable to recover them, do you want to continue ?"));
    mb->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    mb->setDefaultButton(QMessageBox::Cancel);
    int answer = mb->exec();
    delete mb;

    if (answer == QMessageBox::Yes) {
        QFileInfo fileInfo = getSelectedRowFileInfo();
        qDebug() << "selected filePath:" << fileInfo.absoluteFilePath();

        ProgressDialog *dialog = new ProgressDialog(ProgressDialog::WithProgressBar, this);
        this->connect(dialog, SIGNAL(progressDialogDone(ProgressDialog*,bool,bool)), this, SLOT(progressDialogDone_Slot(ProgressDialog*,bool,bool)));
        activeDialogsList << dialog;
        dialog->clean(fsModel, fileInfo.absoluteFilePath(), "PS3UPDAT.PUP");
    }
}

void MyFileSystemView::pupCleanerAllSubfolders_Slot()
{
    bool check = false;
    if (getNumSelectedRows() == 1) {
        QFileInfo fileInfo = getSelectedRowFileInfo();
        if (fileInfo.isDir()) {
            if (fileInfo.isWritable()) {
                check = true;
            }
        }
    }
    if (!check)
        return;

    qDebug() << "PS3UPDAT.PUP cleaner (all subfolders) requested";

    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Warning);
    mb->setWindowTitle(tr("Warning"));
    mb->setText(tr("You're about to delete all PS3UPDAT.PUP in this folder and all subfolders."));
    mb->setInformativeText(tr("You will be unable to recover them, do you want to continue ?"));
    mb->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    mb->setDefaultButton(QMessageBox::Cancel);
    int answer = mb->exec();
    delete mb;

    if (answer == QMessageBox::Yes) {
        QFileInfo fileInfo = getSelectedRowFileInfo();
        qDebug() << "selected filePath:" << fileInfo.absoluteFilePath();

        ProgressDialog *dialog = new ProgressDialog(ProgressDialog::WithProgressBar, this);
        this->connect(dialog, SIGNAL(progressDialogDone(ProgressDialog*,bool,bool)), this, SLOT(progressDialogDone_Slot(ProgressDialog*,bool,bool)));
        activeDialogsList << dialog;
        dialog->cleanRecursive(fsModel, fileInfo.absoluteFilePath(), "PS3UPDAT.PUP");
    }
}

void MyFileSystemView::newDir_Slot()
{
    qDebug() << "New Directory requested";

    // if target selected item at drop time is a file we take his
    // parent as we want to do the directory creation in this parent folder
    QFileInfo fileInfo = getSelectedRowFileInfo();
    QModelIndex modelIndex = getSelectedRowModelIndex();
    if (fileInfo.isFile())
        modelIndex = modelIndex.parent();

    // create new dir and enter edit mode on it
    const QModelIndex newDirIndex = fsModel->createNewDirectory(modelIndex);
    if (newDirIndex.isValid()) {
        this->scrollTo(newDirIndex);
        this->edit(newDirIndex);
    }
}

void MyFileSystemView::split_Slot()
{
    bool check = false;
    if (getNumSelectedRows() == 1) {
        QFileInfo fileInfo = getSelectedRowFileInfo();
        if (fileInfo.isDir()) {
            if (fileInfo.isWritable()) {
                check = true;
            }
        }
    }
    if (!check)
        return;

    qDebug() << "requested split!";

    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Warning);
    mb->setWindowTitle(tr("Warning"));
    mb->setText(tr("You're about to split all files greater than 4GB in this folder and all subfolders."));
    mb->setInformativeText(tr("Do you want to continue ?"));
    mb->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    mb->setDefaultButton(QMessageBox::Cancel);
    int answer = mb->exec();
    delete mb;

    if (answer == QMessageBox::Yes) {
        QFileInfo fileInfo = getSelectedRowFileInfo();
        qDebug() << "selected filePath:" << fileInfo.absoluteFilePath();

        ProgressDialog *dialog = new ProgressDialog(ProgressDialog::WithProgressBar, this);
        this->connect(dialog, SIGNAL(progressDialogDone(ProgressDialog*,bool,bool)), this, SLOT(progressDialogDone_Slot(ProgressDialog*,bool,bool)));
        activeDialogsList << dialog;
        dialog->split(fsModel, fileInfo.absoluteFilePath());
    }
}

void MyFileSystemView::delete_Slot()
{
    qDebug() << "Requested delete";

    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Warning);
    mb->setWindowTitle(tr("Warning"));
    mb->setText(tr("You requested file(s)/folder(s) deletion."));
    mb->setInformativeText(tr("You will be unable to recover them, do you want to continue ?"));
    mb->setStandardButtons(QMessageBox::Yes | QMessageBox::Cancel);
    mb->setDefaultButton(QMessageBox::Cancel);
    int answer = mb->exec();
    delete mb;
    this->update();

    if (answer == QMessageBox::Yes) {
        // handle multi-selection, delete all files/dirs given
        // from selected rows in treeview
        QFileInfoList fileInfoList;
        bool error = false;
        QModelIndexList list = getSelectedRowsModelIndexes();
        int row = -1;
        foreach (QModelIndex index, list) {
            if (index.row()!=row && index.column()==0) {
                row = index.row();

#if defined (Q_WS_WIN) // Windows system
                if ((fsModel->fileInfo(index).absoluteFilePath().length() == 2) \
                    && (fsModel->fileInfo(index).absoluteFilePath().at(1) == ':')) {
#else
                if (fsModel->fileInfo(index).isRoot()) {
#endif
                    qDebug() << "target for deletion is root... aborting";
                    QMessageBox *mb = new QMessageBox(this);
                    mb->setIcon(QMessageBox::Critical);
                    mb->setWindowTitle(tr("Error"));
                    mb->setText(tr("Deletion of root not allowed!"));
                    mb->setInformativeText(tr("Root item will not be deleted..."));
                    mb->setStandardButtons(QMessageBox::Ok);
                    mb->exec();
                    delete mb;
                    this->update();

                    error = true;
                }

                //this->collapse(index);
                fileInfoList << fsModel->fileInfo(index);
            }
        }

        if (!error) {
            ProgressDialog *dialog = new ProgressDialog(ProgressDialog::WithProgressBar, this);
            this->connect(dialog, SIGNAL(progressDialogDone(ProgressDialog*,bool,bool)), this, SLOT(progressDialogDone_Slot(ProgressDialog*,bool,bool)));
            activeDialogsList << dialog;
            dialog->remove(fsModel, fileInfoList);
        }
    }
}

void MyFileSystemView::rename_Slot()
{
    qDebug() << "Requested rename";

    // enter edit mode on the treeview's selected row
    const QModelIndex index = getSelectedRowModelIndex();
    if (index.isValid()) {
        this->scrollTo(index);
        this->edit(index);
    }
}

void MyFileSystemView::fsModelStateChanged_Slot()
{
    requestGameListRefresh();
}

void MyFileSystemView::progressDialogDone_Slot(ProgressDialog *dialog, bool aborted, bool error)
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

    requestGameListRefresh();
}

//
// private
//
void MyFileSystemView::setContextMenu()
{
    // build a Context Menu for treeviews

    contextMenu = new QMenu(this);
    QImage img;
    QIcon icon;

    // create all Context Menu actions
    contextMenu_openAction = new QAction(tr("Open"), this);
    contextMenu->addAction(contextMenu_openAction);
    contextMenu->addSeparator();

    contextMenu_cutAction = new QAction(tr("Cut"), this);
    contextMenu->addAction(contextMenu_cutAction);
    contextMenu_copyAction = new QAction(tr("Copy"), this);
    contextMenu->addAction(contextMenu_copyAction);
    contextMenu_pasteAction = new QAction(tr("Paste"), this);
    contextMenu->addAction(contextMenu_pasteAction);
    contextMenu->addSeparator();

    contextMenu_newDirAction = new QAction(tr("New folder"), this);
    contextMenu->addAction(contextMenu_newDirAction);
    syncSubMenu = new QMenu(contextMenu);
    syncSubMenu->setTitle(tr("Sync folders"));
    syncSubMenu_aAction = new QAction(this);
    syncSubMenu->addAction(syncSubMenu_aAction);
    syncSubMenu_bAction = new QAction(this);
    syncSubMenu->addAction(syncSubMenu_bAction);
    contextMenu->addAction(syncSubMenu->menuAction());
    pupCleanerSubMenu = new QMenu(contextMenu);
    pupCleanerSubMenu->setTitle(tr("PS3UPDAT.PUP cleaner"));
    pupCleanerSubMenu_onlyThisFolderAction = new QAction(tr("This folder only"), this);
    pupCleanerSubMenu->addAction(pupCleanerSubMenu_onlyThisFolderAction);
    pupCleanerSubMenu_allSubfoldersAction = new QAction(tr("This folder and all subfolders"), this);
    pupCleanerSubMenu->addAction(pupCleanerSubMenu_allSubfoldersAction);
    img.load("images/clean.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    contextMenu->addAction(pupCleanerSubMenu->menuAction());
    pupCleanerSubMenu->menuAction()->setIcon(icon);
    pupCleanerSubMenu->menuAction()->setIconVisibleInMenu(true);
    contextMenu_splitAction = new QAction(tr("4GB file splitter"), this);
    contextMenu->addAction(contextMenu_splitAction);
    img.load("images/split.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    contextMenu_splitAction->setIcon(icon);
    contextMenu_splitAction->setIconVisibleInMenu(true);
    contextMenu->addSeparator();

    contextMenu_deleteAction = new QAction(tr("Delete"), this);
    contextMenu->addAction(contextMenu_deleteAction);
    contextMenu_renameAction = new QAction(tr("Rename"), this);
    contextMenu->addAction(contextMenu_renameAction);
}

void MyFileSystemView::setFileSystemModel()
{
    // prepare Filesystem Model for the treeView objects
    fsModel->setRootPath(QDir::rootPath());
    fsModel->setReadOnly(false);
    fsModel->setResolveSymlinks(false);

    // set fs model to TreeView objects
    this->setModel(fsModel);
    this->hideColumn(1);
    this->hideColumn(2);
    this->hideColumn(3);

    this->connect(fsModel, SIGNAL(stateChanged()), this, SLOT(fsModelStateChanged_Slot()));
}

void MyFileSystemView::connectContextMenuSignals()
{
    this->connect(this, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(contextMenu_Slot(const QPoint&)));

    this->connect(contextMenu_openAction, SIGNAL(triggered()), this, SLOT(open_Slot()));
    this->connect(contextMenu_cutAction, SIGNAL(triggered()), this, SLOT(cut_Slot()));
    this->connect(contextMenu_copyAction, SIGNAL(triggered()), this, SLOT(copy_Slot()));
    this->connect(contextMenu_pasteAction, SIGNAL(triggered()), this, SLOT(paste_Slot()));
    this->connect(contextMenu_newDirAction, SIGNAL(triggered()), this, SLOT(newDir_Slot()));
    this->connect(syncSubMenu_aAction, SIGNAL(triggered()), this, SLOT(syncFoldersAtoB_Slot()));
    this->connect(syncSubMenu_bAction, SIGNAL(triggered()), this, SLOT(syncFoldersBtoA_Slot()));
    this->connect(pupCleanerSubMenu_onlyThisFolderAction, SIGNAL(triggered()), this, SLOT(pupCleanerOnlyThisFolder_Slot()));
    this->connect(pupCleanerSubMenu_allSubfoldersAction, SIGNAL(triggered()), this, SLOT(pupCleanerAllSubfolders_Slot()));
    this->connect(contextMenu_splitAction, SIGNAL(triggered()), this, SLOT(split_Slot()));
    this->connect(contextMenu_deleteAction, SIGNAL(triggered()), this, SLOT(delete_Slot()));
    this->connect(contextMenu_renameAction, SIGNAL(triggered()), this, SLOT(rename_Slot()));
}

int MyFileSystemView::getNumSelectedRows()
{
    // returns the numbers of selected rows
    QModelIndexList list = this->selectionModel()->selectedIndexes();

    int row = -1;
    int rows = 0;
    foreach (QModelIndex index, list) {
        if (index.row()!=row && index.column()==0) {
            row = index.row();
            rows++;
        }
    }

    return rows;
}

QFileInfo MyFileSystemView::getSelectedRowFileInfo()
{
    // returns the QFileInfo of 1st selected row in a given QTreeView

    QModelIndexList list = this->selectionModel()->selectedIndexes();
    MyFileSystemModel *model = (MyFileSystemModel *)this->model();
    QFileInfo fileInfo;

    int row = -1;
    foreach (QModelIndex index, list) {
        if (index.row()!=row && index.column()==0) {
            fileInfo = model->fileInfo(index);
            row = index.row();
            break;
        }
    }

    return fileInfo;
}

QModelIndex MyFileSystemView::getSelectedRowModelIndex()
{
    // returns the QModelIndex of 1st selected row
    QModelIndexList list = this->selectionModel()->selectedIndexes();
    QModelIndex index;

    int row = -1;
    foreach (index, list) {
        if (index.row()!=row && index.column()==0) {
            row = index.row();
            break;
        }
    }

    return index;
}

QModelIndexList MyFileSystemView::getSelectedRowsModelIndexes()
{
    // returns the QModelIndex of 1st selected row

    return this->selectionModel()->selectedIndexes();
}

void MyFileSystemView::requestGameListRefresh()
{
    QStringList pathList;
    QModelIndexList list = getSelectedRowsModelIndexes();
    int row = -1;
    foreach (QModelIndex index, list) {
        if (index.row()!=row && index.column()==0) {
            QFileInfo fileInfo = fsModel->fileInfo(index);
            QString path = fileInfo.absoluteFilePath();
            if (!fileInfo.isDir())
                path = fileInfo.absolutePath();
            pathList << path;
            row = index.row();
        }
    }

    emit selectionChanged(pathList);
}

//
// protected slots
//

// reimp!
void MyFileSystemView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    //qDebug() << "rows" << start << "to" << end << "about to removed from" << fsModel->fileInfo(parent).filePath();
    for (int i=start; i<=end; i++) {
        //qDebug() << "row" << i << "=" << fsModel->fileInfo(parent.child(i,0)).filePath();
        this->collapse(parent.child(i,0));
    }

    QAbstractItemView::rowsAboutToBeRemoved(parent, start, end);
}

// reimp!
void MyFileSystemView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    //qDebug() << "selection changed!";

    QTreeView::selectionChanged(selected, deselected);

    requestGameListRefresh();
}
