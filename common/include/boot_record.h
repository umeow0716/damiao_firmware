#ifndef DAMIAO_BOOT_RECORD_H
#define DAMIAO_BOOT_RECORD_H

#include <stdbool.h>
#include <stdint.h>

#define APP_BOOT_RECORD_ADDRESS 0x0001E000UL
#define APP_UPDATE_JOURNAL_MAGIC 0x314A4D44UL
#define APP_UPDATE_JOURNAL_VERSION 1UL
#define APP_UPDATE_JOURNAL_IN_PROGRESS 0x52504749UL

typedef struct
{
    uint32_t boot_request;
    uint32_t application_confirmed;
    uint32_t device_id;
    uint32_t application_identity;
    uint32_t swd_disabled;
} BootPersistentRecord;

/* Loader extension stored immediately after the five legacy words.  Existing
 * loader/application code ignores this area, so the first
 * 20-byte ABI remains unchanged. */
typedef struct
{
    uint32_t magic;
    uint32_t version;
    uint32_t state;
    uint32_t state_inverse;
} BootUpdateJournal;

typedef struct
{
    BootPersistentRecord record;
    BootUpdateJournal update;
} BootRecordSectorPrefix;

bool boot_record_is_normal(const BootPersistentRecord *record);
bool boot_record_is_update_requested(const BootPersistentRecord *record);
void boot_record_request_update(BootPersistentRecord *record);
void boot_record_clear_request(BootPersistentRecord *record);
void boot_record_confirm_application(BootPersistentRecord *record);
void boot_record_set_application_identity(BootPersistentRecord *record,
                                          uint32_t application_identity);
bool boot_update_journal_in_progress(const BootUpdateJournal *journal);
void boot_update_journal_begin(BootUpdateJournal *journal);
void boot_update_journal_clear(BootUpdateJournal *journal);

#endif
