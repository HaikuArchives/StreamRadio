/*
 * Copyright (C) 2017 Kai Niessen <kai.niessen@online.de>
 * Copyright 2023 Haiku, Inc. All rights reserved.
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


#include "StreamIO.h"

#define __USE_GNU
#include <regex.h>

#include <Catalog.h>
#include <MediaIO.h>
#include <NetworkAddressResolver.h>
#include <Socket.h>
#include <Url.h>

#include "Debug.h"
#include "HttpUtils.h"
#include "Station.h"


const char kMpegHeader1 = '\xff';
const char kMpegHeader2 = '\xe0';

#define HTTP_TIMEOUT 30000000


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StreamIO"


StreamIO::StreamIO(Station* station, BLooper* metaListener)
	:
	BAdapterIO(B_MEDIA_STREAMING | B_MEDIA_SEEKABLE, HTTP_TIMEOUT),
	fStation(station),
	fReq(NULL),
	fReqThread(-1),
	fUnsynched(0),
	fMetaInt(0),
	fMetaSize(0),
	fUntilMetaStart(0),
	fUntilMetaEnd(0),
	fMetaListener(metaListener),
	fFrameSync(none),
	fLimit(0),
	fBuffered(0)
{
	BUrl url = station->StreamUrl();

	// FIXME: UGLY HACK!
	// Currently Haiku's HttpRequest is not able to check for connection on a
	// specified IP address/port pair before actually resolving the hostname to
	// that pair. That means, if the first server in the DNS response is down or
	// has its port closed for some reason, the request will simply fail.
	//
	// Creating a socket and trying to connect to the specified IP/port pair is
	// enough for our purposes. That way, if a server is down we can use the
	// next DNS record, and craft a new URL like
	// http://192.168.0.1/path/to/stream
	//
	// On HTTPS, unfortunately, this isn't possible, as the certificate is
	// associated to the hostname, and so we have to make the request using it.
	// The solution to this would be to fix the BHttpRequest class, but, for the
	// moment, we'll just use the hostname directly if HTTPS is used. The number
	// of streams using HTTPS and load balancing between two or more different
	// IP's should be small, anyway.

	if (url.Protocol() == "https") {
		fReq = dynamic_cast<BHttpRequest*>(BUrlProtocolRoster::MakeRequest(
			url.UrlString().String(), this, this));
	} else {
		BUrl* newUrl = new BUrl();
		if (newUrl == NULL)
			return;

		status_t portStatus = HttpUtils::CheckPort(url, newUrl);
		if (portStatus != B_OK)
			return;

		fReq = dynamic_cast<BHttpRequest*>(BUrlProtocolRoster::MakeRequest(
			newUrl->UrlString().String(), this, this));
		delete newUrl;
	}

	if (fReq == NULL)
		return;

	BHttpHeaders* headers = new BHttpHeaders();
	if (headers == NULL)
		return;

	headers->AddHeader("Icy-MetaData", 1);
	headers->AddHeader("Icy-Reset", 1);
	headers->AddHeader("Accept", "audio/*");

	fReq->AdoptHeaders(headers);
	fReq->SetFollowLocation(true);
	fReq->SetMaxRedirections(3);
	fReq->SetStopOnError(true);

	fInputAdapter = BuildInputAdapter();

	const char* mime = fStation->Mime()->Type();
	if (!strcmp(mime, "audio/mpeg") || !strcmp(mime, "audio/aacp"))
		fDataFuncs.Add(&StreamIO::_DataUnsyncedReceived);

	fDataFuncs.Add(&StreamIO::_DataSyncedReceived);
}


StreamIO::~StreamIO()
{
	if (fReqThread >= 0) {
		fReq->Stop();
		status_t status;
		wait_for_thread(fReqThread, &status);
	}

	delete fReq;
}


void
StreamIO::GetFlags(int32* flags) const
{
	*flags = B_MEDIA_STREAMING | B_MEDIA_SEEK_BACKWARD | B_MEDIA_MUTABLE_SIZE;
}


ssize_t
StreamIO::WriteAt(off_t position, const void* buffer, size_t size)
{
	return B_NOT_SUPPORTED;
}


ssize_t
StreamIO::ReadAt(off_t position, void* buffer, size_t size)
{
	if (fLimit == 0 || position < fLimit) {
		ssize_t read = BAdapterIO::ReadAt(position, buffer, size);
		if (read > 0) {
			TRACE("Read %" B_PRIdSSIZE " of %" B_PRIuSIZE
				  " bytes from position %" B_PRIdOFF ", %" B_PRIuSIZE
				  " remaining\n",
				read, size, position, fBuffered);
			fBuffered -= read;
		} else {
			TRACE("Reading %" B_PRIuSIZE " bytes from position %" B_PRIdOFF
				  " failed - %s\n",
				size, position, strerror(read));
		}

		return read;
	} else {
		TRACE("Position %" B_PRIdOFF " has reached limit of %" B_PRIuSIZE
			  ", blocking...\n",
			position, fLimit);

		return 0;
	}
}


status_t
StreamIO::SetSize(off_t size)
{
	return B_NOT_SUPPORTED;
}


status_t
StreamIO::Open()
{
	if (fReq == NULL)
		return B_ERROR;

	fReqThread = fReq->Run();
	if (fReqThread < B_OK)
		return B_ERROR;

	return BAdapterIO::Open();
}


bool
StreamIO::IsRunning() const
{
	return fReqThread >= 0;
}


void
StreamIO::SetLimiter(size_t limit)
{
	fLimit = limit;
}


void
StreamIO::HeadersReceived(BUrlRequest* request)
{
	BHttpRequest* httpReq = dynamic_cast<BHttpRequest*>(request);
	const BHttpResult* httpResult
		= dynamic_cast<const BHttpResult*>(&request->Result());

	if (httpReq->IsRedirectionStatusCode(httpResult->StatusCode())) {
		if (httpResult->StatusCode() == 301) { // Permanent redirect
			fStation->SetStreamUrl(httpResult->Headers()["location"]);
			TRACE("Permanently redirected to %s\n",
				httpResult->Headers()["location"]);
		} else
			TRACE("Redirected to %s\n", httpResult->Headers()["location"]);

		return;
	}

	off_t size = httpResult->Length();
	if (size > 0)
		BAdapterIO::SetSize(size);
	else
		fIsMutable = true;

	const char* sMetaInt = httpResult->Headers()["icy-metaint"];
	fIcyName = httpResult->Headers()["icy-name"];
	if (sMetaInt != NULL) {
		fMetaInt = atoi(sMetaInt);
		fUntilMetaStart = fMetaInt;
		fDataFuncs.Add(&StreamIO::_DataWithMetaReceived, 0);
	}
}


ssize_t
StreamIO::_DataSyncedReceived(const char* data, size_t size, int next)
{
	ssize_t written = fInputAdapter->Write(data, size);
	fBuffered += written;
	return written;
}


ssize_t
StreamIO::_DataWithMetaReceived(const char* data, size_t size, int next)
{
	ssize_t written = 0;

	while (size > 0) {
		if (fUntilMetaEnd != 0) {
			// We are reading metadata
			if (fUntilMetaEnd <= size) {
				// The metadata ends before the buffer
				memcpy(fMetaBuffer + fMetaSize, (void*)data, fUntilMetaEnd);

				written += fUntilMetaEnd;
				data += fUntilMetaEnd;
				size -= fUntilMetaEnd;
				fMetaSize += fUntilMetaEnd;
				fMetaBuffer[fMetaSize] = 0;

				_ProcessMeta();
				fMetaSize = 0;

				fUntilMetaEnd = 0;
				fUntilMetaStart = fMetaInt;
			} else {
				memcpy(fMetaBuffer + fMetaSize, (void*) data, size);

				written += size;
				fMetaSize += size;
				data += size;
				fUntilMetaEnd -= size;
				size = 0;
			}
		} else {
			// No metadata right now, feed content to consumer
			DataFunc nextFunc = fDataFuncs.Item(next);
			if (size <= fUntilMetaStart) {
				written += (*this.*nextFunc)(data, size, next + 1);

				fUntilMetaStart -= size;
				size = 0;
			} else {
				// Metadata starts before the end of the buffer
				written += (*this.*nextFunc)(data, fUntilMetaStart, next + 1);
				size -= fUntilMetaStart;
				data += fUntilMetaStart;

				fUntilMetaStart = 0;
				fUntilMetaEnd = *data * 16;
				if (fUntilMetaEnd == 0) {
					fUntilMetaStart = fMetaInt;
					TRACE("Meta: Empty\n");
				} else if (fUntilMetaEnd > 512) {
					TRACE("Meta: Size of %" B_PRIdOFF " too large\n",
						fUntilMetaEnd);

					fUntilMetaStart = fMetaInt;
					fUntilMetaEnd = 0;
					data--;
					size++;
					written--;
				}

				data++;
				size--;
				written++;
			}
		}
	}

	return written;
}


ssize_t
StreamIO::_DataUnsyncedReceived(const char* data,
	size_t size, int next)
{
	off_t frameStart;
	for (frameStart = 0; frameStart < size; frameStart++) {
		if (fFrameSync == none) {
			if (data[frameStart] == kMpegHeader1) {
				fFrameSync = first;
				continue;
			}
		} else if ((data[frameStart] & kMpegHeader2) == kMpegHeader2) {
			next--;
			fDataFuncs.Remove(next);
			DataFunc nextFunc = fDataFuncs.Item(next);

			(*this.*nextFunc)(&kMpegHeader1, 1, next + 1);
			return frameStart + (*this.*nextFunc)(data + frameStart,
				size - frameStart, next + 1);
		} else
			fFrameSync = none;
	}

	fUnsynched += size;
	if (fUnsynched > 8192) {
		MSG("No mp3 frame header encountered in first %" B_PRIiOFF
			" bytes, giving up...",
			fUnsynched);

		next--;
		fDataFuncs.Remove(next);
	}

	return size;
}


ssize_t
StreamIO::Write(const void* buffer, size_t size)
{
	DataFunc f = fDataFuncs.First();
	if (f)
		return (*this.*f)((const char*)buffer, size, 1);
	return 0;
}


void
StreamIO::RequestCompleted(BUrlRequest* request, bool success)
{
	fReqThread = -1;
}


void
StreamIO::DebugMessage(BUrlRequest* caller, BUrlProtocolDebugMessage type,
	const char* text)
{
	DEBUG("Debug Message: %s\n", text);
}


void
StreamIO::_ProcessMeta()
{
	TRACE("Meta: %s\n", fMetaBuffer);

	if (fMetaListener == NULL)
		return;

	regmatch_t matches[4];
	regex_t regex;

	BMessage* msg = new BMessage(MSG_META_CHANGE);
	msg->AddString("station", fStation->Name()->String());

	int res = regcomp(&regex, "([^=]*)='(([^']|'[^;])*)';",
		RE_ICASE | RE_DOT_NEWLINE | RE_DOT_NOT_NULL | RE_NO_BK_PARENS
			| RE_NO_BK_VBAR | RE_BACKSLASH_ESCAPE_IN_LISTS);

	char* text = fMetaBuffer;
	while ((res = regexec(&regex, text, 3, matches, 0)) == 0) {
		text[matches[1].rm_eo] = 0;
		text[matches[2].rm_eo] = 0;

		if (text + matches[2].rm_so && !(text + matches[2].rm_so)[0])
			msg->AddString(strlwr(text + matches[1].rm_so), fIcyName);
		else
			msg->AddString(
				strlwr(text + matches[1].rm_so), text + matches[2].rm_so);

		text += matches[0].rm_eo;
	}

	fMetaListener->PostMessage(msg);
}
