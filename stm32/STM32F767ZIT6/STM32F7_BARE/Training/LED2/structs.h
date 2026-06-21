#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>

#define __IO volatile

#define PERIPH_BASE 0x40000000UL
#define AHB1_OFFSET 0x20000UL
#define AHB1PERIPH_BASE (PERIPH_BASE + AHB1_OFFSET)
#define GPIOB_OFFSET 0x400UL
#define GPIOB_BASE (AHB1PERIPH_BASE + GPIOB_OFFSET)

#define RCC_OFFSET 0x3800UL
#define RCC_BASE (AHB1PERIPH_BASE + RCC_OFFSET)

#define RCC ((RCC_TypeDef *) RCC_BASE)
#define GPIOB ((GPIO_TypeDef *) GPIOB_BASE)

#define GPIOB_CLK_ENABLE (1U << 1)
#define GPIOB_CLK_DISABLE (0U << 1)

typedef struct {
    __IO uint32_t CRL;
    __IO uint32_t PLLCFGR;
    __IO uint32_t CFGR;
    __IO uint32_t CIR;
    __IO uint32_t AHB1RSTR;
    __IO uint32_t AHB2RSTR;
    __IO uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    __IO uint32_t APB1RSTR;
    __IO uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    __IO uint32_t AHB1ENR;
    __IO uint32_t AHB2ENR;
    __IO uint32_t AHB3ENR;
    uint32_t RESERVED2;
    __IO uint32_t APB1ENR;
    __IO uint32_t APB2ENR;
    uint32_t RESERVED3[2];
    __IO uint32_t AHB1LPENR;
    __IO uint32_t AHB2LPENR;
    __IO uint32_t AHB3LPENR;
    uint32_t RESERVED4;
    __IO uint32_t APB1LPENR;
    __IO uint32_t APB2LPENR;
    uint32_t RESERVED5[2];
    __IO uint32_t BDCR;
    __IO uint32_t CSR;
    uint32_t RESERVED6[2];
    __IO uint32_t SSCGR;
    __IO uint32_t PLLI2SCFGR;
    __IO uint32_t PLLSAICFGR;
    __IO uint32_t DCKCFGR1;
    __IO uint32_t DCKCFGR2;
} RCC_TypeDef;

typedef struct {
    __IO uint32_t MODER;
    __IO uint32_t OTYPER;
    __IO uint32_t OSPEEDR;
    __IO uint32_t PUPDR;
    __IO uint32_t IDR;
    __IO uint32_t ODR;
    __IO uint32_t BSRR;
    __IO uint32_t LCKR;
    __IO uint32_t AFRL;
    __IO uint32_t AFRH;
} GPIO_TypeDef;

#endif // STRUCTS_H