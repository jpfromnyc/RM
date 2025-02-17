#include "../nfc_magic_app_i.h"

void nfc_magic_scene_write_fail_widget_callback(
    GuiButtonType result,
    InputType type,
    void* context) {
    NfcMagicApp* instance = context;
    if(type == InputTypeShort) {
        view_dispatcher_send_custom_event(instance->view_dispatcher, result);
    }
}

void nfc_magic_scene_write_fail_on_enter(void* context) {
    NfcMagicApp* instance = context;
    Widget* widget = instance->widget;

    notification_message(instance->notifications, &sequence_error);

    widget_add_icon_element(widget, 83, 22, &I_WarningDolphinFlip_45x42);
    widget_add_string_element(
        widget, 64, 0, AlignCenter, AlignTop, FontPrimary, "Failed to Write");
    widget_add_string_multiline_element(
        widget, 0, 13, AlignLeft, AlignTop, FontSecondary, "Something went\nwrong while\nwriting");

    widget_add_button_element(
        widget, GuiButtonTypeLeft, "Retry", nfc_magic_scene_write_fail_widget_callback, instance);

    // Setup and start worker
    view_dispatcher_switch_to_view(instance->view_dispatcher, NfcMagicAppViewWidget);
}

bool nfc_magic_scene_write_fail_on_event(void* context, SceneManagerEvent event) {
    NfcMagicApp* instance = context;
    bool consumed = false;

    if(event.type == SceneManagerEventTypeCustom) {
        if(event.event == GuiButtonTypeLeft) {
            consumed = scene_manager_previous_scene(instance->scene_manager);
        }
    } else if(event.type == SceneManagerEventTypeBack) {
        consumed = scene_manager_previous_scene(instance->scene_manager);
    }
    return consumed;
}

void nfc_magic_scene_write_fail_on_exit(void* context) {
    NfcMagicApp* instance = context;

    widget_reset(instance->widget);
}
