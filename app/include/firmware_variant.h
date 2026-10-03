#ifndef DAMIAO_FIRMWARE_VARIANT_H
#define DAMIAO_FIRMWARE_VARIANT_H

/* The factory variant remains the default so ordinary source builds preserve
 * the recovered behavior unless a target explicitly opts into raw feedback. */
#if defined(DAMIAO_FEEDBACK_VARIANT_RAW)
#define FIRMWARE_STATUS_BANNER "DMBOT Motor Driver(with raw result)"
#define FIRMWARE_USES_RAW_CAN_FEEDBACK 1
#else
#define FIRMWARE_STATUS_BANNER "DMBOT Motor Driver"
#define FIRMWARE_USES_RAW_CAN_FEEDBACK 0
#endif

#endif
