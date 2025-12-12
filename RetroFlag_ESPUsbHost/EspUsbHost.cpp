#include "EspUsbHost.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

//WRAPPER TO CALL MEMBER FUNCTION FROM C CALLBACK
void _client_event_callback_wrapper(const usb_host_client_event_msg_t *event_msg, void *arg) {
    EspUsbHost *host = (EspUsbHost *)arg;
    host->_client_event_callback(event_msg);
}

//CALLBACK FOR USB TRANSFER COMPLETION
void _transfer_cb(usb_transfer_t *transfer) {
    EspUsbHost *host = (EspUsbHost *)transfer->context;
    
    //CHECK IF TRANSFER IS INPUT AND SUCCESSFUL
    if ((transfer->bEndpointAddress & 0x80) && transfer->status == 0 && host->hid_device) {
        
        //PASS DATA TO HID DEVICE
        host->hid_device->onData(transfer);
        
        //RESUBMIT TRANSFER TO KEEP RECEIVING DATA
        esp_err_t err = usb_host_transfer_submit(transfer);
        if (err != ESP_OK) {
            Serial0.printf("Failed to resubmit transfer: %x\n", err);
        }
    }
}

//INITIALIZE USB HOST
void EspUsbHost::begin() {
    
    //CONFIG FOR USB HOST
    const usb_host_config_t config = { 
        .skip_phy_setup = false, 
        .intr_flags = ESP_INTR_FLAG_LEVEL1
    };
    
    //INSTALL USB HOST DRIVER
    esp_err_t err = usb_host_install(&config);
    if (err != ESP_OK) {
        Serial0.printf("USB Driver Init Failed: %x\n", err);
        return;
    }

    //CONFIG FOR USB CLIENT
    const usb_host_client_config_t client_config = {
        .is_synchronous = false,
        .max_num_event_msg = 3,
        .async = { .client_event_callback = _client_event_callback_wrapper, .callback_arg = this }
    };
    
    //REGISTER USB CLIENT
    err = usb_host_client_register(&client_config, &client_hdl);
    if (err != ESP_OK) {
        Serial0.printf("USB Client Register Failed: %x\n", err);
        return;
    }
    
    //SET READY FLAG
    is_ready = true;
    Serial0.println(">> USB Host ready");
}

//LINK HID DEVICE TO USB HOST
void EspUsbHost::init(UsbHidDevice *device) {
    hid_device = device;
}

//MAIN USB HOST TASK
void EspUsbHost::task() {
    if (is_ready) {
        
        //HANDLE USB LIBRARY EVENTS
        usb_host_lib_handle_events(0, NULL);
        if (client_hdl) {
            
            //HANDLE USB CLIENT EVENTS
            usb_host_client_handle_events(client_hdl, 0);
        }
    }
}

//SEND DATA TO USB DEVICE
void EspUsbHost::sendWrite(uint8_t *data, size_t len) {
    if (!dev_hdl || out_ep_addr == 0 || !out_transfer) return;
    
    //LIMIT DATA LENGTH TO MAX PACKET SIZE
    if (len > out_transfer->num_bytes) len = out_transfer->num_bytes;
    
    //COPY DATA TO TRANSFER BUFFER
    memcpy(out_transfer->data_buffer, data, len);
    out_transfer->actual_num_bytes = len;
    
    //SUBMIT TRANSFER
    esp_err_t err = usb_host_transfer_submit(out_transfer);
    if (err != ESP_OK) {
        Serial0.printf("USB transfer error: %x\n", err);
    }
}

//FIND ENDPOINT IN CONFIG DESCRIPTOR
bool findEndpoint(const usb_config_desc_t *config_desc, uint8_t ep_addr, uint8_t *mps) {
    const uint8_t *p = (const uint8_t *)config_desc;
    uint16_t offset = 0;
    
    //ITERATE THROUGH DESCRIPTORS
    while (offset < config_desc->wTotalLength) {
        uint8_t desc_len = p[offset];
        uint8_t desc_type = p[offset + 1];
        
        //CHECK IF DESCRIPTOR IS ENDPOINT
        if (desc_type == 0x05) { 
            uint8_t endpoint_addr = p[offset + 2];
            uint16_t endpoint_mps = p[offset + 4] | (p[offset + 5] << 8);
            
            //CHECK IF ADDRESS MATCHES
            if (endpoint_addr == ep_addr) {
                
                //EXTRACT MAX PACKET SIZE
                *mps = endpoint_mps & 0x7FF; 
                return true;
            }
        }
        offset += desc_len;
    }
    return false;
}

//GET INPUT ENDPOINT ADDRESS
uint8_t getInEndpoint(const usb_config_desc_t *config_desc, uint8_t *mps) {
    const uint8_t *p = (const uint8_t *)config_desc;
    uint16_t offset = 0;
    
    //ITERATE THROUGH DESCRIPTORS
    while (offset < config_desc->wTotalLength) {
        uint8_t desc_len = p[offset];
        uint8_t desc_type = p[offset + 1];
        
        //CHECK IF DESCRIPTOR IS ENDPOINT
        if (desc_type == 0x05) { 
            uint8_t endpoint_addr = p[offset + 2];
            uint16_t endpoint_mps = p[offset + 4] | (p[offset + 5] << 8);
            
            //CHECK IF ENDPOINT IS IN DIRECTION
            if (endpoint_addr & 0x80) { 
                
                //EXTRACT MAX PACKET SIZE
                *mps = endpoint_mps & 0x7FF;
                return endpoint_addr;
            }
        }
        offset += desc_len;
    }
    return 0;
}

//GET OUTPUT ENDPOINT ADDRESS
uint8_t getOutEndpoint(const usb_config_desc_t *config_desc, uint8_t *mps) {
    const uint8_t *p = (const uint8_t *)config_desc;
    uint16_t offset = 0;
    
    //ITERATE THROUGH DESCRIPTORS
    while (offset < config_desc->wTotalLength) {
        uint8_t desc_len = p[offset];
        uint8_t desc_type = p[offset + 1];
        
        //CHECK IF DESCRIPTOR IS ENDPOINT
        if (desc_type == 0x05) { 
            uint8_t endpoint_addr = p[offset + 2];
            uint16_t endpoint_mps = p[offset + 4] | (p[offset + 5] << 8);
            
            //CHECK IF ENDPOINT IS OUT DIRECTION
            if (!(endpoint_addr & 0x80)) { 
                
                //EXTRACT MAX PACKET SIZE
                *mps = endpoint_mps & 0x7FF;
                return endpoint_addr;
            }
        }
        offset += desc_len;
    }
    return 0;
}

//HANDLE USB CLIENT EVENTS
void EspUsbHost::_client_event_callback(const usb_host_client_event_msg_t *event_msg) {
    
    //CHECK IF NEW DEVICE CONNECTED
    if (event_msg->event == USB_HOST_CLIENT_EVENT_NEW_DEV) {
        Serial0.printf("\n>> Device Detected Addr: %d\n", event_msg->new_dev.address);
        delay(100);

        usb_device_handle_t temp_handle;
        
        //OPEN USB DEVICE
        esp_err_t err = usb_host_device_open(client_hdl, event_msg->new_dev.address, &temp_handle);
        if (err != ESP_OK) {
            Serial0.printf("Failed to open device: %x\n", err);
            return;
        }

        const usb_device_desc_t *dev_desc;
        
        //GET DEVICE DESCRIPTOR
        usb_host_get_device_descriptor(temp_handle, &dev_desc);
        
        //CHECK IF DEVICE IS A HUB
        if (dev_desc->bDeviceClass == 0x09) {
            Serial0.println("   [Info] Hub Detected. Waiting for Gamepad..."); 
            
            //CLOSE HUB DEVICE
            usb_host_device_close(client_hdl, temp_handle);
            return;
        }

        //ASSIGN DEVICE HANDLE
        dev_hdl = temp_handle;
        
        //CLAIM USB INTERFACE
        err = usb_host_interface_claim(client_hdl, dev_hdl, 0, 0);
        if (err != ESP_OK) {
            Serial0.printf("Failed to claim interface: %x\n", err);
            usb_host_device_close(client_hdl, dev_hdl);
            dev_hdl = NULL;
            return;
        }

        const usb_config_desc_t *config_desc;
        
        //GET CONFIGURATION DESCRIPTOR
        usb_host_get_active_config_descriptor(dev_hdl, &config_desc);
        
        uint8_t in_mps = 0;
        
        //FIND INPUT ENDPOINT
        uint8_t in_ep = getInEndpoint(config_desc, &in_mps);
        
        if (in_ep == 0) {
            Serial0.println("   [Error] No IN endpoint found");
            usb_host_device_close(client_hdl, dev_hdl);
            dev_hdl = NULL;
            return;
        }
        
        Serial0.printf("   [Info] IN Endpoint: 0x%02X, MPS: %d\n", in_ep, in_mps);
        
        //ALLOCATE TRANSFER FOR INPUT
        usb_transfer_t *in_xfer;
        err = usb_host_transfer_alloc(in_mps, 0, &in_xfer);
        if (err != ESP_OK) {
            Serial0.printf("Failed to alloc IN transfer: %x\n", err);
            usb_host_device_close(client_hdl, dev_hdl);
            dev_hdl = NULL;
            return;
        }
        
        //SETUP INPUT TRANSFER
        in_xfer->device_handle = dev_hdl;
        in_xfer->bEndpointAddress = in_ep;
        in_xfer->callback = _transfer_cb;
        in_xfer->context = this;
        in_xfer->num_bytes = in_mps;
        
        //SUBMIT INPUT TRANSFER
        err = usb_host_transfer_submit(in_xfer);
        if (err != ESP_OK) {
            Serial0.printf("Failed to submit IN transfer: %x\n", err);
            usb_host_transfer_free(in_xfer);
            usb_host_device_close(client_hdl, dev_hdl);
            dev_hdl = NULL;
            return;
        }

        uint8_t out_mps = 0;
        
        //FIND OUTPUT ENDPOINT
        out_ep_addr = getOutEndpoint(config_desc, &out_mps);
        
        if (out_ep_addr != 0) {
            Serial0.printf("   [Info] OUT Endpoint: 0x%02X, MPS: %d\n", out_ep_addr, out_mps);
            
            //ALLOCATE TRANSFER FOR OUTPUT
            err = usb_host_transfer_alloc(out_mps, 0, &out_transfer);
            if (err != ESP_OK) {
                Serial0.printf("Failed to alloc OUT transfer: %x\n", err);
                out_transfer = NULL;
            } else {
                
                //SETUP OUTPUT TRANSFER
                out_transfer->device_handle = dev_hdl;
                out_transfer->bEndpointAddress = out_ep_addr;
                out_transfer->callback = _transfer_cb;
                out_transfer->context = this;
                out_transfer->num_bytes = out_mps;
                
                //SEND WAKE UP SIGNAL
                uint8_t wakeup[] = {0x01, 0x03, 0x02}; 
                sendWrite(wakeup, 3);
                Serial0.println("   [Action] Sent Wake-Up Signal.");
            }
        } else {
            Serial0.println("   [Info] No OUT endpoint found");
        }
        
        Serial0.println("   [State] Controller Active.");
    }
}