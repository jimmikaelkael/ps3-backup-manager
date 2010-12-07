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

#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <QDialog>
#include <QCloseEvent>
#include <QEvent>
#include <QImage>
#include <QPixmap>
#include <QFontMetrics>
#include <QFileInfo>
#include <QMessageBox>
#include <QDebug>

#include "cleanerthread.h"
#include "removethread.h"
#include "copythread.h"
#include "movethread.h"
#include "syncthread.h"
#include "splitthread.h"
#include "myfilesystemmodel.h"
#include "myfileinfo.h"

namespace Ui {
    class ProgressDialog;
}

class ProgressDialog : public QDialog
{
    Q_OBJECT

public:

    enum DialogWidgetsType {
        WithProgressBar,
        WithoutProgressBar
    };

    ProgressDialog(DialogWidgetsType widgetsType = WithProgressBar, QWidget *parent = 0);
    ~ProgressDialog();
    void clean(MyFileSystemModel *fsModel, QString path, QString fileName);
    void cleanRecursive(MyFileSystemModel *fsModel, QString path, QString fileName);
    void remove(MyFileSystemModel *fsModel, QFileInfoList fileInfoList);
    void copy(MyFileInfoList fileInfoList);
    void move(MyFileSystemModel *fsModel, MyFileInfoList fileInfoList);
    void sync(MyFileSystemModel *fsModel, QString srcPath, QString destPath);
    void split(MyFileSystemModel *fsModel, QString startPath);

signals:
    void progressDialogDone(ProgressDialog *dialog, bool aborted, bool error);

protected:
    void closeEvent(QCloseEvent *e);
    //bool event(QEvent *e);

private slots:
    void progressRange_Slot(qint64 max);
    void progressUpdated_Slot(QString message, qint64 value);
    void cleanerThreadFinished_Slot(bool aborted, bool error);
    void cleanerThreadAbort_Slot();
    void removeThreadFinished_Slot(bool aborted, bool error);
    void removeThreadAbort_Slot();
    void copyThreadFinished_Slot(bool aborted, bool error);
    void copyThreadAbort_Slot();
    void copyThreadFileConflict_Slot(QString srcPath, QString destPath);
    void moveThreadFinished_Slot(bool aborted, bool error);
    void moveThreadAbort_Slot();
    void moveThreadFileConflict_Slot(QString srcPath, QString destPath);
    void syncThreadFinished_Slot(bool aborted, bool error);
    void syncThreadAbort_Slot();
    void syncThreadFileConflict_Slot(QString srcPath, QString destPath);
    void syncThreadFileConflictRemove_Slot(QString filePath, QString srcPath);
    void splitThreadFinished_Slot(bool aborted, bool error);
    void splitThreadAbort_Slot();
    void splitThreadFileConflict_Slot(QString filePath, qint64 newSize);
    void splitThreadNotEnoughDiskSpace_Slot();

private:
    void clean(MyFileSystemModel *fsModel, QString path, QString fileName, CleanerThread::CleanType type);

    Ui::ProgressDialog *ui;
    CleanerThread *cleanerThread;
    RemoveThread *removeThread;
    CopyThread *copyThread;
    MoveThread *moveThread;
    SyncThread *syncThread;
    SplitThread *splitThread;

    int progressDiv;
};

#endif // PROGRESSDIALOG_H
