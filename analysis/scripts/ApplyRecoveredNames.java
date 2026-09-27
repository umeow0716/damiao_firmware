// Apply evidence-backed semantic names to the stripped HC32F448 images.
// @category DM4310

import java.util.LinkedHashMap;
import java.util.Map;

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.symbol.SourceType;

public class ApplyRecoveredNames extends GhidraScript {
    @Override
    public void run() throws Exception {
        boolean app = currentProgram.getName().toLowerCase().contains("app");
        Map<Long, String> names = app ? appNames() : bootNames();
        int applied = 0;
        for (Map.Entry<Long, String> item : names.entrySet()) {
            Address address = toAddr(item.getKey());
            Function function = getFunctionAt(address);
            if (function == null) {
                /* Absolute RAM thunks are direct machine-code evidence, but
                 * auto-analysis does not always promote their targets to
                 * functions.  Recreate those boundaries here instead of
                 * depending on a one-off manual Ghidra action. */
                disassemble(address);
                function = createFunction(address, item.getValue());
                if (function == null) {
                    println("No function at " + address + " for " + item.getValue());
                    continue;
                }
            }
            try {
                function.setName(item.getValue(), SourceType.USER_DEFINED);
                applied++;
            }
            catch (Exception conflict) {
                println("Could not rename " + address + " to " + item.getValue() +
                        ": " + conflict.getMessage());
            }
        }
        println("Applied " + applied + " recovered names to " + currentProgram.getName());
    }

    private Map<Long, String> appNames() {
        Map<Long, String> m = new LinkedHashMap<>();
        m.put(0x00020474L, "debug_printf");
        m.put(0x00020734L, "runtime_memcpy");
        m.put(0x000207beL, "runtime_memcpy_aligned");
        m.put(0x00021a00L, "adc_sampling_init");
        m.put(0x00021b00L, "calibrate_adc_offsets");
        m.put(0x00021cc8L, "mcan1_configure_classic");
        m.put(0x00021eb0L, "crc_clock_enable");
        m.put(0x00021f20L, "short_busy_wait");
        m.put(0x00021f28L, "delay_ms");
        m.put(0x00021f60L, "delay_us");
        m.put(0x00021fc4L, "mcan1_configure_fd");
        m.put(0x000221f4L, "read_hardware_variant");
        m.put(0x00022244L, "debug_uart_receive_irq");
        m.put(0x00022658L, "load_and_validate_calibration");
        m.put(0x000228c0L, "load_motor_configuration");
        m.put(0x00022c10L, "position_sensor_configure");
        m.put(0x00022ca8L, "power_stage_switch_response_test");
        m.put(0x00022facL, "pwm_timer_init");
        m.put(0x000230c4L, "position_sensor_dma_configure");
        m.put(0x00023170L, "position_sensor_spi_gpio_configure");
        m.put(0x000231ecL, "position_sensor_spi_transfer16");
        m.put(0x00023208L, "system_clock_and_systick_init");
        m.put(0x000234bcL, "system_clock_source_configure");
        m.put(0x00023574L, "debug_uart_gpio_configure");
        m.put(0x0002359cL, "debug_usart1_dma_init");
        m.put(0x0002379cL, "debug_uart_write");
        m.put(0x00024448L, "measure_position_sensor_offset");
        m.put(0x000246f8L, "calibrate_position_sensor");
        m.put(0x00024b20L, "measure_encoder_alignment");
        m.put(0x00024ec0L, "validate_current_sensors");
        m.put(0x00025138L, "derive_control_parameters");
        m.put(0x000252d8L, "copy_persistent_configuration");
        m.put(0x000256c8L, "power_stage_self_test_or_halt");
        m.put(0x000257c4L, "run_motor_parameter_identification");
        m.put(0x000264a4L, "detect_motor_direction_and_pole_pairs");
        m.put(0x000266e8L, "halt_on_bus_overvoltage");
        m.put(0x00026758L, "debug_print_device_info");
        m.put(0x00026d00L, "handle_firmware_control_request");
        m.put(0x00026de8L, "select_configuration_bank_a");
        m.put(0x00026e04L, "select_configuration_bank_b");
        m.put(0x00026e90L, "store_hardware_variant");
        m.put(0x00026eb8L, "enforce_device_key_binding_or_halt");
        m.put(0x1fff8000L, "motor_fault_monitor");
        m.put(0x1fff8136L, "adc_foc_control_irq");
        m.put(0x1fff8744L, "position_sensor_timer_irq");
        m.put(0x1fff8770L, "position_sensor_dma_irq");
        m.put(0x1fff88b8L, "mcan1_receive_irq");
        m.put(0x1fff987cL, "output_sensor_lookup_and_unwrap");
        m.put(0x1fff9950L, "flash_program_words");
        m.put(0x1fff9a38L, "flash_erase_sector");
        m.put(0x1fff9accL, "mcan_write_classic_tx_element");
        m.put(0x1fff9b0eL, "mcan_write_fd_tx_element");
        m.put(0x1fff9ba0L, "mcan_write_fd_brs_tx_element");
        m.put(0x1fff9c40L, "svpwm_write_compare");
        m.put(0x1fff9dc8L, "reset_control_state");
        m.put(0x1fff9e44L, "identification_filter_step");
        m.put(0x1fff9ea6L, "configure_runtime_control_parameters_variant");
        m.put(0x1fff9fdeL, "motion_observer_step");
        m.put(0x1fffa07aL, "current_controller_step");
        m.put(0x1fffa160L, "identification_rls2_step");
        m.put(0x1fffa234L, "identification_flux_observer_step");
        m.put(0x1fffa30cL, "fast_sincos_lut");
        m.put(0x1fffa38aL, "clampf");
        m.put(0x1fffa3aaL, "wrap_periodic_value");
        m.put(0x1fffa414L, "limit_vector_magnitude");
        m.put(0x1fffa46aL, "float_to_uint_packed");
        m.put(0x1fffa492L, "uint_to_float_packed");
        return m;
    }

    private Map<Long, String> bootNames() {
        Map<Long, String> m = new LinkedHashMap<>();
        m.put(0x00000db8L, "mcan_send_frame");
        m.put(0x00001a14L, "delay_ms");
        m.put(0x00002e68L, "mcan1_receive_irq");
        m.put(0x000047bcL, "configure_swd_access");
        m.put(0x00005bb8L, "debug_write");
        m.put(0x00005becL, "debug_uart_error_irq");
        m.put(0x00005c30L, "debug_uart_receive_irq");
        m.put(0x00005e00L, "debug_printf");
        m.put(0x000063b8L, "boot_hardware_init");
        m.put(0x00006410L, "clear_boot_request_record");
        m.put(0x00006430L, "erase_flash_sector_at");
        m.put(0x000064a8L, "debug_putchar");
        m.put(0x000064c4L, "debug_command_handler");
        m.put(0x00006608L, "firmware_update_frame_handler");
        m.put(0x000067c0L, "prepare_device_key_material");
        m.put(0x000067f4L, "jump_to_application");
        m.put(0x00006810L, "flash_region_is_programmed");
        m.put(0x00006834L, "load_persistent_boot_record");
        m.put(0x00006bbcL, "aes_update_chunk_transform");
        m.put(0x00006bfcL, "debug_error_callback");
        m.put(0x00006c18L, "debug_status_callback");
        m.put(0x00006c34L, "store_upgrade_complete_record");
        m.put(0x00006c54L, "reverse_serial_hex_group");
        m.put(0x00006c9cL, "update_persistent_device_id");
        m.put(0x00006ce0L, "authenticate_device_and_print_serial");
        m.put(0x00006ff8L, "authenticate_device_key");
        m.put(0x00007198L, "crc8_maxim");
        m.put(0x1fff8000L, "flash_erase_and_program");
        m.put(0x1fff809cL, "flash_program_and_verify");
        return m;
    }
}
