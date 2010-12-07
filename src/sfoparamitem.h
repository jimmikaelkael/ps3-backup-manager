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

#ifndef SFOPARAMITEM_H
#define SFOPARAMITEM_H

#include <QList>
#include <QString>

class SfoParamItem
{

public:
    SfoParamItem();
    ~SfoParamItem();
    SfoParamItem(QString name, unsigned char *value, int valueLength, int maxValueLength);
    void release();
    QString name();
    unsigned char *value();
    void setValue(unsigned char *buf, int valueLength);
    int valueLength();
    int maxValueLength();

private:
    QString itemName;
    unsigned char *itemValue;
    int itemValueLength;
    int itemMaxValueLength;

};

typedef QList<SfoParamItem> SfoParamItemList;

#endif // SFOPARAMITEM_H
