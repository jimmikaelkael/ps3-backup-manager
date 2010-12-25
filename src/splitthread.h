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

#ifndef SPLITTHREAD_H
#define SPLITTHREAD_H

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

class SplitThread : public QThread
{
    Q_OBJECT

public:

    enum SplitType {
        splitNonRecursive,
        splitRecursive
    };

    enum ConflictAction {
        cancel          = 1,
        ignore          = 2,
        confirm         = 4,
        applyToAll      = 8
    };

    SplitThread(MyFileSystemModel *fsModel, QString startPath, SplitType type, QObject *parent=0) :
        QThread(parent),
        aborted(false),
        result(false)
    {
        // setting params and connect thread finished signal
        this->fsModel = fsModel;
        this->startPath = startPath;
        this->type = type;
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

        result = split(fsModel, startPath, type);

        // quit event loop
        this->quit();
    }

signals:
    void progressRange(qint64 max);
    void finished(bool aborted, bool error);
    void progressUpdated(QString message, qint64 value);
    void fileConflict(QString filePath, qint64 newSize);
    void notEnoughDiskSpace();

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
        qDebug() << "thread finished!";
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
    QString startPath;
    SplitType type;
    qint64 filesToSplit, bytesToWrite, bytesWritten;
    ConflictAction currentConflictAction, fileConflictAction;

    bool checkAborted()
    {
        // for convienience, since we need to lock/unlock the mutex each time
        mutex.lock();
        bool res = aborted;
        mutex.unlock();

        return res;
    }

    bool splitFileByBlock(const QString &fileName, const QString &newName, qint64 startOffset, qint64 maxSize)
    {
        if (fileName.isEmpty()) {
            qDebug() << "copy: Empty or null file name";
            return false;
        }

        if (QFile(newName).exists())
            return false;

        bool error = false;
        QFile in(fileName);
        if (!in.open(QFile::ReadOnly))
            error = true;
        else {
            in.seek(startOffset);

            QFile out(newName);
            if (!out.open(QIODevice::ReadWrite))
                error = true;

            if (error)
                out.close();
            else {
                char block[4096];
                qint64 totalRead = 0;
                int loopCount = 0;

                while (totalRead < maxSize) {
                    int blocksize = sizeof(block);
                    if ((maxSize - totalRead) < sizeof(block))
                        blocksize = (maxSize - totalRead);

                    qint64 inbytes = in.read(block, blocksize);
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

                    bytesWritten += outbytes;

                    loopCount++;
                    if (loopCount == 1024) {
                        emit progressUpdated(tr("splitting") + " " + QString().number(filesToSplit) + " " + tr("file(s)"), bytesWritten);
                        loopCount = 0;
                    }
                }

                if (totalRead != maxSize) {
                    // Unable to read properly from the source.
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

    qint64 splitCount(QString startPath, SplitType type)
    {
        qint64 count = 0;

        // check source existence
        QDir sourceDir(startPath);

        if (startPath == QDir::separator())
            startPath.clear();

        // count files in dir
        QStringList files = sourceDir.entryList(QDir::Files);
        foreach (QString file, files) {
            if (checkAborted())
                return 0;

            MyFileInfo fileInfo(startPath + QDir::separator() + file);
            if (!fileInfo.isSymLink()) {
                if (fileInfo.isFileGreaterThan4GB()) {
                    qDebug() << "file" << fileInfo.absoluteFilePath() << "is greater than 4GB";
                    int parts = 0;
                    qint64 size = fileInfo.size();
                    while (size > 0) {
                        size -= 4294967295LL;
                        parts++;
                    }
                    qDebug() << "needed parts=" << parts;
                    count++;
                    bytesToWrite += fileInfo.size() - 4294967295LL;
                }
            }
        }

        // check elapsed time to flash dots every 500ms
        if (initializeTime.elapsed() > 500) {
            initializingTimeRefresh();
            initializeTime.restart();
        }

        if (type == splitRecursive) {
            // scan and clean all folders
            files.clear();
            files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks); // we do not follow symlinks !!!
            foreach (QString dir, files) {
                if (checkAborted())
                    return 0;
                QString dirPath = startPath + QDir::separator() + dir;
                count += splitCount(dirPath, type);
            }
        }

        return count;
    }

    qint64 makeDirEmpty(MyFileSystemModel *fsModel, QString startPath)
    {
        qint64 count = 0;

        // check source existence
        QDir sourceDir(startPath);
        if (!sourceDir.exists())
            return count;

        if (startPath == QDir::separator())
            startPath.clear();

        // scan each file in dir and delete 'fileName' if found
        QStringList files = sourceDir.entryList(QDir::Files | QDir::Hidden | QDir::System);
        foreach (QString file, files) {
            if (checkAborted())
                return count;

            QString filePath = startPath + QDir::separator() + file;
            //if ((!QFileInfo(filePath).isDir()) && (!QFileInfo(filePath).isSymLink())) {
            if (!QFileInfo(filePath).isDir()) {
                //qDebug() << "removing" << filePath;
                bool res = false;
                FileUtils().setFileWritable(filePath);
                if (fsModel->index(filePath).isValid())
                    res = fsModel->remove(fsModel->index(filePath));
                else
                    res = QFile::remove(filePath);
                if (res)
                    count++;
            }
        }

        // scan and clean all folders
        files.clear();
        files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden | QDir::System);
        foreach (QString dir, files) {
            if (checkAborted())
                return count;
            QString dirPath = startPath + QDir::separator() + dir;
            //qDebug() << "clearing dir" << dirPath << "count=" << count;
            if (!QFileInfo(dirPath).isSymLink())
                count += makeDirEmpty(fsModel, dirPath);
        }

        files.clear();
        files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (QString dir, files) {
            if (checkAborted())
                return count;

            QString dirPath = startPath + QDir::separator() + dir;
            //qDebug() << "removing dir" << dirPath;
            bool res = false;
            if (QFileInfo(dirPath).isSymLink()) {
                FileUtils().setFileWritable(dirPath);
                res = QFile::remove(dirPath);
            }
            else {
                count += makeDirEmpty(fsModel, dirPath);
                if (fsModel->index(dirPath).isValid())
                    res = fsModel->remove(fsModel->index(dirPath));
                else
                    res = QDir().rmdir(dirPath);
            }
            if (res)
                count++;
        }

        return count;
    }

    qint64 splitFiles(MyFileSystemModel *fsModel, QString startPath, SplitType type)
    {
        qint64 count = 0;

        // check source existence
        QDir sourceDir(startPath);

        if (startPath == QDir::separator())
            startPath.clear();

        // count files in dir
        QStringList files = sourceDir.entryList(QDir::Files);
        foreach (QString file, files) {
            if (checkAborted())
                return count;

            MyFileInfo fileInfo(startPath + QDir::separator() + file);
            if (!fileInfo.isSymLink()) {
                if (fileInfo.isFileGreaterThan4GB()) {
                    qDebug() << "file" << fileInfo.absoluteFilePath() << "is greater than 4GB";
                    int parts = 0;
                    qint64 size = fileInfo.size();
                    while (size > 0) {
                        size -= 4294967295LL;
                        parts++;
                    }
                    qDebug() << "needed parts=" << parts;

                    size = fileInfo.size();
                    //for (int i=1; i<parts; i++) {
                    bool res = true;
                    for (int i=parts-1; i>=1; i--) {
                        //size -= 4294967296LL;
                        //qint64 maxSize = 4294967296LL;
                        qint64 maxSize = size % 4294967295LL;
                        if (maxSize == 0)
                            maxSize = 4294967295LL;
                        //if (i == (parts-1))
                        //    maxSize = size;

                        qint64 total, free;
                        bool success = FileUtils().getFreeTotalSpace(fileInfo.absolutePath(), &total, &free);
                        if (success) {
                            qDebug() << "total=" << total << "free=" << free;
                            if (free < ((maxSize/1024) + 100*1024)) {
                                emit notEnoughDiskSpace();
                                return 0;
                            }
                        }

                        QString splitFileName = startPath + QDir::separator() + fileInfo.fileName() + "." + QString::number(i) + ".part";

                        if ((QFile::exists(splitFileName)) && (!(fileConflictAction & applyToAll))) {
                            emit fileConflict(splitFileName, maxSize);
                            conflictAction.wait(&mutex);

                            fileConflictAction = currentConflictAction;

                            if (fileConflictAction & cancel) {
                                abort();
                                return 0;
                            }
                        }

                        if (fileConflictAction & confirm) {

                            if (QFile::exists(splitFileName)) {

                                QFileInfo splitFileInfo(splitFileName);
                                if ((splitFileInfo.isDir()) && (!splitFileInfo.isSymLink())) {
                                    makeDirEmpty(fsModel, splitFileName);
                                    if (fsModel->index(splitFileName).isValid())
                                        res = fsModel->remove(fsModel->index(splitFileName));
                                    else
                                        res = QDir().rmdir(splitFileName);
                                }
                                else {
                                    if (fsModel->index(splitFileName).isValid())
                                        res = fsModel->remove(fsModel->index(splitFileName));
                                    else
                                        res = QFile::remove(splitFileName);
                                }
                            }

                            qDebug() << "split file=" << splitFileName << "size=" << maxSize;
                            res = splitFileByBlock(fileInfo.absoluteFilePath(), splitFileName, i * 4294967295LL, maxSize);

                            size -= maxSize;
                        }
                        if (!res)
                            return 0;
                    }
                    if (res) {
                        QFile(fileInfo.absoluteFilePath()).resize(4294967295LL);
                        count++;
                        filesToSplit--;
                    }
                }
            }
        }

        if (type == splitRecursive) {
            // scan in all folders
            files.clear();
            files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks); // we do not follow symlinks !!!
            foreach (QString dir, files) {
                if (checkAborted())
                    return count;
                QString dirPath = startPath + QDir::separator() + dir;
                count += splitFiles(fsModel, dirPath, type);
            }
        }

        return count;
    }

    bool split(MyFileSystemModel *fsModel, QString startPath, SplitType type)
    {
        // start exec time counter
        QTime executionTime;
        executionTime.start();

        // set a default file conflict action which is replace
        fileConflictAction = confirm;

        // set up Initialize message and time counter for it
        initializingMessage = QString(tr("preparing to split..."));
        initializeTime.restart();

        emit progressUpdated(initializingMessage, 0);

        // set filesToSplit (total files that needs to be written, used
        // as stepping for the progress bar)
        filesToSplit = 0;
        bytesToWrite = 0;
        bytesWritten = 0;

        filesToSplit = splitCount(startPath, type);

        // keep the number for result comparison at the end of this function
        qint64 totalFilesToSplit = filesToSplit;
        qDebug() << "filesToSplit=" << filesToSplit;

        qint64 count = 0;

        if (filesToSplit > 0) {

            qDebug() << "bytesToWrite=" << bytesToWrite;

            emit progressRange(bytesToWrite);

            // just to correct path to windows backslashes type, as
            // path will be emitted in file conflict message
            #if defined (Q_WS_WIN) // Windows system
                startPath = startPath.replace('/', QDir::separator());
            #endif

            count = splitFiles(fsModel, startPath, type);

            qDebug() << "filesToSplit=" << filesToSplit;
            qDebug() << "bytesWritten=" << bytesWritten;
            qDebug() << "count=" << count;
        }
        else {
            // to keep progress bar going
            emit progressRange(1);
            bytesWritten = 1;
        }

        // check if op was aborted
        if (checkAborted())
            emit progressUpdated(QString(tr("file splitting aborted...")), bytesWritten);
        else
            emit progressUpdated(QString(tr("file splitting finished")), bytesWritten);

        // check if all files moved as expected
        if (count != totalFilesToSplit)
            return false;

        return true;
    }

};

#endif // SPLITTHREAD_H
