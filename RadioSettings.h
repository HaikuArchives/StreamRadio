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
#ifndef _RADIO_SETTINGS_H
#define _RADIO_SETTINGS_H


#include <Entry.h>
#include <Message.h>
#include <ObjectList.h>

#include "Station.h"


class StationsList : public BObjectList<Station> {
public:
								StationsList();
	virtual						~StationsList();

	virtual	bool				AddItem(Station* station);
			bool				RemoveItem(Station* station);
			bool				RemoveItem(BString* StationName);
			Station*			FindItem(BString* Name);

			status_t			Load();
			void				Save();
};

class RadioSettings : private BMessage {
public:
								RadioSettings();
								RadioSettings(const RadioSettings& orig);
	virtual						~RadioSettings();

			status_t			Save();

			const char*			StationFinderName();
			void				SetStationFinderName(const char* name);

			bool				GetAllowParallelPlayback();
			void				SetAllowParallelPlayback(bool set);

			StationsList*		Stations;

private:
			status_t			_Load();
};


#endif // _RADIO_SETTINGS_H
