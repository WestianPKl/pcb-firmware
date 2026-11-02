#include "pi.h"

void pi_reset(pi_t *pi)
{
    pi->integral = 0.0f;
}

float pi_update(pi_t *pi, float setpoint, float measurement, float dt)
{
    // 1. Control error
    float error = setpoint - measurement;

    // 2. Proportional term
    float p = pi->kp * error;

    // 3. Integral candidate
    float new_integral = pi->integral + (pi->ki * error * dt);

    // 4. Preliminary output
    float output = pi->feedforward + p + new_integral;

    // 5. Anti-windup
    if (!((output > pi->output_max && error > 0.0f) || (output < pi->output_min && error < 0.0f)))
    {
        pi->integral = new_integral;
    }

    // 6. Final output
    output = pi->feedforward + p + pi->integral;

    // 7. Output saturation
    if (output > pi->output_max)
    {
        output = pi->output_max;
    }
    else if (output < pi->output_min)
    {
        output = pi->output_min;
    }

    return output;
}
