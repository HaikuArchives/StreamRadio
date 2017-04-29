/* 
 * File:   StationFinderRadioNetwork.h
 * Author: user
 *
 * Created on 9. Oktober 2015, 22:51
 */

#ifndef STATIONFINDERRADIONETWORK_H
#define	STATIONFINDERRADIONETWORK_H

#include "StationFinder.h"

class IconLookup {
public:
									IconLookup(Station* station, BUrl iconUrl);
	Station*						fStation;
	BUrl							fIconUrl;
};

class StationFinderRadioNetwork : public StationFinderService {
public:
									StationFinderRadioNetwork();
    virtual							~StationFinderRadioNetwork();
	static void						RegisterSelf();
	static StationFinderService*	Instantiate();
    virtual BObjectList<Station>*	FindBy(FindByCapability capability, const char* searchFor, 
											BLooper* resultUpdateTarget);
private:
    static const char*				baseUrl; 
	thread_id						fIconLookupThread;
	BObjectList<IconLookup>			fIconLookupList;
	BLooper*						fIconLookupNotify;
	
	static int32					IconLookupFunc(void* data);
};

#endif	/* STATIONFINDERRADIONETWORK_H */

