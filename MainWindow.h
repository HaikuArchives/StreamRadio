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
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Window.h>
#include <Message.h>
#include <MenuBar.h>
#include <Menu.h>
#include <StringView.h>
#include <ObjectList.h>
#include "RadioSettings.h"
#include "StationListView.h"
#include "StationFinder.h"
#include "StreamPlayer.h"
#include "StationPanel.h"
#include "Expander.h"

#define MSG_REFS_RECEIVED   	'REFS'
#define MSG_PASTE_URL       	'PURL'
#define MSG_SEARCH          	'mSRC'
#define MSG_CHECK           	'mCKS'           
#define MSG_REMOVE          	'mRMS'
#define MSG_INVOKE_STATION  	'mIST'
#define MSG_HELP				'HELP'
#define MSG_PARALLEL_PLAYBACK	'mPAR'

/** \brief The mainwindow.
*
* This class provides the mainwindow and handels the actions, session
* management, the global setting dialog, the <i>Tip of the day</i> and so on.
*
* \note The desctructor of this function will not necesarily be called, so also the children
* objects will not necesarrily be destroyed. So don't relay on the desctructor of children
* objects to save data, the application state and so on! */
class MainWindow : public BWindow
{
  public:
									MainWindow();
    virtual							~MainWindow();
    
    virtual void					MessageReceived(BMessage* message); 
    virtual bool					QuitRequested();
    virtual void					SetVisible(bool visible);

  private:
    RadioSettings*											fSettings;
    BMenuBar*												fMainMenu;
    StationListView*										fStationList;
    StationFinderWindow*									fStationFinder;
	StationPanel*											fStationPanel;
    BStringView*											fStatusBar;
	Expander*												fExpander;
	BObjectList<StationListViewItem>						activeStations;
	
	void							TogglePlay(StationListViewItem* stationItem);
	
    /** Actualizes the status in the status bar and in the tooltip of the system tray icon. */
    void							AskForKradioripperImport();
    /** Displays the settings_general_dialog. */
    void							DisplaySettings();
    /** Displays the tip of the day (independently from if the user
    *   has disabled them or not). */
    void							DisplayTipOfDay();
    /** Sets the visibility of the streamdirectory and saves this value also in the config file.
    * @param visible Choose \e true to show the streamdirectory and \e false to hide it.
    * \note If #MainWindow is not visible, the streamdirectory will be hidden indepentendly of
    * the parameter. */

    /** Sets up the actions that are supported by this class. */
    void							SetupActions();
    /** Sets up the dock widget with the stream directory. */
    void							SetupStreamDirectory();
    void							SaveSettings();
};

#endif
