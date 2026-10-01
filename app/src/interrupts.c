#include "app_state.h"
#include "app_commands.h"
#include "can_protocol.h"
#include "debug_console.h"
#include "interrupts.h"

#include "factory_layout.h"
#include "parameter_protocol.h"
#include "platform.h"

#if defined(DAMIAO_DM4310)
#include "board_mcan.h"
#include "board_sampling_timer.h"
#define POST_MOTOR_STATE_CHANGE() motor_control_post_state_change()
#else
#define POST_MOTOR_STATE_CHANGE() (g_app.events.motor_state_changed = true)
#endif

/* These semantic names correspond to the five non-default vector entries in
 * the decrypted official APP. The IRQnn wrappers retain the HC32 vector ABI. */

void position_sensor_timer_irq(void)
{
    platform_ack_position_timer_irq();
}

void position_sensor_dma_irq(void)
{
#if defined(DAMIAO_DM4310)
    volatile uint32_t *const dma_count = platform_read_position_dma();
    platform_ack_position_dma_irq(dma_count != NULL, dma_count);
#else
    float rotor_position;
    float rotor_angle;
    float motor_output_position;
    uint16_t raw_position;
    volatile uint32_t *dma_count = NULL;
    const bool sample_ready = platform_read_position(
        &rotor_position, &rotor_angle, &motor_output_position, &raw_position);
    if (sample_ready) {
        g_app.rotor_position = rotor_position;
        g_app.rotor_angle = rotor_angle;
        g_app.motor_output_position = motor_output_position;
        g_app.raw_position = raw_position;
    }
    platform_ack_position_dma_irq(sample_ready, dma_count);
#endif
}

#if defined(DAMIAO_DM4310)
void dm4310_fault_monitor_state_step(void)
{
    (void)safety_update(&g_app.safety, &g_app.config,
                        &g_app.motor.feedback, g_app.motor.armed);
}

void dm4310_reset_control_state_step(void)
{
    motor_control_reset_dynamic_state(&g_app.motor);
}
#endif

void adc_foc_control_irq(void)
{
    AdcSample sample;
#if defined(DAMIAO_DM4310)
    Dm4310OuterLoopReferences references;
    dm4310_motor_control_begin_sample(&references);
    const volatile uint16_t *const raw = platform_read_adc_control();
    {
#else
    if (platform_read_adc(&sample)) {
#endif
#if defined(DAMIAO_DM4310)
        const float voltage_scale =
            dm4310_motor_control_fast_sample_prefix(&g_app.motor, &sample, raw,
                                                    &references);
        volatile struct Dm4310PositionSensorScratch *const scratch =
            platform_finish_adc_sensor_sample(&sample, raw);
#endif
        g_app.position = sample.analog_output_position;
#if !defined(DAMIAO_DM4310)
        g_app.velocity = sample.output_velocity;
#endif
#if defined(DAMIAO_DM4310)
        const float cleared = dm4310_motor_control_fast_transform(&g_app.motor, &g_app.config,
                                            &sample, voltage_scale, scratch, &references);
        platform_finish_adc_velocity_sample(&sample, scratch, cleared, &references);
        g_app.velocity = sample.output_velocity;
        CurrentController *const current_d =
            dm4310_motor_control_fast_prepare(&g_app.motor, &g_app.config,
                                              &sample, cleared, &references);
#else
        const PhaseDuty duty = motor_control_fast_step(
            &g_app.motor, &g_app.config, &sample);
#endif
#if !defined(DAMIAO_DM4310)
        if (g_app.motor.outer_loop_ran) {
            APP_DEFERRED_EVENTS.control_status_tick = true;
        }
#endif
#if defined(DAMIAO_DM4310)
        dm4310_fault_monitor_helper();
        /* The helper already publishes stopped state/event. Keep decoded
         * feedback in sync without rereading fixed fault or console words. */
        if ((uint32_t)g_app.safety.latched_fault > 7U) {
            g_app.motor.feedback.fault = g_app.safety.latched_fault;
        }
#else
        const MotorFault fault = safety_update(&g_app.safety, &g_app.config,
                                                &g_app.motor.feedback,
                                                g_app.motor.armed);
        PhaseDuty applied_duty = duty;
        if (fault != MOTOR_FAULT_NONE) {
            const bool was_armed = g_app.motor.armed;
            g_app.motor.feedback.fault = fault;
            /* The original fault monitor clears the run state and executes
             * reset_control_state in this same ADC interrupt. */
            motor_control_trip(&g_app.motor);
            if (was_armed) {
                POST_MOTOR_STATE_CHANGE();
            }
            applied_duty = (PhaseDuty){
                .a = 0.5f,
                .b = 0.5f,
                .c = 0.5f,
            };
        } else {
            g_app.motor.feedback.fault = g_app.motor.armed ?
                MOTOR_STATUS_ENABLED : MOTOR_FAULT_NONE;
        }
#endif
#if defined(DAMIAO_DM4310)
        /* Factory ordering is controller helpers, fault monitor, enabled
         * test, communication age/selective reset, vector limiting, inverse
         * Park and SVPWM. */
        dm4310_motor_control_fast_apply_state(&g_app.motor, &references);
        const AlphaBeta stationary_voltage =
            dm4310_motor_control_fast_finish(current_d, &references);
        dm4310_svpwm_helper(stationary_voltage.alpha, stationary_voltage.beta);
#else
        /* adc_foc_control_irq@0x1fff8136 always reaches the compare writer.
         * A disabled/faulted controller therefore writes neutral compare
         * values; it never disconnects TMR4 from the bridge pins. */
        platform_write_pwm(applied_duty);
#endif
    }
    platform_ack_adc_irq();
}

void mcan1_receive_irq(void)
{
    CanFrame received;
#if defined(DAMIAO_DM4310)
    Dm4310McanIrqReferences references;
    platform_begin_mcan_irq(&references);
#endif
    /* The factory handler consumes exactly one FIFO0 element selected by
     * RXF0S.F0GI, acknowledges it, then clears the interrupt status.  Any
     * queued successor retriggers IRQ003 instead of being drained here. */
#if defined(DAMIAO_DM4310)
    if (platform_mcan_receive_irq(&received, &references)) {
#else
    if (platform_mcan_receive(&received)) {
#endif
#if !defined(DAMIAO_DM4310)
        ParameterRuntime runtime = {
            .motor_position = g_app.rotor_position,
            .output_position = g_app.motor.feedback.position,
            .output_position_offset = g_app.motor.output_position_offset,
        };
        platform_commissioning_get_output_calibration(
            runtime.output_sensor_calibration);
#endif
#if defined(DAMIAO_DM4310)
        const bool command_frame =
            (received.id & 0xFFU) == references.node_id;
        const uint16_t routed_id = command_frame ? 0U :
            *references.received_id;
        const bool parameter_frame = !command_frame && routed_id == 0x7FFU;
        ParameterProtocolResult parameter = {0};
        if (parameter_frame) {
            dm4310_parameter_protocol_process_irq(
                &received, &parameter, &references);
        }
#else
        ParameterProtocolResult parameter;
        parameter_protocol_process(&received, &g_app.config, &g_app.motor,
                                   &runtime, &parameter);
#endif
        if (
#if defined(DAMIAO_DM4310)
            parameter_frame &&
#endif
            parameter.handled) {
#if !defined(DAMIAO_DM4310)
            if (parameter.filter_update_requested) {
                platform_update_mcan_node_filter(g_app.config.can_id);
            }
#endif
            if (parameter.response_ready) {
#if defined(DAMIAO_DM4310)
                if (parameter.response_prebuilt) {
                    platform_mcan_send_prebuilt_irq(
                        (uint16_t)parameter.response.id,
                        parameter.response.length, &references);
                } else
#endif
                {
                    platform_mcan_send(&parameter.response);
                }
            }
            if (parameter.bootloader_requested) {
                /* The original MCAN IRQ sends "Aupgrade", commits the boot
                 * record, waits 10 ms and resets before its normal ack. */
                platform_enter_bootloader_from_can();
                return;
            }
            if (parameter.store_requested) {
#if defined(DAMIAO_DM4310)
                /* The parser posts +0x0c before building the STORE reply. */
#else
                g_app.pending_store_response = parameter.response;
                g_app.pending_store_response_valid = true;
                g_app.events.save_parameters = true;
#endif
            }
            if (parameter.persist_requested) {
#if defined(DAMIAO_DM4310)
                /* Factory 0x1fff9744 writes a full word through retained
                 * status r10, after the legacy ID callback returns. */
                references.status[0x0CU / 4U] = 1U;
#else
                g_app.pending_store_response_valid = false;
                g_app.events.save_parameters = true;
#endif
            }
            if (parameter.transport_reconfigure_requested) {
#if defined(DAMIAO_DM4310)
                /* Factory 0x1fff968c rereads parser scratch after BLX send,
                 * including rejected/unchanged writes. */
                if (*(volatile const uint8_t *)(uintptr_t)
                        FACTORY_SRAM_ADDRESS(UINT32_C(0x1fffcc6b), UINT32_C(0x1fffcbf7)) == 0x23U) {
                    platform_reconfigure_mcan_irq(&references);
                }
#else
                g_app.events.reconfigure_mcan = true;
#endif
            }
        }
#if defined(DAMIAO_DM4310)
        else if (command_frame) {
#else
        else {
#endif
#if defined(DAMIAO_DM4310)
            /* SETPOINT fully initializes this output; other kinds don't
             * consume it. No factory read of the decoded command mirror. */
            MotorCommand command;
#else
            MotorCommand command = g_app.motor.command;
#endif
#if defined(DAMIAO_DM4310)
            const CanCommandKind kind = dm4310_can_protocol_decode_command_irq(
                &received, &command, &references);
#else
            const CanCommandKind kind = can_protocol_decode_command(
                &received, &g_app.config, &command);
#endif
            if (kind != CAN_COMMAND_NONE) {
#if !defined(DAMIAO_DM4310)
                safety_note_control_frame(&g_app.safety);
#endif
#if defined(DAMIAO_DM4310)
                dm4310_app_apply_can_command_irq(kind, &command, &references);
#else
                app_apply_can_command(kind, &command);
#endif
                CanFrame response;
#if defined(DAMIAO_DM4310)
                dm4310_can_protocol_encode_feedback_irq(&references,
                                                        &response);
                platform_mcan_send_prebuilt_irq((uint16_t)response.id, 8U,
                                                &references);
#else
                can_protocol_encode_feedback(&g_app.motor.feedback,
                                             &g_app.config, &response);
                platform_mcan_send(&response);
#endif
            }
        }
    }
#if defined(DAMIAO_DM4310)
    /* The platform tail performs the no-delay bit-23 callback, rereads IR
     * for bus-off, posts each factory error and then clears IR/NVIC. Do not
     * route its result through the parameter-write 5-ms reconfiguration. */
    (void)platform_ack_mcan_irq(&references);
#else
    const uint8_t can_error = platform_ack_mcan_irq();
    if (can_error != 0U) {
        if (can_error == 1U) {
            /* IR bit 23 dispatches through the currently selected classic
             * or FD configuration function before posting CAN Error 1. */
            platform_reconfigure_mcan();
        }
        APP_DEFERRED_EVENTS.can_error = can_error;
    }
#endif
}

#if defined(DAMIAO_DM4310)
__attribute__((noipa))
#endif
void debug_uart_receive_irq(void)
{
#if defined(DAMIAO_DM4310)
    const uint8_t *data;
    int16_t length;
    if (platform_begin_debug_uart_irq(&data, &length)) {
        debug_console_process_dma_frame(data, length);
        platform_rearm_debug_uart_irq();
    }
#else
    uint8_t byte;
    while (platform_debug_receive(&byte)) {
        debug_console_receive(byte);
    }
    debug_console_end_frame();
#endif
    platform_ack_debug_uart_irq();
}

#if defined(DAMIAO_DM4310)
#define DM4310_IRQ_SECTION(name) \
    __attribute__((section(name), aligned(2), used))
#else
#define DM4310_IRQ_SECTION(name)
#endif

DM4310_IRQ_SECTION(".dm4310_irq000")
void IRQ000_Handler(void) { position_sensor_timer_irq(); }
DM4310_IRQ_SECTION(".dm4310_irq001")
void IRQ001_Handler(void) { position_sensor_dma_irq(); }
DM4310_IRQ_SECTION(".dm4310_irq002")
void IRQ002_Handler(void) { adc_foc_control_irq(); }
DM4310_IRQ_SECTION(".dm4310_irq003")
void IRQ003_Handler(void) { mcan1_receive_irq(); }
#if defined(DAMIAO_DM4310)
__attribute__((section(".dm4310_irq004"), aligned(2), used))
#endif
void IRQ004_Handler(void) { debug_uart_receive_irq(); }
