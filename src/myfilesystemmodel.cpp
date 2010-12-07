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

#include "myfilesystemmodel.h"

#include "progressdialog.h"

MyFileSystemModel::MyFileSystemModel(QObject *parent) :
    QFileSystemModel(parent),
    modelParent(parent)
{
}

QModelIndex MyFileSystemModel::createNewDirectory(const QModelIndex &parent)
{
    QModelIndex index;

    if (!parent.isValid() || isReadOnly())
        return index;

    QString newDirName = getNewDirectoryName(filePath(parent));

    // create the new dir and get back this Model index
    index = this->mkdir(parent, newDirName);
    return index;
}

// reimp!
bool MyFileSystemModel::dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    QList<QUrl> urls = data->urls();
    if (urls[0].toString().left(6) == "ftp://") {

        qDebug() << "dropMimeData: QMimeData contains FTP url(s)";

        QList<QUrl>::const_iterator it = urls.constBegin();
        switch (action) {
            case Qt::CopyAction:
                for (; it != urls.constEnd(); ++it) {
                    QString url = (*it).toString();
                    qDebug() << "dropMimeData copy: url=" << url;
                }
                return false;
            default:
                return false;
        }
    }

    return dropMimeDataLocal((QMimeData *)data, action, row, column, parent);
}

// reimp!
Qt::DropActions MyFileSystemModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

int MyFileSystemModel::activeDialogsCount()
{
    return activeDialogsList.count();
}


//
// private slots
//

void MyFileSystemModel::progressDialogDone_Slot(ProgressDialog *dialog, bool aborted, bool error)
{
    Q_UNUSED(aborted);
    Q_UNUSED(error);

    for (int i=0; i<activeDialogsList.count(); i++) {
        QDialog *pDialog = activeDialogsList.at(i);
        if (pDialog == dialog)
            activeDialogsList.removeAt(i);
    }

    // to update when a game is copied from left panel to this FileSystemModel
    emit stateChanged();
}

//
// private
//

bool MyFileSystemModel::dropMimeDataLocal(QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
    Q_UNUSED(row);
    Q_UNUSED(column);

    if (!parent.isValid() || isReadOnly())
        return false;

    QString to = filePath(parent) + "/";
    QList<QUrl> urls = data->urls();
    QList<QUrl>::const_iterator it = urls.constBegin();

    MyFileInfoList fileInfoList;
    ProgressDialog *dialog = new ProgressDialog(ProgressDialog::WithProgressBar, qobject_cast<QWidget *>(modelParent));
    this->connect(dialog, SIGNAL(progressDialogDone(ProgressDialog*,bool,bool)), this, SLOT(progressDialogDone_Slot(ProgressDialog*,bool,bool)));

    switch (action) {
    case Qt::CopyAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            MyFileInfo fileInfo(path);
            fileInfo.setDestinationFilePath(to + fileInfo.fileName());

            if (!to.contains(fileInfo.filePath())) {
                QString from = fileInfo.absolutePath() + "/";
                if (to == from) {
                    //qDebug() << "dropping at same path!";
                    if (QFile::exists(path))
                        fileInfo.setDestinationFilePath(getAlternativeFileName(fileInfo));
                }
                fileInfoList << fileInfo;
            }
        }
        if (!fileInfoList.isEmpty()) {
            activeDialogsList << dialog;
            dialog->copy(fileInfoList);
        }
        break;

    case Qt::MoveAction:
        for (; it != urls.constEnd(); ++it) {
            QString path = (*it).toLocalFile();
            MyFileInfo fileInfo(path);
            fileInfo.setDestinationFilePath(to + fileInfo.fileName());

            if (!to.contains(fileInfo.filePath())) {
                QString from = fileInfo.absolutePath() + "/";
                if (to != from)
                    fileInfoList << fileInfo;
            }
        }
        if (!fileInfoList.isEmpty()) {
            activeDialogsList << dialog;
            dialog->move(this, fileInfoList);
        }
        break;
    default:
        return false;
    }

    return true;
}

QString MyFileSystemModel::getAlternativeFileName(MyFileInfo fileInfo)
{
    QString path = fileInfo.absolutePath();
    QString fileName = fileInfo.completeBaseName();
    QString extName = fileInfo.suffix();

    qDebug() << "fileName=" << fileName;
    qDebug() << "extName=" << extName;

    // set the new filename by appending '(copy)' before extension
    QString strAppend = tr("copy");
    QString newFileName = path + "/" + fileName + " (" + strAppend + ")";
    if (extName.length() > 0)
        newFileName = newFileName + "." + extName;
    qDebug() << "newFileName=" << newFileName;

    // if it's existing we need to find one suitable name by
    // appending a number to the new file/dir name
    int number = 1;
    while (((QDir().exists(newFileName)) || (QFile::exists(newFileName)))) {
        qDebug() << "file/directory" << newFileName << "already existing...";
        number++;
        newFileName = path + "/" + fileName + " (" + strAppend + " " + QString().number(number) + ")";
        if (extName.length() > 0)
        newFileName = newFileName + "." + extName;
        //qDebug() << "newFileName=" << newFileName;
    }

    qDebug() << "suitable new file/directory name=" << newFileName;

    return newFileName;
}

QString MyFileSystemModel::getNewDirectoryName(QString path)
{
    QString strNewFolder = tr("new folder");
    QString newDirName = path + "/" + strNewFolder;
    qDebug() << "newDirName=" << newDirName;

    // if it's existing we need to find one suitable name by
    // appending a number to the new file/dir name
    int number = 1;
    while (QDir().exists(newDirName)) {
        qDebug() << "directory" << newDirName << "already existing...";
        number++;
        newDirName = path + "/" + strNewFolder + " " + QString().number(number);
        //qDebug() << "newDirName=" << newDirName;
    }

    qDebug() << "suitable new directory name=" << newDirName;

    return QFileInfo(newDirName).fileName();
}
