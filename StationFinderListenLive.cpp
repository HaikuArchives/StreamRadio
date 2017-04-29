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
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/* 
 * File:   StationFinderListenLive.cpp
 * Author: Kai Niessen <kai.niessen@online.de>
 * 
 * Created on March 27, 2017, 12:16 AM
 */

#include "StationFinderListenLive.h"
#define __USE_GNU
#include <regex.h>
#include "Debug.h"

StationFinderListenLive::StationFinderListenLive()
  : StationFinderService(),
	fLookupThread(0),
    fPlsLookupList()
{
    serviceName.SetTo("listenlive.eu [experimental]");
    serviceHomePage.SetUrlString("http://www.listenlive.eu/");
    caps.insert(FindByCountry);
	caps.insert(FindByGenre);
}

StationFinderService*
StationFinderListenLive::Instantiate() {
	return new StationFinderListenLive();
}

void
StationFinderListenLive::RegisterSelf() {
	Register("listenlive.eu [experimental]", &StationFinderListenLive::Instantiate);
}

StationFinderListenLive::~StationFinderListenLive() {
	fPlsLookupList.MakeEmpty(false);
}

BObjectList<Station>* 
StationFinderListenLive::FindBy(FindByCapability capability, const char* searchFor, 
  BLooper* resultUpdateTarget) {
	if (fLookupThread)
		suspend_thread(fLookupThread);
	fPlsLookupList.MakeEmpty(false);
    fLookupNotify = resultUpdateTarget;
	
    BObjectList<Station>* result = new BObjectList<Station>();
    
	BString urlString(baseUrl);
	strlwr((char*)searchFor);
    urlString << searchFor << ".html";
    BUrl url(urlString);
    BMallocIO* data = HttpUtils::GetAll(url);

//	<tr>
//	<td><a href="http://oe1.orf.at/"><b>ORF Ö-1</b></a></td>
//	<td>Vienna</td>
//	<td><img src="wma.gif" width="12" height="12" alt="Windows Media"><br><img src="mp3.gif" width="12" height="12" alt="MP3"></td>
//	<td><a href="mms://apasf.apa.at/oe1_live_worldwide">128 Kbps</a><br><a href="http://mp3stream3.apasf.apa.at:8000/listen.pls">192 Kbps</a></td>
//	<td>Information/Culture</td>
//	</tr>
	
	
    if (data) {
		regmatch_t matches[10];
		regex_t	   regex;
		int res = regcomp(&regex,  
				"<tr>\\s*<td>\\s*<a\\s+href=\"([^\"]*)\"><b>([^<]*)</b>\\s*</a>\\s*</td>\\s*"
				"<td>([^<]*)</td>\\s*"
				"<td>([^<]|<[^/]|</[^t]|</t[^d])*</td>\\s*"
				"<td>\\s*<a\\s+href=\"([^\"]*)\">([^<]*)</a>([^<]|<[^/]|</[^t]|</t[^d])*</td>\\s*"
				"<td>([^<]*)</td>\\s*</tr>",
				RE_ICASE | RE_DOT_NEWLINE | RE_DOT_NOT_NULL | RE_NO_BK_PARENS | RE_NO_BK_VBAR | 
				RE_BACKSLASH_ESCAPE_IN_LISTS);

		char* doc = (char*)data->Buffer();
		doc[data->BufferLength()]=0;
		
		while ((res = regexec(&regex, doc, 10, matches, 0)) == 0) {
			doc[matches[1].rm_eo] = 0;
			doc[matches[2].rm_eo] = 0;
			doc[matches[3].rm_eo] = 0;
			doc[matches[5].rm_eo] = 0;
			doc[matches[6].rm_eo] = 0;
			doc[matches[8].rm_eo] = 0;
			printf("Station %s at %s in %s address %s, data rate=%s, genre=%s\n", 
					doc + matches[2].rm_so, 
					doc + matches[1].rm_so, 
					doc + matches[3].rm_so, 
					doc + matches[5].rm_so, 
					doc + matches[6].rm_so,
					doc + matches[8].rm_so);

			Station* station = new Station(doc + matches[2].rm_so, NULL);
			BString source(doc + matches[5].rm_so);
			if (source.EndsWith(".pls") || source.EndsWith(".m3u")) {
				station->SetSource(BUrl(source));
				fPlsLookupList.AddItem(station);
			} else
				station->SetStreamUrl(BUrl(source));
			
			for (int i = matches[6].rm_so; i < matches[6].rm_eo; i++)
				if (!strchr("0123456789.", doc[i])) {
					doc[i] = 0;
					station->SetBitRate(atof(doc + matches[6].rm_so) * 1000);
					break;
				}
			
			station->SetStation(doc + matches[1].rm_so);
			station->SetGenre(doc + matches[8].rm_so);
			BString country;
			if (capability == FindByCountry) {
				country.SetTo(searchFor);
				country.Append(" - ");
			}
			country.Append(doc + matches[3].rm_so);
			station->SetCountry(country);
			result->AddItem(station);
			if (station->Source().Path().EndsWith(".pls") || station->Source().Path().EndsWith(".m3u"))
				fPlsLookupList.AddItem(station);			
			doc += matches[0].rm_eo;
		}
		
        delete data;
		if (!fPlsLookupList.IsEmpty()) {
			if (!fLookupThread)
				fLookupThread = spawn_thread(&PlsLookupFunc, "plslookup", B_NORMAL_PRIORITY, this);
			resume_thread(fLookupThread);
		}
    } else {
        delete result;
        result = NULL;
    }
    return result;
}

int32
StationFinderListenLive::PlsLookupFunc(void* data) {
    StationFinderListenLive* _this = (StationFinderListenLive*)data;
    while (!_this->fPlsLookupList.IsEmpty()) {
	Station* station = _this->fPlsLookupList.FirstItem();
	TRACE("Looking up stream URL for station %s in %s\n", 
			station->Name()->String(), 
			station->Source().UrlString().String());
	Station* plsStation = Station::LoadIndirectUrl((BString&)station->Source().UrlString());
	if (plsStation) {
		station->SetStreamUrl(plsStation->StreamUrl());
	    BMessage* notification = new BMessage(MSG_UPDATE_STATION);
	    notification->AddPointer("station", station);
		if (_this->fLookupNotify->LockLooper()) {
			_this->fLookupNotify->PostMessage(notification);
			_this->fLookupNotify->UnlockLooper();
		}
	}
	_this->fPlsLookupList.RemoveItem(station, false);
    }
    _this->fLookupThread = 0;
    return B_OK;
}

const char* 
StationFinderListenLive::baseUrl = "http://listenlive.eu/";

