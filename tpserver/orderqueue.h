#ifndef ORDERQUEUE_H
#define ORDERQUEUE_H
/*  OrderQueue class
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

#include <map>
#include <set>
#include <list>

#include <tpserver/result.h>

class Order;

class OrderQueue{
 public:
  OrderQueue();
  ~OrderQueue();

  void setQueueId(uint32_t id);
  uint32_t getQueueId() const;
  
  void addOwner(uint32_t playerid);
  void removeOwner(uint32_t playerid);
  bool isOwner(uint32_t playerid) const;
  std::set<uint32_t> getOwner() const;
  
  void setObjectId(uint32_t oid);
  uint32_t getObjectId() const;
  
  bool checkOrderType(uint32_t type, uint32_t playerid) const;
  
  std::set<uint32_t> getAllowedOrderTypes() const;
  void addAllowedOrderType(uint32_t type);
  void removeAllowedOrderType(uint32_t type);
  void setAllowedOrderTypes(const std::set<uint32_t>& ao);

  uint32_t getNumberOrders() const;
  
  bool addOrder(Order* ord, uint32_t pos, uint32_t playerid);
  Result removeOrder(uint32_t pos, uint32_t playerid);
  Order* getOrder(uint32_t pos, uint32_t playerid);
  Order * getFirstOrder();
  void removeFirstOrder();
  void updateFirstOrder();
  
  void setActive(bool a);
  void setRepeating(bool r);
  bool isActive() const;
  bool isRepeating() const;
  
  uint64_t getModTime() const;
  void touchModTime();

  void removeAllOrders();

  //persistence only
  void setNextOrderId(uint32_t next);
  void setModTime(uint64_t nmt);
  void setOwners(std::set<uint32_t> no);
  void setOrderSlots(std::list<uint32_t> nos);
  std::list<uint32_t> getOrderSlots() const;

 private:
  uint32_t queueid;
  uint32_t objectid;
  std::list<uint32_t> orderlist;
  std::map<uint32_t, Order*> ordercache;
  std::set<uint32_t> allowedtypes;
  std::set<uint32_t> owner;
  uint32_t nextOrderId;
  bool active;
  bool repeating;
  uint64_t modtime;

};

#endif
