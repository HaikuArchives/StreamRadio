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
 * File:   StationFinderListenLive.h
 * Author: Kai Niessen <kai.niessen@online.de>
 *
 * Created on March 27, 2017, 12:16 AM
 */

#ifndef STATIONFINDERLISTENLIVE_H
#define STATIONFINDERLISTENLIVE_H

#include "StationFinder.h"
#include <ObjectList.h>

class StationFinderListenLive : public StationFinderService {
public:
								StationFinderListenLive();
    virtual						~StationFinderListenLive();
    virtual BObjectList<Station>* 
								FindBy(FindByCapability capability, const char* searchFor, 
								  BLooper* resultUpdateTarget);
private:
    static const char*			baseUrl; 
	thread_id					fLookupThread;
	BObjectList<Station>		fPlsLookupList;
	BLooper*					fLookupNotify;
	
	static int32				PlsLookupFunc(void* data);
};

#endif /* STATIONFINDERLISTENLIVE_H */

