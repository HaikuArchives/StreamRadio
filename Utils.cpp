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
 * File:   Utils.cpp
 * Author: Kai Niessen <kai.niessen@online.de>
 *
 * Created on April 16, 2017, 1:15 PM
 */

#include "Utils.h"
#include <Bitmap.h>
#include <Message.h>
#include <Resources.h>


BBitmap*
Utils::ResourceBitmap(int32 id)
{
	size_t size;
	const char* data
		= (const char*) be_app->AppResources()->FindResource('BBMP', id, &size);
	if (size && data) {
		BMessage msg;
		status_t status = msg.Unflatten(data);
		return (BBitmap*) BBitmap::Instantiate(&msg);
	}
	return NULL;
}
