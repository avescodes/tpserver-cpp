/*  ObjectParameterGroup class
 *
 *  Copyright (C) 2007, 2008 Lee Begg and the Thousand Parsec Project
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

#include <stdlib.h>
#include <exception>

#include "frame.h"
#include "position3dobjectparam.h"
#include "velocity3dobjectparam.h"
#include "sizeobjectparam.h"
#include "orderqueueobjectparam.h"
#include "resourcelistobjectparam.h"
#include "referenceobjectparam.h"
#include "refquantitylistobjectparam.h"
#include "integerobjectparam.h"
#include "logging.h"

#include "objectparametergroupdesc.h"


ObjectParameterDesc::ObjectParameterDesc() : type(0), name(), description(){
}

ObjectParameterDesc::ObjectParameterDesc(const ObjectParameterDesc& rhs){
  type = rhs.type;
  name = rhs.name;
  description = rhs.description;
}

ObjectParameterDesc::~ObjectParameterDesc(){
}

ObjectParameterDesc& ObjectParameterDesc::operator=(const ObjectParameterDesc& rhs){
  type = rhs.type;
  name = rhs.name;
  description = rhs.description;
  return *this;
}

void ObjectParameterDesc::setType(uint32_t nt){
  type = nt;
}

void ObjectParameterDesc::setName(const std::string& nn){
  name = nn;
}

void ObjectParameterDesc::setDescription(const std::string& nd){
  description = nd;
}

uint32_t ObjectParameterDesc::getType() const{
  return type;
}

void ObjectParameterDesc::packObjectDescFrame(Frame* f)const {
  f->packString(name);
  f->packInt(type);
  f->packString(description);
}

ObjectParameterGroupDesc::ObjectParameterGroupDesc() : groupid(0), name(), description(), parameters(){
}

ObjectParameterGroupDesc::~ObjectParameterGroupDesc(){
}

uint32_t ObjectParameterGroupDesc::getGroupId() const{
  return groupid;
}

std::string ObjectParameterGroupDesc::getName() const{
  return name;
}

std::string ObjectParameterGroupDesc::getDescription() const{
  return description;
}

void ObjectParameterGroupDesc::setGroupId(uint32_t ni){
  groupid = ni;
}

void ObjectParameterGroupDesc::setName(const std::string& nn){
  name = nn;
}

void ObjectParameterGroupDesc::setDescription(const std::string& nd){
  description = nd;
}

void ObjectParameterGroupDesc::addParameter(ObjectParameterDesc op){
  parameters.push_back(op);
}

void ObjectParameterGroupDesc::packObjectDescFrame(Frame * f) const{

  f->packInt(groupid);
  f->packString(name);
  f->packString(description);
  f->packInt(parameters.size());
  for(std::list<ObjectParameterDesc>::const_iterator itcurr = parameters.begin();
      itcurr != parameters.end(); ++itcurr){
    (*itcurr).packObjectDescFrame(f);
  }
}

ObjectParameterGroupPtr ObjectParameterGroupDesc::createObjectParameterGroup() const{
  ObjectParameterGroupPtr pg;
  pg->setGroupId(groupid);
  for(std::list<ObjectParameterDesc>::const_iterator itcurr = parameters.begin();
      itcurr != parameters.end(); ++itcurr){
    ObjectParameter* param = NULL;
    switch((*itcurr).getType()){
      case obpT_Position_3D:
        param = new Position3dObjectParam();
        break;
      case obpT_Velocity:
        param = new Velocity3dObjectParam();
        break;
      case obpT_Order_Queue:
        param = new OrderQueueObjectParam();
        break;
      case obpT_Resource_List:
        param = new ResourceListObjectParam();
        break;
      case obpT_Reference:
        param = new ReferenceObjectParam();
        break;
      case obpT_Reference_Quantity_List:
        param = new RefQuantityListObjectParam();
        break;
      case obpT_Integer:
        param = new IntegerObjectParam();
        break;
      case obpT_Size:
        param = new SizeObjectParam();
        break;
        
      default:
        Logger::getLogger()->warning("Unknown ObjectParameter type %d in creating ParameterGroup", (*itcurr).getType());
        throw new std::exception();
        break;
    }
    pg->addParameter(param);
  }
  return pg;
}
