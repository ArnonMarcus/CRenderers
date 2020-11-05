#pragma once

#include "lib/core/types.h"
#include "lib/core/string.h"
#include "lib/memory/allocators.h"

GetTicks getTicks;

u64 ticks_per_second;
f64 seconds_per_tick,
    milliseconds_per_tick,
    microseconds_per_tick,
    nanoseconds_per_tick;

Timer render_timer,
      update_timer;

void initTimer(Timer *timer) {
    timer->delta_time = 0;
    timer->ticks_before = 0;
    timer->ticks_after = 0;
    timer->ticks_diff = 0;
    timer->accumulated_ticks = 0;
    timer->accumulated_frame_count = 0;
    timer->ticks_of_last_report = 0;
    timer->seconds = 0;
    timer->milliseconds = 0;
    timer->microseconds = 0;
    timer->nanoseconds = 0;
    timer->average_frames_per_tick = 0;
    timer->average_ticks_per_frame = 0;
    timer->average_frames_per_second = 0;
    timer->average_milliseconds_per_frame = 0;
    timer->average_microseconds_per_frame = 0;
    timer->average_nanoseconds_per_frame = 0;
}

void accumulateTimer(Timer* timer) {
    timer->ticks_diff = timer->ticks_after - timer->ticks_before;
    timer->accumulated_ticks += timer->ticks_diff;
    timer->accumulated_frame_count++;

    timer->seconds      = (u64)(seconds_per_tick      * (f64)(timer->ticks_diff));
    timer->milliseconds = (u64)(milliseconds_per_tick * (f64)(timer->ticks_diff));
    timer->microseconds = (u64)(microseconds_per_tick * (f64)(timer->ticks_diff));
    timer->nanoseconds  = (u64)(nanoseconds_per_tick  * (f64)(timer->ticks_diff));
}

void averageTimer(Timer *timer) {
    timer->average_frames_per_tick = (f64)timer->accumulated_frame_count / timer->accumulated_ticks;
    timer->average_ticks_per_frame = (f64)timer->accumulated_ticks / timer->accumulated_frame_count;
    timer->average_frames_per_second = (u16)(timer->average_frames_per_tick * ticks_per_second);
    timer->average_milliseconds_per_frame = (u16)(timer->average_ticks_per_frame * milliseconds_per_tick);
    timer->average_microseconds_per_frame = (u16)(timer->average_ticks_per_frame * microseconds_per_tick);
    timer->average_nanoseconds_per_frame = (u16)(timer->average_ticks_per_frame * nanoseconds_per_tick);
    timer->accumulated_ticks = timer->accumulated_frame_count = 0;
}

void initTimers(GetTicks platformGetTicks, u64 platformTicksPerSecond) {
    getTicks = platformGetTicks;
    ticks_per_second = platformTicksPerSecond;

    seconds_per_tick      = 1          / (f64)(ticks_per_second);
    milliseconds_per_tick = 1000       / (f64)(ticks_per_second);
    microseconds_per_tick = 1000000    / (f64)(ticks_per_second);
    nanoseconds_per_tick  = 1000000000 / (f64)(ticks_per_second);

    initTimer(&update_timer);
    initTimer(&render_timer);

    update_timer.ticks_before = update_timer.ticks_of_last_report = getTicks();
}

inline void perfStart(Timer* timer) {timer->ticks_before = getTicks();}
inline void perfEnd(Timer* timer) {
    timer->ticks_after = getTicks();
    accumulateTimer(timer);
    averageTimer(timer);
}

inline void startFrameTimer(Timer *timer) {
    timer->ticks_after = timer->ticks_before;
    timer->ticks_before = getTicks();
    timer->ticks_diff = timer->ticks_before - timer->ticks_after;
    timer->delta_time = (f32)(timer->ticks_diff * seconds_per_tick);
}

inline void endFrameTimer(Timer *timer) {
    timer->ticks_after = getTicks();
    accumulateTimer(timer);
    if (timer->accumulated_ticks >= ticks_per_second) averageTimer(timer);
}