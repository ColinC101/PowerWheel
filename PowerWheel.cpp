/*
  PowerWheel.cpp - Open simulation steering wheel library for Arduino USB boards
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


#include "PowerWheel.h"


PowerWheel::PowerWheel()
{
	buttonValues = new uint8_t[BUTTON_BYTE_COUNT];
	axisValues = new uint8_t[AXIS_COUNT];
	hatSwitchValues = new uint8_t[HATSWITCH_COUNT];

	for (uint8_t i = 0 ; i < AXIS_COUNT ; i++)
	{
		axisValues[i] = 0;
	}
    for (uint8_t i = 0; i < BUTTON_BYTE_COUNT; i++)
    {
        buttonValues[i] = 0;
    }
	for (uint8_t i = 0 ; i < HATSWITCH_COUNT ; i++)
	{
		hatSwitchValues[i] = 8;
	}

	uint16_t hidReportDescriptorSize = sizeof(hidReportDescriptor) / sizeof(hidReportDescriptor[0]); //83
	uint16_t pidReportDescriptorSize = sizeof(pidReportDescriptor) / sizeof(pidReportDescriptor[0]); //1227
	HIDSubDescriptor* node = new HIDSubDescriptor(hidReportDescriptor, hidReportDescriptorSize, pidReportDescriptor, pidReportDescriptorSize);
	HID().AppendDescriptor(node);

}


void PowerWheel::updateButton(uint8_t buttonIndex, uint8_t buttonValue)
{
	bitWrite(buttonValues[buttonIndex / 8], buttonIndex % 8, buttonValue);
}


void PowerWheel::updateHatSwitch(uint8_t hatSwitchIndex, uint8_t hatSwitchValue)
{
	hatSwitchValues[hatSwitchIndex] = hatSwitchValue;
}


void PowerWheel::updateAxis(uint8_t axisIndex, uint8_t axisValue)
{
	axisValues[axisIndex] = axisValue;
}


void PowerWheel::updateConditionValue(int16_t springCurPos, int16_t damperCurVel,int16_t inertiaCurAcc,int16_t frictionCurPos)
{
    HID().forceComputer.springCurPos = springCurPos;
    HID().forceComputer.damperCurVel = damperCurVel;
    HID().forceComputer.inertiaCurAcc = inertiaCurAcc;
    HID().forceComputer.frictionCurPos = frictionCurPos;
}


void PowerWheel::updateForces(int32_t* forces)
{
	HID().ReceiveReport();
	HID().forceComputer.ComputeFinalForces(forces);
}


void PowerWheel::pushUpdate()
{
	uint8_t data[hidReportSize];
	uint8_t index = 0;

	data[index++] = buttonValues[0];
	data[index++] = buttonValues[1];
	data[index++] = (hatSwitchValues[0] << 4) | (B00001111 & buttonValues[2]);

	for (uint8_t i = 0 ; i < AXIS_COUNT ; i++)
	{
		data[index++] = axisValues[i];
	}

	HID().SendReport(hidReportId, data, hidReportSize);
}
