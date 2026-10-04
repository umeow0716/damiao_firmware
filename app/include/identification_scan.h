#ifndef IDENTIFICATION_SCAN_H
#define IDENTIFICATION_SCAN_H
#include <stdbool.h>
#include <stdint.h>
/* All coordinates are raw output-side rad, including firmware direction. */
typedef struct {
    float config[10]; /* lower, upper, home, speed, accel, torque, temp, repeats, error, seconds */
    uint16_t configured, legs;
    uint8_t phase, reason;
    bool active, owned;
    uint32_t session, start_tick, end_tick;
    float planned, velocity, target;
} IdentificationScan;
void identification_scan_abort(IdentificationScan *scan, uint8_t reason);
bool identification_scan_start(IdentificationScan *scan, uint32_t sequence, uint32_t tick,
                                float position, float velocity, bool ready);
bool identification_scan_step(IdentificationScan *scan, uint32_t tick, float position,
    float velocity, float torque, float temperature, bool healthy, bool heartbeat);
#endif
