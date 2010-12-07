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

#ifndef REMOVETHREAD_H
#define REMOVETHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTime>
#include <QDebug>

#include "myfilesystemmodel.h"
#include "fileutils.h"

class RemoveThread : public QThread
{
    Q_OBJECT

public:

    RemoveThread(MyFileSystemModel *fsModel, QFileInfoList fileList, QObject *parent=0) :
        QThread(parent),
        aborted(false),
        result(false)
    {   // fsModel is required to remove (allow to not have deadlocks on Win32)
        // setting params and connect thread finished signal
        this->fsModel = fsModel;
        this->fileList = fileList;
        this->connect(this, SIGNAL(finished()), this, SLOT(threadFinished_Slot()));
    }

    void abort()
    {
        mutex.lock();
        aborted = true;
        mutex.unlock();
    }

    // reimp!
    void run()
    {
        mutex.lock();
        aborted = false;
        mutex.unlock();

        result = remove(fsModel, fileList);

        // quit event loop
        this->quit();
    }

signals:
    void progressRange(qint64 max);
    void finished(bool aborted, bool error);
    void progressUpdated(QString message, qint64 value);

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
    bool aborted, result;
    MyFileSystemModel *fsModel;
    QString initializingMessage;
    QTime initializeTime;
    QMutex mutex;
    QFileInfoList fileList;
    qint64 progressCount;
    qint64 filesToRemove;

    bool checkAborted()
    {
        // for convienience, since we need to lock/unlock the mutex each time
        mutex.lock();
        bool res = aborted;
        mutex.unlock();

        return res;
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

    qint64 dirEntryCount(QString startPath)
    {
        qint64 count = 0;

        // check source existence
        QDir sourceDir(startPath);

        if (startPath == QDir::separator())
            startPath.clear();

        QStringList files = sourceDir.entryList(QDir::Files | QDir::Hidden | QDir::System);
        foreach (QString file, files) {
            if (checkAborted())
                return 0;

            QString filePath = startPath + QDir::separator() + file;
            if (!QFileInfo(filePath).isDir())
                count++;
        }

        // to keep tree dots flashing very 500ms
        if (initializeTime.elapsed() > 500) {
            initializingTimeRefresh();
            initializeTime.restart();
        }

        /*
        files.clear();
        files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks | QDir::Hidden); // no follow symlinks!!!
        foreach (QString dir, files) {
            if (checkAborted())
                return 0;
            QString dirPath = startPath + QDir::separator() + dir;
            if (QFileInfo(dirPath).isDir())
                count += dirEntryCount(dirPath);
        }*/

        files.clear();
        files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System);
        foreach (QString dir, files) {
            if (checkAborted())
                return 0;
            QString dirPath = startPath + QDir::separator() + dir;

            if (QFileInfo(dirPath).isDir()) {
                if (!QFileInfo(dirPath).isSymLink())
                    count += dirEntryCount(dirPath);
                count ++;
            }
        }

        return count;
    }

    qint64 makeDirEmpty(MyFileSystemModel *fsModel, QString startPath)
    {
        qint64 count = 0;

        //#if defined (Q_WS_WIN) // Windows system
        //    startPath = startPath.replace('/', QDir::separator());
        //#endif

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

            emit progressUpdated(tr("removing") + " " + QString().number(filesToRemove) + " " + tr("file(s)"), progressCount);

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
                filesToRemove--;
                progressCount++;
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

            filesToRemove--;
            progressCount++;
        }

        return count;
    }

    bool remove(MyFileSystemModel *fsModel, QFileInfoList fileInfoList)
    {
        QTime executionTime;
        executionTime.start();

        progressCount = 0;

        initializingMessage = QString(tr("preparing to remove..."));
        initializeTime.restart();
        emit progressUpdated(initializingMessage, 0);
        filesToRemove = 0;
        foreach (QFileInfo fileInfo, fileInfoList) {
            if ((fileInfo.isDir()) && (!fileInfo.isSymLink()))
                filesToRemove += dirEntryCount(fileInfo.absoluteFilePath());
            filesToRemove++;
        }
        emit progressRange(filesToRemove);
        qint64 totalFilesToRemove = filesToRemove;
        qDebug() << "filesToRemove=" << filesToRemove;

        qint64 count = 0;
        foreach (QFileInfo fileInfo, fileInfoList) {

            bool res = false;

            if (fileInfo.isDir()) {
                //qDebug() << "removing dir." << fileInfo.absoluteFilePath();
                if (!fileInfo.isSymLink())
                    count += makeDirEmpty(fsModel, fileInfo.absoluteFilePath());

                if (fileInfo.isSymLink()) {
                    FileUtils().setFileWritable(fileInfo.absoluteFilePath());
                    res = QFile::remove(fileInfo.absoluteFilePath());
                }
                else
                    //res = fsModel->remove(fsModel->index(fileInfo.absoluteFilePath()));
                    res = fsModel->remove(fsModel->index(fileInfo.absoluteFilePath()));
                    //res = QDir().rmdir(fileInfo.absoluteFilePath());
            }
            else {
                emit progressUpdated(tr("removing") + " " + QString().number(filesToRemove) + " " + tr("file(s)"), progressCount);
                //qDebug() << "removing" << fileInfo.absoluteFilePath();
                FileUtils().setFileWritable(fileInfo.absoluteFilePath());
                res = fsModel->remove(fsModel->index(fileInfo.absoluteFilePath()));
                //res = QFile::remove(fileInfo.absoluteFilePath());

            }
            if (res)
                count++;
            filesToRemove--;
            progressCount++;
        }

        qDebug() << "progressCount=" << progressCount;
        qDebug() << "filesToRemove=" << filesToRemove;
        qDebug() << "count=" << count;

        if (checkAborted())
            emit progressUpdated(QString(tr("file removing aborted...")), progressCount);
        else
            emit progressUpdated(QString(tr("file removing finished")), progressCount);

        if (count != totalFilesToRemove)
            return false;

        return true;
    }
};

#endif // REMOVETHREAD_H
