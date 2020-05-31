/* 
 * File:   StationListView.cpp
 * Author: user
 * 
 * Created on 26.02.2013, 09:22
 */
#include <Application.h>
#include <Resources.h>
#include <TranslationUtils.h>
#include "StreamPlayer.h"
#include "StationListView.h"
#include "Utils.h"

#define SLV_INSET 2
#define SLV_PADDING 6
#define SLV_HEIGHT 50
#define SLV_MAIN_FONT_SIZE 13
#define SLV_SMALL_FONT_SIZE 10
#define SLV_BUTTON_SIZE 20

StationListViewItem::StationListViewItem(Station* station)
  : BListItem(0, true),
	fPlayer(NULL)
{
    fStation = station;
    SetHeight(SLV_HEIGHT);
}

StationListViewItem::~StationListViewItem() {
	if (fPlayer) {
		fPlayer->Stop();
		delete fPlayer;
	}
}

StreamPlayer::PlayState
StationListViewItem::State()  { 
	return (fPlayer) ? 
		fPlayer->State() 
	: 
		fStation->Flags(STATION_URI_VALID) ?
			StreamPlayer::Stopped
		:
			StreamPlayer::InActive;
}	

void StationListViewItem::DrawItem(BView* owner, BRect frame, bool complete) {
	int index = ((BListView*)owner)->IndexOf(this);
    StationListView* ownerList = (StationListView*)owner;
    ownerList->SetHighColor(ui_color(IsSelected() ? B_MENU_SELECTION_BACKGROUND_COLOR : 
		((index % 2) ? B_MENU_BACKGROUND_COLOR : B_DOCUMENT_BACKGROUND_COLOR)));
    ownerList->FillRect(frame);
    
    owner->SetHighColor(ui_color(IsSelected() ? B_MENU_SELECTED_ITEM_TEXT_COLOR : B_MENU_ITEM_TEXT_COLOR));
    owner->SetLowColor(ui_color(IsSelected() ? B_MENU_SELECTION_BACKGROUND_COLOR : ((index % 2) ? B_MENU_BACKGROUND_COLOR : B_DOCUMENT_BACKGROUND_COLOR)));
    if (BBitmap* logo = fStation->Logo()) {
        BRect target(SLV_INSET, SLV_INSET, SLV_HEIGHT - 2 * SLV_INSET, SLV_HEIGHT - 2 * SLV_INSET);
	target.OffsetBy(frame.LeftTop());
        owner->DrawBitmap(logo, logo->Bounds(), target, B_FILTER_BITMAP_BILINEAR);
    }
    owner->SetFontSize(SLV_MAIN_FONT_SIZE);
    font_height fontHeight;
    owner->GetFontHeight(&fontHeight);
    float baseline = SLV_INSET + fontHeight.ascent + fontHeight.leading;
    owner->DrawString(fStation->Name()->String(), frame.LeftTop() + BPoint(SLV_HEIGHT - SLV_INSET + SLV_PADDING, SLV_INSET + baseline));
    
    owner->SetFontSize(SLV_SMALL_FONT_SIZE);
    owner->GetFontHeight(&fontHeight);
    baseline += fontHeight.ascent + fontHeight.descent + fontHeight.leading;
    owner->DrawString(fStation->Source().UrlString(), frame.LeftTop() + BPoint(SLV_HEIGHT - SLV_INSET + SLV_PADDING, baseline));
    baseline += fontHeight.ascent + fontHeight.descent + fontHeight.leading;

    owner->DrawString(BString("") << fStation->BitRate()/1000.0 << " kbps " << fStation->Mime()->Type(), frame.LeftTop() + BPoint(SLV_HEIGHT - SLV_INSET + SLV_PADDING, baseline));
    
    frame = ownerList->ItemFrame(ownerList->StationIndex(fStation));
    if (ownerList->CanPlay() && fStation->Flags(STATION_URI_VALID)) {
        BBitmap* bnBitmap = getButtonBitmap(State());
        if (bnBitmap != NULL) {
            owner->SetDrawingMode(B_OP_ALPHA);
            owner->DrawBitmap(bnBitmap, frame.LeftTop()+BPoint(SLV_INSET, SLV_HEIGHT - SLV_INSET - SLV_BUTTON_SIZE));
            owner->SetDrawingMode(B_OP_COPY);
        }
    }
    
}

void StationListViewItem::DrawBufferFillBar(BView* owner, BRect frame) {
    if (State() == StreamPlayer::Playing || State() == StreamPlayer::Buffering) {
        frame.bottom -= SLV_INSET;
        frame.left    = SLV_HEIGHT - SLV_INSET + SLV_PADDING;
        frame.top     = frame.bottom - 5;
        frame.right  -= SLV_PADDING;
        owner->SetHighColor(tint_color(ui_color(B_MENU_BACKGROUND_COLOR), B_LIGHTEN_2_TINT));
        owner->FillRect(frame);
        owner->SetHighColor(200, 30, 0);
        frame.right = frame.left + frame.Width() * fFillRatio;
        owner->FillRect(frame);
    }
}

void StationListViewItem::Update(BView* owner, const BFont* font) {
    SetHeight(SLV_HEIGHT);
}

void 
StationListViewItem::StateChanged(StreamPlayer::PlayState newState) {
    if (fList && fList->LockLooperWithTimeout(0) == B_OK) {
        fList->InvalidateItem(fList->IndexOf(this));
        fList->UnlockLooper();
    }
}

BBitmap* 
StationListViewItem::getButtonBitmap(StreamPlayer::PlayState state) {
    if (state < 0) return NULL;
    if (buttonBitmap[state] == NULL) 
		buttonBitmap[state] = Utils::ResourceBitmap(RES_BN_STOPPED + state);
    return buttonBitmap[state];
}

BBitmap* StationListViewItem::buttonBitmap[3] = { NULL, NULL, NULL };

void 
StationListViewItem::SetFillRatio(float fillRatio) {
    fFillRatio = fillRatio;
    if (fList->LockLooperWithTimeout(0) == B_OK) {
        BRect frame = fList->ItemFrame(fList->IndexOf(this));
        DrawBufferFillBar(fList, frame);
        fList->UnlockLooper();
    }
}


StationListView::StationListView(bool canPlay) 
  : BListView("Stations", B_SINGLE_SELECTION_LIST),
	fCanPlay(canPlay)
{
    SetResizingMode(B_FOLLOW_ALL_SIDES);
    SetExplicitMinSize(BSize(300, 2 * SLV_HEIGHT));
    playMsg = NULL;
}

StationListView::~StationListView() {
    delete playMsg;
}

bool 
StationListView::AddItem(StationListViewItem* item) {
    item->fList = this;
    return BListView::AddItem(item);
}

bool
StationListView::AddItem(Station* station) {
    return AddItem(new StationListViewItem(station));
}

int32 StationListView::StationIndex(Station* station) {
    for (int32 i = 0; i < CountItems(); i++) {
        if (((StationListViewItem*)ItemAt(i))->GetStation() == station)
            return i;
    }
    return -1;
}

StationListViewItem* StationListView::ItemAt(int32 index) {
    return (StationListViewItem*)BListView::ItemAt(index);
}

Station* StationListView::StationAt(int32 index) {
    StationListViewItem* item = ItemAt(index);
    return (item) ? item->GetStation() : NULL;
}

StationListViewItem* StationListView::Item(Station* station) {
    for (int32 i = 0; i < CountItems(); i++) {
        StationListViewItem* stationItem = (StationListViewItem*)ItemAt(i);
        if (stationItem->GetStation() == station)
            return stationItem;
    }
    return NULL;
}

void StationListView::Sync(StationsList* stations) {
    LockLooper();
    for (int32 i = CountItems() -1; i >= 0; i--) {
        StationListViewItem* stationItem = (StationListViewItem*)ItemAt(i);
        if (!stations->HasItem(stationItem->GetStation())) {
            RemoveItem(i);
            delete stationItem;
        }
    }
    
    for (int32 i = 0; i < stations->CountItems(); i++) {
        Station* station = stations->ItemAt(i);
        if (Item(station) == NULL) {
            AddItem(station);
        }
    }
    Invalidate();
    UnlockLooper();
}

void StationListView::SetPlayMessage(BMessage* playMsg) {
    delete this->playMsg;
    this->playMsg = playMsg;
}

void StationListView::MouseDown(BPoint where) {
    whereDown = where;
    BListView::MouseDown(where);
}

void StationListView::MouseUp(BPoint where) {
    whereDown -= where;
    if (whereDown.x * whereDown.x + whereDown.y * whereDown.y < 5 && playMsg) {
        int stationIndex = IndexOf(where);
        BRect playFrame = ItemFrame(stationIndex);
        playFrame.InsetBy(SLV_INSET, SLV_INSET);
        playFrame.right = playFrame.left + SLV_BUTTON_SIZE;
        playFrame.top = playFrame.bottom - SLV_BUTTON_SIZE;
        if (playFrame.Contains(where)) {
            BMessage* clone = new BMessage(playMsg->what);
            clone->AddInt32("index", stationIndex);
            InvokeNotify(clone);
        }
    }
    whereDown = BPoint(-1, -1);
}


