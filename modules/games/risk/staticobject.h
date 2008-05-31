#ifndef STATICOBJECT_H
#define STATICOBJECT_H
/*  static object class
 *
 *  Copyright (C) 2007  Tyler Shaub and the Thousand Parsec Project
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


#include <string>
#include <utility>

#include <tpserver/vector3d.h>
#include <tpserver/objectbehaviour.h>
#include <tpserver/objecttype.h>

class Position3dObjectParam;
class SizeObjectParam;

namespace RiskRuleset {

class StaticObjectType : public ObjectType{
public:
   StaticObjectType();
   virtual ~StaticObjectType(){};

   void setTypeName(const std::string& nname);
   void setTypeDescription(const std::string& ndesc);

protected:
   virtual ObjectBehaviour* createObjectBehaviour() const;
};

class StaticObject:public ObjectBehaviour {
public:
   StaticObject();
   virtual ~StaticObject() {};

   void setUnitPos(double unitX, double unitY);
   void setUnitPos(const std::pair<double,double>& unitPos);
   std::pair<double,double> getUnitPos() const;

   Vector3d getPosition() const;
   void setPosition(const Vector3d & np);

   uint64_t getSize() const;
   void setSize(uint64_t ns);

   virtual void packExtraData(Frame * frame);
   virtual void doOnceATurn();
   virtual int getContainerType();


private:
   double unitX, unitY;

};

}

#endif
