/*  Design class
 *
 *  Copyright (C) 2005,2006, 2007  Lee Begg and the Thousand Parsec Project
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

#include <time.h>

#include "tpscheme.h"
#include "game.h"

#include "design.h"

Design::Design(){
  valid = false;
    timestamp = time(NULL);
    inuse = 0;
    exist = 0;
}

Design::~Design(){

}


uint32_t Design::getDesignId() const{
  return designid;
}

uint32_t Design::getCategoryId() const{
  return catid;
}

std::string Design::getName() const{
  return name;
}

std::string Design::getDescription() const{
    return description;
}

uint32_t Design::getOwner() const{
  return owner;
}

std::map<uint32_t, uint32_t> Design::getComponents() const{
  return components;
}

uint32_t Design::getNumExist() const{
  return exist;
}

uint32_t Design::getInUse() const{
    return inuse;
}

bool Design::isValid() const{
  return valid;
}

std::string Design::getFeedback() const{
    return feedback;
}

double Design::getPropertyValue(uint32_t propid) const{
  std::map<uint32_t, PropertyValue>::const_iterator itpos = properties.find(propid);
  if(itpos != properties.end()){
    PropertyValue pv = itpos->second;
    return pv.getValue();
  }else
    return 0.0;
}

std::map<uint32_t, PropertyValue> Design::getPropertyValues() const{
    return properties;
}

uint64_t Design::getModTime() const{
    return timestamp;
}

void Design::setDesignId(uint32_t id){
  designid = id;
}

void Design::setCategoryId(uint32_t id){
  catid = id;
}

void Design::setName(const std::string& n){
  name = n;
}

void Design::setDescription(const std::string& d){
  description = d;
}

void Design::setOwner(uint32_t o){
  owner = o;
}

void Design::setComponents(std::map<uint32_t, uint32_t> cl){
  components = cl;
  valid = false;
  properties.clear();
  feedback = "";
}

void Design::setInUse(uint32_t niu){
    inuse = niu;
}

void Design::setNumExist(uint32_t nne){
    exist = nne;
}

void Design::setModTime(uint64_t nmt){
    timestamp = nmt;
}

void Design::eval(){
  // calculate design property values
  
  //first, clear out any old ones
  valid = false;
  properties.clear();
  feedback = "";

  // start calc
  TpScheme* scheme_intr = Game::getGame()->getTpScheme();
  scheme_intr->evalDesign(this);
}

void Design::setPropertyValues(std::map<uint32_t, PropertyValue> pvl){
  properties = pvl;
}

void Design::setValid(bool v, const std::string& f){
  valid = v;
  feedback = f;
}

void Design::addUnderConstruction(uint32_t num){
    inuse += num;
    timestamp = time(NULL);
}

void Design::addComplete(uint32_t num){
    exist += num;
    timestamp = time(NULL);
}

void Design::removeCanceledConstruction(uint32_t num){
    inuse -= num;
    timestamp = time(NULL);
}

void Design::removeDestroyed(uint32_t num){
    inuse -= num;
    timestamp = time(NULL);
}
