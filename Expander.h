/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   Expander.h
 * Author: Kai Niessen <kai.niessen@online.de>
 *
 * Created on April 13, 2017, 4:50 PM
 */

#ifndef EXPANDER_H
#define EXPANDER_H

#include <Button.h>

#define MSG_EXPAND					'eEXP'

class Expander : public BButton {
public:
									Expander(const char* name, BView* target, orientation orientation = B_HORIZONTAL);
	virtual							~Expander();
	void							SetExpanded(bool expanded);
	bool							Expanded() { return fExpanded; }
	virtual status_t				Invoke(BMessage* msg);
	virtual void					Draw(BRect updateRect);
private:
	bool							fExpanded;
	orientation						fOrientation;
	BView*							fTarget;
};

#endif /* EXPANDER_H */

