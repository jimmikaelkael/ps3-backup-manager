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

#include "fileconflictdialog.h"
#include "ui_fileconflictdialog.h"

#include "fileutils.h"

FileConflictDialog::FileConflictDialog(DialogType dialogType, QString srcPath, QString destPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileConflictDialog)
{
    // set up the UI for this dialog
    ui->setupUi(this);

    // set up icon label pixmap from image
    QImage img("images/information.png");
    ui->iconLabel->setPixmap(QPixmap::fromImage(img));

    switch (dialogType) {
        case conflictDialog:
            setupFileConflictDialog(srcPath, destPath);
            break;
        case syncConfirmDialog:
            setupSyncConfirmDialog(srcPath, destPath);
            break;
        case syncRemoveDialog:
            setupSyncRemoveDialog(srcPath, destPath);
            break;
        default:
            break;
    }

    // connect signals to slots
    connectUiSignals();

    // update geometry, important!
    this->updateGeometry();
}

FileConflictDialog::FileConflictDialog(DialogType dialogType, QString filePath, qint64 newSize, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FileConflictDialog)
{
    // set up the UI for this dialog
    ui->setupUi(this);

    // set up icon label pixmap from image
    QImage img("images/information.png");
    ui->iconLabel->setPixmap(QPixmap::fromImage(img));

    switch (dialogType) {
        case splitDialog:
            setupSplitDialog(filePath, newSize);
            break;
        default:
            break;
    }

    // connect signals to slots
    connectUiSignals();

    // update geometry, important!
    this->updateGeometry();
}

FileConflictDialog::~FileConflictDialog()
{
    delete ui;
}


//
// private
//

void FileConflictDialog::connectUiSignals()
{
    connect(ui->cancelPushButton, SIGNAL(clicked()), this, SLOT(cancel_Slot()));
    connect(ui->negativePushButton, SIGNAL(clicked()), this, SLOT(negative_Slot()));
    connect(ui->affirmativePushButton, SIGNAL(clicked()), this, SLOT(affirmative_Slot()));
}

void FileConflictDialog::setupFileConflictDialog(QString srcPath, QString destPath)
{
    this->setWindowTitle(tr("File conflict"));

    // get fileInfo for source/destination
    QFileInfo srcInfo(srcPath);
    QFileInfo destInfo(destPath);

    QString path = destInfo.absolutePath();
    // just to correct path to windows backslashes type, as
    // path will be emitted in file conflict message
    #if defined (Q_WS_WIN) // Windows system
        path = path.replace('/', QDir::separator());
    #endif
    QString dirName = path.right(path.length() - path.lastIndexOf(QDir::separator()) - 1);

    // adjust dialog widgets text, depending if file or directory conflict
    QString titleText;
    QString informativeText;
    QString originalText, originalExtendedInfoText;
    QString replaceText, replaceExtendedInfoText;
    QString checkBoxText;
    QString affirmativeButtonText;

    // use FileIcon provider to get file/folder icon
    QIcon fileIcon(QFileIconProvider().icon(QFileIconProvider::File));
    QIcon folderIcon(QFileIconProvider().icon(QFileIconProvider::Folder));

    // get Last Modified DateTime string for source/destination
    QString srcLastModified = srcInfo.lastModified().toString(tr("MM/dd/yyyy - hh:mm ap")); // french format will be "dd/MM/yyyy - hh:mm"
    QString destLastModified = destInfo.lastModified().toString(tr("MM/dd/yyyy - hh:mm ap")); // french format will be "dd/MM/yyyy - hh:mm"

    // if dir->dir
    if ((srcInfo.isDir()) && (destInfo.isDir())) {
        titleText = tr("Merge folder") + " \"" + destInfo.fileName() + "\" ?";
        informativeText = tr("A folder with the same name is already existing in") + " \"" + dirName + "\".\n" + \
            tr("You'll be asked before to replace any file causing conflict inside this folder.");
        originalText = tr("Original folder");
        originalExtendedInfoText = tr("Type: folder") + "\n" + \
                                   tr("Size:") + " -\n" + \
                                   tr("Last modification:") + " " + srcLastModified;
        replaceText = tr("Merge with");
        replaceExtendedInfoText = tr("Type: folder") + "\n" + \
                                  tr("Size:") + " -\n" + \
                                  tr("Last modification:") + " " + destLastModified;
        checkBoxText = tr("Apply to all folder conflicts");
        affirmativeButtonText = tr("&Merge");
    }
    else if ((srcInfo.isDir()) && (!destInfo.isDir())) { // if dir->file
        titleText = tr("Replace folder") + " \"" + srcInfo.fileName() + "\" ?";
        informativeText = tr("A folder with the same name is already existing in") + " \"" + dirName + "\".\n" + \
            tr("If you choose to replace it all files it contains will be replaced.");
        originalText = tr("Original folder");
        originalExtendedInfoText = tr("Type: folder") + "\n" + \
                                   tr("Size:") + " -\n" + \
                                   tr("Last modification:") + " " + srcLastModified;
        replaceText = tr("Replace with");
        replaceExtendedInfoText = tr("Type: file") + "\n" + \
                                  tr("Size:") + " " + FileUtils().humanReadableFilesize(destInfo.size()) + "\n" + \
                                  tr("Last modification:") + " " + destLastModified;
        checkBoxText = tr("Apply to all file conflicts");
        affirmativeButtonText = tr("&Replace");
    }
    else if ((!srcInfo.isDir()) && (destInfo.isDir())) { // if file->dir
        titleText = tr("Replace file") + " \"" + srcInfo.fileName() + "\" ?";
        informativeText = tr("A file with the same name is already existing in") + " \"" + dirName + "\".\n" + \
            tr("If you choose to replace it contents will be lost.");
        originalText = tr("Original file");
        originalExtendedInfoText = tr("Type: file") + "\n" + \
                                   tr("Size:") + " " + FileUtils().humanReadableFilesize(srcInfo.size()) + "\n" + \
                                   tr("Last modification:") + " " + srcLastModified;
        replaceText = tr("Replace with");
        replaceExtendedInfoText = tr("Type: folder") + "\n" + \
                                  tr("Size:") + " -\n" + \
                                  tr("Last modification:") + " " + destLastModified;
        checkBoxText = tr("Apply to all folder conflicts");
        affirmativeButtonText = tr("&Replace");
    }
    else { // if file->file
        titleText = tr("Replace file") + " \"" + destInfo.fileName() + "\" ?";
        informativeText = tr("A file with the same name is already existing in") + " \"" + dirName + "\".\n" + \
            tr("If you choose to replace it the existing file will be lost.");
        originalText = tr("Original file");
        originalExtendedInfoText = tr("Type: file") + "\n" + \
                                   tr("Size:") + " " + FileUtils().humanReadableFilesize(srcInfo.size()) + "\n" + \
                                   tr("Last modification:") + " " + srcLastModified;
        replaceText = tr("Replace with");
        replaceExtendedInfoText = tr("Type: file") + "\n" + \
                                  tr("Size:") + " " + FileUtils().humanReadableFilesize(destInfo.size()) + "\n" + \
                                  tr("Last modification:") + " " + destLastModified;
        checkBoxText = tr("Apply to all file conflicts");
        affirmativeButtonText = tr("&Replace");
    }

    // set dialogs texts
    ui->titleLabel->setText(titleText);
    ui->informativeLabel->setText(informativeText);
    ui->originalTextLabel->setText(originalText);
    ui->originalExtendedInfoLabel->setText(originalExtendedInfoText);
    ui->replaceTextLabel->setText(replaceText);
    ui->replaceExtendedInfoLabel->setText(replaceExtendedInfoText);
    ui->checkBox->setText(checkBoxText);
    ui->affirmativePushButton->setText(affirmativeButtonText);
    ui->negativePushButton->setText(tr("&Ignore"));

    // set icons
    if (!srcInfo.isDir())
        ui->originalIconLabel->setPixmap(fileIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
    else
        ui->originalIconLabel->setPixmap(folderIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));

    if (!destInfo.isDir())
        ui->replaceIconLabel->setPixmap(fileIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
    else
        ui->replaceIconLabel->setPixmap(folderIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
}

void FileConflictDialog::setupSyncConfirmDialog(QString srcPath, QString destPath)
{
    this->setWindowTitle(tr("Synchronize folders"));

    // get fileInfo for sources/destination
    QFileInfo srcInfo(srcPath);
    QFileInfo destInfo(destPath);

    QString titleText;
    QString informativeText;
    QString originalText, originalExtendedInfoText;
    QString replaceText, replaceExtendedInfoText;
    QString affirmativeButtonText;

    // use FileIcon provider to get file/folder icon
    QIcon folderIcon(QFileIconProvider().icon(QFileIconProvider::Folder));

    // get Last Modified DateTime string for source/destination
    QString srcLastModified = srcInfo.lastModified().toString(tr("MM/dd/yyyy - hh:mm ap")); // french format will be "dd/MM/yyyy - hh:mm"
    QString destLastModified = destInfo.lastModified().toString(tr("MM/dd/yyyy - hh:mm ap")); // french format will be "dd/MM/yyyy - hh:mm"

    titleText = tr("Synchronize") + " \"" + srcInfo.fileName() + "\" " + tr("and") + " \"" + destInfo.fileName() + \
                "\" " + tr("into") + " \"" + destInfo.fileName() + "\" ?";
    informativeText = tr("The 2 folders will be synchronized.") + "\n" + \
        tr("You'll be asked before to replace any file or folder conflicting.");
    originalText = tr("Source folder");
    originalExtendedInfoText = tr("Name:") + " " + srcInfo.fileName() + "\n" + \
                               tr("Last modification:") + " " + srcLastModified;
    replaceText = tr("Destination folder");
    replaceExtendedInfoText = tr("Name:") + " " + destInfo.fileName() + "\n" + \
                              tr("Last modification:") + " " + destLastModified;
    affirmativeButtonText = tr("&Synchronize");

    // set dialogs texts
    ui->titleLabel->setText(titleText);
    ui->informativeLabel->setText(informativeText);
    ui->originalTextLabel->setText(originalText);
    ui->originalExtendedInfoLabel->setText(originalExtendedInfoText);
    ui->replaceTextLabel->setText(replaceText);
    ui->replaceExtendedInfoLabel->setText(replaceExtendedInfoText);
    ui->affirmativePushButton->setText(affirmativeButtonText);

    // set icons
    ui->originalIconLabel->setPixmap(folderIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
    ui->replaceIconLabel->setPixmap(folderIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));

    ui->checkBox->hide();
    ui->negativePushButton->hide();
}

void FileConflictDialog::setupSyncRemoveDialog(QString filePath, QString srcPath)
{
    this->setWindowTitle(tr("File conflict"));

    // get fileInfo for source/destination
    QFileInfo fileInfo(filePath);
    QFileInfo srcInfo(srcPath);

    QString path = srcInfo.absoluteFilePath();
    // just to correct path to windows backslashes type, as
    // path will be emitted in file conflict message
    #if defined (Q_WS_WIN) // Windows system
        path = path.replace('/', QDir::separator());
    #endif
    QString dirName = path.right(path.length() - path.lastIndexOf(QDir::separator()) - 1);

    QString fromPath = fileInfo.absolutePath();
    // just to correct path to windows backslashes type, as
    // path will be emitted in file conflict message
    #if defined (Q_WS_WIN) // Windows system
        fromPath = fromPath.replace('/', QDir::separator());
    #endif
    QString fromDirName = fromPath.right(fromPath.length() - fromPath.lastIndexOf(QDir::separator()) - 1);

    // adjust dialog widgets text, depending if file or directory conflict
    QString titleText;
    QString informativeText;
    QString originalText, originalExtendedInfoText;
    QString replaceText, replaceExtendedInfoText;
    QString checkBoxText;
    QString affirmativeButtonText;

    // use FileIcon provider to get file/folder icon
    QIcon fileIcon(QFileIconProvider().icon(QFileIconProvider::File));
    QIcon folderIcon(QFileIconProvider().icon(QFileIconProvider::Folder));

    // get Last Modified DateTime string for source/destination
    QString fileLastModified = fileInfo.lastModified().toString(tr("MM/dd/yyyy - hh:mm ap")); // french format will be "dd/MM/yyyy - hh:mm"
    QString srcLastModified = srcInfo.lastModified().toString(tr("MM/dd/yyyy - hh:mm ap")); // french format will be "dd/MM/yyyy - hh:mm"

    // if dir
    if (fileInfo.isDir()) {
        titleText = tr("Remove folder") + " \"" + fileInfo.fileName() + "\" ?";
        informativeText = tr("This folder is no longer existing in") + " \"" + dirName + "\".\n" + \
            tr("If you choose to remove it contents will be lost.");
        originalText = tr("Folder");
        originalExtendedInfoText = tr("Name:") + " " + fileInfo.fileName() + "\n" + \
                                   tr("Type: folder") + "\n" + \
                                   tr("Size:") + " -\n" + \
                                   tr("Last modification:") + " " + fileLastModified;
        replaceText = tr("Remove from");
        replaceExtendedInfoText = tr("Name:") + " " + fromDirName + "\n" + \
                                  tr("Type: folder") + "\n" + \
                                  tr("Size:") + " -\n" + \
                                  tr("Last modification:") + " " + srcLastModified;
        checkBoxText = tr("Apply to all conflicts");
        affirmativeButtonText = tr("&Remove");
    }
    else { // if file
        titleText = tr("Remove file") + " \"" + fileInfo.fileName() + "\" ?";
        informativeText = tr("This file is no longer existing in") + " \"" + dirName + "\".\n" + \
            tr("If you choose to remove it you'll be unable to recover it.");
        originalText = tr("File");
        originalExtendedInfoText = tr("Name:") + " " + fileInfo.fileName() + "\n" + \
                                   tr("Type: file") + "\n" + \
                                   tr("Size:") + " " + FileUtils().humanReadableFilesize(fileInfo.size()) + "\n" + \
                                   tr("Last modification:") + " " + fileLastModified;
        replaceText = tr("Remove from");
        replaceExtendedInfoText = tr("Name:") + " " + fromDirName + "\n" + \
                                  tr("Type: folder") + "\n" + \
                                  tr("Size:") + " -\n" + \
                                  tr("Last modification:") + " " + srcLastModified;
        checkBoxText = tr("Apply to all conflicts");
        affirmativeButtonText = tr("&Remove");
    }

    // set dialogs texts
    ui->titleLabel->setText(titleText);
    ui->informativeLabel->setText(informativeText);
    ui->originalTextLabel->setText(originalText);
    ui->originalExtendedInfoLabel->setText(originalExtendedInfoText);
    ui->replaceTextLabel->setText(replaceText);
    ui->replaceExtendedInfoLabel->setText(replaceExtendedInfoText);
    ui->checkBox->setText(checkBoxText);
    ui->affirmativePushButton->setText(affirmativeButtonText);
    ui->negativePushButton->setText(tr("&Ignore"));

    // set icons
    if (!fileInfo.isDir())
        ui->originalIconLabel->setPixmap(fileIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
    else
        ui->originalIconLabel->setPixmap(folderIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));

    ui->replaceIconLabel->setPixmap(folderIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
}

void FileConflictDialog::setupSplitDialog(QString filePath, qint64 newSize)
{
    this->setWindowTitle(tr("File conflict"));

    // get fileInfo for sources/destination
    QFileInfo fileInfo(filePath);

    QString path = fileInfo.absolutePath();
    // just to correct path to windows backslashes type, as
    // path will be emitted in file conflict message
    #if defined (Q_WS_WIN) // Windows system
        path = path.replace('/', QDir::separator());
    #endif
    QString dirName = path.right(path.length() - path.lastIndexOf(QDir::separator()) - 1);

    QString titleText;
    QString informativeText;
    QString originalText, originalExtendedInfoText;
    QString replaceText, replaceExtendedInfoText;
    QString checkBoxText;
    QString affirmativeButtonText;

    // use FileIcon provider to get file/folder icon
    QIcon fileIcon(QFileIconProvider().icon(QFileIconProvider::File));
    QIcon folderIcon(QFileIconProvider().icon(QFileIconProvider::Folder));

    // get Last Modified DateTime string for file
    QString fileLastModified = fileInfo.lastModified().toString(tr("MM/dd/yyyy - hh:mm ap")); // french format will be "dd/MM/yyyy - hh:mm"

    if (fileInfo.isDir()) {
        titleText = tr("Replace folder") + " \"" + fileInfo.fileName() + "\" ?";
        informativeText = tr("A folder with the same name is already existing in") + " \"" + dirName + "\".\n" + \
            tr("If you choose to replace it all files it contains will be replaced.");
        originalText = tr("Original folder");
        originalExtendedInfoText = tr("Type: folder") + "\n" + \
                                   tr("Size:") + " -\n" + \
                                   tr("Last modification:") + " " + fileLastModified;
        replaceText = tr("Replace with");
        replaceExtendedInfoText = tr("Type: file") + "\n" + \
                                  tr("Size:") + " " + FileUtils().humanReadableFilesize(newSize) + "\n" + \
                                  tr("Last modification:") + " -";
        checkBoxText = tr("Apply to all file conflicts");
        affirmativeButtonText = tr("&Replace");
    }
    else {
        titleText = tr("Replace file") + " \"" + fileInfo.fileName() + "\" ?";
        informativeText = tr("A file with the same name is already existing in") + " \"" + dirName + "\".\n" + \
            tr("If you choose to replace it the existing file will be lost.");
        originalText = tr("Original file");
        originalExtendedInfoText = tr("Type: file") + "\n" + \
                                   tr("Size:") + " " + FileUtils().humanReadableFilesize(fileInfo.size()) + "\n" + \
                                   tr("Last modification:") + " " + fileLastModified;
        replaceText = tr("Replace with");
        replaceExtendedInfoText = tr("Type: file") + "\n" + \
                                  tr("Size:") + " " + FileUtils().humanReadableFilesize(newSize) + "\n" + \
                                  tr("Last modification:") + " -";
        checkBoxText = tr("Apply to all file conflicts");
        affirmativeButtonText = tr("&Replace");
    }

    // set dialogs texts
    ui->titleLabel->setText(titleText);
    ui->informativeLabel->setText(informativeText);
    ui->originalTextLabel->setText(originalText);
    ui->originalExtendedInfoLabel->setText(originalExtendedInfoText);
    ui->replaceTextLabel->setText(replaceText);
    ui->replaceExtendedInfoLabel->setText(replaceExtendedInfoText);
    ui->checkBox->setText(checkBoxText);
    ui->affirmativePushButton->setText(affirmativeButtonText);
    ui->negativePushButton->setText(tr("&Ignore"));

    // set icons
    if (!fileInfo.isDir())
        ui->originalIconLabel->setPixmap(fileIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
    else
        ui->originalIconLabel->setPixmap(folderIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
    ui->replaceIconLabel->setPixmap(fileIcon.pixmap(ui->originalIconLabel->width(), ui->originalIconLabel->height()));
}


//
// protected
//

// reimp!
void FileConflictDialog::closeEvent(QCloseEvent *e)
{
    // just ignore close event
    e->ignore();
}


//
// private slots
//

void FileConflictDialog::cancel_Slot()
{
    this->done(cancel);
    this->deleteLater();
}

void FileConflictDialog::negative_Slot()
{
    int result = ignore;
    if (ui->checkBox->isChecked())
        result |= applyToAll;
    this->done(result);
    this->deleteLater();
}

void FileConflictDialog::affirmative_Slot()
{
    int result = confirm;
    if (ui->checkBox->isChecked())
        result |= applyToAll;
    this->done(result);
    this->deleteLater();
}
