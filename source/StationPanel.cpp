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


#include "StationPanel.h"

#include <Catalog.h>
#include <Layout.h>
#include <LayoutBuilder.h>
#include <LayoutItem.h>
#include <TranslationUtils.h>

#include "Debug.h"
#include "MainWindow.h"
#include "Utils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "StationPanel"


StationPanel::StationPanel(MainWindow* mainWindow, bool expanded)
	: BView("stationpanel", B_WILL_DRAW), fStationItem(NULL), fMainWindow(mainWindow)
{
	SetResizingMode(B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_VERTICAL_UNSET));

	fLogo = new BView("logo", B_WILL_DRAW);
	fLogo->SetExplicitSize(BSize(96, 96));
	fLogo->SetViewColor(tint_color(ui_color(B_PANEL_BACKGROUND_COLOR), B_DARKEN_2_TINT));

	fName = new BTextControl("name", B_TRANSLATE("Name"), "-", new BMessage(MSG_CHG_NAME));

	fUrl = new BTextControl("url", B_TRANSLATE("Stream URL"), "-", new BMessage(MSG_CHG_STREAMURL));

	fGenre = new BTextControl("genre", B_TRANSLATE("Genre"), "-", new BMessage(MSG_CHG_GENRE));

	fStationUrl =
		new BTextControl("surl", B_TRANSLATE("Station URL"), "-", new BMessage(MSG_CHG_STATIONURL));

	fVisitStation = new BButton("bnVisitStation", "", new BMessage(MSG_VISIT_STATION));
	fVisitStation->SetIcon(Utils::ResourceBitmap(RES_BN_WEB));

	fVolume = new BSlider("volume", NULL, new BMessage(MSG_CHG_VOLUME), 0, 100, B_VERTICAL);
	fVolume->SetModificationMessage(new BMessage(MSG_CHG_VOLUME));

	BLayoutBuilder::Grid<>(this, B_USE_SMALL_SPACING, B_USE_SMALL_SPACING)
		.SetInsets(B_USE_SMALL_INSETS)
		.SetSpacing(B_USE_ITEM_SPACING, B_USE_SMALL_SPACING)
		.Add(fLogo, 0, 0, 1, 4)
		.AddTextControl(fName, 1, 0, B_ALIGN_LEFT)
		.AddTextControl(fGenre, 3, 0, B_ALIGN_RIGHT, 1, 2)
		.AddTextControl(fUrl, 1, 1, B_ALIGN_LEFT, 1, 4)
		.AddTextControl(fStationUrl, 1, 2, B_ALIGN_LEFT, 1, 3)
		.Add(fVisitStation, 5, 2)
		.Add(fVolume, 6, 0, 1, 3)
		.AddGlue(1, 3, 5, 1)
		.End();
}


StationPanel::~StationPanel() {}


void
StationPanel::AttachedToWindow()
{
	fLogo->SetNextHandler(this);
	fName->SetTarget(this);
	fGenre->SetTarget(this);
	fUrl->SetTarget(this);
	fStationUrl->SetTarget(this);
	fVisitStation->SetTarget(this);
	fVolume->SetTarget(this);

	SetStation(NULL);
}


void
StationPanel::SetStation(StationListViewItem* stationItem)
{
	if (stationItem == NULL) {
		fName->SetText(NULL);
		fName->SetEnabled(false);

		fGenre->SetText(NULL);
		fGenre->SetEnabled(false);

		fUrl->SetText(NULL);
		fUrl->SetEnabled(false);

		fStationUrl->SetText(NULL);
		fStationUrl->SetEnabled(false);

		fVisitStation->SetEnabled(false);

		fLogo->ClearViewBitmap();

		fVolume->SetValue(0);
		fVolume->SetEnabled(false);

		fStationItem = NULL;
	} else {
		if (stationItem->Player() != NULL &&
			stationItem->Player()->State() == StreamPlayer::Playing) {
			fVolume->SetEnabled(true);
			fVolume->SetValue(stationItem->Player()->Volume() * 100);
		} else
			fVolume->SetEnabled(false);

		fStationItem = stationItem;

		Station* station = stationItem->GetStation();
		if (station->Logo()) {
			fLogo->SetViewBitmap(station->Logo(), station->Logo()->Bounds(), fLogo->Bounds(), 0,
				B_FILTER_BITMAP_BILINEAR);
		} else
			fLogo->ClearViewBitmap();

		fName->SetEnabled(true);
		fName->SetText(station->Name()->String());

		fGenre->SetEnabled(true);
		fGenre->SetText(station->Genre().String());

		fUrl->SetEnabled(true);
		fUrl->SetText(station->StreamUrl().UrlString());

		fStationUrl->SetEnabled(true);
		fStationUrl->SetText(station->StationUrl().UrlString());

		fVisitStation->SetEnabled(true);
	}
}


void
StationPanel::StateChanged(StreamPlayer::PlayState newState)
{
	if (fStationItem != NULL && fStationItem->Player() != NULL &&
		fStationItem->Player()->State() == StreamPlayer::Playing) {
		fVolume->SetEnabled(true);
		fVolume->SetValue(fStationItem->Player()->Volume() * 100);
	} else {
		fVolume->SetEnabled(false);
		fVolume->SetValue(0);
	}
}


void
StationPanel::MessageReceived(BMessage* msg)
{
	Station* station = (fStationItem != NULL) ? fStationItem->GetStation() : NULL;
	if (station != NULL) {
		entry_ref ref;
		if (msg->WasDropped() && msg->IsSourceRemote() && msg->FindRef("refs", &ref) == B_OK) {
			BBitmap* bm = BTranslationUtils::GetBitmap(&ref);
			if (bm != NULL) {
				station->SetLogo(bm);
				fLogo->SetViewBitmap(station->Logo(), station->Logo()->Bounds(), fLogo->Bounds(), 0,
					B_FILTER_BITMAP_BILINEAR);
			}

			return;
		}

		switch (msg->what) {
			case MSG_CHG_VOLUME:
			{
				StreamPlayer* player = fStationItem->Player();
				if (player != NULL)
					player->SetVolume(fVolume->Value() / 100.0);

				break;
			}

			case MSG_CHG_NAME:
				station->SetName(fName->Text());
				break;

			case MSG_CHG_STREAMURL:
				station->SetStreamUrl(fUrl->Text());
				break;

			case MSG_CHG_GENRE:
				station->SetGenre(fGenre->Text());
				break;

			case MSG_CHG_STATIONURL:
				station->SetStation(fStationUrl->Text());
				break;

			case MSG_VISIT_STATION:
			{
				MSG("Trying to launch url %s\n", station->StationUrl().UrlString().String());

				if (station->StationUrl().UrlString().IsEmpty()) {
					BString search;
					search.SetToFormat("https://google.com/search?q=%s",
						BUrl::UrlEncode(*station->Name()).String());
					BUrl searchUrl(search);
					searchUrl.OpenWithPreferredApplication(false);
				} else
					station->StationUrl().OpenWithPreferredApplication(false);

				break;
			}

			default:
				BView::MessageReceived(msg);
		}
	} else
		BView::MessageReceived(msg);
}
