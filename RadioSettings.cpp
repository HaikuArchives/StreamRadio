/* 
 * File:   RadioSettings.cpp
 * Author: user
 * 
 * Created on 26. Februar 2013, 04:01
 */
#include <Path.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <File.h>

#include "RadioSettings.h"
#include "Debug.h"

const char* SettingsFileName = "StreamRadio.settings";

StationsList::StationsList() 
  : BObjectList<Station>() 
{
}

StationsList::~StationsList() {
    for (int i = CountItems() - 1; i >= 0; i--) {
        BObjectList<Station>::RemoveItem(ItemAt(i), true);
    }
}

bool    
StationsList::AddItem(Station* station) { 
    if (FindItem(station->Name())) return false;
        return BObjectList<Station>::AddItem(station); 
}

bool
StationsList::RemoveItem(BString* stationName) {
    Station* s = FindItem(stationName); 
    if (s)
        return BObjectList<Station>::RemoveItem(s, true);
    else
        return false; 
}

bool
StationsList::RemoveItem(Station* station) {
	return BObjectList<Station>::RemoveItem(station, false);
}

Station*
StationsList::FindItem(BString* stationName) {
    for (int i = 0; i < CountItems(); i++) {
        Station* item = ItemAt(i);
        if (stationName->Compare(item->Name()->String()) == 0) 
            return item;
    }
    return NULL;
} 

status_t 
StationsList::Load() {
    status_t status;
    BDirectory* stationsDir = Station::StationDirectory();
    BEntry stationEntry;

    while ((status = stationsDir->GetNextEntry(&stationEntry)) == B_OK) {
        Station* station = Station::LoadFromPlsFile(stationEntry.Name());
        if (station && FindItem(station->Name()) == NULL)
            AddItem(station);
    }
    return B_OK;
}


void 
StationsList::Save() {
    status_t status;
    BDirectory* stationsDir = Station::StationDirectory();
    BEntry stationEntry;
    
    // Reset BDirectory so it returns all files
    stationsDir->Rewind();
    
    while ((status = stationsDir->GetNextEntry(&stationEntry)) == B_OK) {
        stationEntry.Remove();
    }
    EachListItemIgnoreResult<Station, status_t>(this, &Station::Save);
}


RadioSettings::RadioSettings() 
  : BMessage() 
{
    Stations = new StationsList();
    Load();
}

RadioSettings::RadioSettings(const RadioSettings& orig) : BMessage(orig) {
    Stations = new StationsList();
    Stations->AddList(orig.Stations);
}

RadioSettings::~RadioSettings() {
    delete Stations;
}

const char*
RadioSettings::StationFinderName() {
	return GetString("stationfinder", NULL);
}

void
RadioSettings::SetStationFinderName(const char* name) {
	RemoveName("stationfinder");
	AddString("stationfinder", name);
}
status_t 
RadioSettings::Load() {
    status_t status;
    BPath configPath;
    BDirectory configDir;
    BFile configFile;
    
    status = find_directory(B_USER_SETTINGS_DIRECTORY, &configPath);
    if (status!=B_OK) return status;

    configDir.SetTo(configPath.Path());
    status = configFile.SetTo(&configDir, SettingsFileName, B_READ_ONLY);
    status = Unflatten(&configFile);
    status = Stations->Load();
    return status;
}

status_t 
RadioSettings::Save() {
    status_t status;
    BPath configPath;
    BDirectory configDir;
    BFile configFile;
    status = find_directory(B_USER_SETTINGS_DIRECTORY, &configPath);
    if (status!=B_OK) return status;
    
    configDir.SetTo(configPath.Path());
    status = configDir.CreateFile(SettingsFileName, &configFile);
    Flatten(&configFile);

    Stations->Save();  
    return B_OK;
}

