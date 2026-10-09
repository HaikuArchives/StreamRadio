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
#ifndef _STATION_FINDER_H
#define _STATION_FINDER_H


#include <iterator>
#include <set>
#include <vector>

#include <GridLayout.h>
#include <private/netservices/HttpRequest.h>
#include <Messenger.h>
#include <ObjectList.h>
#include <OptionPopUp.h>
#include <StatusBar.h>
#include <StringList.h>
#include <TextControl.h>
#include <Window.h>

#include "Station.h"
#include "StationListView.h"


#define MSG_TXSEARCH 'tSRC'
#define MSG_KWSEARCH 'kSRC'
#define MSG_BNSEARCH 'bSRC'
#define MSG_SEARCH_CLOSE 'mCLS'
#define MSG_ADD_STATION 'mADS'
#define MSG_SELECT_SERVICE 'mSSV'
#define MSG_SEARCH_BY 'mSBY'
#define MSG_VISIT_SERVICE 'mVSV'
#define MSG_SELECT_STATION 'mSLS'
#define MSG_UPDATE_STATION 'mUPS'

#define RES_BN_SEARCH 10


#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
typedef BObjectList<Station, true> StationList;
#else
typedef BObjectList<Station> StationList;
#endif

typedef class StationFinderService* (*InstantiateFunc)();

class StationFinderEntry {
public:
	StationFinderEntry() { };
	StationFinderEntry(BString* name, InstantiateFunc instantiateFunc) 
		: Name(name->String())
	{
		this->instantiate = instantiateFunc;
	}		
	BString 				Name;
	StationFinderService*   Get() { return instance != NULL ? instance : instantiate(); }
private:
	InstantiateFunc 		instantiate;
	StationFinderService*	instance = NULL;
};


class StationFinderServices : public BObjectList<StationFinderEntry, true> {
public:
	StationFinderServices() : BObjectList<StationFinderEntry, true>(32) {};
	~StationFinderServices() { MakeEmpty(); };

	void Register(BString* serviceName, InstantiateFunc instantiate) { 
		AddItem(new StationFinderEntry(serviceName, instantiate)); 
	}
	BString* Name(int index)  { return &ItemAt(index)->Name; } 
	StationFinderService* Instantiate(int index) { return ItemAt(index)->Get(); }
	int32 IndexOf(BString* name) { return BinarySearchIndexByKey<BString>(*name, CompareName); }
private:
	static int CompareName(const BString* name, const StationFinderEntry* entry) { return name->Compare(entry->Name); }
};

static StationFinderServices stationFinderServices;

class FindByCapability {
public:
	FindByCapability(const char* name);
	FindByCapability(const char* name, BStringList* keyWords);
	FindByCapability(const char* name, char* keyWords, char* delimiter);
	~FindByCapability();

	bool HasKeyWords() { return !fKeywords.IsEmpty(); }
	const BStringList* KeyWords() { return &fKeywords; }
	const char* Name() { return fName.String(); }

private:
	BString fName;
	BStringList fKeywords;
};

class StationFinderService {
	friend class StationFinderWindow;

public:
	// Overridden in specific StationFinder implementations
									StationFinderService();
	virtual 						~StationFinderService();

	static void 					RegisterSelf();
	static StationFinderService* 	Instantiate();

	virtual StationList* 			FindBy(
			int capabilityIndex, const char* searchFor, BLooper* resultUpdateTarget)
			= 0;

	// Provided by ancestor class
	BString* 						Name() 
									{ return &serviceName; }

	int 							CountCapabilities() const 
									{ return findByCapabilities.CountItems(); }
	FindByCapability* 				Capability(int index) const 
									{ return findByCapabilities.ItemAt(index); }

	static void 					Register(BString* name, InstantiateFunc instantiate);

protected:
	// To be filled by specific StationFinder implementations
    BString 						serviceName;
	BUrl 							serviceHomePage;
	BBitmap* 						serviceLogo;
#if B_HAIKU_VERSION > B_HAIKU_VERSION_1_BETA_5
	BObjectList<FindByCapability, true> 
									findByCapabilities;
#else
	BObjectList<FindByCapability> 	findByCapabilities;
#endif

	// Helper functions
	BBitmap* 						RetrieveLogo(BUrl* url);
	uint32 							RegisterSearchCapability(const char* name);
	uint32 							RegisterSearchCapability(const char* name, BStringList* keyWords);
};

class StationFinderWindow : public BWindow {
public:
									StationFinderWindow(BWindow* parent);
	virtual 						~StationFinderWindow();

	void 							MessageReceived(BMessage* msg);
	virtual bool 					QuitRequested();

	void 							SelectService(int index);
	void 							SelectCapability(int index);
	void 							DoSearch(const char* text);

private:
	StationFinderService* 			fCurrentService;

	BMessenger*						fMessenger;
	BTextControl*					fTxSearch;
	BOptionPopUp*					fKwSearch;
	BButton*						fBnSearch;
	BOptionPopUp*					fDdServices;
	BButton*						fBnVisit;
	BOptionPopUp*					fDdSearchBy;
	StationListView* 				fResultView;
	BButton* 						fBnAdd;

	BGridLayout* 					fSearchGrid;
};


#endif	// _STATION_FINDER_H
