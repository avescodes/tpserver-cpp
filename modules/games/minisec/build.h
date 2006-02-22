#ifndef BUILD_H
#define BUILD_H
/*  BuildFleet order
 *
 *  Copyright (C) 2004-2005  Lee Begg and the Thousand Parsec Project
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

#include <map>

#include <tpserver/order.h>

class Build : public Order{
 public:
  Build();
  virtual ~Build();

  void createFrame(Frame *f, int objID, int pos);
  bool inputFrame(Frame *f, unsigned int playerid);

  bool doOrder(IGObject *ob);

    uint32_t getTimeToGo() const;
    std::map<uint32_t, uint32_t> getShips() const;

    void setTimeToGo(uint32_t ttg);
    void addShips(uint32_t designid, uint32_t count);

  void describeOrder(Frame *f) const;
  Order* clone() const;

 private:
  std::map<uint32_t, uint32_t> fleettype;
  uint32_t turnstogo;
};

#endif
