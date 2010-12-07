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

#include "licensedialog.h"
#include "ui_licensedialog.h"

LicenseDialog::LicenseDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LicenseDialog),
    filterCloseEvent(true)
{
    ui->setupUi(this);

    this->connect(ui->okPushButton, SIGNAL(clicked()), this, SLOT(ok_Slot()));
}

LicenseDialog::~LicenseDialog()
{
    delete ui;
}

void LicenseDialog::setNoAccept()
{
    ui->acceptCheckBox->setHidden(true);
    ui->licenseLabel->setText(tr("This software is licensed under GNU GPL v3:"));
    filterCloseEvent = false;
}

// reimp!
void LicenseDialog::closeEvent(QCloseEvent *e)
{
    if (filterCloseEvent)
        // just ignore close event
        e->ignore();
    else
        e->accept();
}

//
// private slots
//

void LicenseDialog::ok_Slot()
{
    if (ui->acceptCheckBox->isChecked())
        this->accept();
    else
        this->reject();

    this->deleteLater();
}
