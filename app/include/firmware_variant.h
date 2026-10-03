#ifndef DAMIAO_FIRMWARE_VARIANT_H
#define DAMIAO_FIRMWARE_VARIANT_H

/* Both recovered behaviors remain enabled by default.  Product targets may
 * independently select raw measurements and suppress command-frame replies. */
#if defined(DAMIAO_FEEDBACK_VARIANT_RAW) && defined(DAMIAO_COMMAND_FEEDBACK_DISABLED)
#define FIRMWARE_STATUS_BANNER "DMBOT Motor Driver(with raw result and no response)"
#elif defined(DAMIAO_FEEDBACK_VARIANT_RAW)
#define FIRMWARE_STATUS_BANNER "DMBOT Motor Driver(with raw result)"
#elif defined(DAMIAO_COMMAND_FEEDBACK_DISABLED)
#define FIRMWARE_STATUS_BANNER "DMBOT Motor Driver(no response)"
#else
#define FIRMWARE_STATUS_BANNER "DMBOT Motor Driver"
#endif

#if defined(DAMIAO_FEEDBACK_VARIANT_RAW)
#define FIRMWARE_USES_RAW_CAN_FEEDBACK 1
#else
#define FIRMWARE_USES_RAW_CAN_FEEDBACK 0
#endif

#if defined(DAMIAO_COMMAND_FEEDBACK_DISABLED)
#define FIRMWARE_SENDS_COMMAND_FEEDBACK 0
#else
#define FIRMWARE_SENDS_COMMAND_FEEDBACK 1
#endif

#endif
