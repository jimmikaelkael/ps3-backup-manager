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

#ifndef LICENSEDIALOG_H
#define LICENSEDIALOG_H

#include <QDialog>
#include <QCloseEvent>

namespace Ui {
    class LicenseDialog;
}

class LicenseDialog : public QDialog
{
    Q_OBJECT

public:
    LicenseDialog(QWidget *parent = 0);
    ~LicenseDialog();
    void setNoAccept();

private slots:
    void ok_Slot();

private:
    Ui::LicenseDialog *ui;
    bool filterCloseEvent;

protected:
    void closeEvent(QCloseEvent *e);

};

#endif // LICENSEDIALOG_H
