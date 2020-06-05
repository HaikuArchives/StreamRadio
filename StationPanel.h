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
 * File:   StationPanel.h
 * Author: Kai Niessen <kai.niessen@online.de>
 *
 * Created on April 13, 2017, 1:25 PM
 */

#ifndef STATIONPANEL_H
#define STATIONPANEL_H

#include "Station.h"
#include "StationListView.h"
#include <Button.h>
#include <Slider.h>
#include <TextControl.h>
#include <View.h>

#define MSG_CHG_NAME 'cNAM'
#define MSG_CHG_VOLUME 'cVOL'
#define MSG_CHG_GENRE 'cGNR'
#define MSG_CHG_STREAMURL 'cURL'
#define MSG_CHG_STATIONURL 'cSUR'
#define MSG_VISIT_STATION 'vSUR'

class MainWindow;

class StationPanel : public BView
{
public:
	StationPanel(MainWindow* mainWindow, bool expanded = false);
	virtual ~StationPanel();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* msg);
	void SetStation(StationListViewItem* stationItem);
	void StateChanged(StreamPlayer::PlayState newState);

private:
	StationListViewItem* fStationItem;
	MainWindow* fMainWindow;
	BView* fLogo;
	BTextControl* fName;
	BTextControl* fUrl;
	BTextControl* fGenre;
	BTextControl* fStationUrl;
	BButton* fVisitStation;
	BSlider* fVolume;
};

#endif /* STATIONPANEL_H */
