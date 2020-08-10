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
#ifndef _UTILS_H
#define _UTILS_H


#include <Application.h>
#include <Bitmap.h>


#define RES_BANNER 100
#define RES_BN_SEARCH 10
#define RES_BN_STOPPED 1
#define RES_BN_SEARCH 10
#define RES_BN_WEB 11


class Utils {
public:
								Utils() {};
	virtual						~Utils() {};

	static	BBitmap*			ResourceBitmap(int32 id);
};


#endif // _UTILS_H
