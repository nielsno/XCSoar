/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

#include "Formatter/WayPoint.hpp"
#include "XCSoar.h"
#include "WayPoint.hpp"
#include "Protection.hpp"
#include "Settings.hpp"
#include "SettingsComputer.hpp" // for auto-setting of alternates.  Dangerous!
#include "SettingsTask.hpp"
#include "SettingsUser.hpp"
#include <stdio.h>
#include "Interface.hpp"

const TCHAR *FormatterWaypoint::Render(int *color) {
  int thewaypoint = ActiveWayPoint;
  mutexTaskData.Lock();
  if(ValidTaskPoint(thewaypoint))
    {
      int index = task_points[thewaypoint].Index;
      if ((index>=0) && (WayPointList[index].Reachable)) {
	*color = 2; // blue text
      } else {
	*color = 0; // black text
      }
      if ( SettingsMap().DisplayTextType == DISPLAYFIRSTTHREE)
        {
          _tcsncpy(Text,WayPointList[index].Name,3);
          Text[3] = '\0';
        }
      else if( SettingsMap().DisplayTextType == DISPLAYNUMBER)
        {
          _stprintf(Text,_T("%d"),
		    WayPointList[index].Number );
        }
      else
        {
          _tcsncpy(Text,WayPointList[index].Name,
                   (sizeof(Text)/sizeof(TCHAR))-1);
          Text[(sizeof(Text)/sizeof(TCHAR))-1] = '\0';
        }
    }
  else
    {
      Valid = false;
      RenderInvalid(color);
    }
  mutexTaskData.Unlock();

  return(Text);
}

// VENTA3 Alternate destinations
const TCHAR *FormatterAlternate::RenderTitle(int *color) {

  mutexTaskData.Lock();
  if(ValidWayPoint(ActiveAlternate))
    {
      if ( SettingsMap().DisplayTextType == DISPLAYFIRSTTHREE)
        {
          _tcsncpy(Text,WayPointList[ActiveAlternate].Name,3);
          Text[3] = '\0';
        }
      else if( SettingsMap().DisplayTextType == DISPLAYNUMBER)
        {
          _stprintf(Text,_T("%d"),
		    WayPointList[ActiveAlternate].Number );
        }
      else
        {
          _tcsncpy(Text,WayPointList[ActiveAlternate].Name,
                   (sizeof(Text)/sizeof(TCHAR))-1);
          Text[(sizeof(Text)/sizeof(TCHAR))-1] = '\0';
        }
    }
  else
    {
      Valid = false;
      RenderInvalid(color);
    }
  mutexTaskData.Unlock();

  return(Text);
}


/*
 * Currently even if set for FIVV, colors are not used.
 */
const TCHAR *
FormatterAlternate::Render(int *color)
{
  mutexTaskData.Lock();
  if(Valid && ValidWayPoint(ActiveAlternate)) {
    switch (WayPointCalc[ActiveAlternate].VGR ) {
    case 0:
      // impossible, give a magenta debug color;
      *color = 5;
      break;
    case 1:
#ifdef FIVV
      *color = 0; // green
#else
      *color = 0; // blue
#endif
      break;
    case 2:
#ifdef FIVV
      *color = 0; // yellow 4
#else
      *color = 0; // normale white
#endif
      break;
    case 3:
      *color = 1; // red
      break;
    default:
      // even more impossible, give debug color magenta
      *color = 5;
      break;
    }

    Value=WayPointCalc[ActiveAlternate].GR;
    
    _stprintf(Text,Format,Value);
  } else {
    Valid = false;
    RenderInvalid(color);
  }
  mutexTaskData.Unlock();
  return(Text);
}


void FormatterAlternate::AssignValue(int i) {
  mutexTaskData.Lock();
   switch (i) {
   case 67:
     if (!SettingsComputer().EnableAlternate1) { 
       // first run, activate calculations
       SetSettingsComputer().EnableAlternate1 = true;
       Value=INVALID_GR;
     } else {
       if ( ValidWayPoint(Alternate1) ) Value=WayPointCalc[Alternate1].GR;
       else Value=INVALID_GR;
     }
     break;
     /*
       if ( ValidWayPoint(Alternate1) ) Value=WayPointCalc[Alternate1].GR;
       else Value=INVALID_GR;
       break;
     */
   case 68:
     if (!SettingsComputer().EnableAlternate2) { // first run, activate calculations
       SetSettingsComputer().EnableAlternate2 = true;
       Value=INVALID_GR;
     } else {
       if ( ValidWayPoint(Alternate2) ) Value=WayPointCalc[Alternate2].GR;
       else Value=INVALID_GR;
     }
     break;
   case 69:
     if (!SettingsComputer().EnableBestAlternate) { 
       // first run, waiting for slowcalculation loop
       SetSettingsComputer().EnableBestAlternate = true;	  // activate it
       Value=INVALID_GR;
     } else {
       if ( ValidWayPoint(BestAlternate))
	 Value=WayPointCalc[BestAlternate].GR;
       else
	 Value=INVALID_GR;
     }
     break;
   default:
     Value=66.6; // something evil to notice..
     break;
   }

   Valid=false;
   if (Value < INVALID_GR) {
    	Valid = true;
	if (Value >= 100 )
	  {
	    _tcscpy(Format, _T("%1.0f"));
	  }
	else
	  {
	    _tcscpy(Format, _T("%1.1f"));
	  }
   }

   mutexTaskData.Unlock();
}




const TCHAR *FormatterDiffBearing::Render(int *color) {

  if (ValidTaskPoint(ActiveWayPoint)
      && Calculated().WaypointDistance > 10.0) {
    Valid = true;

    Value = Calculated().WaypointBearing -  Basic().TrackBearing;

    if (Value < -180.0)
      Value += 360.0;
    else
    if (Value > 180.0)
      Value -= 360.0;

#ifndef __MINGW32__
    if (Value > 1)
      _stprintf(Text, _T("%2.0f��"), Value);
    else if (Value < -1)
      _stprintf(Text, _T("�%2.0f�"), -Value);
    else
      _tcscpy(Text, _T("��"));
#else
    if (Value > 1)
      _stprintf(Text, _T("%2.0f°»"), Value);
    else if (Value < -1)
      _stprintf(Text, _T("«%2.0f°"), -Value);
    else
      _tcscpy(Text, _T("«»"));
#endif
    *color = 0;
  } else {
    Valid = false;
    RenderInvalid(color);
  }

  return(Text);
}

