#include "app_image.h"
#include "access_protocol.h"
#include "boot_platform.h"
#include "hc32f448.h"
#include "service_protocol.h"
#include "update_protocol.h"

int main(void)
{
    static BootUpdateSession update;
    static BootAccessSession access;
    static BootServiceSession service;
    boot_update_init(&update);
    boot_access_init(&access);
    boot_platform_init();
    boot_service_init(&service, boot_platform_node_id(),
                      boot_platform_protocol_version());
    __enable_irq();
    uint32_t update_last_poll = boot_platform_milliseconds();

    const bool device_authenticated = boot_platform_authenticate_device();

    const bool stay_in_loader = !device_authenticated ||
                                boot_platform_stay_in_loader();
    if (device_authenticated && !stay_in_loader && app_image_installed()) {
        app_image_jump();
    }

    for (;;) {
        const uint32_t update_now = boot_platform_milliseconds();
        boot_update_advance_time(&update, update_now - update_last_poll);
        update_last_poll = update_now;

        BootCanFrame frame;
        if (boot_platform_receive(&frame)) {
            if (!boot_access_accept_can(&frame)) {
                if (device_authenticated) {
                    if (!boot_service_accept(&service, &frame)) {
                        boot_update_accept(&update, &frame);
                    }
                }
            }
        }
        uint8_t debug_byte;
        while (boot_platform_debug_receive(&debug_byte)) {
            boot_access_accept_uart(&access, debug_byte);
        }
        if (device_authenticated && boot_update_is_complete(&update)) {
            if (!app_image_installed_length(update.writer.bytes_written)) {
                static const char invalid[8] =
                    {'A','P','P','E','R','R','O','R'};
                boot_platform_transmit(0x7FEU, invalid, sizeof(invalid));
                update.state = BOOT_UPDATE_ERROR;
            } else {
                static const char complete[8] =
                    {'c','o','m','p','l','e','t','e'};
                static const char upgraded[8] =
                    {'A','u','p','g','r','a','d','e'};
                /* The reference leaves 20 ms after the final packet status,
                 * sends "complete", then sends "Aupgrade" before committing
                 * the boot record.  Explicit queue drains make those final
                 * acknowledgements reliable even on a busy CAN bus. */
                boot_platform_delay_ms(20U);
                boot_platform_transmit(0x7FEU, complete, sizeof(complete));
                boot_platform_flush_transport(2U);
                boot_platform_transmit(0x7FEU, upgraded, sizeof(upgraded));
                if (boot_platform_mark_upgrade_complete()) {
                    boot_platform_reset();
                }
            }
        }
        if (device_authenticated &&
            boot_service_launch_requested(&service)) {
            if (boot_update_has_started(&update) || !app_image_installed()) {
                static const char invalid[8] =
                    {'A','P','P','E','R','R','O','R'};
                boot_platform_transmit(0x7FEU, invalid, sizeof(invalid));
                service.launch_requested = false;
            } else {
                static const char upgraded[8] =
                    {'A','u','p','g','r','a','d','e'};
                boot_platform_transmit(0x7FEU, upgraded, sizeof(upgraded));
                boot_platform_flush_transport(2U);
                if (boot_platform_mark_upgrade_complete()) {
                    boot_platform_reset();
                }
                service.launch_requested = false;
            }
        }
        if (device_authenticated &&
            (update.state == BOOT_UPDATE_WAIT_HEADER) &&
            !boot_update_has_started(&update) &&
            boot_platform_loader_timeout_expired() &&
            app_image_installed() && boot_platform_clear_boot_request()) {
            app_image_jump();
        }
        boot_platform_idle();
    }
}
