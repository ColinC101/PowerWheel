/*
  HPID.cpp - Custom HID handler allowing incoming PID reports

  Copyright (c) 2020, Colin Constans
  Copyright (c) 2015, Arduino LLC
  Original code (pre-library): Copyright (c) 2011, Peter Barrett

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

#include "HPID.h"

#if defined(USBCON)

HID_& HID()
{
	static HID_ obj;
	return obj;
}

int HID_::getInterface(uint8_t* interfaceCount)
{
	*interfaceCount += 1; // uses 1
	HIDDescriptor hidInterface = {
		D_INTERFACE(pluggedInterface, 2, USB_DEVICE_CLASS_HUMAN_INTERFACE, HID_SUBCLASS_NONE, HID_PROTOCOL_NONE),
		D_HIDREPORT(descriptorSize),
		D_ENDPOINT(USB_ENDPOINT_IN(HID_ENDPOINT), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0x01),
		D_ENDPOINT(USB_ENDPOINT_OUT(PID_ENDPOINT), USB_ENDPOINT_TYPE_INTERRUPT, USB_EP_SIZE, 0x01)
	};
	return USB_SendControl(0, &hidInterface, sizeof(hidInterface));
}

int HID_::getDescriptor(USBSetup& setup)
{
	// Check if this is a HID Class Descriptor request
	if (setup.bmRequestType != REQUEST_DEVICETOHOST_STANDARD_INTERFACE) { return 0; }
	if (setup.wValueH != HID_REPORT_DESCRIPTOR_TYPE) { return 0; }

	// In a HID Class Descriptor wIndex cointains the interface number
	if (setup.wIndex != pluggedInterface) { return 0; }

	int total = 0;
	HIDSubDescriptor* node;
	for (node = rootNode; node; node = node->next) {
		int res = USB_SendControl(0, node->data, node->length);
		if (res == -1)
			return -1;
		total += res;
		res = USB_SendControl(TRANSFER_PGM, node->data2, node->length2);
		if (res == -1)
			return -1;
		total += res;
	}
	
	// Reset the protocol on reenumeration. Normally the host should not assume the state of the protocol
	// due to the USB specs, but Windows and Linux just assumes its in report mode.
	protocol = HID_REPORT_PROTOCOL;
	
	return total;
}

uint8_t HID_::getShortName(char *name)
{
	name[0] = 'H';
	name[1] = 'I';
	name[2] = 'D';
	name[3] = 'A' + (descriptorSize & 0x0F);
	name[4] = 'A' + ((descriptorSize >> 4) & 0x0F);
	return 5;
}

void HID_::AppendDescriptor(HIDSubDescriptor *node)
{
	if (!rootNode) {
		rootNode = node;
	} else {
		HIDSubDescriptor *current = rootNode;
		while (current->next) {
			current = current->next;
		}
		current->next = node;
	}
	descriptorSize += (node->length + node->length2);
}

int HID_::SendReport(uint8_t id, const void* data, int len)
{
	auto ret = USB_Send(pluggedEndpoint, &id, 1);
	if (ret < 0) return ret;
	auto ret2 = USB_Send(pluggedEndpoint | TRANSFER_RELEASE, data, len);
	if (ret2 < 0) return ret2;
	return ret + ret2;
}

void HID_::ReceiveReport()
{
	if (USB_Available(PID_ENDPOINT))
	{
		uint8_t ffbReport[PID_REPORT_SIZE];
		if (USB_Recv(PID_ENDPOINT, &ffbReport, PID_REPORT_SIZE) >= 0)
		{
			forceComputer.castReport(ffbReport, PID_REPORT_SIZE);
		}
	}
}

void HID_::getReport(USBSetup& setup) 
{
	if (setup.wValueH == HID_REPORT_TYPE_FEATURE) //Report type, 1=INPUT / 2=OUTPUT / 3=FEATURE
	{
		if (setup.wValueL == 6)
		{
			_delay_us(500);
			USB_SendControl(TRANSFER_RELEASE, (uint8_t*) &forceComputer.blockLoadReport, sizeof(BlockLoadReport_t));
			forceComputer.blockLoadReport.reportId = 0;
		}
		else if (setup.wValueL == 7)
		{
			PoolReport_t poolReport;
			poolReport.reportId = setup.wValueL;
			poolReport.ramPoolSize = 0xffff;
			poolReport.maxSimultaneousEffects = MAX_EFFECTS;
			poolReport.memoryManagement = 3;
			USB_SendControl(TRANSFER_RELEASE, &poolReport, sizeof(PoolReport_t));
		}
	}
}

void HID_::setReport(USBSetup& setup)
{
	if (setup.wValueH == HID_REPORT_TYPE_FEATURE) //Report type, 1=INPUT / 2=OUTPUT / 3=FEATURE
	{
		if (setup.wLength == 0)
		{
			uint8_t data[10];
			USB_RecvControl(&data, setup.wLength);
		}
		if (setup.wValueL == 5) //Report ID
		{
			CreateNewEffectReport_t newEffectReport;
			USB_RecvControl(&newEffectReport, sizeof(CreateNewEffectReport_t));
			forceComputer.createEffect(&newEffectReport);
		}
	}
}

bool HID_::setup(USBSetup& setup)
{
	if (pluggedInterface != setup.wIndex) {
		return false;
	}

	uint8_t request = setup.bRequest;
	uint8_t requestType = setup.bmRequestType;

	if (requestType == REQUEST_DEVICETOHOST_CLASS_INTERFACE)
	{
		if (request == HID_GET_REPORT) {
			getReport(setup);
			return true;
		}
		if (request == HID_GET_PROTOCOL) {
			// TODO: Send8(protocol);
			return true;
		}
		if (request == HID_GET_IDLE) {
			// TODO: Send8(idle);
			return true;
		}
	}

	if (requestType == REQUEST_HOSTTODEVICE_CLASS_INTERFACE)
	{
		if (request == HID_SET_PROTOCOL) {
			// The USB Host tells us if we are in boot or report mode.
			// This only works with a real boot compatible device.
			protocol = setup.wValueL;
			return true;
		}
		if (request == HID_SET_IDLE) {
			idle = setup.wValueL;
			return true;
		}
		if (request == HID_SET_REPORT)
		{
			setReport(setup);
			return true;
		}
	}

	return false;
}


HID_::HID_(void) : PluggableUSBModule(2, 1, epType),
                   rootNode(NULL), descriptorSize(0),
                   protocol(HID_REPORT_PROTOCOL), idle(1)
{
	epType[0] = EP_TYPE_INTERRUPT_IN;
	epType[1] = EP_TYPE_INTERRUPT_OUT;
	PluggableUSB().plug(this);
}

int HID_::begin(void)
{
	return 0;
}

#endif /* if defined(USBCON) */
