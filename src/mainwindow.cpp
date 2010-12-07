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

#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "appclipboard.h"
#include "gamelist.h"
#include "gameitem.h"
#include "myfilesystemview.h"
#include "myfilesystemmodel.h"
#include "mygamestreewidget.h"
#include "myftptreewidget.h"
#include "openurlthread.h"
#include "adrefresher.h"
#include "coverdownloader.h"

#include "sfoeditordialog.h"
#include "licensedialog.h"

#define PROGRAM_ORGANIZATION    "Metagames"
#define PROGRAM_NAME            "PS3 Backup Manager"
#define PROGRAM_VER             "0.2.2 beta"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    appClipboard(new AppClipboard),
    gameList(new GameList),
    fileSystemView(new MyFileSystemView(appClipboard, this)),
    coverDownloader(NULL),
    gamesTreeWidget(NULL),
    ftpTreeWidget(new MyFtpTreeWidget(appClipboard, this)),
    ftpPathLineEdit(new QLineEdit(this)),
    adOpenUrlThread(new OpenUrlThread(QUrl("http://www.metashop-fr.com"), this)),
    adRefresher(new AdRefresher(QUrl("http://www.metagames-eu.com/forums/styles/metagames-eu/pub-metagames/meta.txt"), this)),
    licenseIsAccepted(false),
    coverCacheIsEnabled(true)
{
    this->setWindowTitle(tr(PROGRAM_NAME) + " " + PROGRAM_VER);

    coverDownloadBase = "http://www.metagames-eu.com/images/pochettes/";
    coverCacheDir = QDir::current().absolutePath() + QDir::separator() + \
               "cache" + QDir::separator() + "covers";
    coverCacheDir = coverCacheDir + QDir::separator();

    // create a new CoverDownloader object
    coverDownloader = new CoverDownloader(coverDownloadBase, coverCacheDir, this);

    gamesTreeWidget = new MyGamesTreeWidget(appClipboard, gameList, \
                                            qobject_cast<MyFileSystemModel *>(fileSystemView->fileSystemModel()), \
                                            coverDownloadBase, coverCacheDir,
                                            this);

    // setup the main window UI
    ui->setupUi(this);

    // load all UI icons
    loadUiIcons();

    // setup ftpTreeWidget object
    setFtpTreeWidget();

    // connect ad refresh object
    this->connect(adRefresher, SIGNAL(adRefreshed(QImage,QUrl)), this, SLOT(adRefreshed_Slot(QImage,QUrl)));

    // connect all Ui signals to their respective needed slot
    connectUiSignals();

    // connect 'selectionChanged' signal from our fileSystemView to our GamesTreeWidget's refreshList slot
    this->connect(fileSystemView, SIGNAL(selectionChanged(QStringList)), gamesTreeWidget, SLOT(refreshList(QStringList)));

    // connect our gamesTreeWidget signals
    this->connect(gamesTreeWidget, SIGNAL(scanningForGames()), this, SLOT(scanningForGames_Slot()));
    this->connect(gamesTreeWidget, SIGNAL(gameListLoaded()), this, SLOT(gameListLoaded_Slot()));
    this->connect(gamesTreeWidget, SIGNAL(gameListEmpty()), this, SLOT(gameListEmpty_Slot()));
    this->connect(gamesTreeWidget, SIGNAL(gameListItemUpdated(GameItem*)), this, SLOT(gameListItemUpdated_Slot(GameItem*)));

    // add fileSystemView & gamesTreeWidget widgets to UI groupboxes layouts
    ui->groupBoxLeftLayout->addWidget(fileSystemView);
    ui->groupBoxRightLayout->addWidget(gamesTreeWidget);

    // move splitter to show right panel if hidden
    /*QList<int> sl = ui->splitterBottom->sizes();
    sl[0] = 1;
    sl[1] = 0;
    ui->splitterBottom->setSizes(sl);*/

    QList<int> sl = ui->splitter->sizes();
    sl[0] = 400;
    sl[1] = 1;
    ui->splitter->setSizes(sl);

    // acquire saved settings
    readSettings();

    // if license has never been accepted, prompt with license modal dialog
    if (!licenseIsAccepted) {
        LicenseDialog *dialog = new LicenseDialog(this);
        int ret = dialog->exec();
        if (ret != 0)
            licenseIsAccepted = true;
        delete dialog;
    }

    // starting the ad refresher
    adRefresher->start();
}

MainWindow::~MainWindow()
{
    coverDownloader->abort();
    delete coverDownloader;
    delete adRefresher;
    adOpenUrlThread->wait();
    delete adOpenUrlThread;
    delete ftpPathLineEdit;
    delete ftpTreeWidget;
    delete gamesTreeWidget;
    delete fileSystemView;
    delete gameList;
    delete appClipboard;
    delete ui;
}

bool MainWindow::isLicenseAccepted()
{
    return licenseIsAccepted;
}


//
// private slots
//

void MainWindow::quit_Slot()
{
    this->close();
}

void MainWindow::ftpExplorer_Slot()
{
    qDebug() << "FTP explorer requested";

    if (!ftpTreeWidget->isFtpActive())
        ftpTreeWidget->connectFtp();
    else
        ftpTreeWidget->disconnectFtp();
}

void MainWindow::systemTheme_Slot()
{
    // load empty filesheet to restore system defaults
    loadStyleSheet();

    ui->action_SystemTheme->setChecked(true);
    ui->action_MetagamesTheme->setChecked(false);
}

void MainWindow::metagamesTheme_Slot()
{
    // try to load Metagames stylesheet
    if (loadStyleSheet("metagames.qss")) {
        ui->action_SystemTheme->setChecked(false);
        ui->action_MetagamesTheme->setChecked(true);
    }
    else { // we need this if stylesheet load failed as Qt autocheck
        ui->action_SystemTheme->setChecked(true);
        ui->action_MetagamesTheme->setChecked(false);
    }
}

void MainWindow::coverCache_Slot()
{
    if (coverCacheIsEnabled) {
        coverCacheIsEnabled = false;
        gamesTreeWidget->setCoverCachingEnabled(false);
        gamesTreeWidget->abortCoverCaching_Slot();
        ui->action_CoverCache->setText(tr("Enable cover cache"));
    }
    else {
        coverCacheIsEnabled = true;
        gamesTreeWidget->setCoverCachingEnabled(true);
        gamesTreeWidget->startCoverCaching_Slot();
        ui->action_CoverCache->setText(tr("Disable cover cache"));
    }
}

void MainWindow::about_Slot()
{
    QMessageBox mb(QMessageBox::Question, tr(PROGRAM_NAME) + " " + PROGRAM_VER, \
                   QString("Copyright &copy; 2010 - <b>") + PROGRAM_ORGANIZATION +  "</b><br><a href=\"http://www.metagames-eu.com\">http://www.metagames-eu.com</a><br><br>" + \
                   tr("This software was developped with Qt4."), \
                   QMessageBox::Ok, this);
    mb.exec();
}

void MainWindow::clickAd_Slot()
{
    // check OpenUrlThread doesn't still running and start it
    if (!adOpenUrlThread->isRunning())
        adOpenUrlThread->start();
}

void MainWindow::adRefreshed_Slot(QImage image, QUrl url)
{
    //qDebug() << "Ad downloading finished, url=" << url;

    // setting refreshed image to ad pushbutton icon
    QIcon icon(QPixmap::fromImage(image));
    ui->adPushButton->setMaximumSize(image.size());
    ui->adPushButton->setIcon(icon);

    // setting target url for ad Url Opener thread
    adOpenUrlThread->setTargetUrl(url);
}

void MainWindow::scanningForGames_Slot()
{
    ui->statusBar->showMessage(tr("scanning directory for PS3 games..."), 5000);
}

void MainWindow::gameListLoaded_Slot()
{
    ui->statusBar->showMessage(tr("game list is loaded"), 5000);
}

void MainWindow::gameListEmpty_Slot()
{
    ui->statusBar->showMessage(tr("no PS3 game found in this directory..."), 5000);
}

void MainWindow::gameListItemUpdated_Slot(GameItem *item)
{
    //qDebug() << "gameListItemUpdated_Slot";

    this->disconnect(coverDownloader, SIGNAL(coverDownloaded(bool)), this, SLOT(coverDownloaded_Slot(bool)));

    // we adjust printable games fields
    if (item) {
        ui->titleLineEdit->setText(item->title());
        ui->titleIDLineEdit->setText(item->titleID());
        ui->sizeLineEdit->setText(item->totalSize());
        QString path = item->path();
        #if defined (Q_WS_WIN) // Windows system
        path = path.replace('/', QDir::separator());
        #endif
        ui->pathLineEdit->setText(path);
        ui->versionLineEdit->setText(item->version());
        if (item->category() == "DG")
            ui->categoryLineEdit->setText(item->category() + " - Disc Game");
        else if (item->category() == "DM")
            ui->categoryLineEdit->setText(item->category() + " - Disc Movie");
        else if (item->category() == "DP")
            ui->categoryLineEdit->setText(item->category() + " - Disc Package");
        else if (item->category() == "GD")
            ui->categoryLineEdit->setText(item->category() + " - Game Data");
        else if (item->category() == "HG")
            ui->categoryLineEdit->setText(item->category() + " - Hard disk Game");
        else if (item->category() == "IP")
            ui->categoryLineEdit->setText(item->category() + " - Install Package");
        else if (item->category() == "SD")
            ui->categoryLineEdit->setText(item->category() + " - Save Data");
        else
            ui->categoryLineEdit->setText(item->category());
        QImage img(item->icon0Path());
        ui->icon0Label->setPixmap(QPixmap::fromImage(img));

        gamesTreeWidget->refreshItem(item);

        QString gameID = item->titleID();
        if (gameID != coverDownloader->gameID()) {
            ui->gameCoverLabel->setText(tr("Downloading cover..."));
            this->connect(coverDownloader, SIGNAL(coverDownloaded(bool)), this, SLOT(coverDownloaded_Slot(bool)));
            coverDownloader->download(gameID, coverCacheIsEnabled);
        }
        else {
            ui->gameCoverLabel->setPixmap(QPixmap::fromImage(coverDownloader->image()));
        }

        //if (item->isValid())
        //    delete item;
    }
    else {
        //qDebug() << "no valid item pointer!";
        ui->titleLineEdit->clear();
        ui->titleIDLineEdit->clear();
        ui->sizeLineEdit->clear();
        ui->pathLineEdit->clear();
        ui->versionLineEdit->clear();
        ui->categoryLineEdit->clear();
        ui->icon0Label->clear();
        ui->icon0Label->setText(tr("Icon not available..."));
        ui->gameCoverLabel->setText(tr("Cover not available..."));
    }
}

void MainWindow::coverDownloaded_Slot(bool error)
{
    if (!error)
        ui->gameCoverLabel->setPixmap(QPixmap::fromImage(coverDownloader->image()));
    else
        ui->gameCoverLabel->setText(tr("Cover not available..."));
}

void MainWindow::ftpConnecting_Slot()
{
    ui->statusBar->showMessage(tr("connecting to FTP host"), 5000);
}

void MainWindow::ftpConnected_Slot()
{
    ui->statusBar->showMessage(tr("connected to FTP host"), 5000);
    showFtpTreeWidget();
}

void MainWindow::ftpDisconnected_Slot()
{
    ui->statusBar->showMessage(tr("disconnected from FTP host"), 5000);
    hideFtpTreeWidget();
}

void MainWindow::ftpRootPathChanged_Slot(QString path)
{
    if (!path.length())
        ftpPathLineEdit->setText("/");
    else
        ftpPathLineEdit->setText(path);
}

void MainWindow::editPARAMSFO_Slot()
{
    QString path = ui->pathLineEdit->text();
    #if defined (Q_WS_WIN) // Windows system
    path = path.replace(QDir::separator(), '/');
    #endif

    GameItem *gameItem = gameList->itemByPath(path);
    if (gameItem != NULL) {

        SfoEditorDialog *dialog = new SfoEditorDialog(gameItem, this);
        dialog->exec();
        delete dialog;

        gameListItemUpdated_Slot(gameItem);
    }
}

void MainWindow::viewToolBar_Slot()
{
    if (ui->toolBar->isVisible())
        ui->toolBar->setVisible(false);
    else
        ui->toolBar->setVisible(true);
}

void MainWindow::toolBarVisibilityChanged_Slot(bool visible)
{
    if (!visible)
        ui->action_ViewToolBar->setText(tr("View ToolBar"));
    else
        ui->action_ViewToolBar->setText(tr("Hide ToolBar"));
}

void MainWindow::license_Slot()
{
    LicenseDialog *dialog = new LicenseDialog(this);
    dialog->setNoAccept();
    dialog->exec();
    delete dialog;
}


//
// private
//

void MainWindow::writeSettings()
{   // save some app settings, QSettings is used for portability

    QSettings settings(PROGRAM_ORGANIZATION, PROGRAM_NAME);

    settings.beginGroup("License");
    settings.setValue("accepted", true);
    settings.endGroup();

    settings.beginGroup("MainWindow");
    if (ui->action_SystemTheme->isChecked()) {
        settings.setValue("theme", 0);
    }
    else if (ui->action_MetagamesTheme->isChecked()) {
        settings.setValue("theme", 1);
    }
    settings.setValue("size", this->size());
    settings.setValue("pos", this->pos());
    QList<int> sizes = ui->splitterTreeView->sizes();
    settings.setValue("splitterTreeView_size_left", sizes.at(0));
    settings.setValue("splitterTreeView_size_right", sizes.at(1));
    sizes = ui->splitterBottom->sizes();
    settings.setValue("splitterBottom_size_left", sizes.at(0));
    settings.setValue("splitterBottom_size_right", sizes.at(1));
    settings.setValue("state", this->saveState());
    settings.endGroup();

    settings.beginGroup("Cover_Cache");
    settings.setValue("enabled", coverCacheIsEnabled);
    settings.endGroup();
}

void MainWindow::readSettings()
{   // read some app settings, QSettings is used for portability

    QSettings settings(PROGRAM_ORGANIZATION, PROGRAM_NAME);

    settings.beginGroup("License");
    licenseIsAccepted = settings.value("accepted", false).toBool();
    settings.endGroup();

    settings.beginGroup("MainWindow");
    switch (settings.value("theme", 0).toInt()) {
        case 1:
            ui->action_MetagamesTheme->trigger();
            break;
        default:
            ui->action_SystemTheme->trigger();
            break;
    }
    this->resize(settings.value("size", this->size()).toSize());
    this->move(settings.value("pos", this->pos()).toPoint());
    QList<int> sizes;
    sizes << settings.value("splitterTreeView_size_left", 500).toInt();
    sizes << settings.value("splitterTreeView_size_right", 500).toInt();
    ui->splitterTreeView->setSizes(sizes);
    sizes.clear();
    sizes << settings.value("splitterBottom_size_left", 1).toInt();
    sizes << settings.value("splitterBottom_size_right", 1).toInt();
    ui->splitterBottom->setSizes(sizes);
    this->restoreState(settings.value("state", this->saveState()).toByteArray());
    settings.endGroup();

    if (!ui->toolBar->isHidden())
        ui->action_ViewToolBar->setText(tr("Hide ToolBar"));
    else
        ui->action_ViewToolBar->setText(tr("View ToolBar"));

    settings.beginGroup("Cover_Cache");
    coverCacheIsEnabled = settings.value("enabled", true).toBool();
    settings.endGroup();

    gamesTreeWidget->setCoverCachingEnabled(coverCacheIsEnabled);
    if (coverCacheIsEnabled)
        ui->action_CoverCache->setText(tr("Disable cover cache"));
    else
        ui->action_CoverCache->setText(tr("Enable cover cache"));
}

bool MainWindow::loadStyleSheet(QString qssName)
{
    StyleSheet.clear();

    // load stylesheet
    QFile file(QString("styles/" + qssName));
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        while (!in.atEnd())
            StyleSheet = StyleSheet + in.readLine();
        file.close();

        // apply StyleSheet
        this->setStyleSheet(StyleSheet);

        // refresh
        this->update();

        return true;
    }

    return false;
}

void MainWindow::loadStyleSheet()
{
    StyleSheet.clear();

    // reset style sheets
    this->setStyleSheet(StyleSheet);

    // refresh
    this->update();
}

void MainWindow::loadUiIcons()
{
    // load/set all icons for MainWindow UI

    QImage img;
    QIcon icon;

    img.load("images/ftp-explorer.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    ui->action_FTPExplorer->setIcon(icon);
    ui->action_FTPExplorer_TB->setIcon(icon);

    img.load("images/clean.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    ui->action_PS3UPDATPUPCleaner->setIcon(icon);
    ui->action_PS3UPDATPUPCleaner_TB->setIcon(icon);

    img.load("images/split.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    ui->action_4GBFileSplitter->setIcon(icon);
    ui->action_4GBFileSplitter_TB->setIcon(icon);

    img.load("images/txt.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    ui->action_ExportTXT->setIcon(icon);
    ui->action_ExportTXT_TB->setIcon(icon);

    img.load("images/csv.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    ui->action_ExportCSV->setIcon(icon);
    ui->action_ExportCSV_TB->setIcon(icon);

    img.load("images/quit.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    ui->action_Quit->setIcon(icon);
    ui->action_Quit_TB->setIcon(icon);

    img.load("images/about.png");
    icon.addPixmap(QPixmap::fromImage(img), QIcon::Normal, QIcon::On);
    ui->action_About->setIcon(icon);
}

void MainWindow::setFtpTreeWidget()
{
    // create/setup ftpTreeWidget object
    ftpPathLineEdit->setReadOnly(true);
    ui->groupBoxRightLayout->addWidget(ftpPathLineEdit);
    ui->groupBoxRightLayout->addWidget(ftpTreeWidget);
    ftpPathLineEdit->hide();
    ftpTreeWidget->hide();
    this->connect(ftpTreeWidget, SIGNAL(connecting()), this, SLOT(ftpConnecting_Slot()));
    this->connect(ftpTreeWidget, SIGNAL(connected()), this, SLOT(ftpConnected_Slot()));
    this->connect(ftpTreeWidget, SIGNAL(disconnected()), this, SLOT(ftpDisconnected_Slot()));
    this->connect(ftpTreeWidget, SIGNAL(rootPathChanged(QString)), this, SLOT(ftpRootPathChanged_Slot(QString)));
}

void MainWindow::connectUiSignals()
{
    // connect menu/toolbar actions to their slot
    this->connect(ui->action_Quit, SIGNAL(triggered()), this, SLOT(quit_Slot()));
    this->connect(ui->action_Quit_TB, SIGNAL(triggered()), this, SLOT(quit_Slot()));
    this->connect(ui->action_FTPExplorer, SIGNAL(triggered()), this, SLOT(ftpExplorer_Slot()));
    this->connect(ui->action_FTPExplorer_TB, SIGNAL(triggered()), this, SLOT(ftpExplorer_Slot()));
    this->connect(ui->action_PS3UPDATPUPCleaner, SIGNAL(triggered()), fileSystemView, SLOT(pupCleanerAllSubfolders_Slot()));
    this->connect(ui->action_PS3UPDATPUPCleaner_TB, SIGNAL(triggered()), fileSystemView, SLOT(pupCleanerAllSubfolders_Slot()));
    this->connect(ui->action_4GBFileSplitter, SIGNAL(triggered()), fileSystemView, SLOT(split_Slot()));
    this->connect(ui->action_4GBFileSplitter_TB, SIGNAL(triggered()), fileSystemView, SLOT(split_Slot()));
    this->connect(ui->action_ExportTXT, SIGNAL(triggered()), gamesTreeWidget, SLOT(exportText_Slot()));
    this->connect(ui->action_ExportTXT_TB, SIGNAL(triggered()), gamesTreeWidget, SLOT(exportText_Slot()));
    this->connect(ui->action_ExportCSV, SIGNAL(triggered()), gamesTreeWidget, SLOT(exportCSV_Slot()));
    this->connect(ui->action_ExportCSV_TB, SIGNAL(triggered()), gamesTreeWidget, SLOT(exportCSV_Slot()));
    this->connect(ui->action_ViewToolBar, SIGNAL(triggered()), this, SLOT(viewToolBar_Slot()));
    this->connect(ui->action_SystemTheme, SIGNAL(triggered()), this, SLOT(systemTheme_Slot()));
    this->connect(ui->action_MetagamesTheme, SIGNAL(triggered()), this, SLOT(metagamesTheme_Slot()));
    this->connect(ui->action_CoverCache, SIGNAL(triggered()), this, SLOT(coverCache_Slot()));
    this->connect(ui->action_About, SIGNAL(triggered()), this, SLOT(about_Slot()));
    this->connect(ui->action_About_Qt, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    this->connect(ui->action_License, SIGNAL(triggered()), this, SLOT(license_Slot()));

    // connect edit PARAM.SFO push button to his 'clicked' slot
    this->connect(ui->editPARAMSFOPushButton, SIGNAL(clicked()), this, SLOT(editPARAMSFO_Slot()));

    this->connect(ui->toolBar, SIGNAL(visibilityChanged(bool)), this, SLOT(toolBarVisibilityChanged_Slot(bool)));

    // connect ad to his 'clicked' slot
    this->connect(ui->adPushButton, SIGNAL(clicked()), this, SLOT(clickAd_Slot()));
}

void MainWindow::showFtpTreeWidget()
{
    // hide right games tree wigdet
    gamesTreeWidget->hide();

    // change right groupbox title
    ui->groupBoxRight->setTitle(tr("FTP Explorer"));
    ui->action_FTPExplorer->setText(tr("Close FTP Explorer"));
    ui->action_FTPExplorer_TB->setToolTip(tr("Close FTP Explorer"));

    // show the ftpTreeWidget
    ftpTreeWidget->clear();
    ftpTreeWidget->show();
    ftpPathLineEdit->show();

    // move splitter to show right panel if hidden
    QList<int> sl = ui->splitterTreeView->sizes();
    if (sl[1] == 0) {
        sl[0] = 1;
        sl[1] = 1;
        ui->splitterTreeView->setSizes(sl);
    }

    // changes default action of left QTreeView widget
    fileSystemView->setDefaultDropAction(Qt::CopyAction);

    // refresh
    this->update();
}

void MainWindow::hideFtpTreeWidget()
{
    // hide the ftpTreeWidget
    ftpTreeWidget->hide();
    ftpPathLineEdit->hide();

    // change right groupbox title
    ui->groupBoxRight->setTitle(tr("Game List"));
    ui->action_FTPExplorer->setText(tr("Open FTP Explorer"));
    ui->action_FTPExplorer_TB->setToolTip(tr("Open FTP Explorer"));

    // show right games tree widget
    gamesTreeWidget->show();

    // restore default action of left QTreeView widget
    fileSystemView->setDefaultDropAction(Qt::MoveAction);

    // refresh
    this->update();
}


//
// protected
//

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    MyFileSystemModel *fsModel = qobject_cast<MyFileSystemModel *>(fileSystemView->fileSystemModel());

    if ((gamesTreeWidget->activeDialogsCount() > 0) \
        || (fileSystemView->activeDialogsCount() > 0) \
        || (fsModel->activeDialogsCount() > 0)) {
        QMessageBox mb(QMessageBox::Information, tr(PROGRAM_NAME) + " " + PROGRAM_VER, \
                       tr("You can't exit the application while operations are running.\n" \
                       "Please wait for them to be finished before to try to quit again."), \
                       QMessageBox::Ok, this);
        mb.exec();
        e->ignore();
        return;
    }

    // check if an adOpenUrlThread is running and wait it
    if (adOpenUrlThread->isRunning()) {
        adOpenUrlThread->wait();
    }

    // write app settings before to accept close event
    writeSettings();

    // accept the event
    e->accept();
}
