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
#include <HttpRequest.h>
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


typedef class StationFinderService* (*InstantiateFunc)();


class StationFinderServices {
public:
	StationFinderServices(){};
	~StationFinderServices();

	static void Register(char* serviceName, InstantiateFunc);

	static StationFinderService* Instantiate(char* s);

	static int32 CountItems();
	static char* Name(int i);

private:
	static std::vector<std::pair<char*, InstantiateFunc> > sServices;
};

class FindByCapability {
public:
	FindByCapability(char* name);
	FindByCapability(char* name, char* keyWords, char* delimiter);
	~FindByCapability();

	bool HasKeyWords();
	void SetKeyWords(char* keyWords, char* delimiter);
	const BStringList* KeyWords();

	const char* Name();

private:
	BString fName;
	BStringList fKeywords;
};

class StationFinderService {
	friend class StationFinderWindow;

public:
	// Overridden in specific StationFinder implementations
	StationFinderService();
	virtual ~StationFinderService();

	static void RegisterSelf();
	static StationFinderService* Instantiate();

	virtual BObjectList<Station>* FindBy(
		int capabilityIndex, const char* searchFor, BLooper* resultUpdateTarget)
		= 0;

	// Provided by ancestor class
	const char* Name() const { return serviceName.String(); }

	int CountCapabilities() const { return findByCapabilities.CountItems(); }
	FindByCapability* Capability(int index) const { return findByCapabilities.ItemAt(index); }

	static void Register(char* name, InstantiateFunc instantiate);

protected:
	// To be filled by specific StationFinder implementations
	BString serviceName;
	BUrl serviceHomePage;
	BBitmap* serviceLogo;
	BObjectList<FindByCapability> findByCapabilities;

	// Helper functions
	BBitmap* RetrieveLogo(BUrl url);
	FindByCapability* RegisterSearchCapability(char* name);
	FindByCapability* RegisterSearchCapability(char* name, char* keyWords, char* delimiter);
};

class StationFinderWindow : public BWindow {
public:
	StationFinderWindow(BWindow* parent);
	virtual ~StationFinderWindow();

	void MessageReceived(BMessage* msg);
	virtual bool QuitRequested();

	void SelectService(int index);
	void SelectCapability(int index);
	void DoSearch(const char* text);

private:
	StationFinderService* fCurrentService;

	BMessenger* fMessenger;
	BTextControl* fTxSearch;
	BOptionPopUp* fKwSearch;
	BButton* fBnSearch;
	BOptionPopUp* fDdServices;
	BButton* fBnVisit;
	BOptionPopUp* fDdSearchBy;
	StationListView* fResultView;
	BButton* fBnAdd;

	BGridLayout* fSearchGrid;
};


#endif	// _STATION_FINDER_H
