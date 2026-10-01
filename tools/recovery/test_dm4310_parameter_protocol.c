/* Isolated host test for recovered IRQ003 parameter default/rejection arms.
 * It is intentionally absent from CMake and the normal make graph. */

#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "app_config.h"
#include "parameter_protocol.h"

static uint32_t float_bits(float value)
{
    uint32_t bits;
    memcpy(&bits, &value, sizeof(bits));
    return bits;
}

static uint32_t response_value(const ParameterProtocolResult *result)
{
    return (uint32_t)result->response.data[4] |
           ((uint32_t)result->response.data[5] << 8U) |
           ((uint32_t)result->response.data[6] << 16U) |
           ((uint32_t)result->response.data[7] << 24U);
}

void motor_control_configure_velocity_filter(MotorController *controller,
                                             float bandwidth)
{
    (void)controller;
    (void)bandwidth;
}

void can_protocol_encode_feedback(const MotorFeedback *feedback,
                                  const MotorConfig *config,
                                  CanFrame *frame)
{
    (void)feedback;
    (void)config;
    memset(frame, 0, sizeof(*frame));
}

uint32_t dm4310_float_to_uint_helper(float value, float minimum,
                                     float maximum, uint8_t bits)
{
    const uint32_t limit = (1U << bits) - 1U;
    return (uint32_t)((value - minimum) * (float)limit /
                      (maximum - minimum));
}

static void issue(uint8_t operation, uint8_t address, uint32_t value,
                  MotorConfig *config, MotorController *controller,
                  ParameterProtocolResult *result)
{
    CanFrame request;
    ParameterRuntime runtime;
    memset(&request, 0, sizeof(request));
    memset(&runtime, 0, sizeof(runtime));
    request.id = 0x7FFU;
    request.length = 8U;
    request.data[0] = (uint8_t)value;
    request.data[1] = (uint8_t)(value >> 8U);
    request.data[2] = (uint8_t)(value >> 16U);
    request.data[3] = (uint8_t)(value >> 24U);
    request.data[4] = (uint8_t)config->can_id;
    request.data[5] = (uint8_t)(config->can_id >> 8U);
    request.data[6] = operation;
    request.data[7] = address;
    parameter_protocol_process(&request, config, controller, &runtime, result);
}

int main(void)
{
    MotorConfig config;
    MotorController controller;
    ParameterProtocolResult result;
    memset(&config, 0, sizeof(config));
    memset(&controller, 0, sizeof(controller));
    config.can_id = 0x123U;
    config.master_id = 0x456U;
    config.speed_limit = 12.0f;
    memset(app_config_staging_record(), 0,
           APP_CONFIG_WORD_COUNT * sizeof(uint32_t));

    issue(0x33U, 0x49U, 0U, &config, &controller, &result);
    assert(result.handled && result.response_ready);
    assert(response_value(&result) == config.can_id);

    issue(0x55U, 0x06U, float_bits(-1.0f),
          &config, &controller, &result);
    assert(result.handled && result.response_ready);
    assert(config.speed_limit == 12.0f);
    assert(response_value(&result) == float_bits(12.0f));
    assert(app_config_staging_record()[0x06] == 0U);

    issue(0x55U, 0x06U, float_bits(20.0f),
          &config, &controller, &result);
    assert(config.speed_limit == 20.0f);
    assert(app_config_staging_record()[0x06] == float_bits(20.0f));

    issue(0x55U, 0x14U, 0xFFFFFFFFU, &config, &controller, &result);
    assert(result.handled && result.response_ready);
    assert(response_value(&result) == 0U);

    issue(0xCCU, 5U, 0U, &config, &controller, &result);
    assert(result.handled && !result.response_ready);

    controller.armed = true;
    issue(0xAAU, 0U, 0U, &config, &controller, &result);
    assert(result.handled && !result.store_requested && !result.response_ready);

    controller.armed = false;
    issue(0xAAU, 0U, 0U, &config, &controller, &result);
    assert(result.handled && result.store_requested && result.response_ready);
    assert(result.response.length == 4U);
    assert(result.response.data[2] == 0xAAU);
    assert(result.response.data[3] == 1U);

    issue(0x99U, 0U, 0U, &config, &controller, &result);
    assert(result.handled && !result.response_ready);
    return 0;
}
