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

#ifndef SFOEDITORDIALOG_H
#define SFOEDITORDIALOG_H

#include <QDialog>
#include <QCloseEvent>
#include <QDebug>

#include "gameitem.h"
#include "sfoparamlist.h"
#include "sfoparamitem.h"

namespace Ui {
    class SfoEditorDialog;
}

class SfoEditorDialog : public QDialog
{
    Q_OBJECT

public:
    SfoEditorDialog(GameItem *gameItem, QWidget *parent = 0);
    ~SfoEditorDialog();

private slots:
    void reset_Slot();
    void discard_Slot();
    void save_Slot();

private:
    void setDefaults();
    quint32 read_le_uint32(unsigned char *buf);
    void append_le_uint32(unsigned char *buf, quint32 val);

    Ui::SfoEditorDialog *ui;
    GameItem *gameItem;
    SfoParamList paramList;
    bool haveTitle;
    bool haveTitleID;
    bool haveVersion;
    bool haveAppVer;
    bool havePs3SystemVer;
    bool haveCategory;
    bool haveBootable;
    bool haveParentalLevel;
    bool haveResolution;
    bool haveSoundFormat;
    quint32 resRemainder, sfRemainder;

};

#endif // SFOEDITORDIALOG_H
