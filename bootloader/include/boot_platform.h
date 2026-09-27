#ifndef DM4310_BOOT_PLATFORM_H
#define DM4310_BOOT_PLATFORM_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
    uint32_t id;
    uint8_t length;
    uint8_t data[64];
} BootCanFrame;

void SysTick_Handler(void);
void boot_platform_init(void);
bool boot_platform_authenticate_device(void);
uint16_t boot_platform_node_id(void);
uint16_t boot_platform_protocol_version(void);
bool boot_platform_stay_in_loader(void);
bool boot_platform_loader_timeout_expired(void);
uint32_t boot_platform_milliseconds(void);
bool boot_platform_clear_boot_request(void);
bool boot_platform_mark_update_started(void);
bool boot_platform_mark_upgrade_complete(void);
bool boot_platform_set_swd_disabled(bool disabled);
bool boot_platform_receive(BootCanFrame *frame);
void boot_platform_transmit(uint32_t id, const void *data, size_t length);
bool boot_platform_debug_receive(uint8_t *byte);
void boot_platform_debug_write(const void *data, size_t length);
void boot_platform_delay_ms(uint32_t milliseconds);
bool boot_platform_flush_transport(uint32_t timeout_ms);
void boot_platform_idle(void);
bool boot_platform_flash_program(uint32_t address, const void *data, size_t length);
void boot_platform_reset(void) __attribute__((noreturn));

#endif
