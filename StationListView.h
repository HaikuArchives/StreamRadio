/*
 * File:   StationListView.h
 * Author: user
 *
 * Created on 26. Februar 2013, 09:22
 */

#ifndef STATIONLISTVIEW_H
#define STATIONLISTVIEW_H
#include <Button.h>
#include <ListItem.h>
#include <ListView.h>
#include <Window.h>

#include "RadioSettings.h"
#include "Station.h"
#include "StreamPlayer.h"

#define MSG_STATION_LIST 'STLS'

class StationListView;

class StationListViewItem : public BListItem
{
	friend class StationListView;

public:
	StationListViewItem(Station* station);
	virtual ~StationListViewItem();
	virtual void DrawItem(BView* owner, BRect frame, bool complete);
	void DrawBufferFillBar(BView* owner, BRect frame);
	virtual void Update(BView* owner, const BFont* font);
	Station* GetStation() { return fStation; }
	void StateChanged(StreamPlayer::PlayState newState);
	void SetFillRatio(float fillRatio);
	StreamPlayer* Player() { return fPlayer; }
	StreamPlayer::PlayState State();
	void SetPlayer(StreamPlayer* player) { fPlayer = player; }

private:
	StationListView* fList;
	class Station* fStation;
	StreamPlayer* fPlayer;
	float fFillRatio;
	static BBitmap* getButtonBitmap(StreamPlayer::PlayState state);
	static BBitmap* buttonBitmap[3];
};

class StationListView : public BListView
{
public:
	StationListView(bool canPlay = false);
	virtual ~StationListView();
	void Sync(StationsList* stations);
	virtual bool AddItem(Station* station);
	virtual bool AddItem(StationListViewItem* item);
	int32 StationIndex(Station* station);
	StationListViewItem* Item(Station* station);
	StationListViewItem* ItemAt(int32 index);
	Station* StationAt(int32 index);
	void SetPlayMessage(BMessage* playMsg);
	bool CanPlay() { return fCanPlay; }

private:
	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	BPoint whereDown;
	BMessage* playMsg;
	bool fCanPlay;
};


#endif /* STATIONLISTVIEW_H */
