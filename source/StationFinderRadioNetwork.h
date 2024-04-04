/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
 * Copyright (C) 2020 Jacob Secunda
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
#ifndef _STATION_FINDER_RADIO_NETWORK_H
#define _STATION_FINDER_RADIO_NETWORK_H


#include "StationFinder.h"


class IconLookup {
public:
	IconLookup(Station* station, BUrl iconUrl);

	Station* fStation;
	BUrl fIconUrl;
};


class StationFinderRadioNetwork : public StationFinderService {
public:
	StationFinderRadioNetwork();
	virtual ~StationFinderRadioNetwork();

	static void RegisterSelf();
	static StationFinderService* Instantiate();

	virtual BObjectList<Station>* FindBy(
		int capabilityIndex, const char* searchFor, BLooper* resultUpdateTarget);

private:
	static int32 _IconLookupFunc(void* data);
	status_t _CheckServer();
	void _WaitForIconLookupThread();

private:
	static const char* kBaseUrl;
	static BString sCachedServerUrl;

	thread_id fIconLookupThread;
	BObjectList<IconLookup> fIconLookupList;
	BLooper* fIconLookupNotify;
};


#endif	// _STATION_FINDER_RADIO_NETWORK_H
