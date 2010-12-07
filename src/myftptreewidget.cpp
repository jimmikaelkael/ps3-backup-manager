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

#include "myftptreewidget.h"

#include "appclipboard.h"
#include "ftpconnectdialog.h"

#include "fileutils.h"

MyFtpTreeWidget::MyFtpTreeWidget(AppClipboard *clipboard, QWidget *parent) :
    QTreeWidget(parent),
    appClipboard(clipboard),
    ftpConnectDialog(new FtpConnectDialog(this)),
    ftp(new QFtp(this)),
    ftpConnectTimeoutTimer(new QTimer(this)),
    ftpDisconnectTimeoutTimer(new QTimer(this)),
    ftpKeepAliveTimer(new QTimer(this)),
    isConnecting(false),
    isConnected(false),
    signalCmdFinishedMode(QFtp::None)
{
    // setting some object properties
    this->setAcceptDrops(true);
    this->setDefaultDropAction(Qt::CopyAction);
    this->setDragDropMode(QTreeWidget::DragDrop);
    this->setDragEnabled(true);
    this->setHeaderLabels(QStringList() << tr("Name") << tr("Size"));
    this->headerItem()->setTextAlignment(1, Qt::AlignRight);
    this->setDropIndicatorShown(true);
    this->setRootIsDecorated(false);
    this->setSelectionMode(QTreeWidget::ExtendedSelection);
    this->setContextMenuPolicy(Qt::CustomContextMenu);
    this->setColumnWidth(0, 360);

    // connect ftp signals
    connectFtpSignals(ftp, this);

    // connect item activated signal
    this->connect(this, SIGNAL(itemActivated(QTreeWidgetItem*,int)), this, SLOT(itemActivated_Slot(QTreeWidgetItem*,int)));

    // connect timer for connection timeout
    this->connect(ftpConnectTimeoutTimer, SIGNAL(timeout()), this, SLOT(ftpConnectTimeout_Slot()));
    this->connect(ftpDisconnectTimeoutTimer, SIGNAL(timeout()), this, SLOT(ftpDisconnectTimeout_Slot()));

    // connect/start timer for keep alive
    this->connect(ftpKeepAliveTimer, SIGNAL(timeout()), this, SLOT(ftpKeepAlive_Slot()));
    //ftpKeepAliveTimer->start(10000);
}

MyFtpTreeWidget::~MyFtpTreeWidget()
{
    delete ftpDisconnectTimeoutTimer;
    delete ftpConnectTimeoutTimer;
    delete ftpKeepAliveTimer;

    ftp->deleteLater();

    delete ftpConnectDialog;
}

void MyFtpTreeWidget::connectFtp()
{
    bool res = ftpConnectDialog->exec();
    isConnected = false;
    if (res) {       
        isConnecting = true;
        emit connecting();

        // set transfer mode
        ftp->setTransferMode(ftpConnectDialog->mode());

        qDebug() << "connect to:" << ftpConnectDialog->host() << "on port" << ftpConnectDialog->port();

        disconnectFtp();

        host = ftpConnectDialog->host();
        port = ftpConnectDialog->port();
        ftp->connectToHost(host, port);

        QString userName = ftpConnectDialog->userName();
        if (!userName.length() > 0)
            userName = "anonymous";

        qDebug() << "login as:" << userName;

        ftp->login(userName, ftpConnectDialog->password());

        // clears current path, file listing, ad '..' as 1st dir
        currentPath.clear();
        fileList.clear();
        addDotDotItem();
        ftp->cd(".");
        // list current directory
        ftp->list();
    }
    else
        isConnecting = false;
}

void MyFtpTreeWidget::reconnectFtp()
{
    isConnecting = true;
    emit connecting();
    isConnected = false;

    qDebug() << "reconnecting to:" << host << "port=" << QString().setNum(port);

    ftp->connectToHost(host, port);

    qDebug() << "relogin as:" << userName;

    ftp->login(userName, password);

    // clears file listing, ad '..' as 1st dir
    fileList.clear();
    addDotDotItem();
    // rechange dir to current path (that was active at disconnect)
    if (currentPath.length() > 0) {
        qDebug() << "cd to:" << currentPath;
        ftp->cd(currentPath);
    }
    // list current directory
    ftp->list();
}

bool MyFtpTreeWidget::isFtpActive()
{
    if (isConnecting || isConnected)
        return true;
    return false;
}

void MyFtpTreeWidget::disconnectFtp()
{
    if (isConnected)
        ftp->close();
}


//
// private slots
//
void MyFtpTreeWidget::ftpCommandFinished_Slot(int, bool error)
{
    if (ftp->currentCommand() == QFtp::ConnectToHost) {
        ftpConnectTimeoutTimer->stop();

        qDebug() << "Ftp connect command done:" << !error;

        if (!error)
            isConnected = true;
        else
            qDebug() << ftp->errorString();

        ftpConnectFinished(error);
    }
    else if (ftp->currentCommand() == QFtp::Close) {
        ftpDisconnectTimeoutTimer->stop();

        qDebug() << "Ftp disconnect command done:" << !error;

        isConnected = false;
        ftpDisconnectFinished(error);
    }
    else if (ftp->currentCommand() == QFtp::Login) {
        qDebug() << "Ftp login command done:" << !error;

        if (error)
            qDebug() << ftp->errorString();

        ftpLoginFinished(error);
    }
    else if (ftp->currentCommand() == QFtp::Cd) {
        qDebug() << "Ftp cd command done:" << !error;

        emit rootPathChanged(currentPath);
    }
    else if (ftp->currentCommand() == QFtp::List) {
        qDebug() << "Ftp list command done:" << !error;

        if (!error)
            updateView();
    }
    else if (ftp->currentCommand() == QFtp::Mkdir) {
        qDebug() << "Ftp mkdir command done:" << !error;
    }
    else if (ftp->currentCommand() == QFtp::Rmdir) {
        qDebug() << "Ftp rmdir command done:" << !error;
    }
    else if (ftp->currentCommand() == QFtp::Rename) {
        qDebug() << "Ftp rename command done:" << !error;
    }
    else if (ftp->currentCommand() == QFtp::Remove) {
        qDebug() << "Ftp remove command done:" << !error;
    }
}

void MyFtpTreeWidget::ftpDone_Slot(bool error)
{
    qDebug() << "Ftp Done=" << !error;

    //if (signalCmdFinishedMode == QFtp::Remove)
        //emit removeFinished(error);

    signalCmdFinishedMode = QFtp::None;
}

void MyFtpTreeWidget::ftpListInfo_Slot(const QUrlInfo &urlInfo)
{
    qDebug() << "Ftp list info: name=" << urlInfo.name() << "size=" << urlInfo.size();

    fileList << urlInfo;
}

void MyFtpTreeWidget::ftpStateChanged_Slot(int state)
{
    if ((state == QFtp::Connecting) && (ftp->currentCommand() == QFtp::ConnectToHost)) {
        // start the connection timeout timer
        ftpConnectTimeoutTimer->start(connectionTimeout);
    }
    else if ((state == QFtp::Closing) && (ftp->currentCommand() == QFtp::Close)) {
        // start the disconnection timeout timer
        ftpDisconnectTimeoutTimer->start(connectionTimeout);
    }
    else if ((state == QFtp::Unconnected) && (ftp->currentCommand() != QFtp::Close) \
        && (ftp->currentCommand() != QFtp::ConnectToHost) \
        && (ftp->currentCommand() != QFtp::Login)) {
        qDebug() << "Ftp connection with host lost...";

        isConnected = false;
        ftpConnectionLost();
    }
}

void MyFtpTreeWidget::ftpConnectTimeout_Slot()
{
    ftpConnectTimeoutTimer->stop();

    qDebug() << "Ftp connection time exceeded!";

    ftp->abort();
    ftp->close();
    disconnectFtpSignals(ftp, this);
    ftp->deleteLater();
    ftp = new QFtp(this);
    ftp->setTransferMode(QFtp::Active);
    connectFtpSignals(ftp, this);

    ftpConnectFinished(true);
}

void MyFtpTreeWidget::ftpDisconnectTimeout_Slot()
{
    ftpDisconnectTimeoutTimer->stop();

    qDebug() << "Ftp disconnection time exceeded!";

    ftp->abort();
    ftp->close();
    disconnectFtpSignals(ftp, this);
    ftp->deleteLater();
    ftp = new QFtp(this);
    ftp->setTransferMode(QFtp::Active);
    connectFtpSignals(ftp, this);

    ftpDisconnectFinished(true);
}

void MyFtpTreeWidget::ftpKeepAlive_Slot()
{
    if ((isConnected) && (ftp->currentCommand() == QFtp::None))
        ftp->cd(".");
}

void MyFtpTreeWidget::itemActivated_Slot(QTreeWidgetItem *item, int)
{
    QString name = item->text(0);
    qDebug() << "item" << name << "activated";
    if (isDirectory.value(name)) {
        if (name == "..")
            currentPath = currentPath.left(currentPath.lastIndexOf('/'));
        else
            currentPath = currentPath + "/" + name;
        qDebug() << "currentPath=" << currentPath;
        fileList.clear();
        addDotDotItem();
        ftp->cd(item->text(0));
        ftp->list();
    }
}

//
// private
//

void MyFtpTreeWidget::connectFtpSignals(const QObject *sender, const QObject *receiver)
{
    // connect all ftp needed signals to their slots
    connect(sender, SIGNAL(commandFinished(int,bool)), receiver, SLOT(ftpCommandFinished_Slot(int,bool)));
    connect(sender, SIGNAL(done(bool)), this, SLOT(ftpDone_Slot(bool)));
    connect(sender, SIGNAL(listInfo(QUrlInfo)), receiver, SLOT(ftpListInfo_Slot(QUrlInfo)));
    connect(sender, SIGNAL(stateChanged(int)), receiver, SLOT(ftpStateChanged_Slot(int)));
}

void MyFtpTreeWidget::disconnectFtpSignals(const QObject *sender, const QObject *receiver)
{
    // disconnect all connected ftp signals from their slots
    disconnect(sender, SIGNAL(commandFinished(int,bool)), receiver, SLOT(ftpCommandFinished_Slot(int,bool)));
    disconnect(sender, SIGNAL(done(bool)), this, SLOT(ftpDone_Slot(bool)));
    disconnect(sender, SIGNAL(listInfo(QUrlInfo)), receiver, SLOT(ftpListInfo_Slot(QUrlInfo)));
    disconnect(sender, SIGNAL(stateChanged(int)), receiver, SLOT(ftpStateChanged_Slot(int)));
}

void MyFtpTreeWidget::addDotDotItem()
{
    QUrlInfo urlInfo;
    urlInfo.setName("..");
    urlInfo.setDir(true);
    fileList << urlInfo;
}

void MyFtpTreeWidget::sort(Qt::SortOrder order)
{
    // this function is designed to perform a dir/file sorting,
    // directories will be 1st in the list and files come after.
    // Each being alphabetically sorted (ascending or descending)

    for (int i=fileList.size(); i>0; --i) {
        for (int j=0; j<(i-1); j++) {
            QString str1 = fileList.at(j).name();
            QString str2 = fileList.at(j+1).name();
            switch (order) {
                case Qt::DescendingOrder:
                    if (str1 < str2)
                        fileList.swap(j, j+1);
                    break;
                default:
                    if (str1 > str2)
                        fileList.swap(j, j+1);
            }
        }
    }
    int j = 0;
    for (int i=fileList.size()-1; i>=j; i--) {
        if (fileList.at(i).isDir()) {
            fileList.move(i, 0);
            j++;
            i++;
        }
    }
}

void MyFtpTreeWidget::updateView()
{
    this->clear();
    isDirectory.clear();

    sort();

    foreach (QUrlInfo urlInfo, fileList) {

        QTreeWidgetItem *item = new QTreeWidgetItem;
        //qDebug() << "adding item=" << urlInfo.name();
        item->setText(0, urlInfo.name());
        if (!urlInfo.isDir()) {
            item->setText(1, FileUtils().humanReadableFilesize(urlInfo.size()));
            item->setTextAlignment(1, Qt::AlignRight);
        }

        QFileIconProvider iconProvider;
        QIcon icon;
        if (urlInfo.isDir())
            icon = iconProvider.icon(QFileIconProvider::Folder);
        else
            icon = iconProvider.icon(QFileIconProvider::File);
        item->setIcon(0, icon);

        isDirectory[urlInfo.name()] = urlInfo.isDir();
        this->addTopLevelItem(item);
    }

    if (!(isDirectory.count() > 1))
        this->addTopLevelItem(new QTreeWidgetItem(QStringList() << "<" + tr("empty") + ">"));

    //this->header()->resizeSections(QHeaderView::ResizeToContents);
}

void MyFtpTreeWidget::makeNewDir(QString dirName)
{
    ftp->mkdir(dirName);
    fileList.clear();
    addDotDotItem();
    // list current directory
    ftp->list();
}

void MyFtpTreeWidget::renameSelectedItem()
{
    QList<QTreeWidgetItem *> items = this->selectedItems();
    if (items.count() == 1) {
        QTreeWidgetItem *item = items[0];
        if (item->text(0) != "..")  {
            Qt::ItemFlags itemFlags = item->flags();
            itemFlags |= Qt::ItemIsEditable;
            item->setFlags(itemFlags);
            this->scrollToItem(item);
            this->editItem(item, 0);
            itemFlags ^= Qt::ItemIsEditable;
            item->setFlags(itemFlags);
        }
    }
}

void MyFtpTreeWidget::removeSelectedItems()
{
    signalCmdFinishedMode = QFtp::Remove;

    QList<QTreeWidgetItem *> items = this->selectedItems();
    foreach(QTreeWidgetItem *item, items) {
        const QString name = item->text(0);
        if (name != "..")  {
            if (isDirectory.value(name)) {
                ftp->rmdir(name);
            }
            else {
                ftp->remove(name);
            }
        }
    }

    fileList.clear();
    addDotDotItem();
    // list current directory
    ftp->list();
}

void MyFtpTreeWidget::ftpConnectFinished(bool error)
{
    if (error) {
        qDebug() << "Ftp connection failed!";

        QMessageBox *mb = new QMessageBox(this);
        mb->setIcon(QMessageBox::Critical);
        mb->setWindowTitle(tr("Error"));
        mb->setText(tr("Can't connect to the FTP host!"));
        mb->setInformativeText(tr("Please check the given hostname."));
        mb->setStandardButtons(QMessageBox::Ok);
        mb->exec();
        delete mb;

        connectFtp();
    }
}

void MyFtpTreeWidget::ftpLoginFinished(bool error)
{
    if (error) {
        qDebug() << "Ftp login failed!";

        QMessageBox *mb = new QMessageBox(this);
        mb->setIcon(QMessageBox::Critical);
        mb->setWindowTitle(tr("Error"));
        mb->setText(tr("Can't login to the FTP host!"));
        mb->setInformativeText(tr("Please check the given username and password."));
        mb->setStandardButtons(QMessageBox::Ok);
        mb->exec();
        delete mb;

        connectFtp();
    }
    else {
        isConnecting = false;
        isConnected = true;
        emit connected();
    }
}

void MyFtpTreeWidget::ftpDisconnectFinished(bool)
{
    isConnected = false;
    emit disconnected();
}

void MyFtpTreeWidget::ftpConnectionLost()
{
    qDebug() << "Ftp connection lost!";

    QMessageBox *mb = new QMessageBox(this);
    mb->setIcon(QMessageBox::Critical);
    mb->setWindowTitle(tr("Error"));
    mb->setText(tr("FTP connection with host has been lost!"));
    mb->setInformativeText(tr("Would you like to reconnect ?"));
    mb->setStandardButtons(QMessageBox::No | QMessageBox::Yes);
    mb->setDefaultButton(QMessageBox::Yes);
    int answer = mb->exec();
    delete mb;

    switch (answer) {
        case QMessageBox::Yes:
            reconnectFtp();
            break;
        case QMessageBox::No:
            // hide ftp tree Widget
            emit disconnected();
            break;
    }
}


//
// protected slots
//

//reimp!
void MyFtpTreeWidget::commitData(QWidget *editor)
{
    qDebug() << "commit data!";

    QList<QTreeWidgetItem *> items = this->selectedItems();
    if (items.count() == 1) {

        QTreeWidgetItem *item = items[0];

        QString oldName = item->text(0);

        QAbstractItemView::commitData(editor);

        QString newName = item->text(0);
        if (oldName != newName) {

            if (newName.contains('/')) {
                item->setText(0, oldName);
                //emit renameFinished(true);
            }
            else {

                qDebug() << "new item text=" << newName;

                ftp->rename(oldName, newName);
                fileList.clear();
                addDotDotItem();
                ftp->list();
            }
        }
    }
}

//
// protected
//

// reimp!
void MyFtpTreeWidget::dropEvent(QDropEvent *event)
{
    // we want dropEvent from QAbstractItemView directly, not QTreeWidget, as
    // QTreeWidget's dropEvent mess up/doesn't support Move Action
    QAbstractItemView::dropEvent(event);
}

// reimp!
bool MyFtpTreeWidget::dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action)
{
    qDebug() << "FtpTreeWidget: dropMimeData() index=" << index;
    if (parent)
        qDebug() << "parent=" << parent->text(0);
    else
        qDebug() << "no parent set, we're dropping on root itself";

    QList<QUrl> urls = data->urls();
    QList<QUrl>::const_iterator it = urls.constBegin();

    switch (action) {
        case Qt::CopyAction:
            for (; it != urls.constEnd(); ++it) {
                QString url = (*it).toString();
                if (url.startsWith("file://"))
                    qDebug() << "dropMimeData remote copy: url=" << url;
                else if (url.startsWith("ftp://"))
                    qDebug() << "dropMimeData local copy: url=" << url;
            }
            break;
        case Qt::MoveAction:
            for (; it != urls.constEnd(); ++it) {
                QString url = (*it).toString();
                if (url.startsWith("file://"))
                    qDebug() << "dropMimeData remote move: url=" << url;
                else if (url.startsWith("ftp://"))
                    qDebug() << "dropMimeData local move: url=" << url;
            }
            break;
        default:
            break;
    }

    // we return false everytime: it's a hack to prevent the
    // QFileSystemModel(used in the consurrent QTreeView) model
    // to move files in drops.

    return false;
}

// reimp!
QMimeData* MyFtpTreeWidget::mimeData(const QList<QTreeWidgetItem *> items) const
{
    qDebug() << "FtpTreeWidget: mimeData()";
    bool dragLocking = false;

    QList<QUrl> urls;
    QList<QTreeWidgetItem *>::const_iterator it = items.begin();

    for (; it != items.end(); ++it) {
        QString fileName = (*it)->text(0);

        if ((fileName == "..") || (fileName == QString("<" + tr("empty") + ">")))
            dragLocking = true;

        QString filePath = "ftp:////" + currentPath + "/" + fileName;
        if (isDirectory.value(fileName))
            filePath.append("/");

        urls << filePath;
    }

    QMimeData *data = new QMimeData();
    if (!dragLocking)
        data->setUrls(urls);

    return data;
}

// reimp!
QStringList MyFtpTreeWidget::mimeTypes() const
{
    return QStringList(QLatin1String("text/uri-list"));
}

//reimp!
Qt::DropActions MyFtpTreeWidget::supportedDropActions() const
{
    return (Qt::CopyAction | Qt::MoveAction);
}
