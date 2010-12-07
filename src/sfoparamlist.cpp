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

#include "sfoparamlist.h"

SfoParamList::SfoParamList()
{
}

SfoParamList::~SfoParamList()
{
    clear();
}

void SfoParamList::addItem(SfoParamItem item)
{
    itemList << item;
}

void SfoParamList::clear()
{
    for (int i=0; i<itemList.count(); i++) {
        SfoParamItem item = itemList.at(i);
        item.release();
    }
    itemList.clear();
}

int SfoParamList::count()
{
    return itemList.count();
}

SfoParamItem *SfoParamList::at(int index)
{
    SfoParamItem *p = &itemList[index];

    return p;
}
