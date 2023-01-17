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


#include "Utils.h"

#include <AppFileInfo.h>
#include <Application.h>
#include <Bitmap.h>
#include <Message.h>
#include <Resources.h>
#include <Roster.h>

#include <stdio.h>


static char sUserAgent[32] = {0};


BBitmap*
Utils::ResourceBitmap(int32 id)
{
	size_t size;
	const char* data
		= (const char*)be_app->AppResources()->FindResource('BBMP', id, &size);
	if (size != 0 && data != NULL) {
		BMessage msg;
		if (msg.Unflatten(data) == B_OK)
			return (BBitmap*)BBitmap::Instantiate(&msg);
	}

	return NULL;
}


const char*
Utils::UserAgent()
{
	if (sUserAgent[0] != 0)
		return sUserAgent;

	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	BFile file(&appInfo.ref, B_READ_ONLY);
	if (file.InitCheck() == B_OK) {
		BAppFileInfo appFileInfo(&file);
		struct version_info version;
		if (appFileInfo.GetVersionInfo(&version, B_APP_VERSION_KIND) == B_OK)
			snprintf(sUserAgent, sizeof(sUserAgent),
				"StreamRadio/%" B_PRIu32 ".%" B_PRIu32 ".%" B_PRIu32,
				version.major, version.middle, version.minor);
	}

	if (sUserAgent[0] == 0) {
		// There was an error trying to get the version info
		sprintf(sUserAgent, "StreamRadio");
	}

	return sUserAgent;
}
