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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QCloseEvent>
#include <QSettings>
#include <QImage>
#include <QUrl>
#include <QDir>
#include <QString>
#include <QStringList>
#include <QFileDialog>
#include <QMessageBox>
#include <QLineEdit>
#include <QDesktopWidget>
#include <QIcon>
#include <QPixmap>
#include <QDebug>

namespace Ui {
    class MainWindow;
}

class AppClipboard;
class GameList;
class GameItem;
class MyFileSystemView;
class MyGamesTreeWidget;
class MyFtpTreeWidget;
class OpenUrlThread;
class AdRefresher;
class CoverDownloader;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();
    bool isLicenseAccepted();

private slots:
    void quit_Slot();
    void ftpExplorer_Slot();
    void systemTheme_Slot();
    void metagamesTheme_Slot();
    void coverCache_Slot();
    void about_Slot();
    void clickAd_Slot();
    void adRefreshed_Slot(QImage image, QUrl url);
    void scanningForGames_Slot();
    void gameListLoaded_Slot();
    void gameListEmpty_Slot();
    void gameListItemUpdated_Slot(GameItem *item);
    void coverDownloaded_Slot(bool error);
    void ftpConnecting_Slot();
    void ftpConnected_Slot();
    void ftpDisconnected_Slot();
    void ftpRootPathChanged_Slot(QString path);
    void editPARAMSFO_Slot();
    void viewToolBar_Slot();
    void toolBarVisibilityChanged_Slot(bool visible);
    void license_Slot();

private:
    void writeSettings();
    void readSettings();
    bool loadStyleSheet(QString(qssName));
    void loadStyleSheet();
    void loadUiIcons();
    void setFtpTreeWidget();
    void connectUiSignals();
    void showFtpTreeWidget();
    void hideFtpTreeWidget();

    Ui::MainWindow *ui;
    AppClipboard *appClipboard;
    GameList *gameList;
    MyFileSystemView *fileSystemView;
    CoverDownloader *coverDownloader;
    MyGamesTreeWidget *gamesTreeWidget;
    MyFtpTreeWidget *ftpTreeWidget;
    QLineEdit *ftpPathLineEdit;
    OpenUrlThread *adOpenUrlThread;
    AdRefresher *adRefresher;
    bool licenseIsAccepted;
    bool coverCacheIsEnabled;
    QString coverDownloadBase, coverCacheDir;

    QString StyleSheet;

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

};

#endif // MAINWINDOW_H
