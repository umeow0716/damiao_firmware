#ifndef DAMIAO_MOTOR_MATH_H
#define DAMIAO_MOTOR_MATH_H

#include <stdint.h>

typedef struct
{
    float alpha;
    float beta;
} AlphaBeta;

typedef struct
{
    float d;
    float q;
} DirectQuadrature;

typedef struct
{
    float a;
    float b;
    float c;
    /* Preserve the pre-duty modulation for targets whose timer helper
     * converts (1 - phase) directly and therefore has one fewer rounding. */
    float modulation_a;
    float modulation_b;
    float modulation_c;
} PhaseDuty;

typedef struct
{
    float kp;
    float ki;
    float integral;
    float output_min;
    float output_max;
} PIController;

float motor_clampf(float value, float minimum, float maximum);
float motor_wrapf(float value, float minimum, float maximum);
void motor_wrapf_in_place(float *value, float minimum, float maximum);
float sqrt_helper(float value);
void limit_vector_helper(float limit, volatile float *x, volatile float *y);
float motor_fast_sin(float angle);
void motor_fast_sincos(float angle, float *sine, float *cosine);
void sincos_helper(float angle, volatile float *sine, volatile float *cosine);
void wrap_helper(volatile float *value, float minimum, float maximum);
float clamp_helper(float value, float minimum, float maximum);
float fast_sin(float angle);
#define motor_target_sincos sincos_helper
#define motor_target_sin fast_sin
uint32_t motor_float_to_uint(float value, float minimum, float maximum, uint8_t bits);
float motor_uint_to_float(uint32_t value, float minimum, float maximum, uint8_t bits);
uint32_t float_to_uint_helper(float value, float minimum, float maximum, uint8_t bits);
float uint_to_float_helper(uint32_t value, float minimum, float maximum, uint8_t bits);
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
