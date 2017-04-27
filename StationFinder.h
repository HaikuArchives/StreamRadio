/* 
 * File:   StationFinder.h
 * Author: Kai Niessen
 *
 * Created on 9. Oktober 2015, 22:53
 */


#ifndef STATIONFINDER_H
#define	STATIONFINDER_H
#include <set>
#include <iterator>
#include <Window.h>
#include <Messenger.h>
#include <TextControl.h>
#include <OptionPopUp.h>
#include <HttpRequest.h>
#include <ObjectList.h>
#include <StatusBar.h>
#include "Station.h"
#include "StationListView.h"

#define MSG_TXSEARCH				'tSRC'
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

class IconLookup {
public:
									IconLookup(Station* station, BUrl iconUrl);
	Station*						fStation;
	BUrl							fIconUrl;
};

class StationFinderService {
    friend class					StationFinderWindow;
public:
    enum							FindByCapability {
        FindByName,
        FindByCountry,
        FindByGenre,
        FindByKeyword
    };
	static const char*				FindByCapabilityName[];
	
									StationFinderService();
    virtual							~StationFinderService() {};
    const char*						Name() { return serviceName.String(); }
    set<int>						Capabilities() { return caps; }
    virtual bool					canFindBy(FindByCapability capability) { 
										return caps.count(capability)==1; 
									};
    virtual BObjectList<Station>*   FindBy(FindByCapability capability, 
											const char* searchFor, 
											BLooper* resultUpdateTarget) = 0;

    static BObjectList<StationFinderService>* 
									getStationFinderServices();
    static bool						registerStationFinderService(StationFinderService* service);
    
protected:
    BString							serviceName;
    BUrl							serviceHomePage;
    BBitmap*						serviceLogo;
    set<int>						caps;
    static BObjectList<StationFinderService> 
									stationFinderServices;
    BBitmap*						logo(BUrl url);
};


class StationFinderWindow : public BWindow {
public:
									StationFinderWindow(BWindow* parent);
    virtual							~StationFinderWindow();
    void							MessageReceived(BMessage* msg);
    virtual bool					QuitRequested();
	void							SelectService(int index);
    void							DoSearch(const char* text);
private:
	StationFinderService*			currentService;
    BMessenger*						messenger;
    BTextControl*					txSearch;
    BButton*						bnSearch;
    StationListView*				resultView;
    BButton*						bnAdd;
	BButton*						bnVisit;
    BOptionPopUp*					ddServices;
    BOptionPopUp*					ddSearchBy;
};

#endif	/* STATIONFINDER_H */

