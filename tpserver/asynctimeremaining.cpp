/*  AsyncTimeRemaining class
 *
 *  Copyright (C) 2007  Lee Begg and the Thousand Parsec Project
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

#include "frame.h"

#include "asynctimeremaining.h"

AsyncTimeRemaining::AsyncTimeRemaining(uint32_t rt, uint32_t r): AsyncFrame(), rtime(rt), why(r){
}

AsyncTimeRemaining::~AsyncTimeRemaining(){
}

bool AsyncTimeRemaining::createFrame(Frame* f){
  f->setType(ft02_Time_Remaining);
  f->packInt(rtime);
  if(f->getVersion() == fv0_4){
    f->packInt(why); //reason
  }
  return true;
}
