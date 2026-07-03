#ifndef ETHERNETIF_H
#define ETHERNETIF_H

#include <stdint.h>
#include "stm32f7xx.h"
#include "ethernet.h"
#include "lwip/netif.h"
#include "lwip/pbuf.h"
#include "lwip/err.h"

void ethernetif_input(struct netif *netif);
err_t ethernetif_init(struct netif *netif);

#endif // ETHERNETIF_H
