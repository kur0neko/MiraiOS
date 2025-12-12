#pragma once
#include <Arduino.h>
#include "usb/usb_host.h"

class UsbHidDevice {
public:
    virtual void onData(const usb_transfer_t *transfer) = 0;
};

class EspUsbHost {
public:
    //CLIENT HANDLE FOR USB HOST
    usb_host_client_handle_t client_hdl;
    
    //DEVICE HANDLE FOR CONNECTED USB DEVICE
    usb_device_handle_t dev_hdl = NULL;
    
    //POINTER TO HID DEVICE INSTANCE
    UsbHidDevice *hid_device = NULL;
    
    //ENDPOINT ADDRESS FOR SENDING DATA OUT
    uint8_t out_ep_addr = 0; 
    
    //TRANSFER BUFFER FOR OUTGOING DATA
    usb_transfer_t *out_transfer = NULL;
    
    //FLAG TO CHECK IF USB HOST IS READY
    bool is_ready = false;

    void begin();
    void task();
    void init(UsbHidDevice *device);
    
    //SEND DATA TO USB DEVICE
    void sendWrite(uint8_t *data, size_t len);
    
    //INTERNAL CALLBACK FOR USB EVENTS
    void _client_event_callback(const usb_host_client_event_msg_t *event_msg);
};