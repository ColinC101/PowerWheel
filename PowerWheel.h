/*
  PowerWheel.h - Open simulation steering wheel library for Arduino USB boards
  Copyright (C) 2020 Colin Constans

  This library is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this library. If not, see <https://www.gnu.org/licenses/>.
*/


#ifndef POWERWHEEL_h
#define POWERWHEEL_h

#include "HPID.h"
#include "HidReportDescriptor.h"
#include "PidReportDescriptor.h"

#define BUTTON_COUNT 20 
#define BUTTON_BYTE_COUNT 3
#define HATSWITCH_COUNT 1 
#define AXIS_COUNT 6


class PowerWheel
{
public:
	PowerWheel();
	
	void updateButton(uint8_t buttonIndex, uint8_t buttonValue);
	void updateHatSwitch(uint8_t HatSwitchIndex, uint8_t HatSwitchValue);
	void updateAxis(uint8_t axisIndex, uint8_t axisValue);
	void updateForces(int32_t* forces);
	
private:

	uint8_t* axisValues = NULL;
	uint8_t* hatSwitchValues = NULL;
    uint8_t* buttonValues = NULL;

	uint8_t hidReportSize = 9; //2 * 8btns + (4btns+hat) + 6 * axis
	uint8_t hidReportId = 1;
	
	void pushUpdate();
};

#endif