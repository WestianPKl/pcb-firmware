#include "ai.h"
#include "network.h"
#include "user_init.h"
#include "stai.h"

STAI_NETWORK_CONTEXT_DECLARE(ai_network_context, STAI_NETWORK_CONTEXT_SIZE)

STAI_ALIGNED(32)
static uint8_t ai_activations[STAI_NETWORK_ACTIVATION_1_SIZE_BYTES];

static stai_ptr ai_activation_ptrs[] =
{
    ai_activations
};

static stai_ptr ai_inputs[STAI_NETWORK_IN_NUM];
static stai_ptr ai_outputs[STAI_NETWORK_OUT_NUM];

static stai_size ai_input_count;
static stai_size ai_output_count;


uint8_t ai_init(void)
{
    stai_return_code ret;

    ret = stai_runtime_init();

    if (ret != STAI_SUCCESS)
    {
        return 0U;
    }

    ret = user_stai_network_init(ai_network_context);

    if (ret != STAI_SUCCESS)
    {
        return 0U;
    }

    ret = stai_network_set_activations(ai_network_context, ai_activation_ptrs, STAI_NETWORK_ACTIVATIONS_NUM
    );

    if (ret != STAI_SUCCESS)
    {
        return 0U;
    }

    ret = stai_network_get_inputs(ai_network_context, ai_inputs, &ai_input_count
    );

    if (ret != STAI_SUCCESS)
    {
        return 0U;
    }

    ret = stai_network_get_outputs(ai_network_context, ai_outputs, &ai_output_count
    );

    if (ret != STAI_SUCCESS)
    {
        return 0U;
    }

    return 1U;
}

uint8_t ai_run(float input_value, uint8_t *class_id)
{
    stai_return_code ret;
    float *input;
    float *output;
    uint8_t max_index = 0U;

    if (class_id == NULL)
    {
        return 0U;
    }

    input = (float *)ai_inputs[0];
    output = (float *)ai_outputs[0];

    input[0] = input_value;

    ret = stai_network_run(ai_network_context, STAI_MODE_SYNC);

    if (ret != STAI_SUCCESS)
    {
        return 0U;
    }

    if (output[1] > output[max_index])
    {
        max_index = 1U;
    }

    if (output[2] > output[max_index])
    {
        max_index = 2U;
    }

    *class_id = max_index;

    return 1U;
}

uint8_t ai_temperature_x100_run(int16_t temperature, uint8_t *class_id)
{
    float normalized = (temperature / 100.0f) / 100.0f;

    return ai_run(normalized, class_id);
}