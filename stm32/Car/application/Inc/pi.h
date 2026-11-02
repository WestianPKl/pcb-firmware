#ifndef PI_H
#define PI_H

#include <stdint.h>

typedef struct
{
    float kp;
    float ki;
    float integral;
    float feedforward;
    float output_min;
    float output_max;
} pi_t;

void pi_reset(pi_t *pi);
float pi_update(pi_t *pi, float setpoint, float measurement, float dt);

#endif // PI_H