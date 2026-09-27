#include "app_state.h"
#include "app_commands.h"
#include "can_protocol.h"
#include "debug_console.h"
#include "interrupts.h"
#include "parameter_protocol.h"
#include "platform.h"

/* These semantic names correspond to the five non-default vector entries in
 * the decrypted official APP. The IRQnn wrappers retain the HC32 vector ABI. */

void position_sensor_timer_irq(void)
{
    platform_ack_position_timer_irq();
}

void position_sensor_dma_irq(void)
{
    float rotor_position;
    float rotor_angle;
    float motor_output_position;
    uint16_t raw_position;
    if (platform_read_position(&rotor_position, &rotor_angle,
                               &motor_output_position, &raw_position)) {
        g_app.rotor_position = rotor_position;
        g_app.rotor_angle = rotor_angle;
        g_app.motor_output_position = motor_output_position;
        g_app.raw_position = raw_position;
    }
    platform_ack_position_dma_irq();
}

void adc_foc_control_irq(void)
{
    AdcSample sample;
    if (platform_read_adc(&sample)) {
        g_app.position = sample.analog_output_position;
        g_app.velocity = sample.output_velocity;
        const PhaseDuty duty = motor_control_fast_step(
            &g_app.motor, &g_app.config, &sample);
        if (g_app.motor.outer_loop_ran) {
            g_app.events.control_status_tick = true;
        }
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
                g_app.events.motor_state_changed = true;
            }
            applied_duty = (PhaseDuty){0.5f, 0.5f, 0.5f};
        } else {
            g_app.motor.feedback.fault = g_app.motor.armed ?
                MOTOR_STATUS_ENABLED : MOTOR_FAULT_NONE;
        }
        /* adc_foc_control_irq@0x1fff8136 always reaches the compare writer.
         * A disabled/faulted controller therefore writes neutral compare
         * values; it never disconnects TMR4 from the bridge pins. */
        platform_write_pwm(applied_duty);
    }
    platform_ack_adc_irq();
}

void mcan1_receive_irq(void)
{
    CanFrame received;
    while (platform_mcan_receive(&received)) {
        ParameterRuntime runtime = {
            .motor_position = g_app.rotor_position,
            .output_position = g_app.motor.feedback.position,
            .output_position_offset = g_app.motor.output_position_offset,
        };
        platform_commissioning_get_output_calibration(
            runtime.output_sensor_calibration);
        ParameterProtocolResult parameter;
        parameter_protocol_process(&received, &g_app.config, &g_app.motor,
                                   &runtime, &parameter);
        if (parameter.handled) {
            if (parameter.filter_update_requested) {
                platform_update_mcan_node_filter(g_app.config.can_id);
            }
            if (parameter.response_ready) {
                platform_mcan_send(&parameter.response);
            }
            if (parameter.bootloader_requested) {
                /* The original MCAN IRQ sends "Aupgrade", commits the boot
                 * record, waits 10 ms and resets before its normal ack. */
                platform_enter_bootloader_from_can();
                return;
            }
            if (parameter.store_requested) {
                g_app.pending_store_response = parameter.response;
                g_app.pending_store_response_valid = true;
                g_app.events.save_parameters = true;
            }
            if (parameter.persist_requested) {
                g_app.pending_store_response_valid = false;
                g_app.events.save_parameters = true;
            }
            if (parameter.transport_reconfigure_requested) {
                g_app.events.reconfigure_mcan = true;
            }
            continue;
        }
        MotorCommand command = g_app.motor.command;
        const CanCommandKind kind = can_protocol_decode_command(
            &received, &g_app.config, &command);
        if (kind != CAN_COMMAND_NONE) {
            safety_note_control_frame(&g_app.safety);
            app_apply_can_command(kind, &command);
            CanFrame response;
            can_protocol_encode_feedback(&g_app.motor.feedback,
                                         &g_app.config, &response);
            platform_mcan_send(&response);
        }
    }
    const uint8_t can_error = platform_ack_mcan_irq();
    if (can_error != 0U) {
        g_app.events.can_error = can_error;
    }
}

void debug_uart_receive_irq(void)
{
    uint8_t byte;
    while (platform_debug_receive(&byte)) {
        debug_console_receive(byte);
    }
    debug_console_end_frame();
    platform_ack_debug_uart_irq();
}

void IRQ000_Handler(void) { position_sensor_timer_irq(); }
void IRQ001_Handler(void) { position_sensor_dma_irq(); }
void IRQ002_Handler(void) { adc_foc_control_irq(); }
void IRQ003_Handler(void) { mcan1_receive_irq(); }
void IRQ004_Handler(void) { debug_uart_receive_irq(); }
