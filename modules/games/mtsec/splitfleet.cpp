/*  SplitFleet order
 *
 *  Copyright (C) 2004-2005, 2007  Lee Begg and the Thousand Parsec Project
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

#include <tpserver/order.h>
#include <tpserver/frame.h>
#include <tpserver/object.h>
#include <tpserver/objectmanager.h>
#include <tpserver/game.h>
#include <tpserver/logging.h>
#include "fleet.h"
#include <tpserver/player.h>
#include <tpserver/message.h>
#include <tpserver/design.h>
#include <tpserver/designstore.h>
#include <tpserver/playermanager.h>
#include <tpserver/listparameter.h>
#include <tpserver/orderqueue.h>
#include <tpserver/orderqueueobjectparam.h>
#include <tpserver/ordermanager.h>
#include <tpserver/objectdatamanager.h>

#include "splitfleet.h"

SplitFleet::SplitFleet() : Order(){
  name = "Split Fleet";
  description = "Split the fleet into two";
  
  shiplist = new ListParameter();
  shiplist->setName("ships");
  shiplist->setDescription("The ships to be transferred");
  shiplist->setListOptionsCallback(ListOptionCallback(this, &SplitFleet::generateListOptions));
  parameters.push_back(shiplist);
  
  turns = 1;
}

SplitFleet::~SplitFleet(){
  delete shiplist;
}

std::map<uint32_t, std::pair<std::string, uint32_t> > SplitFleet::generateListOptions(int objID){
  std::map<uint32_t, std::pair<std::string, uint32_t> > options;

  Fleet* of = (Fleet*)(Game::getGame()->getObjectManager()->getObject(objID)->getObjectData());

  std::map<int, int> sotf = of->getShips();
  Game::getGame()->getObjectManager()->doneWithObject(objID);

  DesignStore* ds = Game::getGame()->getDesignStore();

  for(std::map<int, int>::const_iterator itcurr = sotf.begin();
      itcurr != sotf.end(); ++itcurr){
    options[itcurr->first] = std::pair<std::string, uint32_t>(ds->getDesign(itcurr->first)->getName(), itcurr->second);
  }

  return options;
}

bool SplitFleet::doOrder(IGObject * ob){

  Fleet* of = (Fleet*)(ob->getObjectData());

  Message * msg = new Message();
  msg->setSubject("Split Fleet order complete");
  msg->addReference(rst_Object, ob->getID());
  
  IGObject * nfleet = Game::getGame()->getObjectManager()->createNewObject();
  nfleet->setType(Game::getGame()->getObjectDataManager()->getObjectTypeByName("Fleet"));
  Fleet* nf = (Fleet*)(nfleet->getObjectData());
  nf->setSize(2);
  nfleet->setName("A fleet");
  
  nf->setOwner(of->getOwner());
  nf->setPosition(of->getPosition());
  
  OrderQueue *fleetoq = new OrderQueue();
    fleetoq->setQueueId(nfleet->getID());
    fleetoq->addOwner(nf->getOwner());
    Game::getGame()->getOrderManager()->addOrderQueue(fleetoq);
    OrderQueueObjectParam* oqop = static_cast<OrderQueueObjectParam*>(nf->getParameterByType(obpT_Order_Queue));
    oqop->setQueueId(fleetoq->getQueueId());
    nf->setDefaultOrderTypes();
  
  std::map<uint32_t, uint32_t> ships = shiplist->getList();
  for(std::map<uint32_t, uint32_t>::iterator scurr = ships.begin(); scurr != ships.end(); ++scurr){
    if(of->removeShips(scurr->first, scurr->second)){
      nf->addShips(scurr->first, scurr->second);
    }else{
      Logger::getLogger()->debug("Player tried to remove too many ships of type %d from fleet", scurr->first);
      //message for player
    }
  }
  
  if(of->totalShips() == 0){
    // whole fleet moved, put it back
    Logger::getLogger()->debug("Whole fleet split, putting it back");
    for(std::map<uint32_t, uint32_t>::iterator scurr = ships.begin(); scurr != ships.end(); ++scurr){
      of->addShips(scurr->first, scurr->second);
    }
    Game::getGame()->getObjectManager()->discardNewObject(nfleet);
    msg->setBody("Fleet not split, not enough ships");
    msg->addReference(rst_Action_Order, rsorav_Incompatible);
  }else if(nf->totalShips() == 0){
    Logger::getLogger()->debug("Split fleet doesn't have any ships, not creating new fleet");
    Game::getGame()->getObjectManager()->discardNewObject(nfleet);
    msg->setBody("Fleet not split, not enough ships");
    msg->addReference(rst_Action_Order, rsorav_Incompatible);
  }else{
    // add fleet to game universe
    Logger::getLogger()->debug("Split fleet successfully");
    msg->setBody("Split fleet complete");
    msg->addReference(rst_Object, nfleet->getID());
    msg->addReference(rst_Action_Order, rsorav_Completion);
    nfleet->addToParent(ob->getParent());
    Game::getGame()->getObjectManager()->addObject(nfleet);
  }
  
  Game::getGame()->getPlayerManager()->getPlayer(nf->getOwner())->postToBoard(msg);
  
  return true;
}

Order* SplitFleet::clone() const{
  SplitFleet * nsf = new SplitFleet();
  nsf->type = type;
  return nsf;
}
