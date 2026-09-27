#ifndef DM4310_BOOT_RECORD_H
#define DM4310_BOOT_RECORD_H

#include <stdbool.h>
#include <stdint.h>

#define DM4310_BOOT_RECORD_ADDRESS 0x0001E000UL
#define DM4310_APPLICATION_IDENTITY 0x07010005UL
#define DM4310_UPDATE_JOURNAL_MAGIC 0x314A4D44UL
#define DM4310_UPDATE_JOURNAL_VERSION 1UL
#define DM4310_UPDATE_JOURNAL_IN_PROGRESS 0x52504749UL

typedef struct {
    uint32_t boot_request;
    uint32_t application_confirmed;
    uint32_t device_id;
    uint32_t application_identity;
    uint32_t swd_disabled;
} BootPersistentRecord;

/* Source-loader extension stored immediately after the five historical
 * words.  The original loader/application ignore this area, so the first
 * 20-byte ABI remains unchanged. */
typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t state;
    uint32_t state_inverse;
} BootUpdateJournal;

typedef struct {
    BootPersistentRecord record;
    BootUpdateJournal update;
} BootRecordSectorPrefix;

bool boot_record_is_normal(const BootPersistentRecord *record);
bool boot_record_is_update_requested(const BootPersistentRecord *record);
void boot_record_request_update(BootPersistentRecord *record);
void boot_record_clear_request(BootPersistentRecord *record);
void boot_record_confirm_application(BootPersistentRecord *record);
void boot_record_set_application_identity(BootPersistentRecord *record);
bool boot_update_journal_in_progress(const BootUpdateJournal *journal);
void boot_update_journal_begin(BootUpdateJournal *journal);
void boot_update_journal_clear(BootUpdateJournal *journal);

#endif
