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

#include "progressdialog.h"
#include "ui_progressdialog.h"

#include "fileconflictdialog.h"

ProgressDialog::ProgressDialog(DialogWidgetsType widgetsType, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressDialog),
    cleanerThread(NULL),
    removeThread(NULL),
    copyThread(NULL),
    moveThread(NULL),
    splitThread(NULL),
    progressDiv(1)
{
    ui->setupUi(this);

    this->setModal(false);

    switch (widgetsType) {
        case WithProgressBar:
            ui->cancelToolButton->setIcon(QIcon("images/error.png"));
            ui->cancelPushButton->hide();
            break;
        case WithoutProgressBar:
            ui->progressBar->hide();
            ui->cancelToolButton->hide();
            break;
    }
}

ProgressDialog::~ProgressDialog()
{
    delete ui;
}

void ProgressDialog::clean(MyFileSystemModel *fsModel, QString path, QString fileName)
{
    qDebug() << "cleaning path=" << path << "file=" << fileName;

    clean(fsModel, path, fileName, CleanerThread::cleanNonRecursive);
}

void ProgressDialog::cleanRecursive(MyFileSystemModel *fsModel, QString path, QString fileName)
{
    qDebug() << "cleaning recursively path=" << path << "file=" << fileName;

    clean(fsModel, path, fileName, CleanerThread::cleanRecursive);
}

void ProgressDialog::remove(MyFileSystemModel *fsModel, QFileInfoList fileInfoList)
{
    ui->informativeLabel->clear();

    // set dialog title text
    QString titleText = tr("File operations");
    this->setWindowTitle(titleText);

    // start the thread
    qDebug() << "remove thread starting!";
    removeThread = new RemoveThread(fsModel, fileInfoList, this);

    this->connect(removeThread, SIGNAL(finished(bool,bool)), this, SLOT(removeThreadFinished_Slot(bool,bool)));
    ui->progressBar->setValue(0);
    this->connect(removeThread, SIGNAL(progressRange(qint64)), this, SLOT(progressRange_Slot(qint64)));
    this->connect(removeThread, SIGNAL(progressUpdated(QString,qint64)), this, SLOT(progressUpdated_Slot(QString,qint64)));

    this->connect(ui->cancelToolButton, SIGNAL(clicked()), this, SLOT(removeThreadAbort_Slot()));

    // show the dialog
    this->show();
    this->raise();
    this->activateWindow();

    removeThread->start();
}

void ProgressDialog::copy(MyFileInfoList fileInfoList)
{
    ui->informativeLabel->clear();

    // set dialog title text
    QString titleText = tr("File operations");
    this->setWindowTitle(titleText);

    // start the thread
    qDebug() << "copy thread starting!";
    copyThread = new CopyThread(fileInfoList, this);

    this->connect(copyThread, SIGNAL(finished(bool,bool)), this, SLOT(copyThreadFinished_Slot(bool,bool)));
    ui->progressBar->setValue(0);
    this->connect(copyThread, SIGNAL(progressRange(qint64)), this, SLOT(progressRange_Slot(qint64)));
    this->connect(copyThread, SIGNAL(progressUpdated(QString,qint64)), this, SLOT(progressUpdated_Slot(QString,qint64)));
    this->connect(copyThread, SIGNAL(fileConflict(QString,QString)), this, SLOT(copyThreadFileConflict_Slot(QString,QString)));

    this->connect(ui->cancelToolButton, SIGNAL(clicked()), this, SLOT(copyThreadAbort_Slot()));

    // show the dialog
    this->show();
    this->raise();
    this->activateWindow();

    copyThread->start();
}

void ProgressDialog::move(MyFileSystemModel *fsModel, MyFileInfoList fileInfoList)
{
    ui->informativeLabel->clear();

    // set dialog title text
    QString titleText = tr("File operations");
    this->setWindowTitle(titleText);

    // start the thread
    qDebug() << "move thread starting!";
    moveThread = new MoveThread(fsModel, fileInfoList, this);

    this->connect(moveThread, SIGNAL(finished(bool,bool)), this, SLOT(moveThreadFinished_Slot(bool,bool)));
    ui->progressBar->setValue(0);
    this->connect(moveThread, SIGNAL(progressRange(qint64)), this, SLOT(progressRange_Slot(qint64)));
    this->connect(moveThread, SIGNAL(progressUpdated(QString,qint64)), this, SLOT(progressUpdated_Slot(QString,qint64)));
    this->connect(moveThread, SIGNAL(fileConflict(QString,QString)), this, SLOT(moveThreadFileConflict_Slot(QString,QString)));

    this->connect(ui->cancelToolButton, SIGNAL(clicked()), this, SLOT(moveThreadAbort_Slot()));

    // show the dialog
    this->show();
    this->raise();
    this->activateWindow();

    moveThread->start();
}

void ProgressDialog::sync(MyFileSystemModel *fsModel, QString srcPath, QString destPath)
{
    ui->informativeLabel->clear();

    // set dialog title text
    QString titleText = tr("File operations");
    this->setWindowTitle(titleText);

    MyFileInfoList fileInfoList;
    MyFileInfo fileInfo(srcPath);
    fileInfo.setDestinationFilePath(destPath);
    fileInfoList << fileInfo;

    // start the thread
    qDebug() << "sync thread starting!";
    syncThread = new SyncThread(fsModel, fileInfoList, this);

    this->connect(syncThread, SIGNAL(finished(bool,bool)), this, SLOT(syncThreadFinished_Slot(bool,bool)));
    ui->progressBar->setValue(0);
    this->connect(syncThread, SIGNAL(progressRange(qint64)), this, SLOT(progressRange_Slot(qint64)));
    this->connect(syncThread, SIGNAL(progressUpdated(QString,qint64)), this, SLOT(progressUpdated_Slot(QString,qint64)));
    this->connect(syncThread, SIGNAL(fileConflict(QString,QString)), this, SLOT(syncThreadFileConflict_Slot(QString,QString)));
    this->connect(syncThread, SIGNAL(fileConflictRemove(QString,QString)), this, SLOT(syncThreadFileConflictRemove_Slot(QString,QString)));

    this->connect(ui->cancelToolButton, SIGNAL(clicked()), this, SLOT(syncThreadAbort_Slot()));

    // show the dialog
    this->show();
    this->raise();
    this->activateWindow();

    syncThread->start();
}

void ProgressDialog::split(MyFileSystemModel *fsModel, QString startPath)
{
    ui->informativeLabel->clear();

    // set dialog title text
    QString titleText = tr("4GB File Splitter");
    this->setWindowTitle(titleText);

    // start the thread
    qDebug() << "split thread starting!";
    splitThread = new SplitThread(fsModel, startPath, SplitThread::splitRecursive, this);

    this->connect(splitThread, SIGNAL(finished(bool,bool)), this, SLOT(splitThreadFinished_Slot(bool,bool)));
    ui->progressBar->setValue(0);
    this->connect(splitThread, SIGNAL(progressRange(qint64)), this, SLOT(progressRange_Slot(qint64)));
    this->connect(splitThread, SIGNAL(progressUpdated(QString,qint64)), this, SLOT(progressUpdated_Slot(QString,qint64)));
    this->connect(splitThread, SIGNAL(fileConflict(QString,qint64)), this, SLOT(splitThreadFileConflict_Slot(QString,qint64)));

    this->connect(ui->cancelToolButton, SIGNAL(clicked()), this, SLOT(splitThreadAbort_Slot()));

    // show the dialog
    this->show();
    this->raise();
    this->activateWindow();

    splitThread->start();
}


//
// protected
//

// reimp!
void ProgressDialog::closeEvent(QCloseEvent *e)
{
    // just ignore close event
    e->ignore();
}

/*
bool ProgressDialog::event(QEvent *e)
{
    //qDebug() << "event=" << e->type();
    if ((updateSuspended) && (e->type() == QEvent::UpdateRequest))
        return true;

    return QDialog::event(e);
}
*/

//
// private slots
//

void ProgressDialog::progressRange_Slot(qint64 max)
{
    if (max > 2147483647)
        progressDiv = 1048576;
    else
        progressDiv = 1;

    ui->progressBar->setRange(0, max/progressDiv);
}

void ProgressDialog::progressUpdated_Slot(QString message, qint64 value)
{
    QString text = QFontMetrics(ui->informativeLabel->font()).elidedText(message, Qt::ElideMiddle, ui->informativeLabel->width());
    ui->informativeLabel->setText(text);
    ui->progressBar->setValue(value/progressDiv);
    ui->progressBar->update();
}

void ProgressDialog::cleanerThreadFinished_Slot(bool aborted, bool error)
{
    qDebug() << "cleaning thread done!";

    cleanerThread->deleteLater();

    // this will release dialog
    this->accept();
    this->deleteLater();

    emit progressDialogDone(this, aborted, error);
}

void ProgressDialog::cleanerThreadAbort_Slot()
{
    cleanerThread->abort();
    cleanerThread->wait();
    this->reject();
    this->deleteLater();

    emit progressDialogDone(this, true, true);
}

void ProgressDialog::removeThreadFinished_Slot(bool aborted, bool error)
{
    qDebug() << "remove thread done!";

    removeThread->deleteLater();

    // this will release dialog
    this->accept();
    this->deleteLater();

    emit progressDialogDone(this, aborted, error);
}

void ProgressDialog::removeThreadAbort_Slot()
{
    removeThread->abort();
    removeThread->wait();
    this->reject();
    this->deleteLater();

    emit progressDialogDone(this, true, true);
}

void ProgressDialog::copyThreadFinished_Slot(bool aborted, bool error)
{
    qDebug() << "copy thread done!";

    copyThread->deleteLater();

    // this will release dialog
    this->accept();
    this->deleteLater();

    emit progressDialogDone(this, aborted, error);
}

void ProgressDialog::copyThreadAbort_Slot()
{
    copyThread->abort();
    copyThread->wait();
    this->reject();
    this->deleteLater();

    emit progressDialogDone(this, true, true);
}

void ProgressDialog::copyThreadFileConflict_Slot(QString srcPath, QString destPath)
{
    //ui->progressBar->setUpdatesEnabled(false);
    FileConflictDialog *dialog = new FileConflictDialog(FileConflictDialog::conflictDialog, srcPath, destPath, this);
    int res = dialog->exec();
    //ui->progressBar->setUpdatesEnabled(true);
    copyThread->fileConflictAction_Slot((CopyThread::ConflictAction)res);
}

void ProgressDialog::moveThreadFinished_Slot(bool aborted, bool error)
{
    qDebug() << "move thread done!";

    moveThread->deleteLater();

    // this will release dialog
    this->accept();
    this->deleteLater();

    emit progressDialogDone(this, aborted, error);
}

void ProgressDialog::moveThreadAbort_Slot()
{
    moveThread->abort();
    moveThread->wait();
    this->reject();
    this->deleteLater();

    emit progressDialogDone(this, true, true);
}

void ProgressDialog::moveThreadFileConflict_Slot(QString srcPath, QString destPath)
{
    //ui->progressBar->setUpdatesEnabled(false);
    FileConflictDialog *dialog = new FileConflictDialog(FileConflictDialog::conflictDialog, srcPath, destPath, this);
    int res = dialog->exec();
    //ui->progressBar->setUpdatesEnabled(true);
    moveThread->fileConflictAction_Slot((MoveThread::ConflictAction)res);
}

void ProgressDialog::syncThreadFinished_Slot(bool aborted, bool error)
{
    qDebug() << "sync thread done!";

    syncThread->deleteLater();

    // this will release dialog
    this->accept();
    this->deleteLater();

    emit progressDialogDone(this, aborted, error);
}

void ProgressDialog::syncThreadAbort_Slot()
{
    syncThread->abort();
    syncThread->wait();
    this->reject();
    this->deleteLater();

    emit progressDialogDone(this, true, true);
}

void ProgressDialog::syncThreadFileConflict_Slot(QString srcPath, QString destPath)
{
    //ui->progressBar->setUpdatesEnabled(false);
    FileConflictDialog *dialog = new FileConflictDialog(FileConflictDialog::conflictDialog, srcPath, destPath, this);
    int res = dialog->exec();
    //ui->progressBar->setUpdatesEnabled(true);
    syncThread->fileConflictAction_Slot((SyncThread::ConflictAction)res);
}

void ProgressDialog::syncThreadFileConflictRemove_Slot(QString filePath, QString srcPath)
{
    //ui->progressBar->setUpdatesEnabled(false);
    FileConflictDialog *dialog = new FileConflictDialog(FileConflictDialog::syncRemoveDialog, filePath, srcPath, this);
    int res = dialog->exec();
    //ui->progressBar->setUpdatesEnabled(true);
    syncThread->fileConflictAction_Slot((SyncThread::ConflictAction)res);
}

void ProgressDialog::splitThreadFinished_Slot(bool aborted, bool error)
{
    qDebug() << "split thread done!";

    splitThread->deleteLater();

    // this will release dialog
    this->accept();
    this->deleteLater();

    emit progressDialogDone(this, aborted, error);
}

void ProgressDialog::splitThreadAbort_Slot()
{
    splitThread->abort();
    splitThread->wait();
    this->reject();
    this->deleteLater();

    emit progressDialogDone(this, true, true);
}

void ProgressDialog::splitThreadFileConflict_Slot(QString filePath, qint64 newSize)
{
    //ui->progressBar->setUpdatesEnabled(false);
    FileConflictDialog *dialog = new FileConflictDialog(FileConflictDialog::splitDialog, filePath, newSize, this);
    int res = dialog->exec();
    //ui->progressBar->setUpdatesEnabled(true);
    splitThread->fileConflictAction_Slot((SplitThread::ConflictAction)res);
}

void ProgressDialog::splitThreadNotEnoughDiskSpace_Slot()
{
    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Critical);
    mb->setWindowTitle(tr("Error"));
    mb->setText(tr("Not enough space available on Disk."));
    mb->setInformativeText(tr("Operation is aborted."));
    mb->setStandardButtons(QMessageBox::Ok);
    mb->exec();
    delete mb;

    this->reject();
    this->deleteLater();

    emit progressDialogDone(this, false, true);
}

//
// private
//
void ProgressDialog::clean(MyFileSystemModel *fsModel, QString path, QString fileName, CleanerThread::CleanType type)
{
    ui->informativeLabel->clear();

    QString informativeText = tr("Initializing task...");
    ui->informativeLabel->setText(informativeText);

    // set dialog title text
    QString titleText = tr("PS3UPDAT.PUP Cleaner");
    this->setWindowTitle(titleText);

    // start the thread
    qDebug() << "cleaning thread starting!";
    cleanerThread = new CleanerThread(fsModel, path, fileName, type, this);

    this->connect(cleanerThread, SIGNAL(finished(bool,bool)), this, SLOT(cleanerThreadFinished_Slot(bool,bool)));
    ui->progressBar->setValue(0);
    this->connect(cleanerThread, SIGNAL(progressRange(qint64)), this, SLOT(progressRange_Slot(qint64)));
    this->connect(cleanerThread, SIGNAL(progressUpdated(QString,qint64)), this, SLOT(progressUpdated_Slot(QString,qint64)));

    this->connect(ui->cancelToolButton, SIGNAL(clicked()), this, SLOT(cleanerThreadAbort_Slot()));

    // show the dialog
    this->show();
    this->raise();
    this->activateWindow();

    cleanerThread->start();
}
