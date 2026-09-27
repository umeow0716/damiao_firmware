#include "boot_record.h"

bool boot_record_is_normal(const BootPersistentRecord *record)
{
    return (record != 0) && (record->boot_request == 0U) &&
           (record->application_confirmed == 1U);
}

bool boot_record_is_update_requested(const BootPersistentRecord *record)
{
    return (record != 0) && (record->boot_request == 1U) &&
           (record->application_confirmed == 0U);
}

void boot_record_request_update(BootPersistentRecord *record)
{
    if (record != 0) {
        record->boot_request = 1U;
        record->application_confirmed = 0U;
    }
}

void boot_record_clear_request(BootPersistentRecord *record)
{
    if (record != 0) {
        record->boot_request = 0U;
        record->application_confirmed = 0U;
    }
}

void boot_record_confirm_application(BootPersistentRecord *record)
{
    if (record != 0) {
        record->boot_request = 0U;
        record->application_confirmed = 1U;
    }
}

void boot_record_set_application_identity(BootPersistentRecord *record)
{
    if (record != 0) {
        record->application_identity = DM4310_APPLICATION_IDENTITY;
    }
}

bool boot_update_journal_in_progress(const BootUpdateJournal *journal)
{
    return (journal != 0) &&
           (journal->magic == DM4310_UPDATE_JOURNAL_MAGIC) &&
           (journal->version == DM4310_UPDATE_JOURNAL_VERSION) &&
           (journal->state == DM4310_UPDATE_JOURNAL_IN_PROGRESS) &&
           (journal->state_inverse ==
            (uint32_t)~(uint32_t)DM4310_UPDATE_JOURNAL_IN_PROGRESS);
}

void boot_update_journal_begin(BootUpdateJournal *journal)
{
    if (journal != 0) {
        journal->magic = DM4310_UPDATE_JOURNAL_MAGIC;
        journal->version = DM4310_UPDATE_JOURNAL_VERSION;
        journal->state = DM4310_UPDATE_JOURNAL_IN_PROGRESS;
        journal->state_inverse =
            (uint32_t)~(uint32_t)DM4310_UPDATE_JOURNAL_IN_PROGRESS;
    }
}

void boot_update_journal_clear(BootUpdateJournal *journal)
{
    if (journal != 0) {
        journal->magic = UINT32_MAX;
        journal->version = UINT32_MAX;
        journal->state = UINT32_MAX;
        journal->state_inverse = UINT32_MAX;
    }
}
