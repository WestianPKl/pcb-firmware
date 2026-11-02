#include "fmac.h"

#define FMAC_FIR_TAPS      4U

#define FMAC_X2_BASE       0U
#define FMAC_X2_SIZE       4U

#define FMAC_X1_BASE       4U
#define FMAC_X1_SIZE       8U

#define FMAC_Y_BASE        12U
#define FMAC_Y_SIZE        4U


static const int16_t fmac_coeff[FMAC_FIR_TAPS] =
{
    8192,
    8192,
    8192,
    8192
};


uint8_t fmac_init(void)
{
    /*
     * FMAC clock
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_FMACEN;
    (void)RCC->AHB1ENR;

    /*
     * Reset FMAC
     */
    FMAC->CR |= FMAC_CR_RESET;

    uint32_t timeout = 1000000U;

    while ((FMAC->CR & FMAC_CR_RESET) != 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    /*
     * X2 = coefficients
     *
     * base = 0
     * size = 4
     */
    FMAC->X2BUFCFG =
        (FMAC_X2_BASE << FMAC_X2BUFCFG_X2_BASE_Pos) |
        (FMAC_X2_SIZE << FMAC_X2BUFCFG_X2_BUF_SIZE_Pos);

    /*
     * X1 = input samples
     *
     * base = 4
     * size = 8
     * FULL_WM = 0 -> threshold = 1
     */
    FMAC->X1BUFCFG =
        (FMAC_X1_BASE << FMAC_X1BUFCFG_X1_BASE_Pos) |
        (FMAC_X1_SIZE << FMAC_X1BUFCFG_X1_BUF_SIZE_Pos);

    /*
     * Y = output samples
     *
     * base = 12
     * size = 4
     * EMPTY_WM = 0 -> threshold = 1
     */
    FMAC->YBUFCFG =
        (FMAC_Y_BASE << FMAC_YBUFCFG_Y_BASE_Pos) |
        (FMAC_Y_SIZE << FMAC_YBUFCFG_Y_BUF_SIZE_Pos);

    /*
     * Na razie:
     * IRQ off
     * DMA off
     * clipping off
     */
    FMAC->CR = 0U;

        /*
     * LOAD X2
     *
     * P = 4
     * Q = 0
     * FUNC = 2
     * START = 1
     */
    FMAC->PARAM =
        (FMAC_FIR_TAPS << FMAC_PARAM_P_Pos) |
        FMAC_PARAM_FUNC_1 |
        FMAC_PARAM_START;

    for (uint32_t i = 0U; i < FMAC_FIR_TAPS; i++)
    {
        FMAC->WDATA = (uint16_t)fmac_coeff[i];
    }

    timeout = 1000000U;

    while ((FMAC->PARAM & FMAC_PARAM_START) != 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

        /*
     * LOAD X1
     *
     * P = 3
     * FUNC = 1
     */
    FMAC->PARAM =
        (3U << FMAC_PARAM_P_Pos) |
        FMAC_PARAM_FUNC_0 |
        FMAC_PARAM_START;

    FMAC->WDATA = 0U;
    FMAC->WDATA = 0U;
    FMAC->WDATA = 0U;

    timeout = 1000000U;

    while ((FMAC->PARAM & FMAC_PARAM_START) != 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

        FMAC->PARAM =
        (FMAC_FIR_TAPS << FMAC_PARAM_P_Pos) |
        FMAC_PARAM_FUNC_3 |
        FMAC_PARAM_START;

    /*
     * Tym razem START pozostaje = 1.
     * FIR pracuje ciągle, dopóki go nie zatrzymamy.
     */

    return 1U;
}

uint8_t fmac_process_sample(int16_t input,
                            int16_t *output)
{
    if (output == NULL)
    {
        return 0U;
    }

    uint32_t timeout = 1000000U;

    /*
     * Poczekaj aż X1 ma miejsce.
     */
    while ((FMAC->SR & FMAC_SR_X1FULL) != 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    /*
     * Nowa próbka.
     */
    FMAC->WDATA = (uint16_t)input;

    /*
     * YEMPTY = 1 -> jeszcze nie ma wyniku.
     * YEMPTY = 0 -> co najmniej jeden wynik gotowy.
     */
    timeout = 1000000U;

    while ((FMAC->SR & FMAC_SR_YEMPTY) != 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    *output =
        (int16_t)(FMAC->RDATA & FMAC_RDATA_RDATA_Msk);

    return 1U;
}