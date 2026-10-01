#ifndef DAMIAO_MOTOR_MATH_H
#define DAMIAO_MOTOR_MATH_H

#include <stdint.h>

typedef struct {
    float alpha;
    float beta;
} AlphaBeta;

typedef struct {
    float d;
    float q;
} DirectQuadrature;

typedef struct {
    float a;
    float b;
    float c;
    /* Preserve the pre-duty modulation for targets whose timer helper
     * converts (1 - phase) directly and therefore has one fewer rounding. */
    float modulation_a;
    float modulation_b;
    float modulation_c;
} PhaseDuty;

typedef struct {
    float kp;
    float ki;
    float integral;
    float output_min;
    float output_max;
} PIController;

float motor_clampf(float value, float minimum, float maximum);
float motor_wrapf(float value, float minimum, float maximum);
void motor_wrapf_in_place(float *value, float minimum, float maximum);
float dm4310_sqrt_helper(float value);
void dm4310_limit_vector_helper(float limit, volatile float *x, volatile float *y);
float motor_fast_sin(float angle);
void motor_fast_sincos(float angle, float *sine, float *cosine);
#if defined(DAMIAO_DM4310)
void dm4310_sincos_helper(float angle, volatile float *sine, volatile float *cosine);
void dm4310_wrap_helper(volatile float *value, float minimum, float maximum);
float dm4310_clamp_helper(float value, float minimum, float maximum);
float dm4310_fast_sin(float angle);
#define motor_target_sincos dm4310_sincos_helper
#define motor_target_sin dm4310_fast_sin
#else
#define motor_target_sincos motor_fast_sincos
#define motor_target_sin motor_fast_sin
#endif
uint32_t motor_float_to_uint(float value, float minimum, float maximum, uint8_t bits);
float motor_uint_to_float(uint32_t value, float minimum, float maximum, uint8_t bits);
uint32_t dm4310_float_to_uint_helper(float value, float minimum,
                                    float maximum, uint8_t bits);
float dm4310_uint_to_float_helper(uint32_t value, float minimum,
                                 float maximum, uint8_t bits);
AlphaBeta motor_clarke(float phase_u, float phase_v);
DirectQuadrature motor_park(AlphaBeta stationary, float sine, float cosine);
AlphaBeta motor_inverse_park(DirectQuadrature rotating, float sine, float cosine);
PhaseDuty motor_svpwm(AlphaBeta voltage);
/* Modulation-only result; normalized a/b/c fields retain neutral 0.5. */
PhaseDuty motor_svpwm_modulation(AlphaBeta voltage);
PhaseDuty motor_svpwm_modulation_scaled(AlphaBeta voltage, float projection_scale);
float motor_pi_step(PIController *controller, float error, float dt);
void motor_limit_vector(float limit, float *x, float *y);

#endif
