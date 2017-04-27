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
 * File:   Expander.cpp
 * Author: Kai Niessen <kai.niessen@online.de>
 * 
 * Created on April 13, 2017, 4:50 PM
 */

#include "Expander.h"
#include <Shape.h>
#include <Window.h>
#include <Layout.h>


void 
Expander::Draw(BRect updateRect) {
	BRect r = Bounds();
	SetHighColor(ViewColor());
	FillRect(r);
	
	r.InsetBy((Bounds().Width() - 80) / 2, 0);
	BShape shape;
	shape.MoveTo(r.LeftTop());
	shape.BezierTo(r.LeftTop() + BPoint(10,0), r.LeftBottom() + BPoint(10, 0), r.LeftBottom() + BPoint(20, 0));
	shape.LineTo(r.RightBottom() - BPoint(20, 0));
	shape.BezierTo(r.RightBottom() - BPoint(10, 0), r.RightTop() - BPoint(10, 0), r.RightTop());
	
	SetHighColor(ui_color(B_CONTROL_BACKGROUND_COLOR));
	FillShape(&shape);
	
	SetHighColor(ui_color(IsFocus() ? B_CONTROL_HIGHLIGHT_COLOR : B_CONTROL_BORDER_COLOR));
	StrokeShape(&shape);

	shape.Clear();
	r.InsetBy(r.Width() / 3, 2);
	if (fExpanded) {
		shape.MoveTo(r.LeftTop());
		shape.LineTo(r.RightTop());
		shape.LineTo(r.LeftBottom() + BPoint(r.Width() / 2, 0));
		shape.Close();
	} else {
		shape.MoveTo(r.LeftBottom());
		shape.LineTo(r.RightBottom());
		shape.LineTo(r.LeftTop() + BPoint(r.Width() / 2, 0));
		shape.Close();
	}
	SetHighColor(tint_color(ViewColor(), B_DARKEN_2_TINT));
	FillShape(&shape);
}

Expander::Expander(const char* name, BView* target, orientation orientation)
  : BButton(name, "", NULL, B_WILL_DRAW),
	fTarget(target),
	fOrientation(orientation),
	fExpanded(true)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	SetExplicitSize(BSize(B_SIZE_UNSET, 8));
	SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_VERTICAL_UNSET));
}

Expander::~Expander() {
}

void
Expander::SetExpanded(bool expanded) {
	if (fExpanded == expanded)
		return;
	
	if (expanded) {
		fTarget->Show();
	} else {
		fTarget->Hide();
	}
	Window()->Layout(true);
	fExpanded = expanded;
	Invalidate();
}

status_t
Expander::Invoke(BMessage* msg) {
	SetExpanded(!fExpanded);
	return B_OK;
}
