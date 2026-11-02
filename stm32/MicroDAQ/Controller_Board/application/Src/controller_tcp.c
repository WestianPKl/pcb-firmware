#include "controller_tcp.h"
#include "lwip/tcp.h"
#include "protocol.h"

#define CONTROLLER_TCP_PORT 5001U

static struct tcp_pcb *controller_listen_pcb = NULL;
static struct tcp_pcb *controller_client_pcb = NULL;

static void controller_tcp_error(void *arg, err_t err)
{
    (void)arg;
    (void)err;

    controller_client_pcb = NULL;
}

static err_t controller_tcp_receive(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    (void)arg;

    if (err != ERR_OK)
    {
        if (p != NULL)
        {
            pbuf_free(p);
        }

        return err;
    }

    if (p == NULL)
    {
        if (controller_client_pcb == tpcb)
        {
            controller_client_pcb = NULL;
        }

        tcp_close(tpcb);

        return ERR_OK;
    }

    for (struct pbuf *q = p; q != NULL; q = q->next)
    {
        tcp_protocol_rx((const uint8_t *)q->payload, q->len);
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    return ERR_OK;
}

static err_t controller_tcp_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    (void)arg;

    if ((err != ERR_OK) || (newpcb == NULL))
    {
        return ERR_VAL;
    }

    controller_client_pcb = newpcb;

    tcp_recv(newpcb, controller_tcp_receive);
    tcp_err(newpcb, controller_tcp_error);

    return ERR_OK;
}


uint8_t controller_tcp_init(void)
{
    struct tcp_pcb *pcb = tcp_new();

    if (pcb == NULL)
    {
        return 0U;
    }

    err_t err = tcp_bind(pcb, IP_ADDR_ANY, CONTROLLER_TCP_PORT);

    if (err != ERR_OK)
    {
        tcp_close(pcb);
        return 0U;
    }

    controller_listen_pcb = tcp_listen(pcb);

    if (controller_listen_pcb == NULL)
    {
        tcp_close(pcb);
        return 0U;
    }

    tcp_accept(controller_listen_pcb, controller_tcp_accept);

    return 1U;
}

uint8_t controller_tcp_send(const uint8_t *data, uint16_t length)
{
    if ((controller_client_pcb == NULL) || (data == NULL) || (length == 0U))
    {
        return 0U;
    }

    err_t err = tcp_write(controller_client_pcb, data, length, TCP_WRITE_FLAG_COPY);

    if (err != ERR_OK)
    {
        return 0U;
    }

    if (tcp_output(controller_client_pcb) != ERR_OK)
    {
        return 0U;
    }

    return 1U;
}