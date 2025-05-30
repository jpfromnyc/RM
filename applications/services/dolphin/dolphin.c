#include "dolphin/dolphin.h"
#include "dolphin/helpers/dolphin_state.h"
#include "dolphin_i.h"
#include "projdefs.h"
#include <furi_hal.h>
#include <stdint.h>
#include <furi.h>
#include "furi_hal_random.h"
#define DOLPHIN_LOCK_EVENT_FLAG (0x1)

#define TAG "Dolphin"
#define HOURS_IN_TICKS(x) ((x) * 60 * 60 * 1000)

static void dolphin_update_clear_limits_timer_period(Dolphin* dolphin);

void dolphin_deed(DolphinDeed deed) {
    Dolphin* dolphin = (Dolphin*)furi_record_open(RECORD_DOLPHIN);
    DolphinEvent event;
    event.type = DolphinEventTypeDeed;
    event.deed = deed;
    dolphin_event_send_async(dolphin, &event);
    furi_record_close(RECORD_DOLPHIN);
}

DolphinDeed getRandomDeed() {
    DolphinDeed returnGrp[14] = {1, 5, 8, 10, 12, 15, 17, 20, 21, 25, 26, 28, 29, 32};
    static bool rand_generator_inited = false;
    if(!rand_generator_inited) {
        srand(furi_get_tick());
        rand_generator_inited = true;
    }
    uint8_t diceRoll = (rand() % COUNT_OF(returnGrp)); // JUST TO GET IT GOING? AND FIX BUG
    diceRoll = (rand() % COUNT_OF(returnGrp));
    return returnGrp[diceRoll];
}

DolphinStats dolphin_stats(Dolphin* dolphin) {
    furi_check(dolphin);

    DolphinStats stats;
    DolphinEvent event;

    event.type = DolphinEventTypeStats;
    event.stats = &stats;

    dolphin_event_send_wait(dolphin, &event);

    return stats;
}

void dolphin_flush(Dolphin* dolphin) {
    furi_check(dolphin);

    DolphinEvent event;
    event.type = DolphinEventTypeFlush;

    dolphin_event_send_wait(dolphin, &event);
}

void dolphin_butthurt_timer_callback(void* context) {
    Dolphin* dolphin = context;
    furi_assert(dolphin);

    DolphinEvent event;
    event.type = DolphinEventTypeIncreaseButthurt;
    dolphin_event_send_async(dolphin, &event);
}

void dolphin_flush_timer_callback(void* context) {
    Dolphin* dolphin = context;
    furi_assert(dolphin);

    DolphinEvent event;
    event.type = DolphinEventTypeFlush;
    dolphin_event_send_async(dolphin, &event);
}

void dolphin_clear_limits_timer_callback(void* context) {
    Dolphin* dolphin = context;
    furi_assert(dolphin);

    furi_timer_start(dolphin->clear_limits_timer, HOURS_IN_TICKS(24));

    DolphinEvent event;
    event.type = DolphinEventTypeClearLimits;
    dolphin_event_send_async(dolphin, &event);
}

Dolphin* dolphin_alloc(void) {
    Dolphin* dolphin = malloc(sizeof(Dolphin));

    dolphin->state = dolphin_state_alloc();
    dolphin->event_queue = furi_message_queue_alloc(8, sizeof(DolphinEvent));
    dolphin->pubsub = furi_pubsub_alloc();
    dolphin->butthurt_timer =
        furi_timer_alloc(dolphin_butthurt_timer_callback, FuriTimerTypePeriodic, dolphin);
    dolphin->flush_timer =
        furi_timer_alloc(dolphin_flush_timer_callback, FuriTimerTypeOnce, dolphin);
    dolphin->clear_limits_timer =
        furi_timer_alloc(dolphin_clear_limits_timer_callback, FuriTimerTypePeriodic, dolphin);

    return dolphin;
}

void dolphin_event_send_async(Dolphin* dolphin, DolphinEvent* event) {
    furi_assert(dolphin);
    furi_assert(event);
    event->flag = NULL;
    furi_check(
        furi_message_queue_put(dolphin->event_queue, event, FuriWaitForever) == FuriStatusOk);
}

void dolphin_event_send_wait(Dolphin* dolphin, DolphinEvent* event) {
    furi_assert(dolphin);
    furi_assert(event);
    event->flag = furi_event_flag_alloc();
    furi_check(event->flag);
    furi_check(
        furi_message_queue_put(dolphin->event_queue, event, FuriWaitForever) == FuriStatusOk);
    furi_check(
        furi_event_flag_wait(
            event->flag, DOLPHIN_LOCK_EVENT_FLAG, FuriFlagWaitAny, FuriWaitForever) ==
        DOLPHIN_LOCK_EVENT_FLAG);
    furi_event_flag_free(event->flag);
}

void dolphin_event_release(Dolphin* dolphin, DolphinEvent* event) {
    UNUSED(dolphin);
    if(event->flag) {
        furi_event_flag_set(event->flag, DOLPHIN_LOCK_EVENT_FLAG);
    }
}

FuriPubSub* dolphin_get_pubsub(Dolphin* dolphin) {
    furi_check(dolphin);
    return dolphin->pubsub;
}

static void dolphin_update_clear_limits_timer_period(Dolphin* dolphin) {
    furi_assert(dolphin);
    uint32_t now_ticks = furi_get_tick();
    uint32_t timer_expires_at = furi_timer_get_expire_time(dolphin->clear_limits_timer);

    if((timer_expires_at - now_ticks) > HOURS_IN_TICKS(0.1)) {
        DateTime date;
        furi_hal_rtc_get_datetime(&date);
        uint32_t now_time_in_ms = ((date.hour * 60 + date.minute) * 60 + date.second) * 1000;
        uint32_t time_to_clear_limits = 0;

        if(date.hour < 5) {
            time_to_clear_limits = HOURS_IN_TICKS(5) - now_time_in_ms;
        } else {
            time_to_clear_limits = HOURS_IN_TICKS(24 + 5) - now_time_in_ms;
        }

        furi_timer_start(dolphin->clear_limits_timer, time_to_clear_limits);
    }
}

int32_t dolphin_srv(void* p) {
    UNUSED(p);

    if(!furi_hal_is_normal_boot()) {
        FURI_LOG_W(TAG, "Skipping start in special boot mode");
        return 0;
    }

    Dolphin* dolphin = dolphin_alloc();
    furi_record_create(RECORD_DOLPHIN, dolphin);

    dolphin_state_load(dolphin->state);
    furi_timer_restart(dolphin->butthurt_timer, HOURS_IN_TICKS(2 * 24));
    dolphin_update_clear_limits_timer_period(dolphin);
    furi_timer_restart(dolphin->clear_limits_timer, HOURS_IN_TICKS(24));

    DolphinEvent event;
    while(1) {
        if(furi_message_queue_get(dolphin->event_queue, &event, HOURS_IN_TICKS(1)) ==
           FuriStatusOk) {
            if(event.type == DolphinEventTypeDeed) {
                dolphin_state_on_deed(dolphin->state, event.deed);
                DolphinPubsubEvent event = DolphinPubsubEventUpdate;
                furi_pubsub_publish(dolphin->pubsub, &event);
                furi_timer_restart(dolphin->butthurt_timer, HOURS_IN_TICKS(2 * 24));
                furi_timer_restart(dolphin->flush_timer, 30 * 1000);
            } else if(event.type == DolphinEventTypeStats) {
                event.stats->icounter = dolphin->state->data.icounter;
                event.stats->butthurt = dolphin->state->data.butthurt;
                event.stats->timestamp = dolphin->state->data.timestamp;
                event.stats->level = dolphin_get_level(dolphin->state->data.icounter);
                event.stats->level_up_is_pending =
                    !dolphin_state_xp_to_levelup(dolphin->state->data.icounter);
            } else if(event.type == DolphinEventTypeFlush) {
                FURI_LOG_I(TAG, "Flush stats");
                dolphin_state_save(dolphin->state);
            } else if(event.type == DolphinEventTypeClearLimits) {
                FURI_LOG_I(TAG, "Clear limits");
                dolphin_state_clear_limits(dolphin->state);
                dolphin_state_save(dolphin->state);
            } else if(event.type == DolphinEventTypeIncreaseButthurt) {
                FURI_LOG_I(TAG, "Increase butthurt");
                dolphin_state_butthurted(dolphin->state);
                dolphin_state_save(dolphin->state);
            }
            dolphin_event_release(dolphin, &event);
        } else {
            /* once per hour check rtc time is not changed */
            dolphin_update_clear_limits_timer_period(dolphin);
        }
    }

    furi_crash("That was unexpected");

    return 0;
}

void dolphin_upgrade_level(Dolphin* dolphin) {
    furi_check(dolphin);

    dolphin_state_increase_level(dolphin->state);
    dolphin_flush(dolphin);
}
