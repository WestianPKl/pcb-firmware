#include "udp_echo.h"

#include "lwip/udp.h"
#include "lwip/pbuf.h"

#define UDP_ECHO_PORT 5000U

static struct udp_pcb *udp_echo_pcb = NULL;

static void udp_echo_receive(void *arg,
                             struct udp_pcb *pcb,
                             struct pbuf *p,
                             const ip_addr_t *addr,
                             u16_t port)
{
    (void)arg;

    if (p == NULL)
    {
        return;
    }

    udp_sendto(pcb, p, addr, port);

    pbuf_free(p);
}

uint8_t udp_echo_init(void)
{
    udp_echo_pcb = udp_new();

    if (udp_echo_pcb == NULL)
    {
        return 0U;
    }

    err_t err = udp_bind(udp_echo_pcb,
                         IP_ADDR_ANY,
                         UDP_ECHO_PORT);

    if (err != ERR_OK)
    {
        udp_remove(udp_echo_pcb);
        udp_echo_pcb = NULL;

        return 0U;
    }

    udp_recv(udp_echo_pcb,
             udp_echo_receive,
             NULL);

    return 1U;
}