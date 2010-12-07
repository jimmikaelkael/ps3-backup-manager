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

#ifndef FILECONFLICTDIALOG_H
#define FILECONFLICTDIALOG_H

#include <QDialog>
#include <QCloseEvent>
#include <QImage>
#include <QPixmap>
#include <QFileInfo>
#include <QDir>
#include <QFileIconProvider>
#include <QDateTime>
#include <QDebug>

namespace Ui {
    class FileConflictDialog;
}

class FileConflictDialog : public QDialog
{
    Q_OBJECT

public:
    enum DialogType {
        conflictDialog,
        syncConfirmDialog,
        syncRemoveDialog,
        splitDialog
    };

    FileConflictDialog(DialogType dialogType, QString srcPath, QString destPath, QWidget *parent = 0);
    FileConflictDialog(DialogType dialogType, QString filePath, qint64 newSize, QWidget *parent = 0);
    ~FileConflictDialog();

    enum ConflictAction {
        cancel          = 1,
        ignore          = 2,
        confirm         = 4,
        applyToAll      = 8
    };

private slots:
    void cancel_Slot();
    void negative_Slot();
    void affirmative_Slot();

private:
    void connectUiSignals();
    void setupFileConflictDialog(QString srcPath, QString destPath);
    void setupSyncConfirmDialog(QString srcPath, QString destPath);
    void setupSyncRemoveDialog(QString filePath, QString srcPath);
    void setupSplitDialog(QString filePath, qint64 newSize);

    Ui::FileConflictDialog *ui;

protected:
    void closeEvent(QCloseEvent *e);

};

#endif // FILECONFLICTDIALOG_H
