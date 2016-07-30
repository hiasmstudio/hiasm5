/*
 * ElementGrips.h
 *
 *  Created on: Jun 20, 2010
 *      Author: dilma
 */

#ifndef ELEMENTGRIPS_H_
#define ELEMENTGRIPS_H_

#include "share.h"

struct GripValue {
	int *value;
	int dirx;
	int diry;
	GripValue(int *val, int dx, int dy) { value = val; dirx = dx; diry = dy; }
};

typedef std::list<GripValue> GripValueList;

typedef struct {
	gdouble x;
	gdouble y;
	int width;
	int height;
	int stepx;
	int stepy;
	GripValueList values;
	int tag;
	int cursor;

	inline void addValue(int *value, int dx, int dy) { values.push_back(GripValue(value, dx, dy)); }
	inline void move(int x, int y) { this->x = x;  this->y = y; }
} GripInfo;

typedef std::vector<GripInfo*> GripList;

class ElementGrip {
	private:
		GripInfo *active;
		gdouble startx;
		gdouble starty;
	protected:
		GripList grips;

		ElementGrip() { active = NULL; }
		virtual ~ElementGrip();

		virtual void on_grip_change(GripInfo *grip) = 0;
		virtual void on_grip_draw(DrawContext dc, GripInfo *grip) = 0;
		virtual bool on_grip_can_change(GripInfo *grip, int &dx, int &dy) = 0;

		void drawGrips(DrawContext dc);

		GripInfo *gripDown(gdouble x, gdouble y);
		void gripMove(gdouble x, gdouble y);
		void gripUp(gdouble x, gdouble y);

		GripInfo *getGripAtPos(gdouble x, gdouble y);

		GripInfo *addGrip(gdouble x, gdouble y, int width, int height, int sx, int sy, int cursor);
		void clear();

		inline GripInfo *isActiveGrip() { return active; }
};

#endif /* ELEMENTGRIPS_H_ */
