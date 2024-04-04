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
#ifndef _STATION_FINDER_LISTEN_LIVE_H
#define _STATION_FINDER_LISTEN_LIVE_H


#include <ObjectList.h>

#include "StationFinder.h"


class StationFinderListenLive : public StationFinderService {
public:
	StationFinderListenLive();
	virtual ~StationFinderListenLive();

	static void RegisterSelf();
	static StationFinderService* Instantiate();

	virtual BObjectList<Station>* FindBy(
		int capabilityIndex, const char* searchFor, BLooper* resultUpdateTarget);

	BObjectList<Station>* ParseCountryReturn(BMallocIO* data, const char* country);
	BObjectList<Station>* ParseGenreReturn(BMallocIO* data, const char* genre);

private:
	static int32 _PlsLookupFunc(void* data);

private:
	static const char* kBaseUrl;

	BStringList fCountryKeywordAndPath;
	BStringList fGenreKeywordAndPath;
	thread_id fLookupThread;
	BObjectList<Station> fPlsLookupList;

	BLooper* fLookupNotify;
};


#endif	// _STATION_FINDER_LISTEN_LIVE_H
