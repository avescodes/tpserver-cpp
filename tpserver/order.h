#ifndef ORDER_H
#define ORDER_H
/*  Order base class
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

// inbuilt orders only
typedef enum {
	odT_Invalid = -1,
	odT_Nop = 0,
	odT_Move = 1,
	odT_Build = 2,
	odT_Colonise = 3,
	odT_Fleet_Split = 4,
	odT_Fleet_Merge = 5,
	odT_Max
} OrderType;

typedef enum {
	opT_Invalid = -1,
	opT_Space_Coord_Abs = 0,
	opT_Time = 1,
	opT_Object_ID = 2,
	opT_Player_ID = 3,
	opT_Space_Coord_Rel = 4,
	opT_Range = 5,
	opT_List = 6,
	opT_String = 7,

	opT_Max
} OrderParamType;

class Frame;
class IGObject;

class Order {

      public:
	Order();
        virtual ~Order();

	int getType() const;
	void setType(int ntype);

	virtual void createFrame(Frame * f, int objID, int pos);
	virtual bool inputFrame(Frame * f, unsigned int playerid);

	virtual bool doOrder(IGObject * ob) = 0;

	virtual void describeOrder(Frame * f) const;
	virtual Order *clone() const = 0;

      protected:
	 int type;
	 long long descmodtime;

};

#endif