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

#include "sfoparamitem.h"

SfoParamItem::SfoParamItem()
{
}

SfoParamItem::SfoParamItem(QString name, unsigned char *value, int valueLength, int maxValueLength) :
    itemName(name),
    itemValue(new unsigned char[maxValueLength]),
    itemValueLength(valueLength),
    itemMaxValueLength(maxValueLength)
{
    memcpy(itemValue, value, valueLength);
}

SfoParamItem::~SfoParamItem()
{
}

void SfoParamItem::release()
{
    delete[] itemValue;
}

QString SfoParamItem::name()
{
    return itemName;
}

unsigned char *SfoParamItem::value()
{
    return itemValue;
}

void SfoParamItem::setValue(unsigned char *buf, int valueLength)
{
    if (valueLength > itemMaxValueLength)
        valueLength = itemMaxValueLength;

    memcpy(itemValue, buf, valueLength);
    itemValueLength = valueLength;
}

int SfoParamItem::valueLength()
{
    return itemValueLength;
}

int SfoParamItem::maxValueLength()
{
    return itemMaxValueLength;
}
