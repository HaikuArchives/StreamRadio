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
 * File:   StreamIO.cpp
 * Author: Kai Niessen <kai.niessen@online.de>
 * 
 * Created on April 11, 2017, 10:21 PM
 */

#include "Station.h"
#include "Debug.h"
#include "StreamIO.h"
#include "MediaIO.h"
#include "HttpUtils.h"
#define __USE_GNU
#include <regex.h>
#include <NetworkAddressResolver.h>
#include <Catalog.h>

#define HTTP_TIMEOUT 30000000

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StreamIO"

StreamIO::StreamIO(Station* station, BLooper* MetaListener)
  : BAdapterIO(B_MEDIA_STREAMING | B_MEDIA_SEEKABLE, HTTP_TIMEOUT),
    fStation(station),
    fReq(NULL),
    fReqThread(-1),
    fPosition(0),
    fTotalSize(0),
    fMetaInt(0),
    fMetaSize(0),
    fUntilMetaStart(0),
    fUntilMetaEnd(0),
    fMetaListener(MetaListener),
    fFrameSync(none),
	fLimit(0),
	fBuffered(0)
	
{
	BUrl url = station->StreamUrl();
	BUrl* newUrl = new BUrl();
    status_t portStatus = HttpUtils::CheckPort(url, newUrl);
	if (portStatus != B_OK)
    	return;
    
    fReq = dynamic_cast<BHttpRequest*>(BUrlProtocolRoster::MakeRequest(newUrl->UrlString().String(), this));
    
    BHttpHeaders* headers = new BHttpHeaders();
    headers->AddHeader("Icy-MetaData", 1);
    headers->AddHeader("Icy-Reset", 1);
    headers->AddHeader("Accept", "audio/*");
    fReq->AdoptHeaders(headers);
	fReq->SetFollowLocation(true);
	fReq->SetMaxRedirections(3);
    fInputAdapter = BuildInputAdapter();

    const char* mime = fStation->Mime()->Type();
    if (!strcmp(mime, "audio/mpeg") || !strcmp(mime, "audio/aacp")) {
        fDataFuncs.Add(&StreamIO::DataUnsyncedReceived);
    } 
	
    fDataFuncs.Add(&StreamIO::DataSyncedReceived);
    delete newUrl;
}

StreamIO::~StreamIO() {
    if (fReqThread >= 0) {
        fReq->Stop();
        status_t status;
        wait_for_thread(fReqThread, &status);
    }
    delete fReq;
}

void
StreamIO::GetFlags(int32* flags) const {
	*flags = B_MEDIA_STREAMING | B_MEDIA_SEEK_BACKWARD | B_MEDIA_MUTABLE_SIZE;
}

ssize_t
StreamIO::WriteAt(off_t position, const void* buffer, size_t size) {
    return B_NOT_SUPPORTED;
}

ssize_t
StreamIO::ReadAt(off_t position, void* buffer, size_t size) {
	if (fLimit == 0 || position < fLimit) {
		ssize_t read = BAdapterIO::ReadAt(position, buffer, size);
		if (read > 0) {
			TRACE(B_TRANSLATE("Read %") B_PRIdSSIZE B_TRANSLATE(" of %") B_PRIuSIZE 
					B_TRANSLATE(" bytes from position %") B_PRIdOFF 
					B_TRANSLATE(", %") B_PRIuSIZE B_TRANSLATE(" remaining\n"), 
					read, size, position, Buffered());
			fBuffered -= read;
		} else
			TRACE(B_TRANSLATE("Reading %") B_PRIuSIZE B_TRANSLATE(" bytes from position %") B_PRIdOFF 
					B_TRANSLATE(" failed - %s\n"), 
					size, position, strerror(read));
		
		return read;
	} else {
		TRACE(B_TRANSLATE("Position %") B_PRIuSIZE B_TRANSLATE(" has reached limit of %") B_PRIuSIZE 
				B_TRANSLATE(", blocking...\n"), 
				position, fLimit);
		return 0;
	}
}

status_t
StreamIO::SetSize(off_t size) {
    return B_NOT_SUPPORTED;
}

size_t
StreamIO::Buffered() {
	return fBuffered;
}

void 
StreamIO::SetLimiter(size_t limit) {
	fLimit = limit;
}

status_t
StreamIO::Open() {
    fReqThread = fReq->Run();
    if (fReqThread < B_OK) 
        return B_ERROR;

    fReqStartTime = system_time() + HTTP_TIMEOUT;
    return BAdapterIO::Open();
}

bool
StreamIO::IsRunning() const {
    return fReqThread >= 0;
}

status_t
StreamIO::SeekRequested(off_t position) {
    return BAdapterIO::SeekRequested(position);
}

void
StreamIO::HeadersReceived(BUrlRequest* request, const BUrlResult& result) {
    BHttpRequest* httpReq = dynamic_cast<BHttpRequest*>(request);
    const BHttpResult* httpResult = dynamic_cast<const BHttpResult*>(&request->Result());

    if (httpReq->IsServerErrorStatusCode(httpResult->StatusCode())) {
        request->Stop();
        fReqThread = -1;
        return;
    }
	
	if (httpReq->IsRedirectionStatusCode(httpResult->StatusCode())) {
		if (httpResult->Length() > 0) {
			fDataFuncs.Add(&StreamIO::DataRedirectReceived, 0);
		}
		if (httpResult->StatusCode() == 301) { // Permanent redirect
			fStation->SetStreamUrl(httpResult->Headers()["location"]);
			TRACE(B_TRANSLATE("Permanently redirected to %s\n"), httpResult->Headers()["location"]);
		} else 
			TRACE(B_TRANSLATE("Redirected to %s\n"), httpResult->Headers()["location"]);
		return;
	}
	
	if (httpReq->IsSuccessStatusCode(httpResult->StatusCode()))
		httpReq->SetDiscardData(false);
	
    off_t size = result.Length();
    if (size > 0)
        BAdapterIO::SetSize(size);
    else {
        fIsMutable = true;
    }

    const char* sMetaInt = httpResult->Headers()["icy-metaint"];
    icyName = httpResult->Headers()["icy-name"];
    if (sMetaInt) {
        fMetaInt = atoi(sMetaInt);
        fUntilMetaStart = fMetaInt;
        fDataFuncs.Add(&StreamIO::DataWithMetaReceived, 0);
    }
}

void
StreamIO::DataSyncedReceived(BUrlRequest* request, const char* data, off_t position, ssize_t size, int next) {
	fInputAdapter->Write(data, size);
	fBuffered += size;
}

void
StreamIO::DataRedirectReceived(BUrlRequest* request, const char* data, off_t position, ssize_t size, int next) {
	fDataFuncs.Remove(next - 1);
}

void 
StreamIO::DataWithMetaReceived(BUrlRequest* request, const char* data, off_t position, ssize_t size, int next) {
	while (size) {
		if (fUntilMetaEnd != 0) {
			if (fUntilMetaEnd <= size) {
				memcpy(fMetaBuffer + fMetaSize, (void*)data, fUntilMetaEnd);
				data += fUntilMetaEnd;
				size -= fUntilMetaEnd;
				fMetaSize += fUntilMetaEnd;
				fMetaBuffer[fMetaSize] = 0;

				// ProcessMeta;
				ProcessMeta();
				fMetaSize = 0;

				fUntilMetaEnd = 0;
				fUntilMetaStart = fMetaInt;
			} else {
				memcpy(fMetaBuffer + fMetaSize, (void*)data, size);
				fMetaSize += size;
				data += size;
				fUntilMetaEnd -= size;
				size = 0;
			}
		} else {
			DataFunc nextFunc = fDataFuncs.Item(next);
			if (size <= fUntilMetaStart) {
				(*this.*nextFunc)(request, data, position, size, next + 1);
				fUntilMetaStart -= size;
				size = 0;
			} else {
				(*this.*nextFunc)(request, data, position, fUntilMetaStart, next + 1);
				size -= fUntilMetaStart;
				position += fUntilMetaStart;
				data += fUntilMetaStart;
				fUntilMetaStart = 0;
				fUntilMetaEnd = *data * 16;
				if (fUntilMetaEnd == 0) {
					fUntilMetaStart = fMetaInt;
					TRACE(B_TRANSLATE("Meta: Empty\n"));
				} else if (fUntilMetaEnd > 512) {
					TRACE(B_TRANSLATE("Meta: Size of %") B_PRIdOFF B_TRANSLATE(" too large\n"), fUntilMetaEnd);
					fUntilMetaStart = fMetaInt;
					fUntilMetaEnd = 0;
					data--;
					size++;
				}
				data++;
				size--;
			}
		}
	}
}
	
const char mpegHeader1 = '\xff';
const char mpegHeader2 = '\xe0';

void 
StreamIO::DataUnsyncedReceived(BUrlRequest* request, const char* data, off_t position, ssize_t size, int next) {
	off_t frameStart;
	for (frameStart = 0; frameStart < size - 1; frameStart++) {
		if (fFrameSync == none) {
			if (data[frameStart] == mpegHeader1) {
				fFrameSync = first;
				continue;
			}
		} else if ((data[frameStart] & mpegHeader2) == mpegHeader2) {
			next--;
			fDataFuncs.Remove(next);
			DataFunc nextFunc = fDataFuncs.Item(next);
			
			(*this.*nextFunc)(request, &mpegHeader1, position + frameStart - 1, 1, next + 1);
			(*this.*nextFunc)(request, data + frameStart, position + frameStart, size - frameStart, next + 1);
			return;
		} else 
			fFrameSync = none;
	}
	
	if (position + frameStart > 8192) {
		MSG("No mp3 frame header encountered in first %" B_PRIuSIZE 
				" bytes, giving up...", 
				position + frameStart);
		next--;
		fDataFuncs.Remove(next);
		DataFunc nextFunc = fDataFuncs.Item(next);
	}
}

void
StreamIO::DataReceived(BUrlRequest* request, const char* data, off_t position, ssize_t size) {
	DataFunc f = fDataFuncs.First();
	if (f)
		(*this.*f)(request, data, position, size, 1);
}

void
StreamIO::RequestCompleted(BUrlRequest* request, bool success) {
	fReqThread = -1;
}

//void
//StreamIO::DebugMessage(BUrlRequest* caller, BUrlProtocolDebugMessage type, const char* text) {
//	MSG("Debug: %s\n", text);
//}

void
StreamIO::ProcessMeta() {
	TRACE(B_TRANSLATE("Meta: %s\n"), fMetaBuffer);

	if (!fMetaListener) return;
	
	regmatch_t matches[4];
	regex_t	   regex;
	
	BMessage* msg = new BMessage(MSG_META_CHANGE);
	msg->AddString("station", fStation->Name()->String());
	int res = regcomp(&regex,  
			"([^=]*)='(([^']|'[^;])*)';",
			RE_ICASE | RE_DOT_NEWLINE | RE_DOT_NOT_NULL | RE_NO_BK_PARENS | RE_NO_BK_VBAR | 
			RE_BACKSLASH_ESCAPE_IN_LISTS);
	char* text = fMetaBuffer;
	while ((res = regexec(&regex, text, 3, matches, 0)) == 0) {
		text[matches[1].rm_eo] = 0;
		text[matches[2].rm_eo] = 0;
		if (text + matches[2].rm_so && !(text + matches[2].rm_so)[0]) {
			msg->AddString(strlwr(text + matches[1].rm_so), icyName);
		}
		else { 
			msg->AddString(strlwr(text + matches[1].rm_so), text + matches[2].rm_so);
		}
		text += matches[0].rm_eo;
	}
	fMetaListener->PostMessage(msg);
}
