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

#include "sfoeditordialog.h"
#include "ui_sfoeditordialog.h"

SfoEditorDialog::SfoEditorDialog(GameItem *gameItem, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SfoEditorDialog)
{
    this->gameItem = gameItem;

    ui->setupUi(this);

    haveTitle = false;
    haveTitleID = false;
    haveVersion = false;
    haveAppVer = false;
    havePs3SystemVer = false;
    haveCategory = false;
    haveBootable = false;
    haveParentalLevel = false;
    haveResolution = false;
    haveSoundFormat = false;

    setDefaults();

    this->connect(ui->resetPushButton, SIGNAL(clicked()), this, SLOT(reset_Slot()));
    this->connect(ui->discardPushButton, SIGNAL(clicked()), this, SLOT(discard_Slot()));
    this->connect(ui->savePushButton, SIGNAL(clicked()), this, SLOT(save_Slot()));

    this->resize(this->width(), 100);
    this->updateGeometry();
}

SfoEditorDialog::~SfoEditorDialog()
{
    delete ui;
}

//
// private slots
//

void SfoEditorDialog::reset_Slot()
{
    setDefaults();
}

void SfoEditorDialog::discard_Slot()
{
    this->reject();
}

void SfoEditorDialog::save_Slot()
{
    for (int i=0; i<paramList.count(); i++) {
        SfoParamItem *item = paramList.at(i);
        if (item->name() == "TITLE") {
            int len = ui->titleLineEdit->text().toUtf8().length()+1;
            item->setValue((unsigned char *)ui->titleLineEdit->text().toUtf8().data(), len);
        }
        else if (item->name() == "TITLE_ID") {
            int len = ui->titleIDLineEdit->text().toUtf8().length()+1;
            item->setValue((unsigned char *)ui->titleIDLineEdit->text().toUtf8().data(), len);
        }
        else if (item->name() == "VERSION") {
            int len = ui->versionLineEdit->text().toUtf8().length()+1;
            item->setValue((unsigned char *)ui->versionLineEdit->text().toUtf8().data(), len);
        }
        else if (item->name() == "APP_VER") {
            int len = ui->appVerLineEdit->text().toUtf8().length()+1;
            item->setValue((unsigned char *)ui->appVerLineEdit->text().toUtf8().data(), len);
        }
        else if (item->name() == "PS3_SYSTEM_VER") {
            int len = ui->ps3SystemVerLineEdit->text().toUtf8().length()+1;
            item->setValue((unsigned char *)ui->ps3SystemVerLineEdit->text().toUtf8().data(), len);
        }
        else if (item->name() == "CATEGORY") {
            QString category = ui->categoryComboBox->currentText().left(2);
            item->setValue((unsigned char *)category.toUtf8().data(), category.length()+1);
        }
        else if (item->name() == "BOOTABLE") {
            unsigned char data[4];
            if (ui->bootableComboBox->currentIndex() == 0)
                append_le_uint32(data, 1);
            else
                append_le_uint32(data, 0);
            item->setValue(data, sizeof(data));
        }
        else if (item->name() == "PARENTAL_LEVEL") {
            unsigned char data[4];
            append_le_uint32(data, ui->parentalLevelComboBox->currentText().left(2).toInt());
            item->setValue(data, sizeof(data));
        }
        else if (item->name() == "RESOLUTION") {
            unsigned char data[4];
            quint32 res = 0;
            if (ui->res480CheckBox->isChecked())
                res ^= 0x00000001;
            if (ui->res576CheckBox->isChecked())
                res ^= 0x00000002;
            if (ui->res720CheckBox->isChecked())
                res ^= 0x00000004;
            if (ui->res1080CheckBox->isChecked())
                res ^= 0x00000008;
            if (ui->res480169CheckBox->isChecked())
                res ^= 0x00000010;
            if (ui->res576169CheckBox->isChecked())
                res ^= 0x00000020;
            res ^= resRemainder;
            append_le_uint32(data, res);
            item->setValue(data, sizeof(data));
        }
        else if (item->name() == "SOUND_FORMAT") {
            unsigned char data[4];
            quint32 sf = 0;
            if (ui->sfLPCM2CheckBox->isChecked())
                sf ^= 0x00000001;
            if (ui->sfLPCM5CheckBox->isChecked())
                sf ^= 0x00000004;
            if (ui->sfLPCM7CheckBox->isChecked())
                sf ^= 0x00000010;
            if (ui->sfDolbyCheckBox->isChecked())
                sf ^= 0x00000100;
            if (ui->sfDTSCheckBox->isChecked())
                sf ^= 0x00000200;
            sf ^= sfRemainder;
            append_le_uint32(data, sf);
            item->setValue(data, sizeof(data));
        }
    }

    bool res = gameItem->savePARAMSFO(gameItem->psfPath(), &paramList);
    if (!res)
        this->reject();

    this->accept();
}

//
// private
//

void SfoEditorDialog::setDefaults()
{
    bool res = gameItem->parsePARAMSFO(gameItem->psfPath(), &paramList);
    if (res) {

        ui->titleLineEdit->clear();
        ui->titleIDLineEdit->clear();
        ui->versionLineEdit->clear();
        ui->appVerLineEdit->clear();
        ui->ps3SystemVerLineEdit->clear();
        ui->res480CheckBox->setChecked(false);
        ui->res480169CheckBox->setChecked(false);
        ui->res576CheckBox->setChecked(false);
        ui->res576169CheckBox->setChecked(false);
        ui->res720CheckBox->setChecked(false);
        ui->res1080CheckBox->setChecked(false);
        ui->sfDolbyCheckBox->setChecked(false);
        ui->sfDTSCheckBox->setChecked(false);
        ui->sfLPCM2CheckBox->setChecked(false);
        ui->sfLPCM5CheckBox->setChecked(false);
        ui->sfLPCM7CheckBox->setChecked(false);

        for (int i=0; i<paramList.count(); i++) {
            SfoParamItem *item = paramList.at(i);
            if (item->name() == "TITLE") {
                haveTitle = true;
                ui->titleLineEdit->setText(QString().fromUtf8((char *)item->value()));
                ui->titleLineEdit->setMaxLength(item->maxValueLength()-1);
            }
            else if (item->name() == "TITLE_ID") {
                haveTitleID = true;
                ui->titleIDLineEdit->setText(QString().fromUtf8((char *)item->value()));
                ui->titleIDLineEdit->setMaxLength(item->maxValueLength()-1);
            }
            else if (item->name() == "VERSION") {
                haveVersion = true;
                ui->versionLineEdit->setText(QString().fromUtf8((char *)item->value()));
                ui->versionLineEdit->setMaxLength(item->maxValueLength()-1);
            }
            else if (item->name() == "APP_VER") {
                haveAppVer = true;
                ui->appVerLineEdit->setText(QString().fromUtf8((char *)item->value()));
                ui->appVerLineEdit->setMaxLength(item->maxValueLength()-1);
            }
            else if (item->name() == "PS3_SYSTEM_VER") {
                havePs3SystemVer = true;
                ui->ps3SystemVerLineEdit->setText(QString().fromUtf8((char *)item->value()));
                ui->ps3SystemVerLineEdit->setMaxLength(item->maxValueLength()-1);
            }
            else if (item->name() == "CATEGORY") {
                haveCategory = true;
                QString cat = QString().fromUtf8((char *)item->value());
                for (int j=0; j<ui->categoryComboBox->count(); j++) {
                    if (ui->categoryComboBox->itemText(j).startsWith(cat)) {
                        ui->categoryComboBox->setCurrentIndex(j);
                        break;
                    }
                }
            }
            else if (item->name() == "BOOTABLE") {
                haveBootable = true;
                if (read_le_uint32(item->value()) == 1)
                    ui->bootableComboBox->setCurrentIndex(0);
                else
                    ui->bootableComboBox->setCurrentIndex(1);
            }
            else if (item->name() == "PARENTAL_LEVEL") {
                haveParentalLevel = true;
                QString level = QString().number(read_le_uint32(item->value()));
                for (int j=0; j<ui->parentalLevelComboBox->count(); j++) {
                    if (ui->parentalLevelComboBox->itemText(j).startsWith(level)) {
                        ui->parentalLevelComboBox->setCurrentIndex(j);
                        break;
                    }
                }
            }
            else if (item->name() == "RESOLUTION") {
                haveResolution = true;
                quint32 res = read_le_uint32(item->value());
                resRemainder = res;
                if (res & 0x00000001) {
                    ui->res480CheckBox->setChecked(true);
                    resRemainder ^= 0x00000001;
                }
                if (res & 0x00000002) {
                    ui->res576CheckBox->setChecked(true);
                    resRemainder ^= 0x00000002;
                }
                if (res & 0x00000004) {
                    ui->res720CheckBox->setChecked(true);
                    resRemainder ^= 0x00000004;
                }
                if (res & 0x00000008) {
                    ui->res1080CheckBox->setChecked(true);
                    resRemainder ^= 0x00000008;
                }
                if (res & 0x00000010) {
                    ui->res480169CheckBox->setChecked(true);
                    resRemainder ^= 0x00000010;
                }
                if (res & 0x00000020) {
                    ui->res576169CheckBox->setChecked(true);
                    resRemainder ^= 0x00000020;
                }
                //qDebug() << "resRemainder=" << resRemainder;
            }
            else if (item->name() == "SOUND_FORMAT") {
                haveSoundFormat = true;
                quint32 sf = read_le_uint32(item->value());
                sfRemainder = sf;
                if (sf & 0x00000001) {
                    ui->sfLPCM2CheckBox->setChecked(true);
                    sfRemainder ^= 0x00000001;
                }
                if (sf & 0x00000004) {
                    ui->sfLPCM5CheckBox->setChecked(true);
                    sfRemainder ^= 0x00000004;
                }
                if (sf & 0x00000010) {
                    ui->sfLPCM7CheckBox->setChecked(true);
                    sfRemainder ^= 0x00000010;
                }
                if (sf & 0x00000100) {
                    ui->sfDolbyCheckBox->setChecked(true);
                    sfRemainder ^= 0x00000100;
                }
                if (sf & 0x00000200) {
                    ui->sfDTSCheckBox->setChecked(true);
                    sfRemainder ^= 0x00000200;
                }
                //qDebug() << "sfRemainder=" << sfRemainder;
            }
        }

        if (!haveTitle)
            ui->titleLineEdit->setDisabled(true);
        else
            ui->titleLineEdit->setEnabled(true);

        if (!haveTitleID)
            ui->titleIDLineEdit->setDisabled(true);
        else
            ui->titleIDLineEdit->setEnabled(true);

        if (!haveVersion)
            ui->versionLineEdit->setDisabled(true);
        else
            ui->versionLineEdit->setEnabled(true);

        if (!haveAppVer)
            ui->appVerLineEdit->setDisabled(true);
        else
            ui->appVerLineEdit->setEnabled(true);

        if (!havePs3SystemVer)
            ui->ps3SystemVerLineEdit->setDisabled(true);
        else
            ui->ps3SystemVerLineEdit->setEnabled(true);

        if (!haveCategory)
            ui->categoryComboBox->setDisabled(true);
        else
            ui->categoryComboBox->setEnabled(true);

        if (!haveBootable)
            ui->bootableComboBox->setDisabled(true);
        else
            ui->bootableComboBox->setEnabled(true);

        if (!haveParentalLevel)
            ui->parentalLevelComboBox->setDisabled(true);
        else
            ui->parentalLevelComboBox->setEnabled(true);

        if (!haveResolution) {
            ui->res480CheckBox->setDisabled(true);
            ui->res480169CheckBox->setDisabled(true);
            ui->res576CheckBox->setDisabled(true);
            ui->res576169CheckBox->setDisabled(true);
            ui->res720CheckBox->setDisabled(true);
            ui->res1080CheckBox->setDisabled(true);
        }
        else {
            ui->res480CheckBox->setEnabled(true);
            ui->res480169CheckBox->setEnabled(true);
            ui->res576CheckBox->setEnabled(true);
            ui->res576169CheckBox->setEnabled(true);
            ui->res720CheckBox->setEnabled(true);
            ui->res1080CheckBox->setEnabled(true);
        }

        if (!haveSoundFormat) {
            ui->sfDolbyCheckBox->setDisabled(true);
            ui->sfDTSCheckBox->setDisabled(true);
            ui->sfLPCM2CheckBox->setDisabled(true);
            ui->sfLPCM5CheckBox->setDisabled(true);
            ui->sfLPCM7CheckBox->setDisabled(true);
        }
        else {
            ui->sfDolbyCheckBox->setEnabled(true);
            ui->sfDTSCheckBox->setEnabled(true);
            ui->sfLPCM2CheckBox->setEnabled(true);
            ui->sfLPCM5CheckBox->setEnabled(true);
            ui->sfLPCM7CheckBox->setEnabled(true);
        }
    }
}

quint32 SfoEditorDialog::read_le_uint32(unsigned char *buf)
{
    quint32 val;

    val = buf[0];
    val |= (buf[1] << 8);
    val |= (buf[2] << 16);
    val |= (buf[3] << 24);

    return val;
}

void SfoEditorDialog::append_le_uint32(unsigned char *buf, quint32 val)
{
    buf[0] = val & 0xff;
    buf[1] = (val >> 8) & 0xff;
    buf[2] = (val >> 16) & 0xff;
    buf[3] = (val >> 24) & 0xff;
}
