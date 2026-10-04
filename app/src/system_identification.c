#include "system_identification.h"
#include "motor_types.h"

#include <string.h>
#include <math.h>

#define SAMPLE_QUEUE_SIZE 128U
#define LEGACY_QUEUE_SIZE 16U
#define REQUEST_QUEUE_SIZE 4U
#define HEARTBEAT_TICKS 10000U
#define FILTER_NEW_WEIGHT 0.015465039f
#define FILTER_WARMUP_TICKS 2000U
#define COMMAND_LEASE_TICKS 5000U /* 250 ms, independent of persisted TIMEOUT. */
#define LIVE_ID 0x6E0U /* Higher arbitration priority than recording/backfill. */

typedef struct
{
    uint8_t data[8];
} IdentificationRequest;

typedef struct
{
    volatile uint32_t tick;
    volatile uint32_t accepted_command;
    volatile uint32_t applied_command;
    uint32_t applied_tick;
    uint8_t applied_mode;
    volatile bool command_writing;
    volatile bool active;
    volatile uint32_t command_tick;
    volatile bool command_seen, deadman_latched;
    volatile bool live_active, live_pending;
    bool guard_enabled;
    uint8_t guard_configured, guard_reason;
    float guard_pending[4], guard[4];
    uint32_t live_session, live_overwritten;
    uint8_t live_packet[64];
    volatile uint32_t heartbeat_tick;
    volatile uint32_t sample_head;
    volatile uint32_t sample_tail;
    volatile uint32_t request_head;
    volatile uint32_t request_tail;
    uint8_t samples[SAMPLE_QUEUE_SIZE][64];
    uint8_t scan_samples[SAMPLE_QUEUE_SIZE][64];
    bool base_sent, reliable;
    uint32_t send_cursor, sent_high, replay_tick;
    IdentificationScan scan;
    IdentificationRequest requests[REQUEST_QUEUE_SIZE];
    uint32_t sample_sequence;
    uint32_t dropped_samples;
    uint32_t dropped_requests;
    uint32_t tx_retries;
    uint32_t session;
    uint32_t velocity_tick;
    volatile uint32_t position_tick;
    uint32_t warmup;
    uint16_t period;
    uint16_t count;
    uint8_t interval_flags;
    uint8_t stop_reason;
    float filter[4];
    float iq_sum;
    float velocity_sum;
    float interval_velocity_setpoint;
    float session_torque_constant;
    float session_gear_ratio;
    float session_direction;
    uint8_t velocity_count;
    /* The board sender copies word-sized chunks from this buffer. */
    _Alignas(uint32_t) uint8_t reply[64];
    uint8_t reply_length;
    IdentificationMeasurement latest;
} IdentificationState;

static IdentificationState identification;

static uint32_t get_u32(const uint8_t *data)
{
    return (uint32_t)data[0] | ((uint32_t)data[1] << 8U) | ((uint32_t)data[2] << 16U) |
           ((uint32_t)data[3] << 24U);
}

static void put_u32(uint8_t *data, uint32_t value)
{
    for (unsigned int i = 0U; i < 4U; ++i)
        data[i] = (uint8_t)(value >> (8U * i));
}

static void put_float(uint8_t *data, float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    put_u32(data, bits);
}

void identification_request_irq(const uint8_t data[8], uint8_t node)
{
    if (data[1] != node || node == 0U || node > 8U)
        return;
    const uint32_t head = identification.request_head;
    if (head - identification.request_tail == REQUEST_QUEUE_SIZE)
    {
        ++identification.dropped_requests;
        return;
    }
    memcpy(identification.requests[head % REQUEST_QUEUE_SIZE].data, data, 8U);
    __asm volatile("" ::: "memory");
    identification.request_head = head + 1U;
}

void identification_command_begin_irq(void)
{
    identification.command_writing = true;
}

void identification_command_end_irq(bool accepted)
{
    if (accepted)
    {
        identification.command_tick = identification.tick;
        identification.command_seen = true;
        if (identification.scan.owned) {
            identification_scan_abort(&identification.scan, 8U);
            identification.scan.active = identification.scan.owned = false;
            identification.scan.phase = 7U;
            identification.scan.end_tick = identification.tick;
        }
        ++identification.accepted_command;
        if (identification.accepted_command == UINT32_MAX)
            identification.accepted_command = 0U;
    }
    identification.command_writing = false;
}

void identification_command_applied_tick(bool outer_loop_due, uint8_t mode)
{
    const uint32_t previous = identification.applied_command;
    if (identification.command_writing || mode < MOTOR_MODE_MIT || mode > MOTOR_MODE_HYBRID)
        identification.applied_command = UINT32_MAX;
    else if (mode == MOTOR_MODE_MIT || outer_loop_due)
        identification.applied_command = identification.accepted_command;
    else if (mode != identification.applied_mode)
        identification.applied_command = UINT32_MAX;
    if (previous != identification.applied_command || mode != identification.applied_mode)
        identification.applied_tick = identification.tick + 1U;
    identification.applied_mode = mode;
}

void identification_position_updated_irq(void)
{
    identification.position_tick = identification.tick;
}

uint32_t identification_position_tick(void)
{
    return identification.position_tick;
}

float identification_scan_current_limit(float hardware_limit, float torque_constant) {
    if (identification.deadman_latched) return 0.0f;
    if (!isfinite(hardware_limit) || hardware_limit <= 0.0f || !isfinite(torque_constant) || torque_constant <= 0.0f) return 0.0f;
    float torque=identification.guard_enabled?identification.guard[3]:hardware_limit*torque_constant;
    if (identification.scan.owned) torque=fminf(torque,identification.scan.config[5]);
    const float limit = torque / torque_constant;
    return isfinite(limit) && limit > 0.0f ? fminf(hardware_limit, limit) : 0.0f;
}

bool identification_deadman_check(bool armed) {
    IdentificationState *s=&identification;
    if (!armed) return s->deadman_latched;
    const bool valid=s->scan.owned
        ? s->scan.phase!=7U && s->active && (uint32_t)(s->tick-s->heartbeat_tick)<COMMAND_LEASE_TICKS
        : s->command_seen && (uint32_t)(s->tick-s->command_tick)<COMMAND_LEASE_TICKS;
    const IdentificationMeasurement *m=&s->latest;
    uint8_t reason=0;
    if (!s->guard_enabled) reason=1;
    else if (!valid) reason=2;
    else if (!isfinite(m->position)||!isfinite(m->velocity)||!isfinite(m->iq)||!isfinite(m->torque_constant)||!isfinite(m->motor_temperature)||!isfinite(m->mos_temperature)||m->torque_constant<=0) reason=7;
    else if (m->position<s->guard[0] || m->position>s->guard[1]) reason=3;
    else if (fabsf(m->velocity)>s->guard[2]) reason=4;
    else if (fabsf(m->iq*m->torque_constant)>s->guard[3]) reason=5;
    else if (m->motor_temperature>70.f || m->mos_temperature>70.f) reason=6;
    else if ((uint32_t)(s->tick-m->position_tick)>400U || (uint32_t)(s->tick-s->velocity_tick)>400U) reason=8;
    if (reason) {
        if (!s->deadman_latched) s->guard_reason=reason;
        s->deadman_latched=true;
        identification_scan_abort(&s->scan,10U);
    }
    return s->deadman_latched;
}

void identification_deadman_reset(void) {
    identification.deadman_latched=false;
    identification.guard_reason=0;
    identification.command_seen=false; /* Reset alone cannot revive an old command. */
}

static void build_live_packet(const IdentificationMeasurement *m) {
    IdentificationState *s=&identification;uint8_t *p=s->live_packet;
    if (s->live_pending) ++s->live_overwritten;
    memset(p,0,64);p[0]=m->mode==3U?2U:1U;p[1]=0x92;p[2]=m->node;
    p[3]=(m->armed?1U:0U)|(m->current_limited?2U:0U)|(m->voltage_limited?4U:0U)|
        (!m->armed || s->applied_command==UINT32_MAX?16U:0U);
    put_u32(p+4,s->live_session);put_u32(p+8,s->tick);
    put_u32(p+12,m->armed?s->applied_command:UINT32_MAX);
    put_u32(p+16,s->live_overwritten);
    put_float(p+20,m->position);put_float(p+24,m->velocity);
    put_float(p+28,m->mos_temperature);put_float(p+32,m->iq*m->torque_constant);
    put_float(p+36,m->mode==3U?m->applied_velocity_setpoint:m->command_torque);put_float(p+40,m->iq);
    put_u32(p+44,s->applied_tick);put_float(p+48,m->motor_temperature);
    put_u32(p+52,s->velocity_tick);put_u32(p+56,m->position_tick);
    p[60]=40;p[62]=m->mode;p[63]=m->fault;s->live_pending=true;
}

bool identification_scan_keepalive(void) {
    return identification.scan.owned && identification.active &&
           (uint32_t)(identification.tick - identification.heartbeat_tick) < HEARTBEAT_TICKS;
}
bool identification_scan_control_tick(float q, float v, float torque, float temp,
                                      bool healthy, float *command) {
    IdentificationState *s = &identification;
    const bool owned = identification_scan_step(&s->scan, s->tick + 1U, q, v, torque,
                                                temp, healthy, identification_scan_keepalive());
    if (owned) *command = s->scan.velocity;
    return owned;
}
static void build_scan_packet(uint8_t p[64], uint8_t type, uint32_t sequence) {
    const IdentificationState *s = &identification;
    const IdentificationScan *scan = &s->scan;
    memset(p, 0, 64);
    p[0] = 1; p[1] = type; p[2] = s->latest.node; p[3] = scan->phase;
    put_u32(p+4, sequence); put_u32(p+8, s->tick); put_u32(p+12, s->session);
    put_u32(p+16, scan->session);
    p[20] = (uint8_t)scan->legs; p[21] = (uint8_t)(scan->legs >> 8);
    put_float(p+24, scan->planned); put_float(p+28, scan->velocity);
    put_float(p+32, s->latest.iq*s->latest.torque_constant);
    put_float(p+36, s->latest.position); put_float(p+40, scan->config[2]);
    put_float(p+44, scan->target); put_u32(p+48, scan->start_tick);
    put_u32(p+52, scan->end_tick);
    p[56] = scan->reason; p[57] = scan->owned; p[58] = scan->active;
}

void identification_sample_tick(const IdentificationMeasurement *measurement)
{
    IdentificationState *const state = &identification;
    ++state->tick;
    state->latest = *measurement;
    if (measurement->velocity_updated)
        state->velocity_tick = state->tick;
    if (state->live_active) {
        if ((uint32_t)(state->tick-state->heartbeat_tick)>=HEARTBEAT_TICKS)
            state->live_active=state->live_pending=false;
        else if (state->tick%40U==0U) build_live_packet(measurement);
    }
    if (!state->active)
        return;

    if (!isfinite(measurement->iq) || !isfinite(measurement->position) ||
        !isfinite(measurement->velocity) || !isfinite(measurement->torque_constant) ||
        !isfinite(measurement->command_torque) || !isfinite(measurement->bus_voltage) ||
        !isfinite(measurement->motor_temperature) ||
        (measurement->mode==3U && !isfinite(measurement->applied_velocity_setpoint)) ||
        measurement->torque_constant != state->session_torque_constant ||
        measurement->gear_ratio != state->session_gear_ratio ||
        measurement->direction != state->session_direction)
    {
        state->active = false;
        identification_scan_abort(&state->scan, 2U);
        state->stop_reason = 3U;
        return;
    }

    if ((uint32_t)(state->tick - state->heartbeat_tick) >= HEARTBEAT_TICKS)
    {
        state->active = false;
        identification_scan_abort(&state->scan, 3U);
        state->stop_reason = 2U;
        return;
    }
    float value = measurement->iq;
    if (state->warmup == 0U)
        for (unsigned int i = 0U; i < 4U; ++i)
            state->filter[i] = value;
    for (unsigned int i = 0U; i < 4U; ++i)
    {
        state->filter[i] += FILTER_NEW_WEIGHT * (value - state->filter[i]);
        value = state->filter[i];
    }
    if (state->warmup < FILTER_WARMUP_TICKS)
        ++state->warmup;
    state->iq_sum += value;
    if (measurement->velocity_updated)
    {
        state->velocity_sum += measurement->velocity;
        ++state->velocity_count;
    }
    if (!isfinite(state->iq_sum) || !isfinite(state->velocity_sum) ||
        !isfinite(value * measurement->torque_constant))
    {
        state->active = false;
        identification_scan_abort(&state->scan, 2U);
        state->stop_reason = 3U;
        return;
    }
    if (state->count==0U) state->interval_velocity_setpoint=measurement->applied_velocity_setpoint;
    else if (measurement->mode==3U && measurement->applied_velocity_setpoint!=state->interval_velocity_setpoint)
        state->interval_flags|=32U; /* Reject mixed velocity-reference intervals in fitting. */
    state->interval_flags |=
        (measurement->current_limited ? 2U : 0U) | (measurement->voltage_limited ? 4U : 0U);
    if (++state->count != state->period)
        return;

    if (state->reliable && state->sample_head - state->sample_tail >= SAMPLE_QUEUE_SIZE - 1U) {
        /* Stop before generating a sample that cannot be retained. Existing
         * samples stay available for ACK/replay after capture stops. */
        state->active = false;
        state->stop_reason = 4U;
        identification_scan_abort(&state->scan, 9U);
        return;
    }
    const uint32_t sequence = state->sample_sequence++;
    const uint32_t head = state->sample_head;
    if (head - state->sample_tail == (state->reliable ? SAMPLE_QUEUE_SIZE : LEGACY_QUEUE_SIZE))
        ++state->dropped_samples;
    else
    {
        uint8_t *const packet = state->samples[head % SAMPLE_QUEUE_SIZE];
        packet[0] = measurement->mode==3U?2U:SYSID_PROTOCOL_VERSION;
        packet[1] = 0x90U;
        packet[2] = measurement->node;
        packet[3] = state->interval_flags | (measurement->armed ? 1U : 0U) |
                    (state->warmup < FILTER_WARMUP_TICKS ? 8U : 0U) |
                    (!measurement->armed || state->applied_command == UINT32_MAX ? 16U : 0U);
        put_u32(packet + 4U, sequence);
        put_u32(packet + 8U, state->tick);
        put_u32(packet + 12U, measurement->armed ? state->applied_command : UINT32_MAX);
        put_u32(packet + 16U, state->dropped_samples);
        put_float(packet + 20U, measurement->position);
        const float velocity =
            state->velocity_count != 0U ? state->velocity_sum / (float)state->velocity_count : 0.0f;
        put_float(packet + 24U, velocity);
        const float iq = state->iq_sum / (float)state->period;
        put_float(packet + 28U, iq);
        put_float(packet + 32U, iq * measurement->torque_constant);
        put_float(packet + 36U, measurement->mode==3U?measurement->applied_velocity_setpoint:measurement->command_torque);
        put_float(packet + 40U, measurement->iq);
        put_u32(packet + 44U, state->applied_tick);
        put_float(packet + 48U, measurement->motor_temperature);
        put_u32(packet + 52U, state->velocity_tick);
        put_u32(packet + 56U, measurement->position_tick);
        packet[60] = (uint8_t)state->period;
        packet[61] = (uint8_t)(state->period >> 8U);
        packet[62] = measurement->mode;
        packet[63] = measurement->fault;
        __asm volatile("" ::: "memory");
        build_scan_packet(state->scan_samples[head % SAMPLE_QUEUE_SIZE], 0x91U, sequence);
        state->sample_head = head + 1U;
    }
    state->count = 0U;
    state->iq_sum = 0.0f;
    state->velocity_sum = 0.0f;
    state->velocity_count = 0U;
    state->interval_flags = 0U;
}

static uint8_t process_request(const uint8_t request[8])
{
    IdentificationState *const state = &identification;
    if (request[0] != SYSID_PROTOCOL_VERSION)
        return SYSID_BAD_VERSION;
    if (request[2] >= SYSID_CAPTURE_STOP && request[2] <= SYSID_STATUS && request[3] != 0U)
        return SYSID_BAD_ARGUMENT;
    if (request[2] >= SYSID_SCAN_LOWER && request[2] <= SYSID_SCAN_SECONDS) {
        if (request[3] != 0) return SYSID_BAD_ARGUMENT;
        if (state->active || state->scan.active) return SYSID_BUSY;
        float value; const uint32_t bits = get_u32(request+4);
        memcpy(&value, &bits, 4);
        if (!isfinite(value)) return SYSID_BAD_ARGUMENT;
        const unsigned field = request[2] - SYSID_SCAN_LOWER;
        if (field == 0U) state->scan.configured = 0U; /* begin a fresh complete configuration */
        state->scan.config[field] = value;
        state->scan.configured |= (uint16_t)(1U << field);
        state->scan.phase = 1;
        return SYSID_ACCEPTED;
    }
    if (request[2]>=SYSID_GUARD_LOWER && request[2]<=SYSID_GUARD_TORQUE) {
        if (request[3]!=0U) return SYSID_BAD_ARGUMENT;
        float value;uint32_t bits=get_u32(request+4);memcpy(&value,&bits,4);
        if (!isfinite(value)) return SYSID_BAD_ARGUMENT;
        unsigned field=request[2]-SYSID_GUARD_LOWER;
        if (!field) state->guard_configured=0;
        state->guard_pending[field]=value;state->guard_configured|=(uint8_t)(1U<<field);
        return SYSID_ACCEPTED;
    }
    switch (request[2])
    {
    case SYSID_CAPTURE_START: /* Passive capture only; never arms the motor. */
        if (state->active || state->scan.active)
            return SYSID_BUSY;
        if (request[3] > 3U)
            return SYSID_BAD_ARGUMENT;
        if (state->scan.phase == 1U && state->scan.configured != 0x3ffU) return SYSID_BAD_ARGUMENT;
        if (!identification_transport_ready())
            return SYSID_TRANSPORT_NOT_READY;
        if (!isfinite(state->latest.torque_constant) || state->latest.torque_constant <= 0.0f ||
            !isfinite(state->latest.gear_ratio) || state->latest.gear_ratio <= 0.0f)
            return SYSID_MEASUREMENTS_NOT_READY;
        /* A scan configuration is staged explicitly before capture. Ordinary
         * later captures must not inherit a previous completed scan stream. */
        if (state->scan.phase != 1U && !state->scan.owned) {
            state->scan.configured = 0;
            state->scan.phase = 0;
        }
        state->period = (request[3] & 1U) == 0U ? 40U : 20U;
        state->reliable = request[3] >= 2U;
        state->count = 0U;
        state->warmup = 0U;
        state->iq_sum = 0.0f;
        state->velocity_sum = 0.0f;
        state->velocity_count = 0U;
        state->interval_flags = 0U;
        state->sample_tail = state->sample_head;
        state->base_sent = false;
        state->send_cursor = state->sample_head;
        state->sent_high = 0U;
        state->replay_tick = state->tick;
        state->sample_sequence = 0U;
        state->dropped_samples = 0U;
        state->session = get_u32(request + 4U);
        state->session_torque_constant = state->latest.torque_constant;
        state->session_gear_ratio = state->latest.gear_ratio;
        state->session_direction = state->latest.direction;
        state->heartbeat_tick = state->tick;
        state->stop_reason = 0U;
        state->active = true;
        return SYSID_ACCEPTED;
    case SYSID_CAPTURE_STOP: /* Queued data is still drained. */
        identification_scan_abort(&state->scan, 1U);
        if (state->active || state->stop_reason == 0U) state->stop_reason = 1U;
        state->active = false;
        return SYSID_ACCEPTED;
    case SYSID_CAPTURE_HEARTBEAT:
        state->heartbeat_tick = state->tick;
        return SYSID_ACCEPTED;
    case SYSID_SAFETY_STATUS:
        return request[3]==0U && identification_transport_ready()?SYSID_ACCEPTED:SYSID_BAD_ARGUMENT;
    case SYSID_LIVE_START:
        if (request[3]!=0U || !identification_transport_ready()) return SYSID_BAD_ARGUMENT;
        if (state->live_active && state->live_session!=get_u32(request+4)) return SYSID_BUSY;
        state->live_session=get_u32(request+4);state->live_active=true;
        state->live_pending=false;state->heartbeat_tick=state->tick;
        return SYSID_ACCEPTED;
    case SYSID_LIVE_STOP:
        if (request[3]!=0U || state->live_session!=get_u32(request+4)) return SYSID_BAD_ARGUMENT;
        state->live_active=state->live_pending=false;return SYSID_ACCEPTED;
    case SYSID_GUARD_ARM: {
        const float *g=state->guard_pending;
        if (request[3]!=0 || state->guard_configured!=15 || state->deadman_latched ||
            !(g[0]<g[1]) || g[2]<=0 || g[3]<=0 || !isfinite(state->latest.position) ||
            state->latest.position<g[0] || state->latest.position>g[1] ||
            !isfinite(state->latest.velocity) || fabsf(state->latest.velocity)>g[2] || state->latest.fault>1 ||
            (uint32_t)(state->tick-state->latest.position_tick)>400U || (uint32_t)(state->tick-state->velocity_tick)>400U)
            return SYSID_BAD_ARGUMENT;
        memcpy(state->guard,g,sizeof(state->guard));state->guard_enabled=true;
        return SYSID_ACCEPTED;
    }
    case SYSID_DATA_ACK:
    case SYSID_DATA_REPLAY: {
        if (!state->reliable || request[3] != 0U) return SYSID_BAD_ARGUMENT;
        /* Session-bound cumulative count. A single writer owns this capture;
         * the random 32-bit session tag is an attribution fence, not auth. */
        const uint32_t count = get_u32(request + 4U) ^ state->session;
        const uint32_t oldest = state->sample_sequence - (state->sample_head - state->sample_tail);
        const uint32_t sent_count = state->sent_high;
        if (count > state->sample_sequence) return SYSID_BAD_ARGUMENT;
        if (request[2] == SYSID_DATA_ACK) {
            if (count < oldest) return SYSID_ACCEPTED; /* idempotent delayed ACK */
            if (count > sent_count) return SYSID_BAD_ARGUMENT;
            state->sample_tail += count - oldest;
            if (state->send_cursor < state->sample_tail) { state->send_cursor = state->sample_tail; state->base_sent = false; }
        } else {
            if (count < oldest || count > sent_count) return SYSID_BAD_ARGUMENT;
            state->send_cursor = state->sample_tail + count - oldest;
            state->base_sent = false;
            state->replay_tick = state->tick;
        }
        return SYSID_ACCEPTED;
    }
    case SYSID_SCAN_START:
        if (request[3] != 0) return SYSID_BAD_ARGUMENT;
        if (state->scan.active) return SYSID_BUSY;
        if (!identification_scan_start(&state->scan, get_u32(request+4), state->tick,
            state->latest.position, state->latest.velocity,
            state->active && state->warmup >= FILTER_WARMUP_TICKS && state->latest.armed &&
            state->latest.mode == MOTOR_MODE_SPEED && state->latest.fault <= 1 && state->latest.node <= 7)) return SYSID_BAD_ARGUMENT;
        return SYSID_ACCEPTED;
    case SYSID_SCAN_ABORT:
        if (request[3] != 0) return SYSID_BAD_ARGUMENT;
        identification_scan_abort(&state->scan, 1U);
        return SYSID_ACCEPTED;
    case SYSID_SCAN_STATUS:
        if (request[3] != 0) return SYSID_BAD_ARGUMENT;
        return identification_transport_ready() ? SYSID_ACCEPTED : SYSID_TRANSPORT_NOT_READY;
    case SYSID_INFO:
    case SYSID_STATUS:
        return identification_transport_ready() ? SYSID_ACCEPTED : SYSID_TRANSPORT_NOT_READY;
    default:
        return SYSID_UNSUPPORTED_OPERATION;
    }
}

static void build_information(uint8_t packet[64], uint8_t operation, uint32_t sequence)
{
    const IdentificationState *const state = &identification;
    memset(packet, 0, 64U);
    packet[0] = SYSID_PROTOCOL_VERSION;
    packet[1] = operation == SYSID_INFO ? 0x81U : 0x82U;
    packet[2] = state->latest.node;
    packet[3] = (uint8_t)state->active;
    put_u32(packet + 4U, sequence);
    put_u32(packet + 8U, state->session);
    put_u32(packet + 12U, state->tick);
    put_u32(packet + 16U, state->dropped_samples);
    put_u32(packet + 20U, state->dropped_requests);
    put_u32(packet + 24U, state->tx_retries);
    put_u32(packet + 28U, state->sample_sequence);
    put_float(packet + 32U, state->latest.torque_constant);
    put_float(packet + 36U, state->latest.gear_ratio);
    put_float(packet + 40U, state->latest.velocity_previous_weight);
    put_float(packet + 44U, FILTER_NEW_WEIGHT);
    put_float(packet + 48U, state->latest.current_limit);
    put_u32(packet + 52U, state->accepted_command);
    put_u32(packet + 56U, state->applied_command);
    packet[60] = (uint8_t)state->period;
    packet[61] = state->stop_reason;
    packet[62] = state->latest.mode;
    packet[63] = state->latest.fault;
}

void identification_service(void)
{
    /* One pending management response, one sample per main iteration. */
    IdentificationState *const state = &identification;
    uint8_t *const reply = state->reply;
    uint32_t mask = identification_lock();
    if (state->reply_length == 0U && state->request_head != state->request_tail)
    {
        const uint8_t *const request =
            state->requests[state->request_tail % REQUEST_QUEUE_SIZE].data;
        const uint8_t result = process_request(request);
        if (result == SYSID_ACCEPTED && request[2] == SYSID_SCAN_STATUS) {
            build_scan_packet(reply, 0x83U, get_u32(request+4));
            state->reply_length = 64;
        }
        else if (result==SYSID_ACCEPTED && request[2]==SYSID_SAFETY_STATUS) {
            memset(reply,0,64);reply[0]=1;reply[1]=0x84;reply[2]=state->latest.node;
            reply[3]=17U|(state->deadman_latched?2U:0U)|(state->live_active?4U:0U)|(state->guard_enabled?8U:0U);
            put_u32(reply+4,get_u32(request+4));put_u32(reply+8,state->tick);
            put_u32(reply+12,COMMAND_LEASE_TICKS);put_u32(reply+16,40U);
            put_u32(reply+20,state->command_tick);put_u32(reply+24,state->live_overwritten);
            for (unsigned k=0;k<4;++k) put_float(reply+28+4*k,state->guard[k]);
            reply[44]=state->guard_reason;
            state->reply_length=64U;
        }
        else if (result == SYSID_ACCEPTED && (request[2] == SYSID_INFO || request[2] == SYSID_STATUS))
        {
            build_information(reply, request[2], get_u32(request + 4U));
            state->reply_length = 64U;
        }
        else
        {
            reply[0] = SYSID_PROTOCOL_VERSION;
            reply[1] = 0x80U;
            reply[2] = request[2];
            reply[3] = result;
            memcpy(reply + 4U, request + 4U, 4U);
            state->reply_length = 8U;
        }
        ++state->request_tail;
    }
    identification_unlock(mask);
    /* Latest state gets a send opportunity before management/backfill. Never
     * retry an obsolete snapshot; producer overwrites only this live mailbox. */
    uint8_t live[64];uint32_t live_tick=0;bool live_ready;
    mask=identification_lock();live_ready=state->live_pending;
    if (live_ready) {memcpy(live,state->live_packet,64);live_tick=get_u32(live+8);}
    identification_unlock(mask);
    if (live_ready && identification_send(LIVE_ID,live,64)) {
        mask=identification_lock();
        if (get_u32(state->live_packet+8)==live_tick) state->live_pending=false;
        identification_unlock(mask);
    }
    if (state->reply_length != 0U)
    {
        if (!identification_send(SYSID_RESPONSE_ID, reply, state->reply_length))
        {
            ++state->tx_retries;
            return;
        }
        state->reply_length = 0U;
    }
    uint8_t packet[64];
    mask = identification_lock();
    if (state->reliable && state->send_cursor == state->sample_head &&
        state->sample_tail != state->sample_head && (uint32_t)(state->tick-state->replay_tick) >= 400U) {
        state->send_cursor = state->sample_tail;
        state->base_sent = false;
        state->replay_tick = state->tick;
    }
    const uint32_t tail = state->reliable ? state->send_cursor : state->sample_tail;
    const bool available = tail != state->sample_head;
    if (available)
        memcpy(packet, state->base_sent ? state->scan_samples[tail % SAMPLE_QUEUE_SIZE] : state->samples[tail % SAMPLE_QUEUE_SIZE], 64U);
    identification_unlock(mask);
    if (available)
    {
        if (identification_send(state->base_sent ? 0x6F3U : SYSID_SAMPLE_ID, packet, 64U)) {
            if (!state->base_sent && state->scan.configured != 0) state->base_sent = true;
            else { if (state->reliable) { state->send_cursor = tail + 1U;
                    uint32_t count = get_u32(packet + 4U) + 1U;
                    if (count > state->sent_high) state->sent_high = count; }
                   else { state->sample_tail = tail + 1U; }
                   state->base_sent = false; }
        }
        else
            ++state->tx_retries;
    }
}
