#pragma once

void usb_device_init(); // initialize usb device
bool usb_device_attached(); // return True if attached
bool usb_device_seen(); // return True if attached at any point before clear
void usb_device_clear(); // clear the "seen" state
