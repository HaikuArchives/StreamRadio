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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */


#include "Utils.h"

#include <Application.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <Message.h>
#include <Resources.h>
#include <Roster.h>

#include <stdio.h>


BBitmap*
Utils::ResourceBitmap(int32 id, float height)
{
	size_t size;
	const void* data
		= (const char*)be_app->AppResources()->LoadResource(B_VECTOR_ICON_TYPE, id, &size);
	if (size != 0 && data != NULL) {
		BBitmap* bitmap = new BBitmap(BRect(0, 0, height, height), B_RGBA32);
		BIconUtils::GetVectorIcon((const uint8*)data, size, bitmap);
		return (bitmap);
	}

	return NULL;
}
