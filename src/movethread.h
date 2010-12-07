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

#ifndef MOVETHREAD_H
#define MOVETHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDir>
#include <QFile>
#include <QAbstractFileEngine>
#include <QFileInfo>
#include <QTime>
#include <QDebug>

#include "myfilesystemmodel.h"
#include "fileutils.h"
#include "myfileinfo.h"

class MoveThread : public QThread
{
    Q_OBJECT

public:

    enum ConflictAction {
        cancel          = 1,
        ignore          = 2,
        confirm         = 4,
        applyToAll      = 8
    };

    MoveThread(MyFileSystemModel *fsModel, MyFileInfoList fileList, QObject *parent=0) :
        QThread(parent),
        aborted(false),
        result(false)
    {
        // setting params and connect thread finished signal
        this->fsModel = fsModel;
        this->fileList = fileList;
        this->connect(this, SIGNAL(finished()), this, SLOT(threadFinished_Slot()));
    }

    void abort()
    {
        mutex.lock();
        qDebug() << "abort request!";
        aborted = true;
        mutex.unlock();
    }

    // reimp!
    void run()
    {
        mutex.lock();
        aborted = false;
        mutex.unlock();

        result = move(fsModel, fileList);

        // quit event loop
        this->quit();
    }

signals:
    void progressRange(qint64 max);
    void finished(bool aborted, bool error);
    void progressUpdated(QString message, qint64 value);
    void fileConflict(QString srcPath, QString destPath);

public slots:

    void fileConflictAction_Slot(ConflictAction action)
    {
        if (action & ignore)
            qDebug() << "request for ignore file conflict action";
        else if (action & confirm)
            qDebug() << "request for confirm file conflict action";
        else
            qDebug() << "request for cancel conflict action";

        if (action & applyToAll)
            qDebug() << "request for applyToAll file conflict action";

        currentConflictAction = action;

        conflictAction.wakeAll();
    }

private slots:

    void threadFinished_Slot()
    {
        mutex.lock();
        bool error = !result;
        if (aborted)
            error = true;

        // emit finished signal
        emit finished(aborted, error);
        mutex.unlock();
    }

private:
    MyFileSystemModel *fsModel;
    bool aborted, result;
    QString initializingMessage;
    QTime initializeTime;
    QMutex mutex;
    QWaitCondition conflictAction;
    MyFileInfoList fileList;
    qint64 progressCount;
    qint64 filesToMove, bytesToMove, bytesMoved;
    ConflictAction currentConflictAction, dirConflictAction, fileConflictAction;

    bool checkAborted()
    {
        // for convienience, since we need to lock/unlock the mutex each time
        mutex.lock();
        bool res = aborted;
        mutex.unlock();

        return res;
    }

    bool moveFileByBlock(const QString &fileName, const QString &newName)
    {
        if (fileName.isEmpty()) {
            qDebug() << "move: Empty or null file name";
            return false;
        }

        if (QFile(newName).exists())
            return false;

        bool error = false;
        QFile in(fileName);
        if (!in.open(QFile::ReadOnly))
            error = true;
        else {
            QFile out(newName);
            if (!out.open(QIODevice::ReadWrite))
                error = true;

            if (error)
                out.close();
            else {
                char block[4096];
                qint64 totalRead = 0;
                int loopCount = 0;

                while (!in.atEnd()) {
                    qint64 inbytes = in.read(block, sizeof(block));
                    if (inbytes <= 0)
                        break;
                    totalRead += inbytes;
                    qint64 outbytes = out.write(block, inbytes);
                    if (inbytes != outbytes)
                        error = true;

                    if (checkAborted())
                        error = true;

                    if (error)
                        break;

                    bytesMoved += outbytes;

                    loopCount++;
                    if (loopCount == 1024) {
                        emit progressUpdated(tr("moving") + " " + QString().number(filesToMove) + " " + tr("file(s)"), bytesMoved);
                        loopCount = 0;
                    }
                }

                if (totalRead != in.size()) {
                    // Unable to read from the source.
                    error = true;
                }
                out.close();

                if (error)
                    out.remove();
            }
            in.close();
        }
        if (!error) {
            QFile::setPermissions(newName, in.permissions());
            return true;
        }

        return false;
    }

    void initializingTimeRefresh()
    {
        // to have 3 flashing dots
        if (initializingMessage.endsWith("..."))
            initializingMessage.replace("...", ".");
        else
            initializingMessage.append(".");

        emit progressUpdated(initializingMessage, 0);
    }

    qint64 dirEntryCount(QString startPath, qint64 *totalSize)
    {
        qint64 count = 0;

        // check source existence
        QDir sourceDir(startPath);

        if (startPath == QDir::separator())
            startPath.clear();

        // count all files from source directory
        QStringList files = sourceDir.entryList(QDir::Files | QDir::Hidden | QDir::System);
        foreach (QString file, files) {
            if (checkAborted())
                return 0;

            QString filePath = startPath + QDir::separator() + file;
            QFileInfo fileInfo(filePath);
            if (!fileInfo.isDir())
                count++;

            if ((fileInfo.isFile()) && (!fileInfo.isSymLink()))
                *totalSize = *totalSize + fileInfo.size();
        }

        // check elapsed time to flash dots every 500ms
        if (initializeTime.elapsed() > 500) {
            initializingTimeRefresh();
            initializeTime.restart();
        }

        // count all dirs in subfolder from directory
        files.clear();
        files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (QString dir, files) {
            if (checkAborted())
                return 0;
            QString dirPath = startPath + QDir::separator() + dir;
            QFileInfo dirInfo(dirPath);
            if (dirInfo.isDir()) {
                if (!dirInfo.isSymLink())
                    count += dirEntryCount(dirPath, totalSize);
                count ++;
            }
        }

        return count;
    }

    int moveFile(MyFileSystemModel *fsModel, QString src, QString dest)
    {   // handle move of a single file

        emit progressUpdated(tr("moving") + " " + QString().number(filesToMove) + " " + tr("file(s)"), bytesMoved);

        QFileInfo srcInfo(src);

        if (!srcInfo.isDir()) {
            if ((QFile::exists(dest)) && (!(fileConflictAction & applyToAll))) {
                emit fileConflict(src, dest);
                conflictAction.wait(&mutex);

                fileConflictAction = currentConflictAction;

                if (fileConflictAction & cancel) {
                    abort();
                    return 0;
                }
            }

            bool res = true;
            if (fileConflictAction & confirm) {
                //qDebug() << "moving" << src << "to" << dest;

                if (QFile::exists(dest))
                    res = QFile::remove(dest);

                if (res) {
                    res = false;
                    bool removeFlag = true;
                    if (!srcInfo.isSymLink()) {
                        qint64 fileSize = srcInfo.size();
                        FileUtils().setFileWritable(src);
                        // attempt a rename by QAbstractFileEngine
                        res = QFile(src).fileEngine()->rename(dest);
                        if (!res) { // if QAbstractFileEngine failed to rename, move file
                            res = moveFileByBlock(src, dest);
                        }
                        else {
                            removeFlag = false;
                            bytesMoved += fileSize;
                        }
                    }
                    else { // preserves symlinks
                        QFile::remove(dest);
                        res = QFile::link(srcInfo.symLinkTarget(), dest);
                    }

                    if ((removeFlag) && (res)) {
                        qDebug() << "removing" << src;
                        FileUtils().setFileWritable(src);
                        if (fsModel->index(src).isValid())
                            res = fsModel->remove(fsModel->index(src));
                        else
                            res = QFile::remove(src);
                    }
                }
            }
            filesToMove--;
            progressCount++;

            if (res)
                return 1;
            else
                qDebug() << "failed copy of" << src << "to" << dest;
        }

        return 0;
    }

    qint64 moveDir(MyFileSystemModel *fsModel, QString src, QString dest)
    {   // handle move of a single dir

        emit progressUpdated(tr("moving") + " " + QString().number(filesToMove) + " " + tr("file(s)"), bytesMoved);

        qint64 count = 0;
        QFileInfo srcInfo(src);

        if (srcInfo.isDir()) {

            if ((QDir().exists(dest)) && (!(dirConflictAction & applyToAll))) {
                emit fileConflict(src, dest);
                conflictAction.wait(&mutex);

                dirConflictAction = currentConflictAction;

                if (dirConflictAction & cancel) {
                    abort();
                    return 0;
                }
            }

            if (dirConflictAction & confirm) {
                qDebug() << "moving dir" << src << "to" << dest;

                if (!srcInfo.isSymLink()) {
                    count += makeDirMove(fsModel, src, dest);
                    qDebug() << "removing dir" << src;
                    bool res = false;
                    if (fsModel->index(src).isValid())
                        res = fsModel->remove(fsModel->index(src));
                    else
                        res = QDir().rmdir(src);
                    if (res)
                        count++;
                }
                else { // preserves symlinks
                    bool res = true;
                    //if (QFile::exists(dest))
                    //    res = QFile::remove(dest);
                    QFile::remove(dest);
                    //if (res)
                        res = QFile::link(srcInfo.symLinkTarget(), dest);

                        if (res) {
                            if (fsModel->index(src).isValid())
                                res = fsModel->remove(fsModel->index(src));
                            else
                                res = QFile::remove(src);
                        }

                    if (res)
                        count++;
                    else
                        qDebug() << "failed move of dir symlink" << src << "to" << dest;
                }
            }
            filesToMove--;
            progressCount++;
        }

        return count;
    }

    qint64 makeDirMove(MyFileSystemModel *fsModel, QString source, QString dest)
    {
        qint64 count = 0;

        // check source existence
        QDir sourceDir(source);
        if (!sourceDir.exists())
            return count;

        // if destination directory doesn't exists, create it
        QDir destDir(dest);
        if(!destDir.exists()) {
            bool res = destDir.mkdir(dest);
            if (!res)
                return count;
        }

        if (source == QDir::separator())
            source.clear();

        // list and move all files from source directory
        QStringList files = sourceDir.entryList(QDir::Files | QDir::Hidden | QDir::System);
        foreach (QString file, files) {
            if (checkAborted())
                return count;

            QString srcName = source + QDir::separator() + file;
            QString destName = dest + QDir::separator() + file;

            count += moveFile(fsModel, srcName, destName);
        }

        // list and move all folders/files from source directory
        files.clear();
        files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (QString dir, files) {
            if (checkAborted())
                return count;

            QString srcName = source + QDir::separator() + dir;
            QString destName = dest + QDir::separator() + dir;

            count += moveDir(fsModel, srcName, destName);
        }

        return count;
    }

    bool move(MyFileSystemModel *fsModel, MyFileInfoList fileInfoList)
    {
        // start exec time counter
        QTime executionTime;
        executionTime.start();

        // reset progressCount (files that are moved, used
        // as stepping for the progress bar
        progressCount = 0;

        // set a default dir/file conflict action which is replace
        dirConflictAction = fileConflictAction = confirm;

        // set up Initialize message and time counter for it
        initializingMessage = QString(tr("preparing to move..."));
        initializeTime.restart();

        emit progressUpdated(initializingMessage, 0);

        // set filesToMove (total files that needs to be moved, used
        // as stepping for the progress bar)
        filesToMove = 0;
        bytesToMove = 0;
        bytesMoved = 0;
        foreach (MyFileInfo fileInfo, fileInfoList) {

            // just to correct path to windows backslashes type, as
            // path will be emitted in file conflict message
            #if defined (Q_WS_WIN) // Windows system
                fileInfo.setDestinationFilePath(fileInfo.destinationFilePath().replace('/', QDir::separator()));
            #endif

            if ((fileInfo.isDir()) && (!fileInfo.isSymLink()))
                filesToMove += dirEntryCount(fileInfo.absoluteFilePath(), &bytesToMove);
            filesToMove++;
            if ((fileInfo.isFile())  && (!fileInfo.isSymLink()))
                bytesToMove += fileInfo.size();
        }
        qDebug() << "bytesToMove=" << bytesToMove;
        emit progressRange(bytesToMove);

        // keep the number for result comparison at the end of this function
        qint64 totalFilesToMove = filesToMove;
        qDebug() << "filesToMove=" << filesToMove;

        // loop on each fileInfo passed in the list
        qint64 count = 0;
        foreach (MyFileInfo fileInfo, fileInfoList) {
            // if it's a dir we make dir copy
            if (fileInfo.isDir())
                count += moveDir(fsModel, fileInfo.absoluteFilePath(), fileInfo.destinationFilePath());
            else // simple file copy
                count += moveFile(fsModel, fileInfo.absoluteFilePath(), fileInfo.destinationFilePath());

            if (checkAborted())
                break;
        }

        qDebug() << "progressCount=" << progressCount;
        qDebug() << "filesToMove=" << filesToMove;
        qDebug() << "bytesMoved=" << bytesMoved;
        qDebug() << "count=" << count;

        // check if op was aborted
        if (checkAborted())
            emit progressUpdated(QString(tr("file moving aborted...")), bytesMoved);
        else
            emit progressUpdated(QString(tr("file moving finished")), bytesMoved);

        // check if all files moved as expected
        if (count != totalFilesToMove)
            return false;

        return true;
    }
};

#endif // MOVETHREAD_H
