

#include "order.h"
#include "frame.h"
#include "object.h"
#include "game.h"
#include "logging.h"
#include "message.h"
#include "fleet.h"
#include "planet.h"
#include "player.h"
#include "move.h"
#include "combatstrategy.h"

#include "colonise.h"

Colonise::Colonise(){
  type = odT_Colonise;
  moveorder = new Move();
}

Colonise::~Colonise(){
  delete moveorder;
}

void Colonise::createFrame(Frame * f, int objID, int pos){
  Order::createFrame(f, objID, pos);
  f->packInt(moveorder->getETA(Game::getGame()->getObject(objID))); // number of turns
  f->packInt(0); // size of resource list
  f->packInt(planetid);
  
}

bool Colonise::inputFrame(Frame * f){
  f->unpackInt(); // number of turns
  f->unpackInt(); // size of resource list, should be zero
  // TODO: fix in case size of list is not zero
  planetid = f->unpackInt();

  IGObject* target = Game::getGame()->getObject(planetid);

  if(target == NULL || (planetid != 0 && target->getType() != 3)){
    Logger::getLogger()->debug("Player trying to colonise something that is not a planet");
    return false;
  }
  moveorder->setDest(target->getPosition());
  
  return true;
}

bool Colonise::doOrder(IGObject * ob){
  //if not close, move
  if(planetid == 0)
    return true;

  if(moveorder->doOrder(ob)){

    Message * msg = new Message();
    
    Fleet *fleet = (Fleet*)ob->getObjectData();
    Planet *planet = (Planet*)(Game::getGame()->getObject(planetid)->getObjectData());
    
    if(planet->getOwner() != fleet->getOwner()){
      
      if(planet->getOwner() != -1){
	//combat
	CombatStrategy * combat = Game::getGame()->getCombatStrategy();
	combat->setCombatants(ob, Game::getGame()->getObject(planetid));
	combat->doCombat();
      }
	
	
      if(fleet->numShips(1) >= 1){
	planet->setOwner(fleet->getOwner());
	
	fleet->removeShips(1, 1);
	
	msg->setSubject("Colonised planet");
	msg->setBody("You have colonised a planet!");
	msg->setType(0);
      }else{
	msg->setSubject("Colonisation failed");
	msg->setBody("Your fleet did not have a frigate to colonise the planet");
	msg->setType(0);
      }
      
      if(fleet->numShips(0) == 0 && fleet->numShips(1) == 0 && fleet->numShips(2) == 0){
	Game::getGame()->scheduleRemoveObject(ob->getID());
      }
      
    }else{
      msg->setSubject("Colonisation failed");
      msg->setBody("You already own the planet you tried to colonise");
      msg->setType(0);
    }
    
    Game::getGame()->getPlayer(fleet->getOwner())->postToBoard(msg);

    return true;
  }else{
    return false;
  }
}


void Colonise::describeOrder(Frame * f) const{
  Order::describeOrder(f);
  f->packString("Colonise");
  f->packString("Attempt to colonise a planet");
  f->packInt(1);
  f->packString("planet");
  f->packInt(opT_Object_ID);
  f->packString("The target planet to be colonised");
  
}

Order* Colonise::clone() const{
  return new Colonise();
}
