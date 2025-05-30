#pragma once
#include <furi.h>
#include <gui/gui.h>
#include <locale/locale.h>

#define FACE_TYPES 4
typedef enum {
    Rectangular = 0,
    Round,
    DigitalRectangular,
    DigitalRound,
} FaceType;

typedef struct {
    int8_t x;
    int8_t y;
} Point;

typedef struct {
    Point start;
    Point end;
} Line;

typedef struct {
    Line minutes[60];
    Point hours[12];
} ClockFace;

typedef struct {
    bool split;
    uint8_t width;
    uint8_t digits_mod;
    FaceType face_type;
    Point ofs;
    ClockFace face;
} ClockConfig;

void calc_clock_face(ClockConfig* cfg);
void draw_clock(Canvas* canvas, ClockConfig* cfg, DateTime* dt, int ms);

void init_clock_config(ClockConfig* cfg);
void modify_clock_up(ClockConfig* cfg);
void modify_clock_down(ClockConfig* cfg);
void modify_clock_left(ClockConfig* cfg);
void modify_clock_right(ClockConfig* cfg);
void modify_clock_ok(ClockConfig* cfg);
