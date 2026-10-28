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


#include "StationListView.h"

#include <Application.h>
#include <IconUtils.h>
#include <Resources.h>
#include <TranslationUtils.h>

#include "StreamPlayer.h"


#define SLV_INSET 2
#define SLV_PADDING 6


StationListViewItem::StationListViewItem(Station* station)
	: BListItem(0, true),
	  fPlayer(NULL),
	  fStation(station),
	  fList(NULL)
{
	BFont font(be_plain_font);
	fItemHeight = ceilf(font.Size() * 4);

	SetHeight(fItemHeight);
}


StationListViewItem::~StationListViewItem()
{
	if (fPlayer != NULL) {
		fPlayer->Stop();
		delete fPlayer;
	}
	delete fStation;
}


StreamPlayer::PlayState
StationListViewItem::State()
{
	return (fPlayer != NULL)					? fPlayer->State()
		   : fStation->Flags(STATION_URI_VALID) ? StreamPlayer::Stopped
												: StreamPlayer::InActive;
}


void
StationListViewItem::DrawItem(BView* owner, BRect frame, bool complete)
{
	int index = ((BListView*)owner)->IndexOf(this);

	StationListView* ownerList = (StationListView*)owner;
	ownerList->SetHighColor(ui_color(
		IsSelected() ? B_MENU_SELECTION_BACKGROUND_COLOR
					 : ((index % 2) ? B_MENU_BACKGROUND_COLOR : B_DOCUMENT_BACKGROUND_COLOR)));
	ownerList->FillRect(frame);

	owner->SetHighColor(
		ui_color(IsSelected() ? B_MENU_SELECTED_ITEM_TEXT_COLOR : B_MENU_ITEM_TEXT_COLOR));
	owner->SetLowColor(ui_color(
		IsSelected() ? B_MENU_SELECTION_BACKGROUND_COLOR
					 : ((index % 2) ? B_MENU_BACKGROUND_COLOR : B_DOCUMENT_BACKGROUND_COLOR)));

	BRect target(SLV_INSET, SLV_INSET, fItemHeight - 2 * SLV_INSET, fItemHeight - 2 * SLV_INSET);
	target.OffsetBy(frame.LeftTop());

	if (fStation->Logo() != NULL) {
		owner->DrawBitmap(
			fStation->Logo(), fStation->Logo()->Bounds(), target, B_FILTER_BITMAP_BILINEAR);
	}

	BFont font(be_plain_font);
	font_height fontHeight;
	font.GetHeight(&fontHeight);
	float baseline = SLV_INSET + fontHeight.ascent + fontHeight.leading;

	owner->SetFontSize(font.Size());
	owner->DrawString(fStation->Name()->String(),
		frame.LeftTop() + BPoint(fItemHeight - SLV_INSET + SLV_PADDING, SLV_INSET + baseline));

	owner->SetFontSize(ceilf(font.Size() * 0.8));
	owner->GetFontHeight(&fontHeight);
	baseline += fontHeight.ascent + fontHeight.descent + fontHeight.leading;
	owner->DrawString(fStation->Source().UrlString(),
		frame.LeftTop() + BPoint(fItemHeight - SLV_INSET + SLV_PADDING, baseline));
	baseline += fontHeight.ascent + fontHeight.descent + fontHeight.leading;

	owner->DrawString(
		BString("") << fStation->BitRate() / 1000.0 << " kbps " << fStation->Mime()->Type(),
		frame.LeftTop() + BPoint(fItemHeight - SLV_INSET + SLV_PADDING, baseline));

	if (ownerList->CanPlay() && fStation->Flags(STATION_URI_VALID)) {
		BBitmap* bnBitmap = _GetButtonBitmap(ownerList, State());
		if (bnBitmap != NULL) {
			owner->PushState();
			owner->SetDrawingMode(B_OP_OVER);
			owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			owner->DrawBitmap(bnBitmap, bnBitmap->Bounds(), target);
			owner->PopState();

		}
	}
}


void
StationListViewItem::DrawBufferFillBar(BView* owner, BRect frame)
{
	if (State() == StreamPlayer::Playing || State() == StreamPlayer::Buffering) {
		frame.bottom -= SLV_INSET;
		frame.left = fItemHeight - SLV_INSET + SLV_PADDING;
		frame.top = frame.bottom - 5;
		frame.right -= SLV_PADDING;

		owner->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
		owner->FillRect(frame);
		owner->SetHighColor(200, 30, 0);

		frame.right = frame.left + frame.Width() * fFillRatio;
		owner->FillRect(frame);
	}
}


void
StationListViewItem::Update(BView* owner, const BFont* font)
{
	SetHeight(fItemHeight);
}


void
StationListViewItem::StateChanged(StreamPlayer::PlayState newState)
{
	if (fList != NULL && fList->LockLooperWithTimeout(0) == B_OK) {
		fList->InvalidateItem(fList->IndexOf(this));
		fList->UnlockLooper();
	}
}


BBitmap*
StationListViewItem::_GetButtonBitmap(StationListView* owner, StreamPlayer::PlayState state)
{
	if (state < 0)
		return NULL;

	return owner->fStateOverlay[state];
}


void
StationListViewItem::SetFillRatio(float fillRatio)
{
	fFillRatio = fillRatio;
	if (fList->LockLooperWithTimeout(0) == B_OK) {
		BRect frame = fList->ItemFrame(fList->IndexOf(this));
		DrawBufferFillBar(fList, frame);
		fList->UnlockLooper();
	}
}


StationListView::StationListView(bool canPlay)
	: BListView("Stations", B_SINGLE_SELECTION_LIST),
	  fPlayMsg(NULL),
	  fCanPlay(canPlay)
{
	SetResizingMode(B_FOLLOW_ALL_SIDES);
	BFont font(be_plain_font);
	float height = ceilf(font.Size() * 4);
	SetExplicitMinSize(BSize(B_SIZE_UNSET, 3 * height));

	// cache stop/play/buffering state icons
	size_t size;
	for (uint32 i = 0; i < StreamPlayer::COUNT; i++) {
		const void* buf = be_app->AppResources()->LoadResource(B_VECTOR_ICON_TYPE, i + 1, &size);
		if (buf != NULL) {
			fStateOverlay[i] = new BBitmap(BRect(0, 0, height * 4, height * 4), B_RGBA32);
			BIconUtils::GetVectorIcon((const uint8*)buf, size, fStateOverlay[i]);
		}
	}
}


StationListView::~StationListView()
{
	for (uint32 i = 0; i < StreamPlayer::COUNT; i++)
		delete fStateOverlay[i];

	delete fPlayMsg;
	MakeEmpty();
}


bool
StationListView::AddItem(StationListViewItem* item)
{
	item->fList = this;
	return BListView::AddItem(item);
}


bool
StationListView::AddItem(Station* station)
{
	return AddItem(new StationListViewItem(station));
}


void
StationListView::MakeEmpty()
{
	for (int32 i = CountItems() - 1; i >= 0; i--) {
		StationListViewItem* stationItem = (StationListViewItem*)ItemAt(i);
		RemoveItem(i);
		delete stationItem;
	}
	BListView::MakeEmpty();
}


int32
StationListView::StationIndex(Station* station)
{
	for (int32 i = 0; i < CountItems(); i++) {
		if (((StationListViewItem*)ItemAt(i))->GetStation() == station)
			return i;
	}

	return -1;
}


StationListViewItem*
StationListView::ItemAt(int32 index)
{
	return (StationListViewItem*)BListView::ItemAt(index);
}


Station*
StationListView::StationAt(int32 index)
{
	StationListViewItem* item = ItemAt(index);
	return (item != NULL) ? item->GetStation() : NULL;
}


StationListViewItem*
StationListView::Item(Station* station)
{
	for (int32 i = 0; i < CountItems(); i++) {
		StationListViewItem* stationItem = (StationListViewItem*)ItemAt(i);
		if (stationItem->GetStation() == station)
			return stationItem;
	}

	return NULL;
}


void
StationListView::SelectItem(StationListViewItem* item)
{
	int32 index = IndexOf(item);
	Select(index);
	ScrollToSelection();
}


void
StationListView::Sync(StationsList* stations)
{
	LockLooper();

	for (int32 i = CountItems() - 1; i >= 0; i--) {
		StationListViewItem* stationItem = (StationListViewItem*)ItemAt(i);
		if (!stations->HasItem(stationItem->GetStation())) {
			RemoveItem(i);
			delete stationItem;
		}
	}

	for (int32 i = 0; i < stations->CountItems(); i++) {
		Station* station = stations->ItemAt(i);
		if (Item(station) == NULL)
			AddItem(station);
	}

	Invalidate();

	UnlockLooper();
}


void
StationListView::SetPlayMessage(BMessage* fPlayMsg)
{
	delete this->fPlayMsg;
	this->fPlayMsg = fPlayMsg;
}


void
StationListView::MouseDown(BPoint where)
{
	fWhereDown = where;
	BListView::MouseDown(where);
}


void
StationListView::MouseUp(BPoint where)
{
	fWhereDown -= where;
	if (fWhereDown.x * fWhereDown.x + fWhereDown.y * fWhereDown.y < 5 && fPlayMsg != NULL) {
		int stationIndex = IndexOf(where);
		if (stationIndex >= 0) {
			BRect playFrame = ItemFrame(stationIndex);
			float height = ItemAt(stationIndex)->Height();
			playFrame.InsetBy(SLV_INSET, SLV_INSET);
			playFrame.right = playFrame.left + height / 2;
			playFrame.top = playFrame.bottom - height / 2;

			if (playFrame.Contains(where)) {
				BMessage* clone = new BMessage(fPlayMsg->what);
				clone->AddInt32("index", stationIndex);
				InvokeNotify(clone);
			}
		}
	}

	fWhereDown = BPoint(-1, -1);
	BListView::MouseUp(where);
}
