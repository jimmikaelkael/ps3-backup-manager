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

#include "ftpconnectdialog.h"
#include "ui_ftpconnectdialog.h"

FtpConnectDialog::FtpConnectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FtpConnectDialog),
    ftpMode(QFtp::Passive)
{
    ui->setupUi(this);

    this->connect(ui->pushButtonCancel, SIGNAL(clicked()), this, SLOT(cancel_Slot()));
    this->connect(ui->pushButtonConnect, SIGNAL(clicked()), this, SLOT(connect_Slot()));

    this->resize(this->width(), 100);
    this->updateGeometry();
}

FtpConnectDialog::~FtpConnectDialog()
{
    delete ui;
}

void FtpConnectDialog::clearContent()
{
    ui->lineEditHost->clear();
    ui->lineEditPort->setText("21");
    ui->lineEditUserName->clear();
    ui->lineEditPassword->clear();
    ui->ftpModeComboBox->setCurrentIndex(0);
}

QString FtpConnectDialog::host()
{
    return ftpHost;
}

qint16 FtpConnectDialog::port()
{
    return ftpPort.toShort();
}

QString FtpConnectDialog::userName()
{
    return ftpUserName;
}

QString FtpConnectDialog::password()
{
    return ftpPassword;
}

QFtp::TransferMode FtpConnectDialog::mode()
{
    return ftpMode;
}

//
// private slots
//

void FtpConnectDialog::cancel_Slot()
{
    clearContent();

    this->reject();
}

void FtpConnectDialog::connect_Slot()
{
    ftpHost = ui->lineEditHost->text().toLower().trimmed();
    ftpPort = ui->lineEditPort->text().trimmed();
    ftpUserName = ui->lineEditUserName->text();
    ftpPassword = ui->lineEditPassword->text();
    if (ui->ftpModeComboBox->currentIndex() == 0)
        ftpMode = QFtp::Passive;
    else
        ftpMode = QFtp::Active;

    this->accept();
}
