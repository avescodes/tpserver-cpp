
#include "frame.h"

#include "fleet.h"

Fleet::Fleet():OwnedObject()
{
  damage = 0;
}

void Fleet::addShips(int type, int number){
  ships[type] += number;
}

bool Fleet::removeShips(int type, int number){
  if(ships[type] >= number){
    ships[type] -= number;
    return true;
  }
  return false;
}

void Fleet::packExtraData(Frame * frame)
{
	OwnedObject::packExtraData(frame);
	
	frame->packInt(ships.size());
	for(std::map<int, int>::iterator itcurr = ships.begin(); itcurr != ships.end(); ++itcurr){
	  if(itcurr->second > 0){
	    frame->packInt(itcurr->first);
	    frame->packInt(itcurr->second);
	  }
	}
	
	frame->packInt(damage);

}

void Fleet::doOnceATurn(IGObject * obj)
{

}
