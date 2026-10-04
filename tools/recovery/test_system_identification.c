/* Standalone host check. Not linked into firmware or invoked by make. */
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

/* White-box boundary tests also exercise wrapping internal counters. */
#include "../../app/src/system_identification.c"

static bool ready = true;
static bool writable = true;
static unsigned int sent;
static unsigned int attempts;
static unsigned int lock_depth;
static bool in_control_tick;
static uint16_t last_id;
static uint8_t last_length;
static uint8_t last_packet[64];

uint32_t identification_lock(void)
{
    return lock_depth++;
}

void identification_unlock(uint32_t mask)
{
    assert(lock_depth == mask + 1U);
    lock_depth = mask;
}

bool identification_transport_ready(void)
{
    return ready;
}

bool identification_send(uint16_t id, const uint8_t *data, uint8_t length)
{
    assert(lock_depth == 0U);
    assert(!in_control_tick);
    assert(length == 8U || length == 64U);
    ++attempts;
    if (!writable)
        return false;
    ++sent;
    last_id = id;
    last_length = length;
    memcpy(last_packet, data, length);
    return true;
}

static IdentificationMeasurement measurement(void)
{
    return (IdentificationMeasurement){
        .position = 1.25f,
        .velocity = -2.0f,
        .iq = 3.0f,
        .torque_constant = 0.5f,
        .command_torque = -1.0f,
        .bus_voltage = 24.0f,
        .motor_temperature = 30.0f,
        .gear_ratio = 10.0f,
        .direction = 1.0f,
        .velocity_previous_weight = 0.8f,
        .current_limit = 5.0f,
        .position_tick = 7U,
        .node = 1U,
        .mode = 1U,
        .armed = true,
    };
}

static float get_float(const uint8_t *data)
{
    const uint32_t bits = get_u32(data);
    float value;
    memcpy(&value, &bits, sizeof(value));
    return value;
}

static void reset(void)
{
    memset(&identification, 0, sizeof(identification));
    ready = writable = true;
    sent = attempts = lock_depth = 0U;
    const IdentificationMeasurement m = measurement();
    identification_sample_tick(&m);
    identification.latest.position_tick=identification.tick;
    identification.velocity_tick=identification.tick;
}

static void request(uint8_t version, uint8_t op, uint8_t arg, uint32_t sequence)
{
    uint8_t packet[8] = {version, 1U, op, arg};
    put_u32(packet + 4U, sequence);
    identification_request_irq(packet, 1U);
    identification_service();
}

static void ticks(unsigned int count, IdentificationMeasurement *m, bool drain)
{
    for (unsigned int i = 0U; i < count; ++i)
    {
        m->velocity_updated = ((identification.tick + 1U) % 20U) == 0U;
        in_control_tick = true;
        identification_sample_tick(m);
        in_control_tick = false;
        if (drain)
            identification_service();
    }
}

static void test_packets_and_state(void)
{
    reset();
    request(2U, 1U, 0U, 0x12345678U);
    assert(last_length == 8U && last_packet[3] == 1U);
    assert(get_u32(last_packet + 4U) == 0x12345678U);
    request(1U, 1U, 4U, 0U);
    assert(last_packet[3] == 2U);
    ready = false;
    request(1U, 1U, 0U, 0U);
    assert(last_packet[3] == 4U);
    ready = true;
    identification.latest.torque_constant = NAN;
    request(1U, 1U, 0U, 0U);
    assert(last_packet[3] == 6U);
    identification.latest = measurement();
    request(1U, 1U, 0U, 0x12345678U);
    assert(last_packet[3] == 0U && identification.active);
    request(1U, 1U, 0U, 0U);
    assert(last_packet[3] == 3U);
    request(1U, 0x20U, 0U, 0U);
    assert(last_packet[3] == 5U); /* Stage-two operations unsupported. */
    identification_command_begin_irq();
    identification_command_applied_tick(false, 1U);
    assert(identification.applied_command == UINT32_MAX);
    identification_command_end_irq(true);
    identification_command_applied_tick(false, 1U);
    assert(identification.applied_command == 1U);
    IdentificationMeasurement m = measurement();
    m.current_limited = m.voltage_limited = true;
    ticks(40U, &m, true);
    assert(last_id == SYSID_SAMPLE_ID && last_length == 64U);
    assert(last_packet[0] == 1U && last_packet[1] == 0x90U);
    assert(last_packet[2] == 1U && last_packet[3] == 15U);
    assert(get_u32(last_packet + 4U) == 0U);
    assert(get_u32(last_packet + 8U) == 41U);
    assert(get_u32(last_packet + 12U) == 1U);
    assert(get_u32(last_packet + 44U) == 2U);
    assert(get_float(last_packet + 20U) == 1.25f);
    assert(get_float(last_packet + 24U) == -2.0f);
    assert(fabsf(get_float(last_packet + 28U) - 3.0f) < 1e-5f);
    assert(fabsf(get_float(last_packet + 32U) - 1.5f) < 1e-5f);
    assert(get_float(last_packet + 36U) == -1.0f);
    assert(get_float(last_packet + 40U) == 3.0f);
    assert(get_u32(last_packet + 52U) == 40U);
    assert(get_u32(last_packet + 56U) == 7U);
    assert(last_packet[60] == 40U && last_packet[61] == 0U);
    request(1U, 4U, 0U, 99U);
    assert(last_length == 64U && last_packet[1] == 0x81U);
    assert(get_u32(last_packet + 4U) == 99U);
    assert(get_u32(last_packet + 8U) == 0x12345678U);
    assert(get_float(last_packet + 32U) == 0.5f);
    identification_command_begin_irq();
    identification_command_end_irq(true);
    identification_command_applied_tick(false, 3U);
    assert(identification.applied_command == UINT32_MAX);
    identification_command_applied_tick(true, 3U);
    assert(identification.applied_command == 2U);
    identification_command_begin_irq();
    identification_command_end_irq(true);
    identification_command_applied_tick(false, 3U);
    assert(identification.applied_command == 2U);
    m.armed = false;
    m.fault = 8U;
    ticks(40U, &m, true);
    assert(identification.active); /* Capture cannot arm/disarm or clear fault. */
    assert(get_u32(last_packet + 12U) == UINT32_MAX);
    assert((last_packet[3] & 17U) == 16U && last_packet[63] == 8U);
    request(1U, 2U, 0U, 0U);
    assert(!identification.active && identification.stop_reason == 1U);
    request(1U, 2U, 0U, 0U);
    assert(last_packet[3] == 0U);
    request(1U, 3U, 1U, 0U);
    assert(last_packet[3] == 2U);
}

static void test_loss_and_wrap(void)
{
    reset();
    request(1U, 1U, 1U, 0U);
    IdentificationMeasurement m = measurement();
    writable = false;
    const unsigned int before = attempts;
    ticks(20U * 18U, &m, true);
    assert(identification.sample_sequence == 18U);
    assert(identification.dropped_samples == 2U);
    assert(identification.sample_head - identification.sample_tail == 16U);
    assert(attempts - before <= 20U * 18U); /* Never spins on TX busy. */
    assert(identification.tx_retries > 0U);
    writable = true;
    for (unsigned int i = 0U; i < 16U; ++i)
        identification_service();
    ticks(20U, &m, true);
    assert(get_u32(last_packet + 4U) == 18U);
    assert(get_u32(last_packet + 16U) == 2U);
    uint8_t packet[8] = {1U, 1U, 5U, 0U};
    for (unsigned int i = 0U; i < 6U; ++i)
        identification_request_irq(packet, 1U);
    assert(identification.dropped_requests == 2U);
    packet[1] = 2U;
    identification_request_irq(packet, 1U);
    assert(identification.dropped_requests == 2U);
    reset();
    identification.tick = UINT32_MAX - 9U;
    request(1U, 1U, 1U, 0U);
    identification.sample_sequence = UINT32_MAX;
    identification.sample_head = identification.sample_tail = UINT32_MAX - 3U;
    ticks(40U, &m, true);
    assert(get_u32(last_packet + 8U) == 30U);
    assert(get_u32(last_packet + 4U) == 0U);
    assert(identification.sample_sequence == 1U);
    identification.accepted_command = UINT32_MAX - 1U;
    identification_command_end_irq(true);
    assert(identification.accepted_command == 0U);
    ticks(HEARTBEAT_TICKS - 40U, &m, false);
    assert(!identification.active && identification.stop_reason == 2U);
    reset();
    request(1U, 1U, 1U, 0U);
    ticks(9000U, &m, true);
    request(1U, 3U, 0U, 0U);
    ticks(2000U, &m, true);
    assert(identification.active);
    m.iq = INFINITY;
    identification_sample_tick(&m);
    assert(!identification.active && identification.stop_reason == 3U);
    reset();
    request(1U, 1U, 1U, 0U);
    m = measurement();
    m.position = NAN;
    identification_sample_tick(&m);
    assert(!identification.active && identification.stop_reason == 3U);
    reset();
    request(1U, 1U, 1U, 0U);
    m = measurement();
    m.iq = 3e38f;
    ticks(20U, &m, false);
    assert(!identification.active && identification.stop_reason == 3U);
    reset();
    identification_position_updated_irq();
    assert(identification_position_tick() == identification.tick);
    identification_command_applied_tick(false, 0U);
    assert(identification.applied_command == UINT32_MAX);
    reset();
    request(1U, 1U, 1U, 0U);
    m = measurement();
    m.gear_ratio = 11.0f;
    identification_sample_tick(&m);
    assert(!identification.active && identification.stop_reason == 3U);
}

static double measured_gain(double frequency, unsigned int period)
{
    reset();
    request(1U, 1U, period == 40U ? 0U : 1U, 0U);
    IdentificationMeasurement m = measurement();
    double sum_cos = 0.0, sum_sin = 0.0;
    unsigned int outputs = 0U;
    const double pi = acos(-1.0);
    for (unsigned int i = 1U; i <= 40000U; ++i)
    {
        m.iq = (float)cos(2.0 * pi * frequency * i / 20000.0);
        m.velocity_updated = i % 20U == 0U;
        identification.heartbeat_tick = identification.tick;
        in_control_tick = true;
        identification_sample_tick(&m);
        in_control_tick = false;
        if (i % period == 0U)
        {
            identification_service();
            if (i > 20000U)
            {
                const double y = get_float(last_packet + 28U);
                sum_cos += y * cos(2.0 * pi * frequency * i / 20000.0);
                sum_sin += y * sin(2.0 * pi * frequency * i / 20000.0);
                ++outputs;
            }
        }
    }
    return 2.0 * hypot(sum_cos, sum_sin) / outputs;
}

static void test_antialias(void)
{
    const double pi = acos(-1.0);
    const double beta = FILTER_NEW_WEIGHT;
    const unsigned int periods[] = {20U, 40U};
    const double frequencies[] = {20.0, 1200.0};
    for (unsigned int p = 0U; p < 2U; ++p)
        for (unsigned int f = 0U; f < 2U; ++f)
        {
            const double w = 2.0 * pi * frequencies[f] / 20000.0;
            const double pole_gain =
                beta / sqrt(1.0 + (1.0 - beta) * (1.0 - beta) - 2.0 * (1.0 - beta) * cos(w));
            const double box_gain = fabs(sin(periods[p] * w / 2.0) / (periods[p] * sin(w / 2.0)));
            const double expected = pow(pole_gain, 4.0) * box_gain;
            const double actual = measured_gain(frequencies[f], periods[p]);
            assert(fabs(actual - expected) < 2e-5 + expected * 0.001);
            printf("anti-alias %u Hz, input %.0f Hz: gain %.8g (model %.8g)\n", 20000U / periods[p],
                   frequencies[f], actual, expected);
        }
}

static void test_scan_extension(void) {
    reset();
    const float values[10]={1.0f,1.5f,1.25f,.05f,.1f,2,70,2,.02f,120};
    for(unsigned i=0;i<10;++i) {
        uint32_t bits;memcpy(&bits,&values[i],4);
        request(1, (uint8_t)(6+i), 0, bits);
        assert(last_packet[3]==0);
    }
    IdentificationMeasurement m=measurement();m.mode=3;
    identification.latest=m;
    request(1,1,0,123);
    identification.warmup=2000;
    m.velocity=0;identification.latest=m;
    request(1,16,0,456);assert(last_packet[3]==0);
    float command=0;
    assert(identification_scan_control_tick(1.25f,0,0,25,true,&command));
    ticks(40,&m,false);
    identification_service();assert(last_id==SYSID_SAMPLE_ID && identification.base_sent);
    const uint32_t sequence=get_u32(last_packet+4), tick=get_u32(last_packet+8);
    identification_service();assert(last_id==0x6F3 && !identification.base_sent);
    assert(last_packet[1]==0x91 && get_u32(last_packet+4)==sequence && get_u32(last_packet+8)==tick);
    assert(get_u32(last_packet+12)==123 && get_u32(last_packet+16)==456);
    request(1,17,0,789);assert(last_packet[1]==0x83 && get_u32(last_packet+4)==789);
    identification_command_begin_irq();identification_command_end_irq(true);
    assert(!identification.scan.owned && !identification.scan.active && identification.scan.reason==8);
}

static void test_reliable_retention(void)
{
    reset();
    request(1, SYSID_CAPTURE_START, 2, 0x12345678U);
    assert(identification.reliable);
    IdentificationMeasurement m = measurement();
    ticks(40U * 3U, &m, false);
    for (unsigned i=0;i<3;++i) identification_service();
    assert(identification.sample_tail == 0 && identification.sent_high == 3);
    uint8_t ack[8] = {1,1,SYSID_DATA_ACK,0};
    put_u32(ack+4, identification.session ^ 4U);
    assert(process_request(ack)==SYSID_BAD_ARGUMENT); /* no ACK of unsent data */
    put_u32(ack+4, identification.session ^ 2U);
    assert(process_request(ack)==SYSID_ACCEPTED && identification.sample_tail==2);
    assert(process_request(ack)==SYSID_ACCEPTED && identification.sample_tail==2);
    uint8_t replay[8] = {1,1,SYSID_DATA_REPLAY,0};
    put_u32(replay+4, identification.session ^ 2U);
    assert(process_request(replay)==SYSID_ACCEPTED);
    identification_service(); assert(get_u32(last_packet+4)==2);
    put_u32(ack+4, identification.session ^ 3U);
    assert(process_request(ack)==SYSID_ACCEPTED && identification.sample_tail==3);
    put_u32(replay+4, identification.session ^ 1U);
    assert(process_request(replay)==SYSID_BAD_ARGUMENT); /* already acknowledged */
    reset(); identification.scan.configured=0x3ff; identification.scan.phase=1;
    request(1,SYSID_CAPTURE_START,2,123);
    ticks(40,&m,false); identification_service();
    uint8_t original[64]; memcpy(original,last_packet,64);
    assert(identification.base_sent && identification.sent_high==0);
    put_u32(ack+4,identification.session^1U);
    assert(process_request(ack)==SYSID_BAD_ARGUMENT); /* half pair is not ACKable */
    identification_service(); assert(last_id==0x6F3 && identification.sent_high==1);
    assert(identification.sample_tail==0);
    identification.tick+=400; identification_service();
    assert(last_id==SYSID_SAMPLE_ID && memcmp(original,last_packet,64)==0);
    identification_service();assert(last_id==0x6F3);
    assert(process_request(ack)==SYSID_ACCEPTED && identification.sample_tail==1);
    reset(); request(1,SYSID_CAPTURE_START,3,77);
    writable=false; ticks(20U*128U,&m,false);
    assert(!identification.active && identification.stop_reason==4);
    assert(identification.sample_sequence==127 && identification.dropped_samples==0);
    assert(identification.sample_head-identification.sample_tail==127);
    writable=true; for(unsigned i=0;i<127;++i) identification_service();
    assert(identification.sample_tail==0 && identification.sent_high==127);
}

static void print_fixture(bool velocity)
{
    reset();
    identification.tick = 0U;
    request(1U, 1U, 0U, 0x12345678U);
    identification_command_begin_irq();
    identification_command_end_irq(true);
    identification_command_applied_tick(velocity, velocity?3U:1U);
    IdentificationMeasurement m = measurement();
    if (velocity) { m.mode=3; m.applied_velocity_setpoint=-.06f; }
    m.position_tick = 39U;
    ticks(40U, &m, true);
    for (unsigned int i = 0U; i < 64U; ++i)
        printf("%02X%s", last_packet[i], i == 63U ? "\n" : " ");
}

static void wide_guard(void) {
    identification.guard_enabled=true;
    identification.guard[0]=-12.5f;identification.guard[1]=12.5f;
    identification.guard[2]=30.f;identification.guard[3]=50.f;
}
static void guard_field(uint8_t op,float value) {
    uint32_t bits;memcpy(&bits,&value,4);request(1,op,0,bits);
    assert(last_packet[3]==SYSID_ACCEPTED);
}
static void test_guard_envelope(void) {
    reset();wide_guard();
    guard_field(SYSID_GUARD_LOWER,-2);guard_field(SYSID_GUARD_UPPER,2);
    guard_field(SYSID_GUARD_SPEED,3);guard_field(SYSID_GUARD_TORQUE,2);
    assert(identification.guard[3]==50); /* Staging never relaxes active limits. */
    request(1,SYSID_GUARD_ARM,0,123);assert(last_packet[3]==SYSID_ACCEPTED);
    assert(identification.guard[0]==-2 && identification.guard[3]==2);
    assert(identification_scan_current_limit(20,.5f)==4);
    guard_field(SYSID_GUARD_LOWER,2);guard_field(SYSID_GUARD_UPPER,-2);
    guard_field(SYSID_GUARD_SPEED,3);guard_field(SYSID_GUARD_TORQUE,2);
    request(1,SYSID_GUARD_ARM,0,124);assert(last_packet[3]==SYSID_BAD_ARGUMENT);
    assert(identification.guard[0]==-2);
    for (unsigned reason=3;reason<=8;++reason) {
        reset();wide_guard();identification_command_end_irq(true);
        switch(reason) {
        case 3:identification.latest.position=13;break;
        case 4:identification.latest.velocity=31;break;
        case 5:identification.latest.iq=101;break;
        case 6:identification.latest.mos_temperature=71;break;
        case 7:identification.latest.velocity=NAN;break;
        case 8:identification.latest.position_tick=identification.tick-401U;break;
        }
        assert(identification_deadman_check(true));assert(identification.guard_reason==reason);
        assert(identification_scan_current_limit(20,.5f)==0);
        identification_command_end_irq(true);assert(identification_deadman_check(true));
    }
    reset();wide_guard();identification_command_end_irq(true);
    identification.latest.position_tick=identification.velocity_tick=identification.tick-400U;
    assert(!identification_deadman_check(true)); /* Exact nominal 20 ms boundary. */
    ++identification.tick;assert(identification_deadman_check(true) && identification.guard_reason==8);
    reset();wide_guard();identification_command_end_irq(true);
    identification.velocity_tick=identification.tick-401U;
    assert(identification_deadman_check(true) && identification.guard_reason==8);
    reset();wide_guard();
    guard_field(SYSID_GUARD_LOWER,-2);guard_field(SYSID_GUARD_UPPER,2);
    guard_field(SYSID_GUARD_SPEED,3);guard_field(SYSID_GUARD_TORQUE,2);
    identification.latest.position_tick=identification.tick-401U;
    request(1,SYSID_GUARD_ARM,0,125);assert(last_packet[3]==SYSID_BAD_ARGUMENT);
    assert(identification.guard[3]==50);
    puts("guard: atomic staging/readback, limits, torque cap, MOS/NaN/stale publication and latch passed");
}

static void test_deadman_and_live(void) {
    reset();assert(!identification_deadman_check(false));
    assert(identification_deadman_check(true)); /* Enable without a new setpoint. */
    reset();wide_guard();identification_command_end_irq(true);
    uint32_t start=identification.tick;
    identification.tick=start+4999;identification.latest.position_tick=identification.velocity_tick=identification.tick;assert(!identification_deadman_check(true));
    request(1,SYSID_CAPTURE_HEARTBEAT,0,123);
    identification.tick=start+5000;assert(identification_deadman_check(true));
    identification_command_end_irq(true);assert(identification_deadman_check(true));
    assert(identification_scan_current_limit(5,.5f)==0);
    identification_deadman_reset();assert(identification_deadman_check(true));
    identification_deadman_reset();wide_guard();identification_command_end_irq(true);
    assert(!identification_deadman_check(true));
    reset();wide_guard();identification.tick=UINT32_MAX-50U;identification_command_end_irq(true);
    identification.tick+=5000;assert(identification_deadman_check(true));
    reset();wide_guard();identification.scan.owned=true;identification.scan.active=true;
    identification.scan.phase=3;identification.active=true;identification.heartbeat_tick=identification.tick;
    assert(!identification_deadman_check(true));identification.tick+=5000;
    assert(identification_deadman_check(true) && identification.scan.reason==10);
    reset();wide_guard();identification.scan.owned=true;identification.scan.phase=6;
    identification.active=true;identification.heartbeat_tick=identification.tick;
    assert(!identification_deadman_check(true)); /* Normal completed home wait. */
    reset();request(1,SYSID_SAFETY_STATUS,0,999);
    assert(last_id==SYSID_RESPONSE_ID && last_packet[1]==0x84 && last_packet[3]==17);
    assert(get_u32(last_packet+12)==5000 && get_u32(last_packet+16)==40);
    request(1,SYSID_LIVE_START,0,777);assert(identification.live_active);
    writable=false;IdentificationMeasurement m=measurement();ticks(120,&m,true);
    assert(identification.live_pending && identification.live_overwritten>=2);
    assert(get_u32(identification.live_packet+4)==777 && get_u32(identification.live_packet+8)==identification.tick/40*40);
    assert(get_u32(identification.live_packet+16)==identification.live_overwritten);
    writable=true;identification_service();assert(!identification.live_pending && last_id==LIVE_ID);
    request(1,SYSID_LIVE_STOP,0,778);assert(identification.live_active);
    request(1,SYSID_LIVE_STOP,0,777);assert(!identification.live_active);
    reset();request(1,SYSID_LIVE_START,0,777);ticks(10001,&m,false);
    assert(!identification.live_active && !identification.live_pending);
    puts("deadman: no-setpoint/late/heartbeat/wrap/scan; live: overwrite, ownership, expiry passed");
}

static float velocity_field(const uint8_t *p) { float value;memcpy(&value,p,4);return value; }
static void test_applied_velocity_setpoint(void) {
    for (unsigned rate=0;rate<2;++rate) {
        reset(); IdentificationMeasurement m=measurement(); m.mode=3;
        m.applied_velocity_setpoint=-.06f;identification.latest=m;
        request(1,SYSID_CAPTURE_START,(uint8_t)rate,123);
        const unsigned period=rate?20:40;
        ticks(period,&m,false);identification_service();
        assert(last_packet[0]==2 && last_packet[1]==0x90);
        assert(velocity_field(last_packet+36)==-.06f && !(last_packet[3]&32));
        ticks(period/2,&m,false);m.applied_velocity_setpoint=-.04f;
        ticks(period/2,&m,false);identification_service();
        assert(last_packet[0]==2 && velocity_field(last_packet+36)==-.04f && (last_packet[3]&32));
        request(1,SYSID_LIVE_START,0,456);ticks(40,&m,true);
        assert(identification.live_packet[0]==2 && velocity_field(identification.live_packet+36)==-.04f);
    }
    puts("applied velocity: versioned VEL field, stable/mixed intervals at 500/1000 Hz and live passed");
}

int main(int argc, char **argv)
{
    if (argc == 2 && strcmp(argv[1], "--fixture") == 0)
    {
        print_fixture(false);
        return 0;
    }
    if (argc==2 && strcmp(argv[1],"--velocity-fixture")==0) { print_fixture(true);return 0; }
    reset();
    assert(identification_scan_current_limit(5,.5f)==5);
    identification.scan.owned=true; identification.scan.config[5]=1;
    assert(identification_scan_current_limit(5,.5f)==2);
    assert(identification_scan_current_limit(1,.5f)==1);
    assert(identification_scan_current_limit(5,0)==0);
    test_guard_envelope();
    test_deadman_and_live();
    test_reliable_retention();
    test_applied_velocity_setpoint();
    test_scan_extension();
    test_packets_and_state();
    test_loss_and_wrap();
    test_antialias();
    puts("system identification host checks passed");
    return 0;
}
