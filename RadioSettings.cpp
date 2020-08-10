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


#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>

#include "Debug.h"
#include "RadioSettings.h"


const char* kSettingsFileName = "StreamRadio.settings";


StationsList::StationsList()
	:
	BObjectList<Station>()
{
}


StationsList::~StationsList()
{
	for (int32 i = CountItems() - 1; i >= 0; i--)
		BObjectList<Station>::RemoveItem(ItemAt(i), true);
}


bool
StationsList::AddItem(Station* station)
{
	if (FindItem(station->Name()))
		return false;

	return BObjectList<Station>::AddItem(station);
}


bool
StationsList::RemoveItem(BString* stationName)
{
	Station* station = FindItem(stationName);
	if (station != NULL)
		return BObjectList<Station>::RemoveItem(station, true);
	else
		return false;
}


bool
StationsList::RemoveItem(Station* station)
{
	return BObjectList<Station>::RemoveItem(station, false);
}


Station*
StationsList::FindItem(BString* stationName)
{
	for (int32 i = 0; i < CountItems(); i++) {
		Station* item = ItemAt(i);
		if (stationName->Compare(item->Name()->String()) == 0)
			return item;
	}

	return NULL;
}


status_t
StationsList::Load()
{
	status_t status;
	BDirectory* stationsDir = Station::StationDirectory();
	BEntry stationEntry;

	while ((status = stationsDir->GetNextEntry(&stationEntry)) == B_OK) {
		Station* station = Station::LoadFromPlsFile(stationEntry.Name());
		if (station != NULL && FindItem(station->Name()) == NULL)
			AddItem(station);
	}

	return B_OK;
}


void
StationsList::Save()
{
	status_t status;
	BDirectory* stationsDir = Station::StationDirectory();
	BEntry stationEntry;

	// Reset BDirectory so it returns all files
	stationsDir->Rewind();

	while ((status = stationsDir->GetNextEntry(&stationEntry)) == B_OK)
		stationEntry.Remove();

	EachListItemIgnoreResult<Station, status_t>(this, &Station::Save);
}


RadioSettings::RadioSettings()
	:
	BMessage()
{
	Stations = new StationsList();
	_Load();
}


RadioSettings::RadioSettings(const RadioSettings& orig)
	:
	BMessage(orig)
{
	Stations = new StationsList();
	Stations->AddList(orig.Stations);
}


RadioSettings::~RadioSettings()
{
	delete Stations;
}


status_t
RadioSettings::Save()
{
	status_t status;
	BPath configPath;
	BDirectory configDir;
	BFile configFile;
	status = find_directory(B_USER_SETTINGS_DIRECTORY, &configPath);
	if (status != B_OK)
		return status;

	configDir.SetTo(configPath.Path());
	status = configDir.CreateFile(kSettingsFileName, &configFile);
	Flatten(&configFile);

	Stations->Save();

	return B_OK;
}


bool
RadioSettings::GetAllowParallelPlayback()
{
	return GetBool("allowParallelPlayback");
}


void
RadioSettings::SetAllowParallelPlayback(bool set)
{
	SetBool("allowParallelPlayback", set);
}


const char*
RadioSettings::StationFinderName()
{
	return GetString("stationfinder", NULL);
}


void
RadioSettings::SetStationFinderName(const char* name)
{
	RemoveName("stationfinder");
	AddString("stationfinder", name);
}


status_t
RadioSettings::_Load()
{
	status_t status;
	BPath configPath;
	BDirectory configDir;
	BFile configFile;

	status = find_directory(B_USER_SETTINGS_DIRECTORY, &configPath);
	if (status != B_OK)
		return status;

	configDir.SetTo(configPath.Path());
	status = configFile.SetTo(&configDir, kSettingsFileName, B_READ_ONLY);
	status = Unflatten(&configFile);
	status = Stations->Load();

	return status;
}
