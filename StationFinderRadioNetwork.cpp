/* 
 * File:   StationFinderRadioNetwork.cpp
 * Author: user
 * 
 * Created on 9. Oktober 2015, 22:51
 */

#include "StationFinderRadioNetwork.h"
#include <HttpRequest.h>
#include <HttpResult.h>
#include <DataIO.h>
#include <libxml/parser.h>
#include <libxml/tree.h>
#include "HttpUtils.h"
#include "StationFinderListenLive.h"

IconLookup::IconLookup(Station* station, BUrl iconUrl) 
  : fStation(station),
    fIconUrl(iconUrl)
{ }

StationFinderRadioNetwork::StationFinderRadioNetwork() 
  : StationFinderService(),
    fIconLookupThread(0),
    fIconLookupList()
{
    serviceName.SetTo("Radio Network");
    serviceHomePage.SetUrlString("http://www.radio-browser.info");
    caps.insert(FindByKeyword);
}

StationFinderRadioNetwork::~StationFinderRadioNetwork() {
	fIconLookupList.MakeEmpty(true);
}

StationFinderService*
StationFinderRadioNetwork::Instantiate() {
	return new StationFinderRadioNetwork();
}

void
StationFinderRadioNetwork::RegisterSelf() {
	Register("Radio Network", &StationFinderRadioNetwork::Instantiate);
}

BObjectList<Station>* 
StationFinderRadioNetwork::FindBy(FindByCapability capability, const char* searchFor, 
		BLooper* resultUpdateTarget) {
	if (fIconLookupThread)
		suspend_thread(fIconLookupThread);
	fIconLookupList.MakeEmpty(true);
    fIconLookupNotify = resultUpdateTarget;
    BObjectList<Station>* result = new BObjectList<Station>();
    BString urlString(baseUrl);
	searchFor = BUrl::UrlEncode(searchFor, true, true);
    urlString << searchFor;
    BUrl url(urlString);
    BMallocIO* data = HttpUtils::GetAll(url);
    if (data) {
        data->Seek(0, SEEK_SET);
        const char* xmlProlog = "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>";
        xmlParserCtxt* parserCtxt = xmlCreatePushParserCtxt(NULL, NULL, xmlProlog, strlen(xmlProlog), "noname.xml");
        xmlParseChunk(parserCtxt, ((const char*)data->Buffer()), data->BufferLength(), 1);
        xmlDocPtr doc = parserCtxt->myDoc;
        xmlFreeParserCtxt(parserCtxt);
        for (xmlNode* nStation = doc->last->children; nStation; nStation = nStation->next) {
            if  (strcmp((char*)nStation->name, "station") == 0) {
                Station* station = new Station("unknown");
                for (xmlAttr* prop = nStation->properties; prop; prop = prop->next) {
                    if (strcmp((char*)prop->name, "name") == 0) {
                        station->SetName((char*)XML_GET_CONTENT(prop->children));
                    } else if (strcmp((char*)prop->name, "url") == 0) {
                        BString sourceString((char*)XML_GET_CONTENT(prop->children));
                        station->SetSource(BUrl(sourceString));
                    } else if (strcmp((char*)prop->name, "favicon") == 0) {
                        BUrl iconUrl((char*)XML_GET_CONTENT(prop->children));
						fIconLookupList.AddItem(new IconLookup(station, iconUrl));
                    } else if (strcmp((char*)prop->name, "homepage") == 0) {
						station->SetStation(((char*)XML_GET_CONTENT(prop->children)));
					} else if (strcmp((char*)prop->name, "tags") == 0) {
						station->SetGenre((char*)XML_GET_CONTENT(prop->children));
					} else if (strcmp((char*)prop->name, "country") == 0) {
						station->SetCountry((char*)XML_GET_CONTENT(prop->children));
					} else if (strcmp((char*)prop->name, "bitrate") == 0) {
						BString kbitRateStr((char*)XML_GET_CONTENT(prop->children));
						int32 kbitRate = atoi(kbitRateStr);
						station->SetBitRate(kbitRate * 1000);
					}
                }
                if (station->Source().Path().EndsWith(".pls") || 
						station->Source().Path().EndsWith(".m3u") || 
						station->Source().Path().EndsWith(".m3u8"))
                    station->RetrieveStreamUrl();
                else 
                    station->SetStreamUrl((const BUrl)station->Source());
                result->AddItem(station);
            }
        }
        delete data;
		if (!fIconLookupList.IsEmpty()) {
			if (!fIconLookupThread) 
				fIconLookupThread = spawn_thread(&IconLookupFunc, "iconlookup", B_NORMAL_PRIORITY, this);
			resume_thread(fIconLookupThread);
		}
    } else {
        delete result;
        result = NULL;
    }
    return result;
}

int32
StationFinderRadioNetwork::IconLookupFunc(void* data) {
    StationFinderRadioNetwork* _this = (StationFinderRadioNetwork*)data;
    while (!_this->fIconLookupList.IsEmpty()) {
		IconLookup* item = _this->fIconLookupList.FirstItem();
		BBitmap* logo = _this->logo(item->fIconUrl);
		if (logo) {
			item->fStation->SetLogo(logo);
			BMessage* notification = new BMessage(MSG_UPDATE_STATION);
			notification->AddPointer("station", item->fStation);
			if (_this->fIconLookupNotify->LockLooper()) {
				_this->fIconLookupNotify->PostMessage(notification);
				_this->fIconLookupNotify->UnlockLooper();
			}
		}
		_this->fIconLookupList.RemoveItem(item, true);
    }
    _this->fIconLookupThread = 0;
    return B_OK;
}

const char* 
StationFinderRadioNetwork::baseUrl = "http://www.radio-browser.info/webservice/xml/stations/";