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

#ifndef CLEANERTHREAD_H
#define CLEANERTHREAD_H

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QDir>
#include <QTime>
#include <QDebug>

#include "myfilesystemmodel.h"

class CleanerThread : public QThread
{
    Q_OBJECT

public:

    enum CleanType {
        cleanNonRecursive,
        cleanRecursive
    };

    CleanerThread(MyFileSystemModel *fsModel, QString path, QString fileName, CleanType type, QObject *parent=0) :
        QThread(parent),
        aborted(false),
        result(false)
    {   // fsModel is required to remove (allow to not have deadlocks on Win32)
        // setting params and connect thread finished signal
        this->fsModel = fsModel;
        this->path = path;
        this->fileName = fileName;
        this->type = type;
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

        result = clean(fsModel, path, fileName, type);

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
    MyFileSystemModel *fsModel;
    bool aborted, result;
    QString initializingMessage;
    QTime initializeTime;
    QMutex mutex;
    QString path, fileName;
    CleanType type;
    qint64 filecount;

    bool checkAborted()
    {   // for convienience, since we need to lock/unlock the mutex each time
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

    qint64 dirFilesCount(QString startPath, CleanType type)
    {
        qint64 count = 0;

        // check source existence
        QDir sourceDir(startPath);

        if (startPath == QDir::separator())
            startPath.clear();

        // count files in dir
        QStringList files = sourceDir.entryList(QDir::Files);
        count += files.count();

        // check elapsed time to flash dots every 500ms
        if (initializeTime.elapsed() > 500) {
            initializingTimeRefresh();
            initializeTime.restart();
        }

        if (type == cleanRecursive) {
            // scan and clean all folders
            files.clear();
            files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks); // we do not follow symlinks !!!
            foreach (QString dir, files) {
                if (checkAborted())
                    return 0;
                QString dirPath = startPath + QDir::separator() + dir;
                count += dirFilesCount(dirPath, type);
            }
        }

        return count;
    }

    bool _clean(MyFileSystemModel *fsModel, QString startPath, QString fileName, CleanType type)
    {
        bool error = false;

        // just to correct path to windows backslashes type, as
        // path will be emitted in progress message
        #if defined (Q_WS_WIN) // Windows system
            startPath = startPath.replace('/', QDir::separator());
        #endif

        // check source existence
        QDir sourceDir(startPath);
        if (!sourceDir.exists())
            return false;

        if (startPath == QDir::separator())
            startPath.clear();

        emit progressUpdated(QString(tr("scanning in") + " " + startPath + QDir::separator()), filecount);

        // scan each file in dir and delete 'fileName' if found
        QStringList files = sourceDir.entryList(QDir::Files);
        foreach (QString file, files) {

            // regular check for abort request
            if (checkAborted())
                return false;

            if (file == fileName) {
                QString filePath = startPath + QDir::separator() + file;
                if (!QFileInfo(filePath).isSymLink()) {
                    qDebug() << "removing" << filePath;
                    bool res = fsModel->remove(fsModel->index(filePath));
                    if (!res)
                        return false;
                }
            }
            filecount++;
        }

        if (type == cleanRecursive) {
            // scan and clean all folders
            files.clear();
            files = sourceDir.entryList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
            foreach (QString dir, files) {

                // regular check for abort request
                if (checkAborted())
                    return false;

                QString dirPath = startPath + QDir::separator() + dir;
                bool res = _clean(fsModel, dirPath, fileName, type);
                if (!res)
                    error = true;
            }
        }

        if (error)
            return false;

        return true;
    }

    bool clean(MyFileSystemModel *fsModel, QString startPath, QString fileName, CleanType type)
    {
        // initialize exec time counter
        QTime executionTime;
        executionTime.start();

        // reset filecount (total files that will be scanned, used
        // as stepping for the progress bar )
        filecount = 0;

        // set up Initialize message and time counter for it
        initializingMessage = QString(tr("cleaner initializing..."));
        initializeTime.restart();

        emit progressUpdated(initializingMessage, 0);
        emit progressRange(dirFilesCount(startPath, type));

        bool res = _clean(fsModel, startPath, fileName, type);

        // check op was aborted or finished
        if (checkAborted())
            emit progressUpdated(QString(tr("scanning aborted...")), filecount);
        else
            emit progressUpdated(QString(tr("scanning done")), filecount);

        return res;
    }
};

#endif // CLEANERTHREAD_H
