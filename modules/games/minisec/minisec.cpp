/*  MiniSec ruleset
 *
 *  Copyright (C) 2003-2005, 2007, 2008  Lee Begg and the Thousand Parsec Project
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
#include <sstream>
#include <fstream>
#include <cstring>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <tpserver/game.h>
#include <tpserver/object.h>
#include <tpserver/objectview.h>
#include <tpserver/objectmanager.h>
#include "universe.h"
#include "emptyobject.h"
#include "planet.h"
#include "fleet.h"
#include <tpserver/objecttypemanager.h>
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
#include <tpserver/settings.h>
#include <tpserver/prng.h>
#include <tpserver/playermanager.h>
#include <tpserver/orderqueue.h>
#include <tpserver/orderqueueobjectparam.h>
#include "minisecturn.h"

#include "minisec.h"

static char const * const defaultNames[] = {
  "Barnard's Star",  "Gielgud",             "Ventana",
  "Aleph Prime",     "Ventil",              "Sagitaria",
  "Drifter",         "Ptelemicus",          "Centanis",
  "Mendelis",        "Cassious' Shadow",    "Llentim",
  "Redoubt",         "Kelper",              "Cemara",
  "Cilantarius",     "Kya",                 "Lanternis",
  "Illatis",         "Rintim",              "Uvaharim",
  "Plaetais",        "Denderis",            "Desiderata",
  "Illuntara",       "Ivemteris",           "Wetcher",
  "Monanara",        "Clesasia",            "RumRunner",
  "Last Chance",     "Kiuper Shadow",       "NGC 42059",
  "Ceti Alpha",      "Surreptitious",       "Lupus Fold",
  "Atlantis",        "Draconis",            "Muir's Gold",
  "Fools Errand",    "Wrenganis",           "Humph",
  "Byzantis",        "Torontis",            "Radiant Pool"};

/**
* Base class for various ways to get names for starsystems.
*/
class Names {
	uint64_t systems;

public:
	Names() {
		systems = 0;
	}

	/**
	 * Get a name which is "System xx".
	 */
	virtual std::string getName() {
			std::ostringstream name;
			name.str("");
			name << "System " << ++systems;

			return name.str();
  }
	virtual ~Names() {};
};

/**
 * Use a predefined list of names in this file and then fall back to "System xx" names.
 */
class NamesSet : public Names {

  std::set<const char*> names;
  Random* rand;

public:
  NamesSet(Random* r) :
      Names(),
      names(defaultNames, defaultNames + (sizeof(defaultNames) / sizeof(defaultNames[0]))) 
  {
    rand  = r;
  }

  std::string getName() {
    if (names.size() > 0) {
      // Choose a random name
      unsigned int choice = rand->getInRange(0U, names.size() - 1);

      std::set<const char*>::iterator name = names.begin();
      advance(name, choice);
      assert(name != names.end());

      names.erase(name);

      return std::string(*name);
    } else {
      // Opps we ran out of precreated names!
      return Names::getName();
    }
  }
};


// FIXME: These belong in some type of string helper file
#define WHITESPACE " \t\f\v\n\r"

/**
 * Strip any trailing or leading characters of a given type
 * 
 * @param str The string to strip (unmodified)
 * @param sep The chars types to strip
 * @return    A stripped string
 */
inline std::string strip(std::string const& str, char const* sep) {
    std::string::size_type first = str.find_first_not_of(sep);
    std::string::size_type last  = str.find_last_not_of(sep);
    if ( first == std::string::npos || last  == std::string::npos )
        return std::string("");

    return str.substr(first, (last-first)+1);
}
/**
 * Does a string start with another string?
 * 
 * @param str      The string to match against
 * @param starting What the string should start with.
 * @return Does the string start with starting?
 */
inline bool startswith(std::string const& str, std::string const& starting) {
    if (str.length() < starting.length())
        return false;

    return str.substr(0, starting.length()) == starting;
}


#define BUFFERSIZE 1024

/**
 * Use a list of names from a file then fall back to "System xx" names.
 */
class NamesFile : public Names {
  std::istream* m_file;

  /**
   * Read a line from a stream.
   */ 
  std::string readline() {
    std::string buffer;    

    // Get the next line
    // Loop while the buffer is empty 
    while (buffer.size() == 0 || m_file->fail()) {
        // Temporary storage for the line
        char cbuffer[BUFFERSIZE];
        uint32_t cbuffer_amount = 0;

        m_file->getline(cbuffer, BUFFERSIZE);
        cbuffer_amount = m_file->gcount();      // The amount of data which was put in our buffer

        if (cbuffer_amount > 0)
            buffer.append(std::string(cbuffer, cbuffer_amount-1));

        // Have we reached the end of the file
        if (m_file->eof())
            break;
    }

    return buffer;
  }

public:
  NamesFile(std::istream* f) : Names() {
    m_file = f;

  }

  ~NamesFile() {
    delete m_file;
  }

  std::string getName() {
    while (!m_file->eof()) {
      // Choose a random name
      std::string s = strip(readline(), WHITESPACE);
      if (s.length() == 0)
        continue;

      return s;
    }

    // Opps we ran out of precreated names!
    return Names::getName();
  }

};


extern "C" {
  #define tp_init libminisec_LTX_tp_init
  bool tp_init(){
    return Game::getGame()->setRuleset(new MiniSec());
  }
}

MiniSec::MiniSec() : random(NULL){
}

MiniSec::~MiniSec(){
  if(random != NULL){
    delete random;
  }
}

std::string MiniSec::getName(){
  return "MiniSec";
}

std::string MiniSec::getVersion(){
  return "0.3";
}

void MiniSec::initGame(){
  Game* game = Game::getGame();
  
  MinisecTurn* turn = new MinisecTurn();
  game->setTurnProcess(turn);
  

  ObjectTypeManager* obdm = game->getObjectTypeManager();
  obdm->addNewObjectType(new UniverseType());
  EmptyObjectType * eo = new EmptyObjectType();
  eo->setTypeName("Galaxy");
  eo->setTypeDescription("The Galaxy Object type");
  obdm->addNewObjectType(eo);
  eo = new EmptyObjectType();
  eo->setTypeName("Star System");
  eo->setTypeDescription("The Star System Object type");
  obdm->addNewObjectType(eo);
  uint32_t pt = obdm->addNewObjectType(new PlanetType());
  uint32_t ft = obdm->addNewObjectType(new FleetType());
  
  turn->setPlanetType(pt);
  turn->setFleetType(ft);

  OrderManager* ordm = game->getOrderManager();
  ordm->addOrderType(new Nop());
  ordm->addOrderType(new Move());
  ordm->addOrderType(new Build());
  ordm->addOrderType(new Colonise());
  ordm->addOrderType(new SplitFleet());
  ordm->addOrderType(new MergeFleet());

}

void MiniSec::createGame(){
  Game* game = Game::getGame();
  
  std::set<uint32_t> catids;
  
    DesignStore *ds = game->getDesignStore();
    Category * cat = new Category();
    cat->setName("Ships");
    cat->setDescription("The Ship design and component category");
    ds->addCategory(cat);
    catids.insert(cat->getCategoryId());
    
    Property* prop = new Property();
    prop->setCategoryIds(catids);
    prop->setRank(0);
    prop->setName("Speed");
    prop->setDisplayName("Speed");
    prop->setDescription("The number of units the ship can move each turn");
    prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (string-append (number->string (/ n 1000000)) \" mega-units\")) ) )");
    prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
    ds->addProperty(prop);
    
    prop = new Property();
    prop->setCategoryIds(catids);
    prop->setRank(0);
    prop->setName("BuildTime");
    prop->setDisplayName("Build Time");
    prop->setDescription("The number of turns to build the ship");
    prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (string-append (number->string n) \" turns\")) ) )");
    prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
    ds->addProperty(prop);
    
    prop = new Property();
    prop->setCategoryIds(catids);
    prop->setRank(0);
    prop->setName("Armour");
    prop->setDisplayName("Armour");
    prop->setDescription("The amount of amour on the ship");
    prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (string-append (number->string n) \" HP\")) ) )");
    prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
    ds->addProperty(prop);
    
    prop = new Property();
    prop->setCategoryIds(catids);
    prop->setRank(0);
    prop->setName("WeaponWin");
    prop->setDisplayName("Weapon Strength at Win");
    prop->setDescription("The number of HP to do to the fired at ship when RSP wins");
    prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (string-append (number->string n) \" HP\")) ) )");
    prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
    ds->addProperty(prop);
    
    prop = new Property();
    prop->setCategoryIds(catids);
    prop->setRank(0);
    prop->setName("WeaponDraw");
    prop->setDisplayName("Weapon Strength at Draw");
    prop->setDescription("The number of HP to do to the fired at ship when RSP draws");
    prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (string-append (number->string n) \" HP\")) ) )");
    prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
    ds->addProperty(prop);
    
    prop = new Property();
    prop->setCategoryIds(catids);
    prop->setRank(0);
    prop->setName("Colonise");
    prop->setDisplayName("Can Colonise Planets");
    prop->setDescription("Can the ship colonise planets");
    prop->setTpclDisplayFunction("(lambda (design bits) (let ((n (apply + bits))) (cons n (if (= n 1) \"Yes\" \"No\")) ) )");
    prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
    ds->addProperty(prop);
    
    prop = new Property();
    prop->setCategoryIds(catids);
    prop->setRank(0);
    prop->setName("_num-components");
    prop->setDisplayName("Number of Conponents");
    prop->setDescription("The total number of components in the design");
    prop->setTpclDisplayFunction(
        "(lambda (design bits)"
        "(let ((n (apply + bits)))"
        "(cons n (string-append (number->string n) \" components\"))))");
        prop->setTpclRequirementsFunction("(lambda (design) (cons #t \"\"))");
    ds->addProperty(prop);
    
    std::map<unsigned int, std::string> propertylist;
    
    Component* comp = new Component();
    comp->setCategoryIds(catids);
    comp->setName("ScoutHull");
    comp->setDescription("The scout hull, fitted out with everything a scout needs");
    comp->setTpclRequirementsFunction(
        "(lambda (design) "
            "(if (= (designType._num-components design) 1) "
            "(cons #t \"\") "
            "(cons #f \"This is a complete component, nothing else can be included\")))");
    propertylist[1] = "(lambda (design) 500000000)";
    propertylist[2] = "(lambda (design) 1)";
    propertylist[3] = "(lambda (design) 2)";
    propertylist[4] = "(lambda (design) 0)";
    propertylist[5] = "(lambda (design) 0)";
    propertylist[7] = "(lambda (design) 1)";
    comp->setPropertyList(propertylist);
    ds->addComponent(comp);
    
    comp = new Component();
    comp->setCategoryIds(catids);
    comp->setName("FrigateHull");
    comp->setDescription("The frigate hull, fitted out with everything a frigate needs");
    comp->setTpclRequirementsFunction(
        "(lambda (design) "
            "(if (= (designType._num-components design) 1) "
            "(cons #t \"\") "
            "(cons #f \"This is a complete component, nothing else can be included\")))");
    propertylist.clear();
    propertylist[1] = "(lambda (design) 200000000)";
    propertylist[2] = "(lambda (design) 2)";
    propertylist[3] = "(lambda (design) 4)";
    propertylist[4] = "(lambda (design) 2)";
    propertylist[5] = "(lambda (design) 0)";
    propertylist[6] = "(lambda (design) 1)";
    propertylist[7] = "(lambda (design) 1)";
    comp->setPropertyList(propertylist);
    ds->addComponent(comp);
    
    comp = new Component();
    comp->setCategoryIds(catids);
    comp->setName("BattleshipHull");
    comp->setDescription("The battleship hull, fitted out with everything a battleship needs");
    comp->setTpclRequirementsFunction(
            "(lambda (design) "
            "(if (= (designType._num-components design) 1) "
                "(cons #t \"\") "
                "(cons #f \"This is a complete component, nothing else can be included\")))");
    propertylist.clear();
    propertylist[1] = "(lambda (design) 300000000)";
    propertylist[2] = "(lambda (design) 4)";
    propertylist[3] = "(lambda (design) 6)";
    propertylist[4] = "(lambda (design) 3)";
    propertylist[5] = "(lambda (design) 1)";
    propertylist[7] = "(lambda (design) 1)";
    comp->setPropertyList(propertylist);
    ds->addComponent(comp);
  
  
  ObjectManager* obman = game->getObjectManager();
  ObjectTypeManager* otypeman = game->getObjectTypeManager();
  
  uint32_t obT_Universe = otypeman->getObjectTypeByName("Universe");
  uint32_t obT_Galaxy = otypeman->getObjectTypeByName("Galaxy");
  uint32_t obT_Star_System = otypeman->getObjectTypeByName("Star System");
  uint32_t obT_Planet = otypeman->getObjectTypeByName("Planet");

  IGObject* universe = game->getObjectManager()->createNewObject();
  otypeman->setupObject(universe, obT_Universe);
  Universe* theuniverse = (Universe*)(universe->getObjectBehaviour());
  theuniverse->setSize(1000000000000ll);
  universe->setName("The Universe");
  theuniverse->setPosition(Vector3d(0ll, 0ll, 0ll));
  obman->addObject(universe);
  
  //add contained objects
  IGObject *mw_galaxy = game->getObjectManager()->createNewObject();
  otypeman->setupObject(mw_galaxy, obT_Galaxy);
  EmptyObject* eo = (EmptyObject*)(mw_galaxy->getObjectBehaviour());
  eo->setSize(100000000000ll);
  mw_galaxy->setName("Milky Way Galaxy");
  eo->setPosition(Vector3d(0ll, -6000ll, 0ll));
  mw_galaxy->addToParent(universe->getID());
  obman->addObject(mw_galaxy);
  
  // star system 1
  IGObject *sol = game->getObjectManager()->createNewObject();
  otypeman->setupObject(sol, obT_Star_System);
  EmptyObject* thesol = (EmptyObject*)(sol->getObjectBehaviour());
  thesol->setSize(60000ll);
  sol->setName("Sol/Terra System");
  thesol->setPosition(Vector3d(3000000000ll, 2000000000ll, 0ll));
  sol->addToParent(mw_galaxy->getID());
  obman->addObject(sol);

  // star system 2
  IGObject *ac = game->getObjectManager()->createNewObject();
  otypeman->setupObject(ac, obT_Star_System);
  EmptyObject* theac = (EmptyObject*)(ac->getObjectBehaviour());
  theac->setSize(90000ll);
  ac->setName("Alpha Centauri System");
  theac->setPosition(Vector3d(-1500000000ll, 1500000000ll, 0ll));
  ac->addToParent(mw_galaxy->getID());
  obman->addObject(ac);
  
  // star system 3
  IGObject *sirius = game->getObjectManager()->createNewObject();
  otypeman->setupObject(sirius, obT_Star_System);
  EmptyObject* thesirius = (EmptyObject*)(sirius->getObjectBehaviour());
  thesirius->setSize(60000ll);
  sirius->setName("Sirius System");
  thesirius->setPosition(Vector3d(-250000000ll, -4000000000ll, 0ll));
  sirius->addToParent(mw_galaxy->getID());
  obman->addObject(sirius);

  
  // now for some planets
  IGObject *earth = game->getObjectManager()->createNewObject();
  otypeman->setupObject(earth, obT_Planet);
  Planet * theearth = (Planet*)(earth->getObjectBehaviour());
  theearth->setSize(2);
  earth->setName("Earth/Terra");
  theearth->setPosition(thesol->getPosition() + Vector3d(14960ll, 0ll, 0ll));
  OrderQueue *planetoq = new OrderQueue();
  planetoq->setObjectId(earth->getID());
  planetoq->addOwner(0);
  game->getOrderManager()->addOrderQueue(planetoq);
  OrderQueueObjectParam* oqop = static_cast<OrderQueueObjectParam*>(earth->getParameterByType(obpT_Order_Queue));
  oqop->setQueueId(planetoq->getQueueId());
  theearth->setDefaultOrderTypes();
  earth->addToParent(sol->getID());
  obman->addObject(earth);
  
  IGObject *venus = game->getObjectManager()->createNewObject();
  otypeman->setupObject(venus, obT_Planet);
  Planet* thevenus = (Planet*)(venus->getObjectBehaviour());
  thevenus->setSize(2);
  venus->setName("Venus");
  thevenus->setPosition(thesol->getPosition() + Vector3d(0ll, 10800ll, 0ll));
  planetoq = new OrderQueue();
  planetoq->setObjectId(venus->getID());
  planetoq->addOwner(0);
  game->getOrderManager()->addOrderQueue(planetoq);
  oqop = static_cast<OrderQueueObjectParam*>(venus->getParameterByType(obpT_Order_Queue));
  oqop->setQueueId(planetoq->getQueueId());
  thevenus->setDefaultOrderTypes();
  venus->addToParent(sol->getID());
  obman->addObject(venus);
  
  IGObject *mars = game->getObjectManager()->createNewObject();
  otypeman->setupObject(mars, obT_Planet);
  Planet* themars = (Planet*)(mars->getObjectBehaviour());
  themars->setSize(2);
  mars->setName("Mars");
  themars->setPosition(thesol->getPosition() + Vector3d(-22790ll, 0ll, 0ll));
  planetoq = new OrderQueue();
  planetoq->setObjectId(mars->getID());
  planetoq->addOwner(0);
  game->getOrderManager()->addOrderQueue(planetoq);
  oqop = static_cast<OrderQueueObjectParam*>(mars->getParameterByType(obpT_Order_Queue));
  oqop->setQueueId(planetoq->getQueueId());
  themars->setDefaultOrderTypes();
  mars->addToParent(sol->getID());
  obman->addObject(mars);
  
  IGObject *acprime = game->getObjectManager()->createNewObject();
  otypeman->setupObject(acprime, obT_Planet);
  Planet* theacprime = (Planet*)(acprime->getObjectBehaviour());
  theacprime->setSize(2);
  acprime->setName("Alpha Centauri Prime");
  theacprime->setPosition(theac->getPosition() + Vector3d(-6300ll, 78245ll, 0ll));
  planetoq = new OrderQueue();
  planetoq->setObjectId(acprime->getID());
  planetoq->addOwner(0);
  game->getOrderManager()->addOrderQueue(planetoq);
  oqop = static_cast<OrderQueueObjectParam*>(acprime->getParameterByType(obpT_Order_Queue));
  oqop->setQueueId(planetoq->getQueueId());
  theacprime->setDefaultOrderTypes();
  acprime->addToParent(ac->getID());
  obman->addObject(acprime);
  
  IGObject *s1 = game->getObjectManager()->createNewObject();
  otypeman->setupObject(s1, obT_Planet);
  Planet* thes1 = (Planet*)(s1->getObjectBehaviour());
  thes1->setSize(2);
  s1->setName("Sirius 1");
  thes1->setPosition(thesirius->getPosition() + Vector3d(45925ll, -34262ll, 0ll));
  planetoq = new OrderQueue();
  planetoq->setObjectId(s1->getID());
  planetoq->addOwner(0);
  game->getOrderManager()->addOrderQueue(planetoq);
  oqop = static_cast<OrderQueueObjectParam*>(s1->getParameterByType(obpT_Order_Queue));
  oqop->setQueueId(planetoq->getQueueId());
  thes1->setDefaultOrderTypes();
  s1->addToParent(sirius->getID());
  obman->addObject(s1);
 
  //create random systems
  Random * currandom;

  std::string randomseed = Settings::getSettings()->get("minisec_debug_random_seed");
  if( randomseed != ""){
    random = new Random();
    random->seed(atoi(randomseed.c_str()));
    currandom = random;
  } else {
    currandom = game->getRandom();
  }

  std::string namesfile  = Settings::getSettings()->get("minisec_system_names");

  Names* names;
  if(namesfile == ""){
    names = new NamesSet(currandom);
  } else {
    std::ifstream* f = new std::ifstream(namesfile.c_str());
    if (f->fail()) {
      Logger::getLogger()->error("Could not open system names file %s", namesfile.c_str());
      delete f;
      // Fall back to the names set
      names = new NamesSet(currandom);
    } else {
      names = new NamesFile(new std::ifstream(namesfile.c_str()));
    }
  }

  uint32_t min_systems = atoi(Settings::getSettings()->get("minisec_min_systems").c_str());
  uint32_t max_systems = atoi(Settings::getSettings()->get("minisec_max_systems").c_str());
  uint32_t num_systems;
  if(min_systems == max_systems){
    num_systems = min_systems;
  }else{
    num_systems =  currandom->getInRange(min_systems ,max_systems);
  }

  uint32_t total_planets = atoi(Settings::getSettings()->get("minisec_total_planets").c_str());
  if(total_planets == 0)
    total_planets = 0x0fffffff;
  for (uint32_t counter = 0; counter < num_systems; counter++) {
    createStarSystem( mw_galaxy, total_planets, names);
    if(total_planets <= 0)
      break;
  }
  
    //setup Resources
    ResourceDescription* res = new ResourceDescription();
    res->setNameSingular("Ship part");
    res->setNamePlural("Ship parts");
    res->setUnitSingular("part");
    res->setUnitPlural("parts");
    res->setDescription("Ships parts that can be used to create ships");
    res->setMass(0);
    res->setVolume(0);
    game->getResourceManager()->addResourceDescription(res);
    
    res = new ResourceDescription();
    res->setNameSingular("Home Planet");
    res->setNamePlural("Home Planets");
    res->setUnitSingular("unit");
    res->setUnitPlural("units");
    res->setDescription("The home planet for a race.");
    res->setMass(0);
    res->setVolume(0);
    game->getResourceManager()->addResourceDescription(res);
  
    game->getPlayerManager()->createNewPlayer("guest", "guest");

  delete names;    
}

void MiniSec::startGame(){
    
    if(Game::getGame()->getResourceManager()->getResourceDescription(1) == NULL){
        Logger::getLogger()->info("Setting up ship part resource that had not been setup");
        ResourceDescription* res = new ResourceDescription();
        res->setNameSingular("Ship part");
        res->setNamePlural("Ship parts");
        res->setUnitSingular("part");
        res->setUnitPlural("parts");
        res->setDescription("Ships parts that can be used to create ships");
        res->setMass(0);
        res->setVolume(0);
        Game::getGame()->getResourceManager()->addResourceDescription(res);
    }
    if(Game::getGame()->getResourceManager()->getResourceDescription(2) == NULL){
      Logger::getLogger()->info("Setting up home planet resource that had not been setup");
        ResourceDescription* res = new ResourceDescription();
        res->setNameSingular("Home Planet");
        res->setNamePlural("Home Planets");
        res->setUnitSingular("unit");
        res->setUnitPlural("units");
        res->setDescription("The home planet for a race.");
        res->setMass(0);
        res->setVolume(0);
        Game::getGame()->getResourceManager()->addResourceDescription(res);
    }
    
  Settings* settings = Settings::getSettings();
  if(settings->get("turn_length_over_threshold") == ""){
    settings->set("turn_length_over_threshold", "600");
    settings->set("turn_player_threshold", "0");
    settings->set("turn_length_under_threshold", "600");
  }
}

bool MiniSec::onAddPlayer(Player* player){
  
  return true;
}

void MiniSec::onPlayerAdded(Player* player){
  Game *game = Game::getGame();

  Logger::getLogger()->debug("MiniSec::onPlayerAdded");
  
  PlayerView* playerview = player->getPlayerView();
  
  if(std::string(player->getName()) == "guest"){
    player->setIsAlive(false);
  }else{
    
    //Can see all other designs
    std::set<uint32_t> allotherdesigns = Game::getGame()->getDesignStore()->getDesignIds();
    for(std::set<uint32_t>::const_iterator desid = allotherdesigns.begin(); desid != allotherdesigns.end(); ++desid){
      DesignView* dv = new DesignView();
      dv->setDesignId(*desid);
      dv->setIsCompletelyVisible(true);
      playerview->addVisibleDesign(dv);
    }
    
    std::set<uint32_t> mydesignids;
    
    //temporarily add the components as usable to get the designs done
    playerview->addUsableComponent(1);
    playerview->addUsableComponent(2);
    playerview->addUsableComponent(3);
  
    Design* scout = new Design();
    scout->setCategoryId(1);
    scout->setName("Scout");
    scout->setDescription("Scout ship");
    scout->setOwner(player->getID());
      std::map<unsigned int, unsigned int> cl;
      cl[1] = 1;
    scout->setComponents(cl);
    game->getDesignStore()->addDesign(scout);
    mydesignids.insert(scout->getDesignId());
  
      Design* frigate = new Design();
    frigate->setCategoryId(1);
    frigate->setName("Frigate");
    frigate->setDescription("Frigate ship");
    frigate->setOwner(player->getID());
    cl.clear();
      cl[2] = 1;
    frigate->setComponents(cl);
    game->getDesignStore()->addDesign(frigate);
    unsigned int frigateid = frigate->getDesignId();
    mydesignids.insert(frigate->getDesignId());
  
    Design* design = new Design();
    design->setCategoryId(1);
    design->setName("Battleship");
    design->setDescription("Battleship ship");
    design->setOwner(player->getID());
    cl.clear();
      cl[3] = 1;
    design->setComponents(cl);
    game->getDesignStore()->addDesign(design);
    mydesignids.insert(design->getDesignId());
  
    //remove temporarily added usable components
    playerview->removeUsableComponent(1);
    playerview->removeUsableComponent(2);
    playerview->removeUsableComponent(3);
    // the components are still visible
    
    
    uint32_t obT_Fleet = game->getObjectTypeManager()->getObjectTypeByName("Fleet");
    uint32_t obT_Planet = game->getObjectTypeManager()->getObjectTypeByName("Planet");
    uint32_t obT_Star_System = game->getObjectTypeManager()->getObjectTypeByName("Star System");
  
    Random * currandom;
    if(random != NULL){
      currandom = random;
    }else{
      currandom = game->getRandom();
    }
    
    ObjectTypeManager* otypeman = game->getObjectTypeManager();
    
    const char* name = player->getName().c_str();
    IGObject *star = game->getObjectManager()->createNewObject();
    otypeman->setupObject(star, obT_Star_System);
    EmptyObject* thestar = (EmptyObject*)(star->getObjectBehaviour());
    thestar->setSize(80000ll);
    char* temp = new char[strlen(name) + 13];
    strncpy(temp, name, strlen(name));
    strncpy(temp + strlen(name), " Star System", 12);
    temp[strlen(name) + 12] = '\0';
    star->setName(temp);
    delete[] temp;
    thestar->setPosition(Vector3d((long long)(currandom->getInRange(-5000, 5000) * 10000000),
                              (long long)(currandom->getInRange(-5000, 5000) * 10000000),
                              /*(long long)(((rand() % 1000) - 500) * 10000000)*/ 0));
    
    star->addToParent(1);
    game->getObjectManager()->addObject(star);
    
    IGObject *planet = game->getObjectManager()->createNewObject();
    otypeman->setupObject(planet, obT_Planet);
    
    temp = new char[strlen(name) + 8];
    strncpy(temp, name, strlen(name));
    strncpy(temp + strlen(name), " Planet", 7);
    temp[strlen(name) + 7] = '\0';
    planet->setName(temp);
    delete[] temp;
    
    Planet* theplanet = (Planet*)(planet->getObjectBehaviour());
    
    theplanet->setSize(2);
    
    theplanet->setOwner(player->getID());
    theplanet->addResource(2, 1);
    theplanet->setPosition(thestar->getPosition() + Vector3d((long long)(currandom->getInRange(-5000, 5000)* 10),
                                                      (long long)(currandom->getInRange(-5000, 5000) * 10),
                                                      /*(long long)((rand() % 10000) - 5000)*/ 0));
    
    OrderQueue *planetoq = new OrderQueue();
    planetoq->setObjectId(planet->getID());
    planetoq->addOwner(player->getID());
    game->getOrderManager()->addOrderQueue(planetoq);
    OrderQueueObjectParam* oqop = static_cast<OrderQueueObjectParam*>(planet->getParameterByType(obpT_Order_Queue));
    oqop->setQueueId(planetoq->getQueueId());
    theplanet->setDefaultOrderTypes();
    
    planet->addToParent(star->getID());
    game->getObjectManager()->addObject(planet);
    playerview->addOwnedObject(planet->getID());
    
    IGObject *fleet = game->getObjectManager()->createNewObject();
    otypeman->setupObject(fleet, obT_Fleet);
    Fleet* thefleet = (Fleet*)(fleet->getObjectBehaviour());
    thefleet->setSize(2);
    temp = new char[strlen(name) + 13];
    strncpy(temp, name, strlen(name));
    strncpy(temp + strlen(name), " First Fleet", 12);
    temp[strlen(name) + 12] = '\0';
    fleet->setName(temp);
    delete[] temp;
    thefleet->setOwner(player->getID());
    thefleet->setPosition(thestar->getPosition() + Vector3d((long long)(currandom->getInRange(-5000, 5000) * 10),
                                                      (long long)(currandom->getInRange(-5000, 5000) * 10),
                                                      /*(long long)((rand() % 10000) - 5000)*/ 0));
    thefleet->setVelocity(Vector3d(0LL, 0ll, 0ll));
    
    OrderQueue *fleetoq = new OrderQueue();
    fleetoq->setObjectId(fleet->getID());
    fleetoq->addOwner(player->getID());
    game->getOrderManager()->addOrderQueue(fleetoq);
    oqop = static_cast<OrderQueueObjectParam*>(fleet->getParameterByType(obpT_Order_Queue));
    oqop->setQueueId(fleetoq->getQueueId());
    thefleet->setDefaultOrderTypes();
    thefleet->addShips(frigateid, 1);
    
    fleet->addToParent(star->getID());
    game->getObjectManager()->addObject(fleet);
    playerview->addOwnedObject(fleet->getID());
    
    //number 2
    fleet = game->getObjectManager()->createNewObject();
    otypeman->setupObject(fleet, obT_Fleet);
    thefleet = (Fleet*)(fleet->getObjectBehaviour());
    thefleet->setSize(2);
    temp = new char[strlen(name) + 14];
    strncpy(temp, name, strlen(name));
    strncpy(temp + strlen(name), " Second Fleet", 13);
    temp[strlen(name) + 13] = '\0';
    fleet->setName(temp);
    delete[] temp;
    thefleet->setOwner(player->getID());
    thefleet->setPosition(thestar->getPosition() + Vector3d((long long)(currandom->getInRange(-5000, 5000) * 10),
                                                      (long long)(currandom->getInRange(-5000, 5000) * 10),
                                                      /*(long long)((rand() % 10000) - 5000)*/ 0));
    thefleet->setVelocity(Vector3d(0LL, 0ll, 0ll));
    
    fleetoq = new OrderQueue();
    fleetoq->setObjectId(fleet->getID());
    fleetoq->addOwner(player->getID());
    game->getOrderManager()->addOrderQueue(fleetoq);
    oqop = static_cast<OrderQueueObjectParam*>(fleet->getParameterByType(obpT_Order_Queue));
    oqop->setQueueId(fleetoq->getQueueId());
    thefleet->setDefaultOrderTypes();
    thefleet->addShips(frigateid, 1);
    
    fleet->addToParent(star->getID());
    game->getObjectManager()->addObject(fleet);
    playerview->addOwnedObject(fleet->getID());

    //number 3
    fleet = game->getObjectManager()->createNewObject();
    otypeman->setupObject(fleet, obT_Fleet);
    thefleet = (Fleet*)(fleet->getObjectBehaviour());
    thefleet->setSize(2);
    temp = new char[strlen(name) + 13];
    strncpy(temp, name, strlen(name));
    strncpy(temp + strlen(name), " Thrid Fleet", 12);
    temp[strlen(name) + 12] = '\0';
    fleet->setName(temp);
    delete[] temp;
    thefleet->setOwner(player->getID());
    thefleet->setPosition(thestar->getPosition() + Vector3d((long long)(currandom->getInRange(-5000, 5000) * 10),
                                                      (long long)(currandom->getInRange(-5000, 5000) * 10),
                                                      /*(long long)((rand() % 10000) - 5000)*/ 0));
    thefleet->setVelocity(Vector3d(0LL, 0ll, 0ll));
    
    fleetoq = new OrderQueue();
    fleetoq->setObjectId(fleet->getID());
    fleetoq->addOwner(player->getID());
    game->getOrderManager()->addOrderQueue(fleetoq);
    oqop = static_cast<OrderQueueObjectParam*>(fleet->getParameterByType(obpT_Order_Queue));
    oqop->setQueueId(fleetoq->getQueueId());
    thefleet->setDefaultOrderTypes();
    thefleet->addShips(frigateid, 1);
    
    fleet->addToParent(star->getID());
    game->getObjectManager()->addObject(fleet);
    playerview->addOwnedObject(fleet->getID());
    
    
    frigate->addUnderConstruction(3);
    frigate->addComplete(3);
    game->getDesignStore()->designCountsUpdated(frigate);
    
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

  std::set<uint32_t> objids = game->getObjectManager()->getAllIds();
  for(std::set<uint32_t>::iterator itcurr = objids.begin(); itcurr != objids.end();
      ++itcurr){
    ObjectView* obv = new ObjectView();
    obv->setObjectId(*itcurr);
    obv->setCompletelyVisible(true);
    playerview->addVisibleObject(obv);
  }
  
  game->getPlayerManager()->updatePlayer(player->getID());
  
  
  
}

// Create a random star system
IGObject* MiniSec::createStarSystem( IGObject* mw_galaxy, uint32_t& max_planets, Names* names)
{
    Logger::getLogger()->debug( "Entering MiniSec::createStarSystem");
    Game*          game  = Game::getGame();
    ObjectManager* obman = game->getObjectManager();
    ObjectTypeManager* otypeman = game->getObjectTypeManager();
    IGObject*      star  = obman->createNewObject();
    unsigned int   nplanets = 0;
    std::ostringstream     formatter;
   
    // FIXME: This is repeated everywhere put it in a getter
    Random * currandom;
    if(random != NULL){
      currandom = random;
    }else{
      currandom = game->getRandom();
    }

    // Create a variable number of planets for each star system
    uint32_t maxplanets = atoi(Settings::getSettings()->get("minisec_max_planets").c_str());
    uint32_t minplanets = atoi(Settings::getSettings()->get("minisec_min_planets").c_str());
    if(minplanets == maxplanets){
      nplanets = minplanets;
    }else{
      nplanets = currandom->getInRange(minplanets, maxplanets);
    }
    if(max_planets < nplanets)
      nplanets = max_planets;

    uint32_t obT_Star_System = otypeman->getObjectTypeByName("Star System");
    uint32_t obT_Planet      = otypeman->getObjectTypeByName("Planet");
    
    otypeman->setupObject(star, obT_Star_System );
    EmptyObject* thestar = (EmptyObject*)(star->getObjectBehaviour());
    thestar->setSize(nplanets * 60000ll);

    std::string name = names->getName();

    // FIXME: Would it not be better that this method takes a std::string?
    star->setName(name.c_str());

    thestar->setPosition( Vector3d( currandom->getInRange(0, 8000) * 1000000ll - 4000000000ll,
                                 currandom->getInRange(0, 8000) * 1000000ll - 4000000000ll,
                                 0ll));
    star->addToParent( mw_galaxy->getID());
    obman->addObject( star);

    for(uint i = 1; i <= nplanets; i++){
        IGObject*  planet = game->getObjectManager()->createNewObject();
        formatter.str("");

				if (startswith(star->getName(), std::string("System"))) {
						formatter << star->getName() << ", Planet " << i;
				} else {
						formatter << star->getName() << " " << i;
				}

        otypeman->setupObject(planet, obT_Planet);
        
        Planet* theplanet = (Planet*)(planet->getObjectBehaviour());
        
        theplanet->setSize( 2);
        planet->setName( formatter.str().c_str());
        theplanet->setPosition( thestar->getPosition() + Vector3d( i * 40000ll,
                                                             i * -35000ll,
                                                             0ll));
        OrderQueue *planetoq = new OrderQueue();
        planetoq->setObjectId(planet->getID());
        planetoq->addOwner(0);
        game->getOrderManager()->addOrderQueue(planetoq);
        OrderQueueObjectParam* oqop = static_cast<OrderQueueObjectParam*>(planet->getParameterByType(obpT_Order_Queue));
        oqop->setQueueId(planetoq->getQueueId());
        theplanet->setDefaultOrderTypes();
        
        planet->addToParent( star->getID());
        obman->addObject( planet);
        max_planets--;
    }

    Logger::getLogger()->debug( "Exiting MiniSec::createStarSystem");
    return star;
}
