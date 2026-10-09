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
#ifndef _HTTP_UTILS_H
#define _HTTP_UTILS_H


#include <DataIO.h>
#include <HttpRequest.h>
#include <Socket.h>
#include <StringList.h>
#include <Url.h>
#include <ObjectList.h>

using namespace BPrivate::Network;

#define HTTP_DEFAULT_TIMEOUT 3000

class HttpUtils {
public:
	static BString			jsonType;
	static BMallocIO* 		GetAll(BUrl* url, BHttpHeaders* responseHeaders = NULL, 
								    bigtime_t timeOut = HTTP_DEFAULT_TIMEOUT,
								    BString* contentType = NULL, size_t sizeLimit = 0);
	static BMessage*		GetMsgFromREST(BUrl* url, BHttpHeaders* responseHeaders = NULL, 
									bigtime_t timeOut = HTTP_DEFAULT_TIMEOUT);
	static BStringList* 	GetStringsFromREST(BUrl* url, BString* path, 
									BHttpHeaders* responseHeaders = NULL, 
									bigtime_t timeout = HTTP_DEFAULT_TIMEOUT); 
	static const char*		UserAgent();
private:
	static char				sUserAgent[50];
};


#endif	// _HTTP_UTILS_H
