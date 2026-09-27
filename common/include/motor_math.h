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
float motor_fast_sin(float angle);
void motor_fast_sincos(float angle, float *sine, float *cosine);
uint32_t motor_float_to_uint(float value, float minimum, float maximum, uint8_t bits);
float motor_uint_to_float(uint32_t value, float minimum, float maximum, uint8_t bits);
AlphaBeta motor_clarke(float phase_u, float phase_v);
DirectQuadrature motor_park(AlphaBeta stationary, float sine, float cosine);
AlphaBeta motor_inverse_park(DirectQuadrature rotating, float sine, float cosine);
PhaseDuty motor_svpwm(AlphaBeta voltage);
float motor_pi_step(PIController *controller, float error, float dt);
void motor_limit_vector(float limit, float *x, float *y);

#endif
