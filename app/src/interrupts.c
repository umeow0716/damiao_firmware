#include "app_state.h"
#include "app_commands.h"
#include "can_protocol.h"
#include "debug_console.h"
#include "firmware_variant.h"
#include "interrupts.h"

#include "memory_layout.h"
#include "parameter_protocol.h"
#include "platform.h"

#include "board_mcan.h"
#include "board_sampling_timer.h"
#define POST_MOTOR_STATE_CHANGE() motor_control_post_state_change()

/* Semantic handlers for the five non-default vector entries. The IRQnn
 * wrappers retain the HC32 vector ABI. */

void position_sensor_timer_irq(void)
{
    platform_ack_position_timer_irq();
}

void position_sensor_dma_irq(void)
{
    volatile uint32_t *const dma_count = platform_read_position_dma();
    platform_ack_position_dma_irq(dma_count != NULL, dma_count);
}

void fault_monitor_state_step(void)
{
    (void)safety_update(&g_app.safety, &g_app.config, &g_app.motor.feedback, g_app.motor.armed);
}

void reset_control_state_step(void)
{
    motor_control_reset_dynamic_state(&g_app.motor);
}

void adc_foc_control_irq(void)
{
    AdcSample sample;
    OuterLoopContext references;
    motor_control_begin_sample(&references);
    const volatile uint16_t *const raw = platform_read_adc_control();
    {
        const float voltage_scale =
            motor_control_fast_sample_prefix(&g_app.motor, &sample, raw, &references);
        volatile struct PositionSensorScratch *const scratch =
            platform_finish_adc_sensor_sample(&sample, raw);
        g_app.position = sample.analog_output_position;
        const float cleared = motor_control_fast_transform(&g_app.motor, &g_app.config, &sample,
                                                           voltage_scale, scratch, &references);
        platform_finish_adc_velocity_sample(&sample, scratch, cleared, &references);
        g_app.velocity = sample.output_velocity;
        CurrentController *const current_d =
            motor_control_fast_prepare(&g_app.motor, &g_app.config, &sample, cleared, &references);
        fault_monitor_helper();
        /* The helper already publishes stopped state/event. Keep decoded
         * feedback in sync without rereading fixed fault or console words. */
        if ((uint32_t)g_app.safety.latched_fault > 7U)
        {
            g_app.motor.feedback.fault = g_app.safety.latched_fault;
        }
        /* Firmware ordering is controller helpers, fault monitor, enabled
         * test, communication age/selective reset, vector limiting, inverse
         * Park and SVPWM. */
        motor_control_fast_apply_state(&g_app.motor, &references);
        const AlphaBeta stationary_voltage = motor_control_fast_finish(current_d, &references);
        svpwm_helper(stationary_voltage.alpha, stationary_voltage.beta);
    }
    platform_ack_adc_irq();
}

void mcan1_receive_irq(void)
{
    CanFrame received;
    McanIrqContext references;
    platform_begin_mcan_irq(&references);
    /* The firmware handler consumes exactly one FIFO0 element selected by
     * RXF0S.F0GI, acknowledges it, then clears the interrupt status.  Any
     * queued successor retriggers IRQ003 instead of being drained here. */
    if (platform_mcan_receive_irq(&received, &references))
    {
        const bool command_frame = (received.id & 0xFFU) == references.node_id;
        const uint16_t routed_id = command_frame ? 0U : *references.received_id;
        const bool parameter_frame = !command_frame && routed_id == 0x7FFU;
        ParameterProtocolResult parameter = {0};
        if (parameter_frame)
        {
            parameter_protocol_process_irq(&received, &parameter, &references);
        }
        if (parameter_frame && parameter.handled)
        {
            if (parameter.response_ready)
            {
                if (parameter.response_prebuilt)
                {
                    platform_mcan_send_prebuilt_irq((uint16_t)parameter.response.id,
                                                    parameter.response.length, &references);
                }
                else
                {
                    platform_mcan_send(&parameter.response);
                }
            }
            if (parameter.bootloader_requested)
            {
                /* The MCAN IRQ sends "Aupgrade", commits the boot
                 * record, waits 10 ms and resets before its normal ack. */
                platform_enter_bootloader_from_can();
                return;
            }
            if (parameter.store_requested)
            {
                /* The parser posts +0x0c before building the STORE reply. */
            }
            if (parameter.persist_requested)
            {
                /* This path writes a full word through retained
                 * status r10, after the legacy ID callback returns. */
                references.status[0x0CU / 4U] = 1U;
            }
            if (parameter.transport_reconfigure_requested)
            {
                /* This path rereads parser scratch after BLX send,
                 * including rejected/unchanged writes. */
                if (*(volatile const uint8_t *)(uintptr_t)MEMORY_LAYOUT_ADDRESS(
                        UINT32_C(0x1fffcc6b), UINT32_C(0x1fffcbf7)) == 0x23U)
                {
                    platform_reconfigure_mcan_irq(&references);
                }
            }
        }
        else if (command_frame)
        {
            /* SETPOINT fully initializes this output; other kinds don't
             * consume it. No firmware read of the decoded command mirror. */
            MotorCommand command;
            const CanCommandKind kind =
                can_protocol_decode_command_irq(&received, &command, &references);
            if (kind != CAN_COMMAND_NONE)
            {
                app_apply_can_command_irq(kind, &command, &references);
#if FIRMWARE_SENDS_COMMAND_FEEDBACK
                CanFrame response;
                can_protocol_encode_feedback_irq(&references, &response);
                platform_mcan_send_prebuilt_irq((uint16_t)response.id, 8U, &references);
#endif
            }
        }
    }
    /* The platform tail performs the no-delay bit-23 callback, rereads IR
     * for bus-off, posts each firmware error and then clears IR/NVIC. Do not
     * route its result through the parameter-write 5-ms reconfiguration. */
    (void)platform_ack_mcan_irq(&references);
}

__attribute__((noipa)) void debug_uart_receive_irq(void)
{
    const uint8_t *data;
    int16_t length;
    if (platform_begin_debug_uart_irq(&data, &length))
    {
        debug_console_process_dma_frame(data, length);
        platform_rearm_debug_uart_irq();
    }
    platform_ack_debug_uart_irq();
}

#define RAM_IRQ_SECTION(name) __attribute__((section(name), aligned(2), used))

RAM_IRQ_SECTION(".irq000")
void IRQ000_Handler(void)
{
    position_sensor_timer_irq();
}
RAM_IRQ_SECTION(".irq001")
void IRQ001_Handler(void)
{
    position_sensor_dma_irq();
}
RAM_IRQ_SECTION(".irq002")
void IRQ002_Handler(void)
{
    adc_foc_control_irq();
}
RAM_IRQ_SECTION(".irq003")
void IRQ003_Handler(void)
{
    mcan1_receive_irq();
}
__attribute__((section(".irq004"), aligned(2), used)) void IRQ004_Handler(void)
{
    debug_uart_receive_irq();
}
