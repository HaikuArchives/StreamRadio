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

	virtual StationList* FindBy(
		int capabilityIndex, const char* searchFor, BLooper* resultUpdateTarget);

private:
	static const char* serviceNameInternal;

	static int32 _GetKeywords(void* data);
	static int32 _IconLookupFunc(void* data);
	void _WaitForIconLookupThread();
	uint32 _RegisterUnknownKeywordsSearch(const char* name, const char* path, int32 minStations);

private:
	static BString sBaseUrl;

	thread_id fIconLookupThread;

	int searchCapabilityName = -1;
	int searchCapabilityTag = -1;
	int searchCapabilityLanguage = -1;
	int searchCapabilityCountry = -1;
	int searchCapabilityCodec = -1;
	int searchCapabilityUuid = -1;

	BObjectList<IconLookup, true> fIconLookupList;
	BLooper* fIconLookupNotify;

	thread_id fKeywordsThread[4];
	unsigned fKeywordsThreadCount;
};


#endif	// _STATION_FINDER_RADIO_NETWORK_H
