/* 
 * File:   StationFinder.h
 * Author: Kai Niessen
 *
 * Created on 9. Oktober 2015, 22:53
 */


#ifndef STATIONFINDER_H
#define	STATIONFINDER_H
#include <set>
#include <vector>
#include <iterator>
#include <Window.h>
#include <Messenger.h>
#include <TextControl.h>
#include <OptionPopUp.h>
#include <HttpRequest.h>
#include <ObjectList.h>
#include <StringList.h>
#include <StatusBar.h>
#include <GridLayout.h>
#include "Station.h"
#include "StationListView.h"

#define MSG_TXSEARCH				'tSRC'
#define MSG_KWSEARCH				'kSRC'
#define MSG_BNSEARCH				'bSRC'
#define MSG_SEARCH_CLOSE			'mCLS'
#define MSG_ADD_STATION				'mADS'
#define MSG_SELECT_SERVICE			'mSSV'
#define MSG_SEARCH_BY				'mSBY'
#define MSG_VISIT_SERVICE			'mVSV'
#define MSG_SELECT_STATION			'mSLS'
#define MSG_UPDATE_STATION			'mUPS'

#define RES_BN_SEARCH				10


using namespace std;

typedef class StationFinderService* (*InstantiateFunc)();

class StationFinderServices {
public:
									StationFinderServices() { };
									~StationFinderServices();
	static void						Register(char* serviceName, InstantiateFunc);
	static int						CountItems();
	static StationFinderService*	Instantiate(char* s);
	static char*					Name(int i);
private:
	static vector<pair<char*, InstantiateFunc> >
									fServices;
};

class FindByCapability {
public:
									FindByCapability(char* name);
									FindByCapability(char* name, char* KeyWords, char* delimiter);
									~FindByCapability();
	bool							HasKeyWords();
	void							SetKeyWords(char* KeyWords, char* delimiter);
	const BStringList*				KeyWords();
	const char*						Name();
private:
	BString							fName;
	BStringList						fKeywords;
};

class StationFinderService {
    friend class					StationFinderWindow;
public:
	// To be overridden in specific StationFinder implementations
									StationFinderService();
    virtual							~StationFinderService();
    static void						RegisterSelf();
	static StationFinderService*	Instantiate(); 
    virtual BObjectList<Station>*   FindBy(int capabilityIndex, 
											const char* searchFor, 
											BLooper* resultUpdateTarget) = 0;

	// Provided by ancestor class
    const char*						Name() const { return serviceName.String(); }
    int								CountCapabilities() const { 
										return findByCapabilities.CountItems(); 
									}; 
	FindByCapability*				Capability(int index) const { 
										return findByCapabilities.ItemAt(index);
									};
    static void						Register(char* name, InstantiateFunc instantiate);
protected:
	// To be filled by specific StationFinder implementations 
    BString							serviceName;
    BUrl							serviceHomePage;
    BBitmap*						serviceLogo;
	BObjectList<FindByCapability>	findByCapabilities;

	// Helper functions
    BBitmap*						logo(BUrl url);
	FindByCapability*				RegisterSearchCapability(char* name);
	FindByCapability*				RegisterSearchCapability(char* name, 
											char* keyWords, char* delimiter);
};

class StationFinderWindow : public BWindow {
public:
									StationFinderWindow(BWindow* parent);
    virtual							~StationFinderWindow();
    void							MessageReceived(BMessage* msg);
    virtual bool					QuitRequested();
	void							SelectService(int index);
	void							SelectCapability(int index);
    void							DoSearch(const char* text);
private:
	StationFinderService*			currentService;
    BMessenger*						messenger;
    BTextControl*					txSearch;
	BOptionPopUp*					kwSearch;
    BButton*						bnSearch;
    StationListView*				resultView;
    BButton*						bnAdd;
	BButton*						bnVisit;
    BOptionPopUp*					ddServices;
    BOptionPopUp*					ddSearchBy;
	BGridLayout*					searchGrid;
};

#endif	/* STATIONFINDER_H */

