#include "usb.h"

#define USB_SOF 8U // PA8
#define USB_VBUS 9U // PA9
#define USB_ID 10U // PA10
#define USB_DM 11U // PA11
#define USB_DP 12U // PA12
#define USB_GPIO_OUT 6U // PG6

void usb_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_OTGFSEN;
    (void)USB_OTG_FS->GAHBCFG;

    

}