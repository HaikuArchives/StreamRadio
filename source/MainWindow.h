/*
	Copyright (C) 2008-2010  Lukas Sommer < SommerLuk at gmail dot com >

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License as
	published by the Free Software Foundation; either version 2 of
	the License or (at your option) version 3 or any later version
	accepted by the membership of KDE e.V. (or its successor approved
	by the membership of KDE e.V.), which shall act as a proxy
	defined in Section 14 of version 3 of the license.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H


#include <Menu.h>
#include <MenuBar.h>
#include <Message.h>
#include <ObjectList.h>
#include <StringView.h>
#include <Window.h>

#include "RadioSettings.h"
#include "StationFinder.h"
#include "StationListView.h"
#include "StationPanel.h"
#include "StreamPlayer.h"


#define MSG_REFS_RECEIVED 'REFS'
#define MSG_PASTE_URL 'PURL'
#define MSG_SEARCH 'mSRC'
#define MSG_CHECK 'mCKS'
#define MSG_REMOVE 'mRMS'
#define MSG_INVOKE_STATION 'mIST'
#define MSG_HELP 'HELP'
#define MSG_PARALLEL_PLAYBACK 'mPAR'


class MainWindow : public BWindow {
public:
	MainWindow();
	virtual ~MainWindow();

	virtual void MessageReceived(BMessage* message);
	virtual bool QuitRequested();

	virtual void SetVisible(bool visible);

private:
	void _Invoke(StationListViewItem* stationItem);
	void _TogglePlay(StationListViewItem* stationItem);

private:
	RadioSettings* fSettings;
	BMenuBar* fMainMenu;
	StationListView* fStationList;
	StationFinderWindow* fStationFinder;
	StationPanel* fStationPanel;
	BStringView* fStatusBar;
	BObjectList<StationListViewItem> fActiveStations;
	bool fAllowParallelPlayback;
	BMenuItem* fMenuParallelPlayback;
};


#endif	// _MAIN_WINDOW_H
