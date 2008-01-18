/*  RFTS rulesset
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

#include <cassert>
#include <cmath>

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

#include "buildfleet.h"
#include "move.h"
#include "productionorder.h"
#include "splitfleet.h"
#include "mergefleet.h"
#include "renamefleet.h"
#include "colonise.h"
#include "bombard.h"

#include "staticobject.h"
#include "planet.h"
#include "universe.h"
#include "fleet.h"

#include "rftsturn.h"
#include "productioninfo.h"
#include "playerinfo.h"
#include "map.h"

#include "rfts.h"

// hacky define :p
#define DEBUG_FN_PRINT() (Logger::getLogger()->debug(__PRETTY_FUNCTION__))

extern "C" {
  #define tp_init librfts_LTX_tp_init
  bool tp_init(){
    return Game::getGame()->setRuleset(new RFTS_::Rfts());
  }
}

namespace RFTS_ {

using std::string;
using std::map;
using std::set;
using std::vector;
using std::advance;
using std::pair;

Rfts::Rfts() {

}

Rfts::~Rfts() {
   PlayerInfo::clear();
}

std::string Rfts::getName() {
   return "TP RFTS";
}

std::string Rfts::getVersion() {
   return "0.5";
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

   ObjectTypeManager* obdm = Game::getGame()->getObjectTypeManager();
   StaticObjectType *eo;

   obdm->addNewObjectType(new UniverseType());

   // galaxy added for tp03
   eo = new StaticObjectType();
   eo->setTypeName("Galaxy");
   eo->setTypeDescription("Galaxy");
   obdm->addNewObjectType(eo);

   eo = new StaticObjectType();
   eo->setTypeName("Star System"); 
   eo->setTypeDescription("A system of stars!");
   obdm->addNewObjectType(eo);

   obdm->addNewObjectType(new PlanetType);
   obdm->addNewObjectType(new FleetType);
}

void Rfts::setOrderTypes() const {
   OrderManager* orm = Game::getGame()->getOrderManager();

   orm->addOrderType(new BuildFleet());
   orm->addOrderType(new ProductionOrder());
   
   orm->addOrderType(new Move());
   orm->addOrderType(new SplitFleet());
   orm->addOrderType(new MergeFleet());
   orm->addOrderType(new RenameFleet());
   orm->addOrderType(new Colonise());
   orm->addOrderType(new Bombard());
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
   prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits)))\
                                 (cons n (string-append (number->string\
                                 (/ n 10)) \" speedy units\")) ) )");
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

void Rfts::createUniverse() {
   DEBUG_FN_PRINT();

   ObjectManager *objman = Game::getGame()->getObjectManager();
   ObjectTypeManager *otypeman = Game::getGame()->getObjectTypeManager();

   uint32_t uniType = otypeman->getObjectTypeByName("Universe");
   IGObject *universe = objman->createNewObject();

   otypeman->setupObject(universe, uniType);
   universe->setName("The Universe");
   StaticObject* uniData = static_cast<StaticObject*>(universe->getObjectBehaviour());
   uniData->setUnitPos(0,0);
   uniData->setSize(123456789123ll);
   objman->addObject(universe);
   
   createStarSystem(*universe, "Acrux", .875, .87);
   createStarSystem(*universe, "Adara", .18, .24);
   createStarSystem(*universe, "Agena", .18, .075);
   createStarSystem(*universe, "Algol", .28, .75);
   createStarSystem(*universe, "Alhema", .86, .24);
   createStarSystem(*universe, "Alioth", .37, .5);
   createStarSystem(*universe, "Almak", .84, .125);
   createStarSystem(*universe, "Altair", .58, .62);
   createStarSystem(*universe, "Aludra", .71, .95);
   createStarSystem(*universe, "Antares", .075, .35);
   createStarSystem(*universe, "Arneb", .485, .37);
   createStarSystem(*universe, "Ascella", .43, .875);
   createStarSystem(*universe, "Canopus", .55, .55);
   createStarSystem(*universe, "Capella", .76, .075);
   createStarSystem(*universe, "Caph", .79, .33);
   createStarSystem(*universe, "Castor", .05, .95);
   createStarSystem(*universe, "Deneb", .85, .55);
   createStarSystem(*universe, "Diphda", .1, .75);
   createStarSystem(*universe, "Dubhe", .475, .5);
   createStarSystem(*universe, "Enif", .62, .77);
   createStarSystem(*universe, "Furud", .64, .075);
   createStarSystem(*universe, "Gemma", .5, .625);
   createStarSystem(*universe, "Gienah", .8, .75);
   createStarSystem(*universe, "Hamal", .75, .625);
   createStarSystem(*universe, "Izar", .29, .21);
   createStarSystem(*universe, "Kochab", .18, .38);
   createStarSystem(*universe, "Megrez", .69, .8);
   createStarSystem(*universe, "Mintaka", .57, .27);
   createStarSystem(*universe, "Mirzam", .37, .8);
   createStarSystem(*universe, "Mizar", .6, .375);
   createStarSystem(*universe, "Nath", .7, .375);
   createStarSystem(*universe, "Nihal", .23, .6);
   createStarSystem(*universe, "Nunki", .39, .25);
   createStarSystem(*universe, "Phaeda", .58, .95);
   createStarSystem(*universe, "Polaris", .65, .2);
   createStarSystem(*universe, "Pollux", .68, .875);
   createStarSystem(*universe, "Procyon", .15, .6);
   createStarSystem(*universe, "Rastaban", .29, .375);
   createStarSystem(*universe, "Regulus", .69, .6);
   createStarSystem(*universe, "Rigel", .8, .8);
   createStarSystem(*universe, "Ross", .88, .61);
   createStarSystem(*universe, "Sabik", .6, .745);
   createStarSystem(*universe, "Saiph", .23, .875);
   createStarSystem(*universe, "Schedar", .27, .6);
   createStarSystem(*universe, "Sirius", .42, .18);
   createStarSystem(*universe, "Shedir", .57, .47);
   createStarSystem(*universe, "Spica", .475, .79);
   createStarSystem(*universe, "Tarazed", .9, .42);
   createStarSystem(*universe, "Thuban", .83, .4);
   createStarSystem(*universe, "Vega", .23, .7);
   createStarSystem(*universe, "Wesen", .39, .65);
   createStarSystem(*universe, "Wolf", .54, .92);
   createStarSystem(*universe, "Zosma", .86, .05);
   
}

IGObject* Rfts::createStarSystem(IGObject& universe, const string& name,
                  double unitX, double unitY)
{
   Game *game = Game::getGame();
   ObjectTypeManager *otypeman = game->getObjectTypeManager();
   
   IGObject *starSys = game->getObjectManager()->createNewObject();

   otypeman->setupObject(starSys, otypeman->getObjectTypeByName("Star System"));
   starSys->setName(name);
   StaticObject* starSysData = dynamic_cast<StaticObject*>(starSys->getObjectBehaviour());
   starSysData->setUnitPos(unitX, unitY);
   
   starSys->addToParent(universe.getID());
   game->getObjectManager()->addObject(starSys);

   Random* rand = game->getRandom();

   int numPlanets = rand->getInRange(0, 3);
   string planetName;
   
   for(char i = '1'; i < numPlanets + '1'; i++)
   {
      planetName = starSys->getName() + " " + i;
      createPlanet(*starSys, planetName, starSysData->getPosition() + getRandPlanetOffset());
   }

   return starSys;
}

IGObject* Rfts::createPlanet(IGObject& parentStarSys, const string& name,const Vector3d& location) {

   Game *game = Game::getGame();
   ObjectTypeManager *otypeman = game->getObjectTypeManager();

   IGObject *planet = game->getObjectManager()->createNewObject();

   otypeman->setupObject(planet, otypeman->getObjectTypeByName("Planet"));
   planet->setName(name);
   Planet* planetData = static_cast<Planet*>(planet->getObjectBehaviour());
   planetData->setPosition(location); // OK because unit pos isn't useful for planets
   planetData->setDefaultResources();
   
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

void Rfts::createResources() {
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
   res->setDescription("Population available for colonisation");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);

   res = new ResourceDescription();
   res->setNameSingular("Ship Technology");
   res->setNamePlural("Ship Technology");
   res->setUnitSingular("point");
   res->setUnitPlural("points");
   res->setDescription("Research points in ship/pdb technology");
   res->setMass(0);
   res->setVolume(0);
   resMan->addResourceDescription(res);

   pair<ResourceDescription*,ResourceDescription*> pdbRes;
   
   pdbRes = createPdbResource('1');
   resMan->addResourceDescription(pdbRes.first);
   resMan->addResourceDescription(pdbRes.second);

   pdbRes = createPdbResource('2');
   resMan->addResourceDescription(pdbRes.first);
   resMan->addResourceDescription(pdbRes.second);

   pdbRes = createPdbResource('3');
   resMan->addResourceDescription(pdbRes.first);
   resMan->addResourceDescription(pdbRes.second);
}

pair<ResourceDescription*,ResourceDescription*> Rfts::createPdbResource(char level) const {

   string name = string("PDB") + level;
   
   ResourceDescription* pdb = new ResourceDescription();
   pdb->setNameSingular(name);
   pdb->setNamePlural(name + "s");
   pdb->setUnitSingular("unit");
   pdb->setUnitPlural("units");
   pdb->setDescription("Planetary Defense Bases: defend against attacking fleets!");
   pdb->setMass(0);
   pdb->setVolume(0);

   name += " Maintenance";

   ResourceDescription* maint = new ResourceDescription();
   maint->setNameSingular(name);
   maint->setNamePlural(name);
   maint->setUnitSingular("unit");
   maint->setUnitPlural("units");
   maint->setDescription("Planetary Defense Bases Maintenance: keep your bases in working order!\
                           (1:1 ration to PDBs required)");
   maint->setMass(0);
   maint->setVolume(0);

   return pair<ResourceDescription*,ResourceDescription*>(pdb, maint);
}

void Rfts::startGame() {
   DEBUG_FN_PRINT();
   
   Settings* settings = Settings::getSettings();
   if(settings->get("turn_length_over_threshold") == "")
   {
      settings->set("turn_length_over_threshold", "180");
      settings->set("turn_player_threshold", "0");
      settings->set("turn_length_under_threshold", "180");
   }
   
   if(settings->get("max_players") == "")
      settings->set("max_players", "4");
      
   if(settings->get("game_length") == "")
      settings->set("game_length", "60");
}

bool Rfts::onAddPlayer(Player *player) {
   DEBUG_FN_PRINT();
   unsigned players = Game::getGame()->getPlayerManager()->getNumPlayers();
   unsigned maxPlayers = strtol(Settings::getSettings()->get("max_players").c_str(), NULL, 10);
   return players < maxPlayers;
}
void Rfts::onPlayerAdded(Player *player) {
   DEBUG_FN_PRINT();

   Game *game = Game::getGame();
   ObjectManager *om = Game::getGame()->getObjectManager();

   PlayerView* playerview = player->getPlayerView();

   //Assuming that all designs should be visible.
   // Please fix if this is not the case
   std::set<uint32_t> allotherdesigns = Game::getGame()->getDesignStore()->getDesignIds();
   for(std::set<uint32_t>::const_iterator desid = allotherdesigns.begin(); desid != allotherdesigns.end(); ++desid){
     DesignView* dv = new DesignView();
     dv->setDesignId(*desid);
     dv->setIsCompletelyVisible(true);
     playerview->addVisibleDesign(dv);
   }
   
   std::set<uint32_t> mydesignids;
   
   for(uint32_t i = 1; i <= game->getDesignStore()->getMaxComponentId(); ++i){
      playerview->addUsableComponent(i);
   }

   // test : set the 2nd object - a planet - to be owned by the player
   IGObject *homePlanet = choosePlayerPlanet();
   Planet* pData = dynamic_cast<Planet*>(homePlanet->getObjectBehaviour());
   pData->setOwner(player->getID());

   Logger::getLogger()->debug("Making player's fleet");
   
   IGObject* fleet = createEmptyFleet( player, om->getObject(homePlanet->getParent()),
                                        player->getName() + "'s fleet");

   // give 'em a scout to start
   Design* scout = createScoutDesign(player);
   
   dynamic_cast<Fleet*>(fleet->getObjectBehaviour())->addShips( scout->getDesignId(), 1);
   scout->addUnderConstruction(2);
   scout->addComplete(2);
   game->getDesignStore()->designCountsUpdated(scout);
   mydesignids.insert(scout->getDesignId());

   // start them out with access to mark1
   Design* mark1 = createMarkDesign(player, '1');
   mydesignids.insert(mark1->getDesignId());

   // and a transport (save the transport id for easy searching later)
   Design *trans = createTransportDesign(player);
   PlayerInfo::getPlayerInfo(player->getID()).setTransportId(trans->getDesignId());
   mydesignids.insert(trans->getDesignId());

   game->getObjectManager()->addObject(fleet);

   Logger::getLogger()->debug( "done making fleet");

   // set visible objects
   set<uint32_t> gameObjects = om->getAllIds();
   set<uint32_t> ownedObjects;
   findOwnedObjects(player->getID(), gameObjects, ownedObjects);
   setVisibleObjects(player, ownedObjects);

   Message *welcome = new Message();
   welcome->setSubject("Welcome to TP RFTS! Here's a brief reminder of some rules");
   welcome->setBody("<b><u>3 Turn Cycle</b></u>:<br />\
                     1st turn: planetary production, fleet building, and fleet movement<br />\
                     2nd turn: fleet building and fleet movement<br />\
                     3rd turn: fleet movement only<br />\
                     *repeat*<br /><br />\
                     <b><u>COSTS:</b></u><br />\
                     <table border=\"0\">\
                     <tr><th>Industry: 10        <td> Social Env:4\
                     <tr><th>Planetary Env: 8    <td> Pop. Maint: 1\
                     <tr><th>Colonists: 5        <td> Scouts: 3\
                     <tr><th>Mark1: 14           <td> Mark2: 30\
                     <tr><th>Mark3: 80           <td> Mark4: 120\
                     <tr><th>PDB: 4/8/16         <td> PDB Maint.: 1/2/2\
                     <tr><th>Ship Tech: 1</table><br /><br />\
                     Ship Tech. Upgrade Levels (determind Marks and PDBs availble):<br />\
                     Level 2: 400 <br />\
                     Level 3: 1,000 <br />\
                     Level 4: 2,000 <br /><br />\
                     <a href=\"http://www.thousandparsec.net/wiki/TP_RFTS\"><h5>More information on how\
                      to play</h5></a>");

   player->postToBoard(welcome);

   Game::getGame()->getPlayerManager()->updatePlayer(player->getID());
   
   //let everyone see our designs
   // Again, modify if this isn't the case
   std::set<uint32_t> playerids = game->getPlayerManager()->getAllIds();
    for(std::set<uint32_t>::iterator playerit = playerids.begin(); playerit != playerids.end(); ++playerit){
      if(*playerit == player->getID())
        continue;
      
      Player* oplayer = game->getPlayerManager()->getPlayer(*playerit);
      for(std::set<uint32_t>::const_iterator desid = mydesignids.begin(); desid != mydesignids.end(); ++desid){
        DesignView* dv = new DesignView();
        dv->setDesignId(*desid);
        dv->setIsCompletelyVisible(true);
        oplayer->getPlayerView()->addVisibleDesign(dv);
      }
      game->getPlayerManager()->updatePlayer(oplayer->getID());
    }
   
}

// make sure to start the player in a non-occupied area
IGObject* Rfts::choosePlayerPlanet() const {
   DEBUG_FN_PRINT();

   Game *game = Game::getGame();
   ObjectManager *om = game->getObjectManager();
   Random *rand = game->getRandom();

   IGObject *universe = om->getObject(0);
   set<uint32_t> starSystems = universe->getContainedObjects();

   IGObject *homePlanet = NULL;
   unsigned searchedSystems = 0;

   while(homePlanet == NULL && searchedSystems < starSystems.size() * 4./3)
   {
      // pick rand Star System to search
      set<uint32_t>::iterator starSysI = starSystems.begin();
      advance(starSysI, rand->getInRange(static_cast<uint32_t>(1), starSystems.size()-1));
      IGObject *starSys = om->getObject(*starSysI);

      searchedSystems++;
      
      set<uint32_t> planets = starSys->getContainedObjects(); // (might not -actually- be planets)
      unsigned starSysClear = 0;
      int primaryPlanet = -1;
      
      for(set<uint32_t>::iterator i = planets.begin(); i != planets.end(); i++)
      {
         ObjectBehaviour *objDataI = om->getObject(*i)->getObjectBehaviour();
         
         if(dynamic_cast<OwnedObject*>(objDataI)->getOwner() == 0) // no owner
            starSysClear++;
         if(dynamic_cast<StaticObject*>(objDataI)->getSize() == ProductionInfo::PRIMARY)
            primaryPlanet = *i;
         
      }

      if(planets.size() != 0 && starSysClear == planets.size() && primaryPlanet != -1)
      {
         homePlanet = om->getObject(primaryPlanet); // must be a planet because it's owner-less
         Planet *homePlanetData =  dynamic_cast<Planet*>(homePlanet->getObjectBehaviour());
         
         // set initial planet values
         homePlanetData->setResource("Resource Point", 200);
         homePlanetData->setResource("Population", 50); 
         homePlanetData->setResource("Industry", 15);
         homePlanetData->setResource("Social Environment", 52);
         homePlanetData->setResource("Planetary Environment", 55);
         homePlanetData->setResource("PDB1", 12);
      }
   }

   assert(homePlanet != NULL); // possible, will write a last-ditch effort later maybe
   
   return homePlanet;
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
      "(if (< (designType._num-components design) 3) "
      "(cons #t \"\") "
      "(cons #f \"This is a complete component, nothing else can be included\")))");
   propList[ds->getPropertyByName("Speed")] = string("(lambda (design) (* .1 ") +  techLevel + string("))");
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
      "(if (< (designType._num-components design) 3) "
      "(cons #t \"\") "
      "(cons #f \"This is a complete component, nothing else can be included\")))");
   propList[ ds->getPropertyByName("Attack") ] = string("(lambda (design) (* 5 ") + techLevel + string("))");
   propList[ ds->getPropertyByName("Armour") ] = string("(lambda (design) (* 5 ") + techLevel + string("))");
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
      "(if (< (designType._num-components design) 3) "
      "(cons #t \"\") "
      "(cons #f \"This is a complete component, nothing else can be included\")))");
   propList[ ds->getPropertyByName("Colonise") ] = "(lambda (design) 1)";
   trans->setPropertyList(propList);
   return trans;
}

Design* createMarkDesign(Player *owner, char level) {
   Design *mark = new Design();
   DesignStore *ds = Game::getGame()->getDesignStore();
   map<unsigned int, unsigned int> componentList;

   string name = string("Mark") + level;

   mark->setCategoryId(ds->getCategoryByName("Ships"));
   mark->setName( name );
   mark->setDescription( name + string(" battle ship") );
   mark->setOwner( owner->getID() );
   componentList[ ds->getComponentByName(string("Engine") + level) ] = 1;
   componentList[ ds->getComponentByName(string("Battle") + level) ] = 1;
   mark->setComponents(componentList);

   ds->addDesign(mark);

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

   ds->addDesign(scout);

   return scout;
}

Design* createTransportDesign(Player *owner) {
   Design* trans = new Design();
   map<unsigned int, unsigned int> componentList;

   DesignStore *ds = Game::getGame()->getDesignStore();

   trans->setCategoryId(ds->getCategoryByName("Ships"));
   trans->setName("Transport");
   trans->setDescription("Transport ship");
   trans->setOwner( owner->getID());
   componentList[ ds->getComponentByName("Engine1") ] = 1;
   componentList[ ds->getComponentByName("Transport") ] = 1;
   trans->setComponents(componentList);

   ds->addDesign(trans);

   return trans;
}

}
