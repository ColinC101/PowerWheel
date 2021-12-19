/*
  HidReportDescriptor.h - USB-HID report descriptor for PowerWheel interface devices
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


#ifndef HIDREPORTDESCRIPTOR_H
#define HIDREPORTDESCRIPTOR_H

#define HID_REPORT_DESCRIPTOR_SIZE  83


static const uint8_t hidReportDescriptor[] PROGMEM = 
{
	//HEADER
	0x05, 0x01,       //USAGE_PAGE (Generic Desktop)
	0x09, 0x04,       //USAGE (Joystick)
	0xa1, 0x01,       //COLLECTION (Application)
	0x09, 0x01,       //USAGE (Pointer)
	0x85, 0x01,       //REPORT_ID
	0xa1, 0x00,       //COLLECTION (Physical)
	//BUTTONS
	0x05, 0x09,       //USAGE_PAGE (Button)
	0x19, 0x01,       //USAGE_MINIMUM (1)
	0x29, 0x14,       //USAGE_MAXIMUM (20 = 0x14)
	0x15, 0x00,       //LOGICAL_MINIMUM (0)
	0x25, 0x01,       //LOGICAL_MAXIMUM (1)
	0x75, 0x01,       //REPORT_SIZE (1)
	0x95, 0x14,       //REPORT_COUNT (20 = 0x14)
	0x55, 0x00,       //UNIT_EXPONENT (0)
	0x65, 0x00,       //UNIT (None)
	0x81, 0x02,       //INPUT (Data, Var, Abs)
	//HEADER
	0x05, 0x01,       //USAGE_PAGE (Generic Desktop)
	//HATSWITCH
	0x09,0x39,       //USAGE (Hat Switch)
	0x15, 0x00,       //LOGICAL_MINIMUM (0)
	0x25, 0x07,       //LOGICAL_MAXIMUM (7)
	0x35, 0x00,       //PHYSICAL_MINIMUM (0)
	0x46, 0x3B, 0x01, //PHYSICAL_MAXIMUM (315)
	0x65, 0x14,       //UNIT (Eng Rot : Angular Pos)
	0x75, 0x04,       //REPORT_SIZE (4)
	0x95, 0x01,       //REPORT_COUNT (1)
	0x81, 0x02,       //INPUT (Data, Var, Abs)
	//AXIS
	0x09, 0x01,       //USAGE (Pointer)
	0x16, 0x00, 0x00, //LOGICAL_MINIMUM (0)
	0x26, 0xff, 0x00, //LOGICAL_MAXIMUM (255)
	0x75, 0x08,       //REPORT_SIZE (8)
	0x95, 0x06,       //REPORT_COUNT (6)
	0xA1, 0x00,       //COLLECTION (Physical)
	0x09, 0x30,       //USAGE (X)
	0x09, 0x31,       //USAGE (Y)
	0x09, 0x32,       //USAGE (Z)
	0x09, 0x33,       //USAGE (Rx)
	0x09, 0x34,       //USAGE (Ry)
	0x09, 0x35,       //USAGE (Rz)
	0x81, 0x02,       //INPUT (Data, Var, Abs)
	0xc0,             //END_COLLECTION (Physical)
	0xc0,             //END_COLLECTION
};

#endif