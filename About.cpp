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
 * File:   About.cpp
 * Author: Kai Niessen <kai.niessen@online.de>
 * 
 * Created on March 20, 2017, 4:11 PM
 */

#include <Catalog.h>
#include <TranslationUtils.h>
#include <Bitmap.h>
#include <StringView.h>
#include <Application.h>
#include <Roster.h>
#include <AppFileInfo.h>
#include <Resources.h>
#include <Archivable.h>
#include "RadioApp.h"
#include "About.h"
#include "Utils.h"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "About"

About::About() 
  : BWindow(BRect(0, 0, 180, 50), B_TRANSLATE("About"), B_TITLED_WINDOW, B_NOT_ZOOMABLE | B_NOT_RESIZABLE) 
{
    BRect bounds = Bounds();
    BStringView* versionView = new BStringView(bounds, "versionView", GetAppVersion(), B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
    AddChild(versionView);
    versionView->SetDrawingMode(B_OP_COPY);
    versionView->SetViewColor(238, 238, 235, 255);
    versionView->SetHighColor(134, 135, 138, 255);
    versionView->SetLowColor(134, 135, 138, 0);
    BBitmap* banner = Utils::ResourceBitmap(RES_BANNER);
    if (banner) {
        bounds = banner->Bounds();
        ResizeTo(bounds.Width(), bounds.Height() + versionView->PreferredSize().height);
        versionView->SetFontSize(11);
        versionView->SetAlignment(B_ALIGN_RIGHT);
        versionView->ResizeTo(bounds.Width(), versionView->PreferredSize().height + 2);
        versionView->MoveTo(0, bounds.Height());
        BView* bannerView = new BView(bounds, "bannerView", B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP, B_WILL_DRAW);
        AddChild(bannerView, versionView);
        bannerView->SetViewBitmap(banner, B_FOLLOW_ALL_SIDES);
    } else {
        versionView->SetFontSize(40);
        versionView->SetAlignment(B_ALIGN_CENTER);
        BSize preferredSize = versionView->PreferredSize();
        ResizeTo(preferredSize.width + 10, preferredSize.height);
    }
    CenterOnScreen();
}

About::~About() {
}

BString About::GetAppVersion() {
    BString versionString;
    app_info appInfo;
    version_info versionInfo;
    BAppFileInfo appFileInfo;
    BFile appFile;
	
    if (be_app->GetAppInfo(&appInfo) == B_OK &&
            appFile.SetTo(&appInfo.ref, B_READ_ONLY) == B_OK &&
            appFile.InitCheck() == B_OK &&
            appFileInfo.SetTo(&appFile) == B_OK &&
            appFileInfo.GetVersionInfo(&versionInfo, B_APP_VERSION_KIND) == B_OK) {
        versionString.SetTo(versionInfo.long_info);
    } else {
		versionString = "©Fishpond 2012-2017";
    }
    return versionString;
}

