#include "identification_scan.h"
#include <math.h>

void identification_scan_abort(IdentificationScan *s, uint8_t reason) {
    if (s->active && s->phase != 8U) { s->reason = reason; s->phase = 8U; }
}

bool identification_scan_start(IdentificationScan *s, uint32_t seq, uint32_t tick,
                                float q, float v, bool ready) {
    const float *c = s->config;
    for (unsigned i = 0; i < 10; ++i) if (!isfinite(c[i])) return false;
    if (!ready || s->active || s->configured != 0x3ffU ||
        !isfinite(q) || !isfinite(v) || fabsf(v) > 0.05f ||
        c[0] >= c[1] || c[3] <= 0 || c[4] <= 0 || c[5] <= 0 || c[6] <= 0 ||
        c[7] < 1 || c[7] > 100 || floorf(c[7]) != c[7] || c[8] <= 0 ||
        c[9] <= 0 || c[9] > 3600) return false;
    const float margin = c[8] + c[3] * 0.002f + c[3] * c[3] / (2 * c[4]);
    if (2 * margin >= c[1] - c[0] || c[2] < c[0] + margin || c[2] > c[1] - margin ||
        fabsf(q - c[2]) > c[8]) return false;
    s->planned = q; s->velocity = 0; s->target = c[0] + margin;
    s->session = seq; s->start_tick = tick; s->end_tick = 0; s->legs = 0;
    s->phase = 2; s->reason = 0; s->active = s->owned = true;
    return true;
}

bool identification_scan_step(IdentificationScan *s, uint32_t tick, float q,
    float v, float torque, float temperature, bool healthy, bool heartbeat) {
    if (!s->owned) return false;
    const float *c = s->config;
    if (s->active) {
        if (!healthy || !isfinite(q) || !isfinite(v) || !isfinite(torque) || !isfinite(temperature))
            identification_scan_abort(s, 2);
        else if (!heartbeat) identification_scan_abort(s, 3);
        else if (q <= c[0] || q >= c[1]) identification_scan_abort(s, 4);
        else if (fabsf(torque) > c[5] || temperature > c[6] || fabsf(v) > 1.5f*c[3] + 0.05f)
            identification_scan_abort(s, 5);
        else if (fabsf(q - s->planned) > c[8]) identification_scan_abort(s, 6);
        else if ((float)(uint32_t)(tick - s->start_tick) * 0.00005f > c[9]) identification_scan_abort(s, 7);
    }
    if (s->phase == 8) {
        const float step = c[4] * 0.001f;
        if (!healthy || !isfinite(q) || q <= c[0] || q >= c[1]) s->velocity = 0;
        else if (s->velocity > step) s->velocity -= step;
        else if (s->velocity < -step) s->velocity += step;
        else s->velocity = 0;
        if (s->velocity == 0) { s->active = false; s->phase = 7; s->end_tick = tick; }
        return true;
    }
    if (!s->active) { s->velocity = 0; return true; }
    const float remaining = s->target - s->planned;
    if (fabsf(remaining) < 0.0001f && fabsf(s->velocity) < c[4]*0.001f && fabsf(v) < 0.05f) {
        const float margin = c[8] + c[3]*0.002f + c[3]*c[3]/(2*c[4]);
        if (s->phase == 2) { s->phase = 3; s->target = c[1] - margin; }
        else if (s->phase == 3) { ++s->legs; s->phase = 4; s->target = c[0] + margin; }
        else if (s->phase == 4) {
            ++s->legs;
            if (s->legs >= (uint16_t)c[7]*2U) { s->phase = 5; s->target = c[2]; }
            else { s->phase = 3; s->target = c[1] - margin; }
        } else { s->active = false; s->phase = 6; s->end_tick = tick; s->velocity = 0; }
        return true;
    }
    float desired = sqrtf(2*c[4]*fabsf(remaining));
    if (desired > c[3]) desired = c[3];
    if (remaining < 0) desired = -desired;
    const float old = s->velocity, step = c[4]*0.001f;
    if (desired > old + step) desired = old + step;
    if (desired < old - step) desired = old - step;
    float next = s->planned + (old + desired)*0.0005f;
    if ((s->target - s->planned)*(s->target - next) <= 0) { next = s->target; }
    s->planned = next; s->velocity = desired;
    return true;
}
