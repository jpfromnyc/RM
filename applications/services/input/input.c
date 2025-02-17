#include "input_i.h"

// #define INPUT_DEBUG

#define GPIO_Read(input_pin) (furi_hal_gpio_read(input_pin.pin->gpio) ^ (input_pin.pin->inverted))

static Input* input = NULL;

void input_press_timer_callback(void* arg) {
    InputPinState* input_pin = arg;
    InputEvent event;
    event.sequence_source = INPUT_SEQUENCE_SOURCE_HARDWARE;
    event.sequence_counter = input_pin->counter;
    event.key = input_pin->pin->key;
    input_pin->press_counter++;
    if(input_pin->press_counter == INPUT_LONG_PRESS_COUNTS) {
        event.type = InputTypeLong;
        furi_pubsub_publish(input->event_pubsub, &event);
    } else if(input_pin->press_counter > INPUT_LONG_PRESS_COUNTS) {
        input_pin->press_counter--;
        event.type = InputTypeRepeat;
        furi_pubsub_publish(input->event_pubsub, &event);
    }
}

void input_isr(void* _ctx) {
    UNUSED(_ctx);
    furi_thread_flags_set(input->thread_id, INPUT_THREAD_FLAG_ISR);
}

const char* input_get_key_name(InputKey key) {
    for(size_t i = 0; i < input_pins_count; i++) {
        if(input_pins[i].key == key) {
            return input_pins[i].name;
        }
    }
    return "Unknown";
}

const char* input_get_type_name(InputType type) {
    switch(type) {
    case InputTypePress:
        return "Press";
    case InputTypeRelease:
        return "Release";
    case InputTypeShort:
        return "Short";
    case InputTypeLong:
        return "Long";
    case InputTypeRepeat:
        return "Repeat";
    default:
        return "Unknown";
    }
}

#include <cli/cli_i.h>

static void input_cli_wrapper(Cli* cli, FuriString* args, void* context) {
    cli_plugin_wrapper("input_cli", 1, cli, args, context);
}

int32_t input_srv(void* p) {
    UNUSED(p);
    input = malloc(sizeof(Input));
    input->thread_id = furi_thread_get_current_id();
    input->event_pubsub = furi_pubsub_alloc();
    furi_record_create(RECORD_INPUT_EVENTS, input->event_pubsub);

#if INPUT_DEBUG
    furi_hal_gpio_init_simple(&gpio_ext_pa4, GpioModeOutputPushPull);
#endif

#ifdef SRV_CLI
    input->cli = furi_record_open(RECORD_CLI);
    cli_add_command(input->cli, "input", CliCommandFlagParallelSafe, input_cli_wrapper, input);
#else
    UNUSED(input_cli_wrapper);
#endif

    input->pin_states = malloc(input_pins_count * sizeof(InputPinState));

    for(size_t i = 0; i < input_pins_count; i++) {
        furi_hal_gpio_add_int_callback(input_pins[i].gpio, input_isr, NULL);
        input->pin_states[i].pin = &input_pins[i];
        input->pin_states[i].state = GPIO_Read(input->pin_states[i]);
        input->pin_states[i].debounce = INPUT_DEBOUNCE_TICKS_HALF;
        input->pin_states[i].press_timer = furi_timer_alloc(
            input_press_timer_callback, FuriTimerTypePeriodic, &input->pin_states[i]);
        input->pin_states[i].press_counter = 0;
    }

    while(1) {
        bool is_changing = false;
        for(size_t i = 0; i < input_pins_count; i++) {
            bool state = GPIO_Read(input->pin_states[i]);
            if(state) {
                if(input->pin_states[i].debounce < INPUT_DEBOUNCE_TICKS)
                    input->pin_states[i].debounce += 1;
            } else {
                if(input->pin_states[i].debounce > 0) input->pin_states[i].debounce -= 1;
            }

            if(input->pin_states[i].debounce > 0 &&
               input->pin_states[i].debounce < INPUT_DEBOUNCE_TICKS) {
                is_changing = true;
            } else if(input->pin_states[i].state != state) {
                input->pin_states[i].state = state;

                // Common state info
                InputEvent event;
                event.sequence_source = INPUT_SEQUENCE_SOURCE_HARDWARE;
                event.key = input->pin_states[i].pin->key;

                // Short / Long / Repeat timer routine
                if(state) {
                    input->counter++;
                    input->pin_states[i].counter = input->counter;
                    event.sequence_counter = input->pin_states[i].counter;
                    furi_timer_start(input->pin_states[i].press_timer, INPUT_PRESS_TICKS);
                } else {
                    event.sequence_counter = input->pin_states[i].counter;
                    furi_timer_stop(input->pin_states[i].press_timer);
                    while(furi_timer_is_running(input->pin_states[i].press_timer))
                        furi_delay_tick(1);
                    if(input->pin_states[i].press_counter < INPUT_LONG_PRESS_COUNTS) {
                        event.type = InputTypeShort;
                        furi_pubsub_publish(input->event_pubsub, &event);
                    }
                    input->pin_states[i].press_counter = 0;
                }

                // Send Press/Release event
                event.type = input->pin_states[i].state ? InputTypePress : InputTypeRelease;
                furi_pubsub_publish(input->event_pubsub, &event);
            }
        }

        if(is_changing) {
#if INPUT_DEBUG
            furi_hal_gpio_write(&gpio_ext_pa4, 1);
#endif
            furi_delay_tick(1);
        } else {
#if INPUT_DEBUG
            furi_hal_gpio_write(&gpio_ext_pa4, 0);
#endif
            furi_thread_flags_wait(INPUT_THREAD_FLAG_ISR, FuriFlagWaitAny, FuriWaitForever);
        }
    }

    return 0;
}
