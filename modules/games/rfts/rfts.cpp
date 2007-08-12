/*  RFTS rulesset
 *
 *  Copyright (C) 2007  Tyler Shaub and the Thousand Parsec Project
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

#include <tpserver/player.h>
#include <tpserver/playermanager.h>
#include <tpserver/playerview.h>
#include <tpserver/game.h>
#include <tpserver/logging.h>

#include <tpserver/property.h>
#include <tpserver/component.h>
#include <tpserver/design.h>
#include <tpserver/category.h>
#include <tpserver/designstore.h>

#include <tpserver/object.h>
#include <tpserver/objectdatamanager.h>
#include <tpserver/ordermanager.h>
#include <tpserver/orderqueue.h>
#include <tpserver/orderqueueobjectparam.h>
#include <tpserver/objectmanager.h>
#include <tpserver/objectparameter.h>
#include <tpserver/resourcemanager.h>
#include <tpserver/resourcedescription.h>

#include "nop.h"
#include "buildfleet.h"
#include "move.h"
#include "productionorder.h"
#include "staticobject.h"
#include "planet.h"
#include "universe.h"
#include "fleet.h"
#include "rftsturn.h"

#include "rfts.h"

// hacky define :p
#define DEBUG_FN_PRINT() (Logger::getLogger()->debug(__FUNCTION__))

extern "C" {
  #define tp_init librfts_LTX_tp_init
  bool tp_init(){
    return Game::getGame()->setRuleset(new RFTS_::Rfts());
  }
}

namespace RFTS_ {

using std::string;
using std::map;

Rfts::Rfts() {

}

Rfts::~Rfts() {

}

std::string Rfts::getName() {
   return "TP RFTS";
}

std::string Rfts::getVersion() {
   return "0.0";
}

const ProductionInfo& Rfts::getProductionInfo() {
   static ProductionInfo prodInfo = ProductionInfo();
   return prodInfo;
}

void Rfts::initGame() {
   DEBUG_FN_PRINT();
   
   Game::getGame()->setTurnProcess(new RftsTurn());

   setObjectTypes();

   setOrderTypes();
}

void Rfts::setObjectTypes() const {

   DEBUG_FN_PRINT();

   ObjectDataManager* obdm = Game::getGame()->getObjectDataManager();
   StaticObject *eo;

   obdm->addNewObjectType(new Universe);

   // galaxy added for tp03
   eo = new StaticObject();
   eo->setTypeName("Galaxy");
   eo->setTypeDescription("Galaxy");
   obdm->addNewObjectType(eo);

   eo = new StaticObject();
   eo->setTypeName("Star System"); 
   eo->setTypeDescription("A system of stars!");
   obdm->addNewObjectType(eo);

   obdm->addNewObjectType(new Planet);
   obdm->addNewObjectType(new Fleet);
}

void Rfts::setOrderTypes() const {
   OrderManager* orm = Game::getGame()->getOrderManager();

   orm->addOrderType(new Nop());
   orm->addOrderType(new BuildFleet());
   orm->addOrderType(new Move());
   orm->addOrderType(new ProductionOrder());
}

void Rfts::createGame() {
   DEBUG_FN_PRINT();
   
   Game *game = game->getGame();
   
   // create general category
   Category* cat = new Category();
   cat->setName("Ships");
   cat->setDescription("The ship design & component category");
   game->getDesignStore()->addCategory(cat);

   createProperties();

   createComponents();

   createResources();
   
   // set up universe (universe -> star sys -> planets)
   createUniverse(); // wow that looks like a powerful function!
}

void Rfts::createProperties() {
   Property* prop = new Property();
   DesignStore *ds = Game::getGame()->getDesignStore();

   // speed   

   prop->addCategoryId(ds->getCategoryByName("Ships"));
   prop->setRank(0);
   prop->setName("Speed");
   prop->setDisplayName("Speed");
   prop->setDescription("The number of units the ship can move each turn");
   prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (string-append (number->string (/ n 1000)) \" speedy units\")) ) )");
   prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
   ds->addProperty(prop);
   
   // attack
   prop = new Property();
   prop->addCategoryId(ds->getCategoryByName("Ships"));
   prop->setRank(0);
   prop->setName("Attack");
   prop->setDisplayName("Attack");
   prop->setDescription("The offensive strength of a ship");
   prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (number->string n))))");
   prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
   ds->addProperty(prop);

   // armour
   prop = new Property();
   prop->addCategoryId(ds->getCategoryByName("Ships"));
   prop->setRank(0);
   prop->setName("Armour");
   prop->setDisplayName("Armour");
   prop->setDescription("The defensive strength of a ship");
   prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (number->string n))))");
   prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
   ds->addProperty(prop);

   // colonise
   prop = new Property();
   prop->addCategoryId(ds->getCategoryByName("Ships"));
   prop->setName("Colonise");
   prop->setDisplayName("Can Colonise");
   prop->setDescription("The ship colonise planets");
   prop->setRank(0);
   prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (if (= n 1) \"Yes\" \"No\")) ) )");
   prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
   ds->addProperty(prop);

   prop = new Property();
   prop->addCategoryId(ds->getCategoryByName("Ships"));
   prop->setName("RP Cost");
   prop->setDisplayName("RP Cost");
   prop->setDescription("The number of resource points required to build this ship");
   prop->setRank(0);
   prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (string-append (number->string n) \" RP\")) ) )");
   prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
   ds->addProperty(prop);
}

void Rfts::createComponents() {
   DesignStore *ds = Game::getGame()->getDesignStore();  
   
   // movement
   ds->addComponent(createEngineComponent('1'));
   ds->addComponent(createEngineComponent('2'));
   ds->addComponent(createEngineComponent('3'));
   ds->addComponent(createEngineComponent('4'));

   // attack
   ds->addComponent(createBattleComponent('1'));
   ds->addComponent(createBattleComponent('2'));
   ds->addComponent(createBattleComponent('3'));
   ds->addComponent(createBattleComponent('4'));

   // colonise
   ds->addComponent(createTransportComponent());
}

void Rfts::createUniverse() const {
   DEBUG_FN_PRINT();

   ObjectManager *objman = Game::getGame()->getObjectManager();

   uint32_t uniType = Game::getGame()->getObjectDataManager()->getObjectTypeByName("Universe");
   IGObject *universe = objman->createNewObject();

   universe->setType(uniType);
   universe->setName("The Universe");
   StaticObject* uniData = static_cast<StaticObject*>(universe->getObjectData());
   uniData->setPosition(Vector3d(0ll, 0ll, 0ll));
   objman->addObject(universe);   
   
   createStarSystems(universe);
}

void Rfts::createStarSystems(IGObject *universe) const {
   DEBUG_FN_PRINT();
   // TODO (make all the systems... and functions for each)
   // just create a single test system for now

   Game *game = Game::getGame();
   ObjectManager *objman = game->getObjectManager();
   IGObject *starSys = game->getObjectManager()->createNewObject();
   IGObject *planet = game->getObjectManager()->createNewObject();
   IGObject *starSys2 = game->getObjectManager()->createNewObject();
   IGObject *planet2 = game->getObjectManager()->createNewObject();
   
   uint32_t ssType = game->getObjectDataManager()->getObjectTypeByName("Star System");
   uint32_t planetType = game->getObjectDataManager()->getObjectTypeByName("Planet");

   starSys->setType(ssType);
   starSys->setName("Star System1");
   StaticObject* starSysData = static_cast<StaticObject*>(starSys->getObjectData());
   starSysData->setPosition(Vector3d(30000ll, 20000ll, 0ll));
   
   starSys->addToParent(universe->getID());
   objman->addObject(starSys);
   
   planet->setType(planetType);
   planet->setName("Planet1");
   Planet* planetData = static_cast<Planet*>(planet->getObjectData());
   planetData->setSize(2);
   planetData->setPosition(starSysData->getPosition() + Vector3d(20ll, 20ll, 0ll));
   planetData->setDefaultResources();
   
   OrderQueue *planetOrders = new OrderQueue();
   planetOrders->setObjectId(planet->getID());
   planetOrders->addOwner(0);
   game->getOrderManager()->addOrderQueue(planetOrders);
   OrderQueueObjectParam* oqop = static_cast<OrderQueueObjectParam*>(planetData->getParameterByType(obpT_Order_Queue));
   oqop->setQueueId(planetOrders->getQueueId());
   planetData->setDefaultOrderTypes();
  
   planet->addToParent(starSys->getID());
   objman->addObject(planet);


   starSys2->setType(ssType);
   starSys2->setName("Star System2");
   StaticObject* starSysData2 = static_cast<StaticObject*>(starSys2->getObjectData());
   starSysData2->setPosition(Vector3d(20000ll, 30000ll, 0ll));
   
   starSys2->addToParent(universe->getID());
   objman->addObject(starSys2);
   
   planet2->setType(planetType);
   planet2->setName("Planet2");
   Planet* planetData2 = static_cast<Planet*>(planet2->getObjectData());
   planetData2->setSize(2);
   planetData2->setPosition(starSysData2->getPosition() + Vector3d(20ll, 20ll, 0ll));
   
   OrderQueue *planetOrders2 = new OrderQueue();
   planetOrders2->setObjectId(planet2->getID());
   planetOrders2->addOwner(0);
   game->getOrderManager()->addOrderQueue(planetOrders2);
   OrderQueueObjectParam* oqop2 = static_cast<OrderQueueObjectParam*>(planetData2->getParameterByType(obpT_Order_Queue));
   oqop2->setQueueId(planetOrders2->getQueueId());
   planetData2->setDefaultOrderTypes();
  
   planet2->addToParent(starSys2->getID());
   objman->addObject(planet2);
}

void Rfts::createResources() const {
   ResourceManager* resMan = Game::getGame()->getResourceManager();
   
   ResourceDescription* res = new ResourceDescription();
   res->setNameSingular("Resource Point");
   res->setNamePlural("Resource Points");
   res->setUnitSingular("point");
   res->setUnitPlural("points");
   res->setDescription("Resource points");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);

   res = new ResourceDescription();
   res->setNameSingular("Industry");
   res->setNamePlural("Industry");
   res->setUnitSingular("units");
   res->setUnitPlural("units");
   res->setDescription("Industrial strength");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);

   res = new ResourceDescription();
   res->setNameSingular("Population");
   res->setNamePlural("Population");
   res->setUnitSingular("unit");
   res->setUnitPlural("units");
   res->setDescription("Population of the planet");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);

   res = new ResourceDescription();
   res->setNameSingular("Social Environment");
   res->setNamePlural("Social Environment");
   res->setUnitSingular("point");
   res->setUnitPlural("points");
   res->setDescription("Social Env. describes the health of the population. (Influences Population)");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);

   res = new ResourceDescription();
   res->setNameSingular("Planetary Environment");
   res->setNamePlural("Planetary Environment");
   res->setUnitSingular("point");
   res->setUnitPlural("points");
   res->setDescription("Planetary Env. describes the health of the planet. (Influences Social Env.)");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);

   res = new ResourceDescription();
   res->setNameSingular("Population Maintenance");
   res->setNamePlural("Population Maintenance");
   res->setUnitSingular("point");
   res->setUnitPlural("points");
   res->setDescription("Population Maintenance is the cost required to maintain the current population.\
                         A 1:1 ratio with population is required for healthy populations.");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);

   res = new ResourceDescription();
   res->setNameSingular("Colonist");
   res->setNamePlural("Colonists");
   res->setUnitSingular("unit");
   res->setUnitPlural("units");
   res->setDescription("Population availabl for colonisation");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);
}

void Rfts::startGame() {
	DEBUG_FN_PRINT();
}

bool Rfts::onAddPlayer(Player *player) {
   DEBUG_FN_PRINT();
   if(Game::getGame()->getPlayerManager()->getNumPlayers() < MAX_PLAYERS)
      return true;
   return false;
}
void Rfts::onPlayerAdded(Player *player) {
   DEBUG_FN_PRINT();

   PlayerView* playerview = player->getPlayerView();
   playerview->setVisibleObjects( Game::getGame()->getObjectManager()->getAllIds() );

   for(uint32_t itcurr = 1; itcurr <= 9; ++itcurr){
      playerview->addUsableComponent(itcurr);
   }

   // test : set the 1st object - a planet - to be owned by the player
   Planet* pData = dynamic_cast<Planet*>(Game::getGame()->getObjectManager()->getObject(2)->getObjectData());
   pData->setOwner(player->getID());

   Logger::getLogger()->debug("Making player's fleet");
   Game *game = Game::getGame();
   IGObject* fleet = createEmptyFleet( player, game->getObjectManager()->getObject(1), "Fleet1");

   Design* scout = createScoutDesign( player);

   dynamic_cast<Fleet*>(fleet->getObjectData())->addShips( scout->getDesignId(), 2);
   game->getDesignStore()->designCountsUpdated(scout);

   game->getObjectManager()->addObject(fleet);

   Logger::getLogger()->debug( "done making fleet");

   Game::getGame()->getPlayerManager()->updatePlayer(player->getID());
}


// helper functions

Component* createEngineComponent(char techLevel) {

   Component* engine = new Component();
   map<unsigned int, string> propList;

   DesignStore *ds = Game::getGame()->getDesignStore();

   engine->addCategoryId(ds->getCategoryByName("Ships"));
   engine->setName( string("Engine") + techLevel);
   engine->setDescription( "A ship engine, required if you want your ship to move!");
   engine->setTpclRequirementsFunction(
      "(lambda (design) "
      "(if (= (designType._num-components design) 1) "
      "(cons #t \"\") "
      "(cons #f \"This is a complete component, nothing else can be included\")))");
   propList[ds->getPropertyByName("Speed")] = string("(lambda (design) (* 100 ") +  techLevel + string("))");
   propList[ds->getPropertyByName("RP Cost")] = string("(lambda (design) (* 3 ") +  techLevel + string("))");
   engine->setPropertyList(propList);

   return engine;
}

Component* createBattleComponent(char techLevel) {
   Component *battle = new Component();
   map<unsigned int, string> propList;

   DesignStore *ds = Game::getGame()->getDesignStore();

   battle->addCategoryId(ds->getCategoryByName("Ships"));
   battle->setName( string("Battle") + techLevel);
   battle->setDescription( "Guns and armour for a ship");
   battle->setTpclRequirementsFunction(
      "(lambda (design) "
      "(if (= (designType._num-components design) 1) "
      "(cons #t \"\") "
      "(cons #f \"This is a complete component, nothing else can be included\")))");
   propList[ ds->getPropertyByName("Attack") ] = string("(lambda (design) (* 5") + techLevel + string("))");
   propList[ ds->getPropertyByName("Armour") ] = string("(lambda (design) (* 5") + techLevel + string("))");
   //propList[ ds->getPropertyByName("RP Cost") ] = string("(lamda (design) (* 9") + techLevel + string("))");
   battle->setPropertyList(propList);
   return battle;
}


Component* createTransportComponent() {
   Component *trans = new Component();
   map<unsigned int, string> propList;

   DesignStore *ds = Game::getGame()->getDesignStore();

   trans->addCategoryId(ds->getCategoryByName("Ships"));
   trans->setName( "Transport");
   trans->setDescription( "A colonist transport bay");
   trans->setTpclRequirementsFunction(
      "(lambda (design) "
      "(if (= (designType._num-components design) 1) "
      "(cons #t \"\") "
      "(cons #f \"This is a complete component, nothing else can be included\")))");
   propList[ ds->getPropertyByName("Colonise") ] = "(lambda (design) 1)";
   //propList[ ds->getPropertyByName("RP Cost") ] = "(lambda (design) 2)";
   trans->setPropertyList(propList);
   return trans;
}

Design* createMarkDesign(Player *owner, char level) {
   Design *mark = new Design();
   DesignStore *ds = Game::getGame()->getDesignStore();
   map<unsigned int, unsigned int> componentList;

   string name = "Mark " + level;

   mark->setCategoryId(ds->getCategoryByName("Ships"));
   mark->setName( name );
   mark->setDescription( name + string(" battle ship") );
   mark->setOwner( owner->getID() );
   componentList[ ds->getComponentByName(string("Engine") + level) ] = 1;
   componentList[ ds->getComponentByName(string("Battle") + level) ] = 1;
   mark->setComponents(componentList);

   ds->addDesign(mark); // check

   return mark;
}


Design* createScoutDesign(Player *owner) {
   Design* scout = new Design();
   map<unsigned int, unsigned int> componentList;

   DesignStore *ds = Game::getGame()->getDesignStore();

   scout->setCategoryId(ds->getCategoryByName("Ships"));
   scout->setName( "Scout");
   scout->setDescription("Scout ship");
   scout->setOwner( owner->getID());
   componentList[ ds->getComponentByName("Engine1") ] = 1;
   scout->setComponents(componentList);

   ds->addDesign(scout); // check

   return scout;
}

}
