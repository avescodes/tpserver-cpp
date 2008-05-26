/*  Risk rulesset class
 *
 *  Copyright (C) 2008  Ryan Neufeld and the Thousand Parsec Project
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
 
// System includes 
#include <sstream> 

// tpserver includes 
#include <tpserver/player.h>
#include <tpserver/playermanager.h>
#include <tpserver/playerview.h>
#include <tpserver/game.h>
#include <tpserver/logging.h>

#include <tpserver/property.h>
#include <tpserver/component.h>
#include <tpserver/design.h>
#include <tpserver/designview.h>
#include <tpserver/category.h>
#include <tpserver/designstore.h>

#include <tpserver/object.h>
#include <tpserver/objecttypemanager.h>
#include <tpserver/ordermanager.h>
#include <tpserver/orderqueue.h>
#include <tpserver/orderqueueobjectparam.h>
#include <tpserver/objectmanager.h>
#include <tpserver/objectparameter.h>
#include <tpserver/resourcemanager.h>
#include <tpserver/resourcedescription.h>
#include <tpserver/prng.h>
#include <tpserver/settings.h>
#include <tpserver/message.h>

/* Minisec Includes
#include <tpserver/game.h>
#include <tpserver/object.h>
#include <tpserver/objectview.h>
#include <tpserver/objectmanager.h>
#include "universe.h"
#include "emptyobject.h"
#include "planet.h"
#include "fleet.h"

#include <tpserver/player.h>
#include <tpserver/playerview.h>
#include "rspcombat.h"
#include <tpserver/designstore.h>
#include <tpserver/ordermanager.h>
#include "nop.h"
#include "move.h"
#include "build.h"
#include "colonise.h"
#include "splitfleet.h"
#include "mergefleet.h"
#include <tpserver/property.h>
#include <tpserver/component.h>
#include <tpserver/design.h>
#include <tpserver/designview.h>
#include <tpserver/category.h>
#include <tpserver/logging.h>
#include <tpserver/playermanager.h>
#include <tpserver/resourcedescription.h>
#include <tpserver/resourcemanager.h>

#include <tpserver/prng.h>
#include <tpserver/playermanager.h>
#include <tpserver/orderqueue.h>
#include <tpserver/orderqueueobjectparam.h>
#include "minisecturn.h"
*/

// risk includes 
#include "risk.h"
#include "riskturn.h" 
#include "colonize.h"
#include "move.h"
#include "reinforce.h"
#include "containertypes.h"
#include "map.h"
#include "staticobject.h"
#include "universe.h"
#include "ownedobject.h"
#include "planet.h"

//Tyler's "hacky define :p"
#define DEBUG_FN_PRINT() (Logger::getLogger()->debug(__PRETTY_FUNCTION__))

//the libtool magic required
extern "C" { 
    #define tp_init librisk_LTX_tp_init 
    bool tp_init(){ 
        return Game::getGame()->setRuleset(new RiskRuleset::Risk());
    } 
}

namespace RiskRuleset {
  
using std::string;
using std::map;
using std::set;
using std::vector;
using std::advance;
using std::pair;

Risk::Risk(){
//Minisec has a parent of random(NULL), whats with that?	
}

Risk::~Risk(){
//minisec simply deletes the random if not NULL    
}

std::string Risk::getName(){
    return "Risk";
}

std::string Risk::getVersion(){
    return "0.0";
}

void Risk::initGame(){
    Logger::getLogger()->info("Risk initialised");

    Game::getGame()->setTurnProcess(new RiskTurn());
    
    setObjectTypes();

    setOrderTypes();
}

void Risk::setObjectTypes() const{
    ObjectTypeManager* obdm = Game::getGame()->getObjectTypeManager();
    StaticObjectType* eo;    
    
    obdm->addNewObjectType(new UniverseType());
    
    eo = new StaticObjectType();
    eo->setTypeName("Galaxy");
    eo->setTypeDescription("A set of star systems that provides a bonus of reinforcements to any player completely controlling it.");
    obdm->addNewObjectType(eo);
    //Need to add the constellation bonus for complete ownership
    
    eo = new StaticObjectType();
    eo->setTypeName("Star System");
    eo->setTypeDescription("A territory capable of being controlled and having any number of armies.");
    obdm->addNewObjectType(eo);
    
    obdm->addNewObjectType(new PlanetType);   //may need to special some stuff here
    //There are no fleets in risk - hence no fleet type
}

void Risk::setOrderTypes() const{
    OrderManager* orm = Game::getGame()->getOrderManager();
    
    //To be an action availible on all unowned planets
    //With planet selected order is to colonize with NUMBER armies
    orm->addOrderType(new Colonize());
    
    //To be an action availible on all player owned planets
    //With planet selected order is to reinforce with NUMBER armies
    orm->addOrderType(new Reinforce());
    
    //To be an action availible on all player owned planets
    //With planet selected order is to reinforce with NUMBER armies
    orm->addOrderType(new Move());
    
}

void Risk::createGame(){
    Logger::getLogger()->info("Risk created");
    
    //set up universe (universe->galaxies->star sys->planet)
    createUniverse();
}

void Risk::createUniverse() {
  DEBUG_FN_PRINT();
  ObjectManager *objman = Game::getGame()->getObjectManager();
  ObjectTypeManager *otypeman = Game::getGame()->getObjectTypeManager();

  uint32_t uniType = otypeman->getObjectTypeByName("Universe");
  IGObject *universe = objman->createNewObject();

  otypeman->setupObject(universe, uniType);
  universe->setName("The Universe");
  StaticObject* uniData = static_cast<StaticObject*>(universe->getObjectBehaviour());
	uniData->setSize(123456789123ll);
  uniData->setUnitPos(.5,.5);
  objman->addObject(universe);
   
  //create galaxies and keep reference for system creation
  IGObject *gal_cassiopea = createGalaxy(*universe, "Cassiopea"); //North America
  IGObject *gal_cygnus = createGalaxy(*universe, "Cygnus"); //South America
  IGObject *gal_cepheus = createGalaxy(*universe, "Cepheus"); //Europe
  IGObject *gal_orion = createGalaxy(*universe, "Orion"); //Africa
  IGObject *gal_scorpius = createGalaxy(*universe, "Scorpius"); //Russia
  IGObject *gal_crux = createGalaxy(*universe, "Crux Australis"); //Australia
  
  //create systems
  // Cassiopea Systems (North America, Bonus 5)
  createStarSystem(*gal_cassiopea, "cassiopea01", .1, .1);  //SOME TEST VALUES
  createStarSystem(*gal_cassiopea, "cassiopea02", .1, .2);
  createStarSystem(*gal_cassiopea, "cassiopea03", .1, .3);
  createStarSystem(*gal_cassiopea, "cassiopea04", .1, .4);
  createStarSystem(*gal_cassiopea, "cassiopea05", .1, .5);
  createStarSystem(*gal_cassiopea, "cassiopea06", .1, .6);
  createStarSystem(*gal_cassiopea, "cassiopea07", .1, .7);
  createStarSystem(*gal_cassiopea, "cassiopea08", .1, .8);
  createStarSystem(*gal_cassiopea, "cassiopea09", .1, .9);

  // Cygnus Systems (South America, Bonus 2)
  createStarSystem(*gal_cygnus, "cygnus01", .2, .1);  //SOME TEST VALUES
  createStarSystem(*gal_cygnus, "cygnus02", .2, .2);
  createStarSystem(*gal_cygnus, "cygnus03", .2, .3);
  createStarSystem(*gal_cygnus, "cygnus04", .2, .4);
    
  // Cepheus Systems (Europe, Bonus 5)
  createStarSystem(*gal_cepheus, "cepheus01", .3, .1);  //SOME TEST VALUES
  createStarSystem(*gal_cepheus, "cepheus02", .3, .2);
  createStarSystem(*gal_cepheus, "cepheus03", .3, .3);
  createStarSystem(*gal_cepheus, "cepheus04", .3, .4);
  createStarSystem(*gal_cepheus, "cepheus05", .3, .5);
  createStarSystem(*gal_cepheus, "cepheus06", .3, .6);
  createStarSystem(*gal_cepheus, "cepheus07", .3, .7);

  // Orion Systens (Africa, Bonus 3)
  createStarSystem(*gal_orion, "orion01", .4, .1);  //SOME TEST VALUES
  createStarSystem(*gal_orion, "orion02", .4, .2);
  createStarSystem(*gal_orion, "orion03", .4, .3);
  createStarSystem(*gal_orion, "orion04", .4, .4);
  createStarSystem(*gal_orion, "orion05", .4, .5);
  createStarSystem(*gal_orion, "orion06", .4, .6);
  
  // Scorpius Systems (Russia, Bonus 7)
  createStarSystem(*gal_scorpius, "cassiopea01", .5, .1);  //SOME TEST VALUES
  createStarSystem(*gal_scorpius, "cassiopea02", .5, .2);
  createStarSystem(*gal_scorpius, "cassiopea03", .5, .3);
  createStarSystem(*gal_scorpius, "cassiopea04", .5, .4);
  createStarSystem(*gal_scorpius, "cassiopea05", .5, .5);
  createStarSystem(*gal_scorpius, "cassiopea06", .5, .6);
  createStarSystem(*gal_scorpius, "cassiopea07", .5, .7);
  createStarSystem(*gal_scorpius, "cassiopea08", .5, .8);
  createStarSystem(*gal_scorpius, "cassiopea09", .5, .9);
  createStarSystem(*gal_scorpius, "cassiopea10", .5, 1.0);
  createStarSystem(*gal_scorpius, "cassiopea11", .5, 1.1);
  createStarSystem(*gal_scorpius, "cassiopea12", .5, 1.2);
  
  // Crux Systens
  createStarSystem(*gal_crux, "crux01", .6, .1);  //SOME TEST VALUES
  createStarSystem(*gal_crux, "crux02", .6, .2);
  createStarSystem(*gal_crux, "crux03", .6, .3);
  createStarSystem(*gal_crux, "crux04", .6, .4);
}

IGObject* Risk::createGalaxy(IGObject& parent, const string& name) {
  DEBUG_FN_PRINT();
  Game *game = Game::getGame();
  ObjectTypeManager *otypeman = game->getObjectTypeManager();
  
  IGObject *galaxy = game->getObjectManager()->createNewObject();
  
  otypeman->setupObject(galaxy, otypeman->getObjectTypeByName("Galaxy"));
  galaxy->setName(name);
  
  galaxy->addToParent(parent.getID());
  game->getObjectManager()->addObject(galaxy);
  
  return galaxy;
}

IGObject* Risk::createStarSystem(IGObject& parent, const string& name, double unitX, double unitY) {
  Game *game = Game::getGame();
  ObjectTypeManager *otypeman = game->getObjectTypeManager();

  IGObject *starSys = game->getObjectManager()->createNewObject();

  otypeman->setupObject(starSys, otypeman->getObjectTypeByName("Star System"));
  starSys->setName(name);
  StaticObject* starSysData = dynamic_cast<StaticObject*>(starSys->getObjectBehaviour());
  starSysData->setUnitPos(unitX, unitY);
  
  starSys->addToParent(parent.getID());
  game->getObjectManager()->addObject(starSys);

  string planetName;
  
  planetName = starSys->getName() + " Prime";
  createPlanet(*starSys, planetName, starSysData->getPosition() + getRandPlanetOffset());
  return starSys;
}

IGObject* Risk::createPlanet(IGObject& parentStarSys, const string& name,const Vector3d& location) {

   Game *game = Game::getGame();
   ObjectTypeManager *otypeman = game->getObjectTypeManager();

   IGObject *planet = game->getObjectManager()->createNewObject();

   otypeman->setupObject(planet, otypeman->getObjectTypeByName("Planet"));
   planet->setName(name);
   Planet* planetData = static_cast<Planet*>(planet->getObjectBehaviour());
   planetData->setPosition(location); // OK because unit pos isn't useful for planets
   
   OrderQueue *planetOrders = new OrderQueue();
   planetOrders->setObjectId(planet->getID());
   planetOrders->addOwner(0);
   game->getOrderManager()->addOrderQueue(planetOrders);
   OrderQueueObjectParam* oqop = static_cast<OrderQueueObjectParam*>
                                       (planet->getParameterByType(obpT_Order_Queue));
   oqop->setQueueId(planetOrders->getQueueId());
   planetData->setOrderTypes();
  
   planet->addToParent(parentStarSys.getID());
   game->getObjectManager()->addObject(planet);

   return planet;
}

void Risk::startGame(){
    Logger::getLogger()->info("Risk started");
    
    //Modelling minisec
    /*
    There is some resource setup that is not applicable to risk
    followed by getting settings, this will be applicable and as such here is the snippet:
    Settings* settings = Settings::getSettings();
    if(settings->get("turn_length_over_threshold") == ""){
      settings->set("turn_length_over_threshold", "600");
      settings->set("turn_player_threshold", "0");
      settings->set("turn_length_under_threshold", "600");
      
     They look to be defaults if a turnlength isn't set
    */
}

bool Risk::onAddPlayer(Player* player){
    Logger::getLogger()->debug("Risk onAddPlayer"); 
    Game* game = Game::getGame();

    bool canJoin = true;            
    uint32_t max_players = atoi(Settings::getSettings()->get("risk_max_players").c_str() );
    bool isStarted = game->isStarted();
    uint32_t cur_players = game->getPlayerManager()->getNumPlayers();
    
    //If ( max players exceeded OR (game's started AND there are no open spaces))    
        //disallow joining
    if ( (cur_players >= max_players)||(isStarted && !isBoardClaimed()) )
        canJoin = false;  
    else
        canJoin = true;
    
    return canJoin; 
}

bool Risk::isBoardClaimed() const{
    //This method will run through board and check if all territories
        //are claimed or not

    //TODO: Check board to determine "claimedness"

    return false;
}

void Risk::onPlayerAdded(Player* player){
    Logger::getLogger()->debug("Risk onPlayerAdded");
    
    //Modelling minisec
    /*
    Game reference is grabed
    
    playerview reference is grabbed
    
    if player is guest, set IsAlive to false, otherwise 
    
    set the player to see other designs (not sure what this does -- believe it relates 
        to ships)
    Player gets some default objects created for them (NA to risk)
    (risk) -> should simply indicate the player gets some resources to spend 
        (ideally players won't join midgame thogh)
        
    create a system for player (NA to risk)
        
    */   
}

} //end namespace RiskRuleset

