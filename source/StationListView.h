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
#ifndef _STATION_LIST_VIEW_H
#define _STATION_LIST_VIEW_H


#include <Button.h>
#include <ListItem.h>
#include <ListView.h>
#include <Window.h>

#include "RadioSettings.h"
#include "Station.h"
#include "StreamPlayer.h"


#define MSG_STATION_LIST 'STLS'


class StationListView;


class StationListViewItem : public BListItem {
	friend class StationListView;

public:
								StationListViewItem(Station* station);
	virtual						~StationListViewItem();

	virtual void				DrawItem(BView* owner, BRect frame,
									bool complete);

			void				DrawBufferFillBar(BView* owner, BRect frame);
	virtual void				Update(BView* owner, const BFont* font);
			Station*			GetStation() { return fStation; }
			void				ClearStation() { fStation = NULL; }
			void 				StateChanged(StreamPlayer::PlayState newState);
			void 				SetFillRatio(float fillRatio);

			StreamPlayer*		Player() { return fPlayer; }
			StreamPlayer::PlayState	State();
			void 				SetPlayer(StreamPlayer* player)
									{ fPlayer = player; }

private:
	static	BBitmap*			_GetButtonBitmap(StreamPlayer::PlayState state);

private:
	static	BBitmap*			sButtonBitmap[3];

			StreamPlayer*		fPlayer;
			class Station*		fStation;
			StationListView*	fList;

			float				fFillRatio;
};


class StationListView : public BListView {
public:
								StationListView(bool canPlay = false);
	virtual						~StationListView();

			void				Sync(StationsList* stations);

	virtual bool				AddItem(Station* station);
	virtual bool				AddItem(StationListViewItem* item);
	virtual void				MakeEmpty();

			int32				StationIndex(Station* station);
			StationListViewItem*	Item(Station* station);
			StationListViewItem*	ItemAt(int32 index);
			Station*			StationAt(int32 index);
			void				SelectItem(StationListViewItem* item);

			void				SetPlayMessage(BMessage* playMsg);
			bool				CanPlay() { return fCanPlay; }

private:
	virtual void				MouseDown(BPoint where);
	virtual void				MouseUp(BPoint where);

private:
			BPoint				fWhereDown;
			BMessage*			fPlayMsg;
			bool				fCanPlay;
};


#endif // _STATION_LIST_VIEW_H
