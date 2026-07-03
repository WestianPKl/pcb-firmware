#include "ethernetif.h"
#include "netif/etharp.h"
#include <string.h> 

static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    static uint8_t tx_frame[ETH_BUF_SIZE];
    uint16_t offset = 0;

    for (struct pbuf *q = p; q != NULL; q = q->next) {
        if (offset + q->len > sizeof(tx_frame)) {
            return ERR_BUF;
        }
        memcpy(&tx_frame[offset], q->payload, q->len);
        offset += q->len;
    }

    if (eth_send_frame(tx_frame, offset) != 0) {
        return ERR_IF;
    }

    return ERR_OK;
}

static struct pbuf *low_level_input(struct netif *netif)
{
    static uint8_t rx_frame[ETH_BUF_SIZE];

    uint16_t len = eth_recv_frame(rx_frame, sizeof(rx_frame));
    if (len == 0) {
        return NULL;
    }

    struct pbuf *p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);
    if (p != NULL) {
        pbuf_take(p, rx_frame, len);
    }

    return p;
}

void ethernetif_input(struct netif *netif)
{
    struct pbuf *p;
    while ((p = low_level_input(netif)) != NULL) {
        if (netif->input(p, netif) != ERR_OK) {
            pbuf_free(p);
        }
    }
}

err_t ethernetif_init(struct netif *netif)
{
    netif->name[0] = 'e';
    netif->name[1] = 'n';

    netif->output     = etharp_output;
    netif->linkoutput = low_level_output;

    netif->hwaddr_len = 6;
    netif->hwaddr[0] = 0x00;
    netif->hwaddr[1] = 0x00;
    netif->hwaddr[2] = 0x00;
    netif->hwaddr[3] = 0x00;
    netif->hwaddr[4] = 0x00;
    netif->hwaddr[5] = 0x02;

    netif->mtu = 1500;
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    ethernet_init();

    return ERR_OK;
}

