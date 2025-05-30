#pragma once

#include "desktop.h"
#include "animations/animation_manager.h"
#include "views/desktop_view_pin_timeout.h"
#include "views/desktop_view_pin_input.h"
#include "views/desktop_view_locked.h"
#include "views/desktop_view_main.h"
#include "views/desktop_view_lock_menu.h"
#include "views/desktop_view_debug.h"
#include "views/desktop_view_slideshow.h"
#include <desktop/desktop_settings.h>
#include <bt/bt_settings.h>

#include <furi.h>
#include <gui/gui.h>
#include <gui/view_stack.h>
#include <gui/view_dispatcher.h>
#include <gui/modules/popup.h>
#include <gui/scene_manager.h>
#include <bt/bt_service/bt.h>
#include <storage/storage.h>

#include <loader/loader.h>
#include <notification/notification_app.h>

#define STATUS_BAR_Y_SHIFT 13

typedef enum {
    DesktopViewIdMain,
    DesktopViewIdLockMenu,
    DesktopViewIdLocked,
    DesktopViewIdHwMismatch,
    DesktopViewIdPinInput,
    DesktopViewIdPinTimeout,
    DesktopViewIdSlideshow,
    DesktopViewIdTotal,
} DesktopViewId;

struct Desktop {
    // Scene
    FuriThread* scene_thread;
    // GUI
    Gui* gui;
    ViewDispatcher* view_dispatcher;
    SceneManager* scene_manager;

    Popup* hw_mismatch_popup;
    DesktopLockMenuView* lock_menu;
    DesktopDebugView* _debug_view; // Unused, kept for compatibility
    DesktopViewLocked* locked_view;
    DesktopMainView* main_view;
    DesktopViewPinTimeout* pin_timeout_view;
    DesktopSlideshowView* slideshow_view;

    ViewStack* main_view_stack;
    ViewStack* locked_view_stack;

    DesktopSettings settings;
    DesktopViewPinInput* pin_input_view;

    ViewPort* lock_icon_viewport;
    ViewPort* dummy_mode_icon_viewport;
    ViewPort* topbar_icon_viewport;
    ViewPort* sdcard_icon_viewport;
    ViewPort* bt_icon_viewport;
    ViewPort* clock_viewport;
    ViewPort* stealth_mode_icon_viewport;

    ViewPort* lock_icon_slim_viewport;
    ViewPort* dummy_mode_icon_slim_viewport;
    ViewPort* topbar_icon_slim_viewport;
    ViewPort* sdcard_icon_slim_viewport;
    ViewPort* bt_icon_slim_viewport;
    ViewPort* clock_slim_viewport;
    ViewPort* stealth_mode_icon_slim_viewport;

    AnimationManager* animation_manager;

    Loader* loader;
    NotificationApp* notification;

    FuriPubSubSubscription* app_start_stop_subscription;
    FuriPubSub* input_events_pubsub;
    FuriPubSubSubscription* input_events_subscription;
    FuriPubSubSubscription* storage_sub;
    FuriTimer* auto_lock_timer;
    FuriTimer* update_clock_timer;

    Bt* bt;

    bool sdcard_status;

    FuriPubSub* status_pubsub;
    uint8_t hour;
    uint8_t minute;
    bool clock_type : 1; // true - 24h false - 12h

    bool in_transition : 1;

    FuriSemaphore* animation_semaphore;
};

Desktop* desktop_alloc(void);

void desktop_free(Desktop* desktop);
void desktop_lock(Desktop* desktop);
void desktop_unlock(Desktop* desktop);
void desktop_set_dummy_mode_state(Desktop* desktop, bool enabled);
void desktop_set_stealth_mode_state(Desktop* desktop, bool enabled);
