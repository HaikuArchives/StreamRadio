/*
 * File:   RadioSettings.h
 * Author: user
 *
 * Created on 26. Februar 2013, 04:01
 */

#ifndef RADIOSETTINGS_H
#define RADIOSETTINGS_H

#include "Station.h"
#include <Entry.h>
#include <Message.h>
#include <ObjectList.h>

class StationsList : public BObjectList<Station>
{
public:
	StationsList();
	virtual ~StationsList();
	virtual bool AddItem(Station* station);
	bool RemoveItem(Station* station);
	bool RemoveItem(BString* StationName);
	Station* FindItem(BString* Name);
	status_t Load();
	void Save();
};

class RadioSettings : private BMessage
{
public:
	RadioSettings();
	RadioSettings(const RadioSettings& orig);
	virtual ~RadioSettings();
	status_t Save();
	const char* StationFinderName();
	void SetStationFinderName(const char* name);
	StationsList* Stations;
	bool GetAllowParallelPlayback();
	void SetAllowParallelPlayback(bool set);

private:
	status_t Load();
};
#endif /* RADIOSETTINGS_H */
