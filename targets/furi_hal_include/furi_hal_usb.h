#pragma once

#include "usb.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct FuriHalUsbInterface FuriHalUsbInterface;

struct FuriHalUsbInterface {
    void (*init)(usbd_device* dev, FuriHalUsbInterface* intf, void* ctx);
    void (*deinit)(usbd_device* dev);
    void (*wakeup)(usbd_device* dev);
    void (*suspend)(usbd_device* dev);

    struct usb_device_descriptor* dev_descr;

    void* str_manuf_descr;
    void* str_prod_descr;
    void* str_serial_descr;

    void* cfg_descr;
};

/** USB device interface modes */
extern FuriHalUsbInterface usb_cdc_single;
extern FuriHalUsbInterface usb_cdc_dual;
extern FuriHalUsbInterface usb_hid;
extern FuriHalUsbInterface usb_hid_u2f;
extern FuriHalUsbInterface usb_ccid;

typedef enum {
    FuriHalUsbStateEventReset,
    FuriHalUsbStateEventWakeup,
    FuriHalUsbStateEventSuspend,
    FuriHalUsbStateEventDescriptorRequest,
} FuriHalUsbStateEvent;

typedef void (*FuriHalUsbStateCallback)(FuriHalUsbStateEvent state, void* context);

/** USB device low-level initialization
 */
void furi_hal_usb_init(void);

/** Set USB device configuration
 *
 * @param      mode new USB device mode
 * @param      ctx context passed to device mode init function
 * @return     true - mode switch started, false - mode switch is locked
 */
bool furi_hal_usb_set_config(FuriHalUsbInterface* new_if, void* ctx);

/** Get USB device configuration
 *
 * @return    current USB device mode
 */
FuriHalUsbInterface* furi_hal_usb_get_config(void);

/** Get USB device configuration context
 *
 * @return    current USB device context
 */
void* furi_hal_usb_get_config_context(void);

/** Lock USB device mode switch
 */
void furi_hal_usb_lock(void);

/** Unlock USB device mode switch
 */
void furi_hal_usb_unlock(void);

/** Check if USB device mode switch locked
 * 
 * @return    lock state
 */
bool furi_hal_usb_is_locked(void);

/** Disable USB device
 */
void furi_hal_usb_disable(void);

/** Enable USB device
 */
void furi_hal_usb_enable(void);

/** Set USB state callback
 */
void furi_hal_usb_set_state_callback(FuriHalUsbStateCallback cb, void* ctx);

/** Restart USB device
 */
void furi_hal_usb_reinit(void);

#ifdef __cplusplus
}
#endif
