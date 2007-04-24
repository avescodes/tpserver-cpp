/*  Persistence Interface
 * All methods return false or NULL, except init().
 *
 *  Copyright (C) 2005, 2007  Lee Begg and the Thousand Parsec Project
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

#include "object.h"
#include "orderqueue.h"
#include "order.h"
#include "board.h"
#include "message.h"
#include "resourcedescription.h"
#include "player.h"
#include "category.h"
#include "design.h"
#include "component.h"
#include "property.h"

#include "persistence.h"

Persistence::~Persistence(){
}

bool Persistence::init(){
    return true;
}

void Persistence::shutdown(){
}

bool Persistence::saveObject(IGObject* ob){
    return false;
}

bool Persistence::updateObject(IGObject* ob){
    return false;
}

IGObject* Persistence::retrieveObject(uint32_t obid){
    return NULL;
}

bool Persistence::removeObject(uint32_t obid){
    return false;
}

uint32_t Persistence::getMaxObjectId(){
    return 0;
}

std::set<uint32_t> Persistence::getObjectIds(){
    return std::set<uint32_t>();
}

bool Persistence::saveOrderQueue(const OrderQueue* oq){
  return false;
}

OrderQueue* Persistence::retrieveOrderQueue(uint32_t oqid){
  return NULL;
}

bool Persistence::removeOrderQueue(uint32_t oqid){
  return false;
}

std::set<uint32_t> Persistence::getOrderQueueIds(){
  return std::set<uint32_t>();
}

uint32_t Persistence::getMaxOrderQueueId(){
  return 0;
}

bool Persistence::saveOrder(uint32_t queueid, uint32_t ordid, Order* ord){
    return false;
}

bool Persistence::updateOrder(uint32_t queueid, uint32_t ordid, Order* ord){
    return false;
}

Order* Persistence::retrieveOrder(uint32_t queueid, uint32_t ordid){
    return NULL;
}

bool Persistence::removeOrder(uint32_t queueid, uint32_t ordid){
    return false;
}


bool Persistence::saveBoard(Board* board){
    return false;
}

bool Persistence::updateBoard(Board* board){
    return false;
}

Board* Persistence::retrieveBoard(uint32_t boardid){
    return NULL;
}

uint32_t Persistence::getMaxBoardId(){
    return 0;
}

std::set<uint32_t> Persistence::getBoardIds(){
    return std::set<uint32_t>();
}

bool Persistence::saveMessage(uint32_t msgid, Message* msg){
    return false;
}

Message* Persistence::retrieveMessage(uint32_t msgid){
    return NULL;
}

bool Persistence::removeMessage(uint32_t msgid){
    return false;
}

bool Persistence::saveMessageList(uint32_t bid, std::list<uint32_t> list){
    return false;
}

std::list<uint32_t> Persistence::retrieveMessageList(uint32_t bid){
    return std::list<uint32_t>();
}

uint32_t Persistence::getMaxMessageId(){
    return 0;
}

bool Persistence::saveResource(ResourceDescription* res){
    return false;
}

ResourceDescription* Persistence::retrieveResource(uint32_t restype){
    return NULL;
}

uint32_t Persistence::getMaxResourceId(){
    return 0;
}

std::set<uint32_t> Persistence::getResourceIds(){
    return std::set<uint32_t>();
}

bool Persistence::savePlayer(Player* player){
    return false;
}

bool Persistence::updatePlayer(Player* player){
    return false;
}

Player* Persistence::retrievePlayer(uint32_t playerid){
    return NULL;
}

uint32_t Persistence::getMaxPlayerId(){
    return 0;
}

std::set<uint32_t> Persistence::getPlayerIds(){
    return std::set<uint32_t>();
}

bool Persistence::saveCategory(Category* cat){
    return false;
}

Category* Persistence::retrieveCategory(uint32_t catid){
    return NULL;
}

uint32_t Persistence::getMaxCategoryId(){
    return 0;
}

std::set<uint32_t> Persistence::getCategoryIds(){
    return std::set<uint32_t>();
}

bool Persistence::saveDesign(Design* design){
    return false;
}

bool Persistence::updateDesign(Design* design){
    return false;
}

Design* Persistence::retrieveDesign(uint32_t designid){
    return NULL;
}

uint32_t Persistence::getMaxDesignId(){
    return 0;
}

std::set<uint32_t> Persistence::getDesignIds(){
    return std::set<uint32_t>();
}

bool Persistence::saveComponent(Component* comp){
    return false;
}

Component* Persistence::retrieveComponent(uint32_t compid){
    return NULL;
}

uint32_t Persistence::getMaxComponentId(){
    return 0;
}

std::set<uint32_t> Persistence::getComponentIds(){
    return std::set<uint32_t>();
}

bool Persistence::saveProperty(Property* prop){
    return false;
}

Property* Persistence::retrieveProperty(uint32_t propid){
    return NULL;
}

uint32_t Persistence::getMaxPropertyId(){
    return 0;
}

std::set<uint32_t> Persistence::getPropertyIds(){
    return std::set<uint32_t>();
}
