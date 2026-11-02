#include "ethernetif.h"
#include "ethernet.h"

#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "netif/ethernet.h"

#define ETHERNETIF_TX_BUFFER_SIZE 1536U

static uint8_t ethernetif_tx_buffer[ETHERNETIF_TX_BUFFER_SIZE];


static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    (void)netif;

    if (p == NULL)
    {
        return ERR_ARG;
    }

    if (p->tot_len > ETHERNETIF_TX_BUFFER_SIZE)
    {
        return ERR_BUF;
    }

    uint16_t offset = 0U;

    for (struct pbuf *q = p; q != NULL; q = q->next)
    {
        uint8_t *src = (uint8_t *)q->payload;

        for (uint16_t i = 0U; i < q->len; i++)
        {
            ethernetif_tx_buffer[offset] = src[i];
            offset++;
        }
    }

    if (ethernet_tx(ethernetif_tx_buffer, offset) == 0U)
    {
        return ERR_IF;
    }

    return ERR_OK;
}


static struct pbuf *low_level_input(struct netif *netif)
{
    (void)netif;

    uint8_t *frame = NULL;
    uint16_t length = 0U;

    if (ethernet_rx_poll(&frame, &length) == 0U)
    {
        return NULL;
    }

    struct pbuf *p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);

    if (p == NULL)
    {
        ethernet_rx_release();
        return NULL;
    }

    if (pbuf_take(p, frame, length) != ERR_OK)
    {
        pbuf_free(p);
        ethernet_rx_release();
        return NULL;
    }

    ethernet_rx_release();

    return p;
}

void ethernetif_input(struct netif *netif)
{
    struct pbuf *p;

    while (1)
    {
        p = low_level_input(netif);

        if (p == NULL)
        {
            break;
        }

        if (netif->input(p, netif) != ERR_OK)
        {
            pbuf_free(p);
        }
    }
}

err_t ethernetif_init(struct netif *netif)
{
    if (netif == NULL)
    {
        return ERR_ARG;
    }

    netif->name[0] = 'e';
    netif->name[1] = 'n';

    netif->output = etharp_output;
    netif->linkoutput = low_level_output;

    netif->hwaddr_len = ETH_HWADDR_LEN;

    const uint8_t *mac = ethernet_get_mac();

    for (uint8_t i = 0U; i < 6U; i++)
    {
        netif->hwaddr[i] = mac[i];
    }

    netif->mtu = 1500U;

    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET;

    return ERR_OK;
}