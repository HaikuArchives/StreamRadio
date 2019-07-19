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
#include <Catalog.h>
#include "Debug.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StationFinderListenLive"


const char* genreList =
"Top 40|top40\r\
Hot Adult Contemporary|hotac\r\
Adult Contemporary|ac\r\
Oldies/80s/90s|oldies\r\
Country music|country\r\
RnB/HipHop|rnb\r\
Dance/Techno|dance\r\
Rock/Classic Rock|rock\r\
Alternative|alternative\r\
Chillout/Lounge|chillout";


const char* countryList = 
"Andorra|andorra\r\
Armenia|armenia\r\
Austria|austria\r\
Azerbaijan|azerbaijan\r\
Belarus|belarus\r\
Belgium|belgium\r\
Bosnia-Herzegovina|bosnia\r\
Bulgaria|bulgaria\r\
Croatia|croatia\r\
Cyprus|cyprus\r\
Czech Republic|czech-republic\r\
Denmark|denmark\r\
Estonia|estonia\r\
Faroe Islands|faroe\r\
Finland|finland\r\
France|france\r\
Georgia|georgia\r\
Germany|germany\r\
Gibraltar|gibraltar\r\
Greece|greece\r\
Hungary|hungary\r\
Iceland|iceland\r\
Ireland|ireland\r\
Italy|italy\r\
Kosovo|kosovo\r\
Latvia|latvia\r\
Liechtenstein|liechtenstein\r\
Lithuania|lithuania\r\
Luxembourg|luxembourg\r\
Macedonia|macedonia\r\
Malta|malta\r\
Moldova|moldova\r\
Monaco|monaco\r\
Montenegro|montenegro\r\
Netherlands|netherlands\r\
Norway|norway\r\
Poland|poland\r\
Portugal|portugal\r\
Romania|romania\r\
Russia|russia\r\
San Marino|san-marino\r\
Serbia|serbia\r\
Slovakia|slovakia\r\
Slovenia|slovenia\r\
Spain|spain\r\
Sweden|sweden\r\
Switzerland|switzerland\r\
Turkey|turkey\r\
Ukraine|ukraine\r\
United Kingdom|uk\r\
Vatican State|vatican";

StationFinderListenLive::StationFinderListenLive()
  : StationFinderService(),
	countryKeywordAndPath(),
	genreKeywordAndPath(),
	fLookupThread(0),
    fPlsLookupList()
{
    serviceName.SetTo(B_TRANSLATE("listenlive.eu [experimental]"));
    serviceHomePage.SetUrlString("http://www.listenlive.eu/");
	BString tmp(countryList);
	tmp.Split("\r", true, countryKeywordAndPath);
	FindByCapability* findByCountry = RegisterSearchCapability("Country");
	BStringList* keywords = (BStringList*)findByCountry->KeyWords();
	for (int i = 0; i < countryKeywordAndPath.CountStrings(); i++) {
		BString keyword(countryKeywordAndPath.StringAt(i));
		keyword.Truncate(keyword.FindFirst('|'));
		keywords->Add(keyword);
	}
	tmp.SetTo(genreList);
	tmp.Split("\r", true, genreKeywordAndPath);
	FindByCapability* findByGenre = RegisterSearchCapability("Genre");
	keywords = (BStringList*)findByGenre->KeyWords();
	for (int i = 0; i < genreKeywordAndPath.CountStrings(); i++) {
		BString keyword(genreKeywordAndPath.StringAt(i));
		keyword.Truncate(keyword.FindFirst('|'));
		keywords->Add(keyword);
	}
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
	status_t status;
	if (fLookupThread)
		wait_for_thread(fLookupThread, &status);
}

BObjectList<Station>* 
StationFinderListenLive::FindBy(int capabilityIndex, const char* searchFor, 
  BLooper* resultUpdateTarget) {
	if (fLookupThread)
		suspend_thread(fLookupThread);
	fPlsLookupList.MakeEmpty(false);
    fLookupNotify = resultUpdateTarget;
	
    BObjectList<Station>* result = NULL;
    
	BString urlString(baseUrl);
	strlwr((char*)searchFor);
	int keywordIndex = Capability(capabilityIndex)->KeyWords()->IndexOf(searchFor);
	BStringList* keywordAndPaths = capabilityIndex ? &genreKeywordAndPath : &countryKeywordAndPath;
	BString path(keywordAndPaths->StringAt(keywordIndex));
	path.Remove(0, path.FindFirst('|') + 1);
    urlString << path << ".html";
    BUrl url(urlString);
    BMallocIO* data = HttpUtils::GetAll(url);

    if (data) {
		switch(capabilityIndex) {
		case 0: // findByCountry
			result = ParseCountryReturn(data, searchFor);
			break;
		case 1: // findByGenre
			result = ParseGenreReturn(data, searchFor);
			break;
		}

		data->Flush();
        delete data;
		if (!fPlsLookupList.IsEmpty()) {
			if (!fLookupThread)
				fLookupThread = spawn_thread(&PlsLookupFunc, "plslookup", B_NORMAL_PRIORITY, this);
			resume_thread(fLookupThread);
		}
    } 
    return result;
}

BObjectList<Station>*
StationFinderListenLive::ParseCountryReturn(BMallocIO* data, const char* searchFor) {
	BObjectList<Station>* result = new BObjectList<Station>(20, true);
	regmatch_t matches[10];
	regex_t	   regex;
	
//	<tr>
//	<td><a href="http://oe1.orf.at/"><b>ORF Ö-1</b></a></td>
//	<td>Vienna</td>
//	<td><img src="wma.gif" width="12" height="12" alt="Windows Media"><br><img src="mp3.gif" width="12" height="12" alt="MP3"></td>
//	<td><a href="mms://apasf.apa.at/oe1_live_worldwide">128 Kbps</a><br><a href="http://mp3stream3.apasf.apa.at:8000/listen.pls">192 Kbps</a></td>
//	<td>Information/Culture</td>
//	</tr>
	
	int res = regcomp(&regex,  
			"<tr>\\s*<td>\\s*<a\\s+href=\"([^\"]*)\"><b>([^<]*)</b>\\s*</a>\\s*</td>\\s*"
			"<td>([^<]*)</td>\\s*"
			"<td>([^<]|<[^/]|</[^t]|</t[^d])*</td>\\s*"
			"<td>\\s*<a\\s+href=\"([^\"]*)\">([^<]*)</a>([^<]|<[^/]|</[^t]|</t[^d])*</td>\\s*"
			"<td>([^<]*)</td>\\s*</tr>",
			RE_ICASE | RE_DOT_NEWLINE | RE_DOT_NOT_NULL | RE_NO_BK_PARENS | RE_NO_BK_VBAR | 
			RE_BACKSLASH_ESCAPE_IN_LISTS);

	char* doc = (char*)data->Buffer();
	off_t size;
	data->GetSize(&size);
	doc[size]=0;

	while ((res = regexec(&regex, doc, 10, matches, 0)) == 0) {
		doc[matches[1].rm_eo] = 0;
		doc[matches[2].rm_eo] = 0;
		doc[matches[3].rm_eo] = 0;
		doc[matches[5].rm_eo] = 0;
		doc[matches[6].rm_eo] = 0;
		doc[matches[8].rm_eo] = 0;
		printf(B_TRANSLATE("Station %s at %s in %s address %s, data rate=%s, genre=%s\n"), 
				doc + matches[2].rm_so, 
				doc + matches[1].rm_so, 
				doc + matches[3].rm_so, 
				doc + matches[5].rm_so, 
				doc + matches[6].rm_so,
				doc + matches[8].rm_so);

		Station* station = new Station(doc + matches[2].rm_so, NULL);
		result->AddItem(station);

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
		country.SetTo(searchFor);
		country.Append(" - ");
		country.Append(doc + matches[3].rm_so);
		station->SetCountry(country);
		doc += matches[0].rm_eo;
	}	
	return result;
}

BObjectList<Station>*
StationFinderListenLive::ParseGenreReturn(BMallocIO* data, const char* searchFor) {
	BObjectList<Station>* result = new BObjectList<Station>(20, true);
	regmatch_t matches[9];
	regex_t	   regex;
	
//	<tr>
//	<td><a href="http://oe1.orf.at/"><b>ORF Ö-1</b></a></td>
//	<td>Vienna</td>
//	<td><img src="wma.gif" width="12" height="12" alt="Windows Media"><br><img src="mp3.gif" width="12" height="12" alt="MP3"></td>
//	<td><a href="mms://apasf.apa.at/oe1_live_worldwide">128 Kbps</a><br><a href="http://mp3stream3.apasf.apa.at:8000/listen.pls">192 Kbps</a></td>
//	<td>Information/Culture</td>
//	</tr>
	
//	<tr>
//	<td><a href="http://www.radiostephansdom.at/"><b>Radio Stephansdom</b></a></td>
//	<td>Vienna</td>
//	<td>Austria</td>
//	<td><img src="mp3.gif" width="12" height="12" alt="MP3"><br></td>
//	<td><a href="http://streaming.lxcluster.at:8000/live32.m3u">32 Kbps</a>, <a href="http://streaming.lxcluster.at:8000/live56.m3u">56 Kbps</a><br><a href="http://streaming.lxcluster.at:8000/live128.m3u">128 Kbps</a></td>
//	</tr>	
	
	int res = regcomp(&regex,  
			"<tr>\\s*<td>\\s*<a\\s+href=\"([^\"]*)\"><b>([^<]*)</b>\\s*</a>\\s*</td>\\s*"
			"<td>([^<]*)</td>\\s*"
			"<td>([^<]*)</td>\\s*"
			"<td>([^<]|<[^/]|</[^t]|</t[^d])*</td>\\s*"
			"<td>\\s*<a\\s+href=\"([^\"]*)\">([^<]*)</a>([^<]|<[^/]|</[^t]|</t[^d])*</td>\\s*"
			"</tr>",
			RE_ICASE | RE_DOT_NEWLINE | RE_DOT_NOT_NULL | RE_NO_BK_PARENS | RE_NO_BK_VBAR | 
			RE_BACKSLASH_ESCAPE_IN_LISTS);

	char* doc = (char*)data->Buffer();
	off_t size;
	data->GetSize(&size);
	doc[size]=0;

	while ((res = regexec(&regex, doc, 9, matches, 0)) == 0) {
		doc[matches[1].rm_eo] = 0;
		doc[matches[2].rm_eo] = 0;
		doc[matches[3].rm_eo] = 0;
		doc[matches[4].rm_eo] = 0;
		doc[matches[6].rm_eo] = 0;
		doc[matches[7].rm_eo] = 0;
		printf(B_TRANSLATE("Station %s at %s in %s - %s address %s, data rate=%s, genre=%s\n"), 
				doc + matches[2].rm_so, 
				doc + matches[1].rm_so, 
				doc + matches[3].rm_so, 
				doc + matches[4].rm_so, 
				doc + matches[6].rm_so, 
				doc + matches[7].rm_so,
				searchFor);

		Station* station = new Station(doc + matches[2].rm_so, NULL);
		result->AddItem(station);
		BString source(doc + matches[6].rm_so);
		if (source.EndsWith(".pls") || source.EndsWith(".m3u")) {
			station->SetSource(BUrl(source));
			fPlsLookupList.AddItem(station);
		} else
			station->SetStreamUrl(BUrl(source));

		for (int i = matches[7].rm_so; i < matches[7].rm_eo; i++)
			if (!strchr("0123456789.", doc[i])) {
				doc[i] = 0;
				station->SetBitRate(atof(doc + matches[7].rm_so) * 1000);
				break;
			}

		station->SetStation(doc + matches[1].rm_so);
		station->SetGenre(searchFor);
		BString country(doc + matches[4].rm_so);
		country.Append(" - ");
		country.Append(doc + matches[5].rm_so);
		station->SetCountry(country);
		doc += matches[0].rm_eo;
	}
	return result;
}

int32
StationFinderListenLive::PlsLookupFunc(void* data) {
    StationFinderListenLive* _this = (StationFinderListenLive*)data;
    while (!_this->fPlsLookupList.IsEmpty()) {
	Station* station = _this->fPlsLookupList.FirstItem();
	TRACE("Looking up stream URL for station %s in %s\n"), 
			station->Name()->String(), 
			station->Source().UrlString().String();
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

