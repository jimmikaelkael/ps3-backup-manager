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

#ifndef FTPCONNECTDIALOG_H
#define FTPCONNECTDIALOG_H

#include <QDialog>
#include <QFtp>

namespace Ui {
    class FtpConnectDialog;
}

class FtpConnectDialog : public QDialog
{
    Q_OBJECT

public:
    FtpConnectDialog(QWidget *parent = 0);
    ~FtpConnectDialog();
    void clearContent();
    QString host();
    qint16 port();
    QString userName();
    QString password();
    QFtp::TransferMode mode();

private slots:
    void cancel_Slot();
    void connect_Slot();

private:
    Ui::FtpConnectDialog *ui;
    QString ftpHost;
    QString ftpPort;
    QString ftpUserName;
    QString ftpPassword;
    QFtp::TransferMode ftpMode;

};

#endif // FTPCONNECTDIALOG_H
