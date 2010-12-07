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

#ifndef MYFTPTREEWIDGET_H
#define MYFTPTREEWIDGET_H

#include <QTreeWidget>
#include <QFtp>
#include <QUrl>
#include <QMimeData>
#include <QFileIconProvider>
#include <QTimer>
#include <QHeaderView>
#include <QDropEvent>
#include <QMessageBox>
#include <QDebug>

class AppClipboard;
class FtpConnectDialog;

class MyFtpTreeWidget : public QTreeWidget
{
    Q_OBJECT

public:
    MyFtpTreeWidget(AppClipboard *clipboard, QWidget *parent = 0);
    ~MyFtpTreeWidget();
    void connectFtp();
    void reconnectFtp();
    bool isFtpActive();
    void disconnectFtp();

signals:
    void connecting();
    void connected();
    void disconnected();
    void rootPathChanged(QString path);

private slots:
    void ftpCommandFinished_Slot(int, bool error);
    void ftpDone_Slot(bool error);
    void ftpListInfo_Slot(const QUrlInfo &urlInfo);
    void ftpStateChanged_Slot(int state);
    void ftpConnectTimeout_Slot();
    void ftpDisconnectTimeout_Slot();
    void ftpKeepAlive_Slot();
    void itemActivated_Slot(QTreeWidgetItem *item, int);

private:
    enum FtpTimeout {
        connectionTimeout = 5000
    };

    void connectFtpSignals(const QObject *sender, const QObject *receiver);
    void disconnectFtpSignals(const QObject *sender, const QObject *receiver);
    void addDotDotItem();
    void sort(Qt::SortOrder order = Qt::AscendingOrder);
    void updateView();
    void makeNewDir(QString dirName);
    void renameSelectedItem();
    void removeSelectedItems();
    void ftpConnectFinished(bool error);
    void ftpLoginFinished(bool error);
    void ftpDisconnectFinished(bool error);
    void ftpConnectionLost();

    AppClipboard *appClipboard;
    FtpConnectDialog *ftpConnectDialog;
    QFtp *ftp;
    QTimer *ftpConnectTimeoutTimer, *ftpDisconnectTimeoutTimer, *ftpKeepAliveTimer;
    bool isConnecting, isConnected;
    QString host, userName, password;
    quint16 port;

    QList<QUrlInfo> fileList;
    QHash<QString, bool> isDirectory;
    QString currentPath;

    QFtp::Command signalCmdFinishedMode;

protected slots:
    // reimp!
    void commitData(QWidget *editor);

protected:
    // reimp!
    void dropEvent(QDropEvent *event);
    bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action);
    QMimeData *mimeData(const QList<QTreeWidgetItem *> items) const;
    QStringList mimeTypes() const;
    Qt::DropActions supportedDropActions() const;

};

#endif // MYFTPTREEWIDGET_H
