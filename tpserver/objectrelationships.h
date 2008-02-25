#ifndef OBJECTRELATIONSHIPS_H
#define OBJECTRELATIONSHIPS_H
/*  ObjectRelationships class
 *
 *  Copyright (C) 2008  Lee Begg and the Thousand Parsec Project
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdint.h>
#include <set>
#include <boost/shared_ptr.hpp>

class Frame;

class ObjectRelationshipsData;

typedef boost::shared_ptr<ObjectRelationshipsData> ObjectRelationshipsPtr;

class ObjectRelationshipsData {
  public:
    ObjectRelationshipsData();
    ~ObjectRelationshipsData();
    
    uint32_t getParent() const;
    std::set<uint32_t> getChildren() const;

    void setParent(uint32_t np);
    void addChild(uint32_t nc);
    void removeChild(uint32_t oc);
    void setChildren(const std::set<uint32_t> nc);

    void packFrame(Frame* f, uint32_t playerid);
    void unpackModFrame(Frame* f);
    
    bool isDirty() const;
    void setIsDirty(bool id);

  private:
    uint32_t parentid;
    std::set<uint32_t> children;
    bool dirty;

};

#endif
