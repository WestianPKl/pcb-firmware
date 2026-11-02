#include "tcp_echo.h"

#include "lwip/tcp.h"

#define TCP_ECHO_PORT 5001U

static struct tcp_pcb *tcp_echo_listen_pcb = NULL;


static err_t tcp_echo_receive(void *arg,
                              struct tcp_pcb *tpcb,
                              struct pbuf *p,
                              err_t err)
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

    /*
     * p == NULL oznacza, że druga strona
     * zamknęła połączenie.
     */
    if (p == NULL)
    {
        tcp_close(tpcb);
        return ERR_OK;
    }

    /*
     * Informujemy lwIP, że odebrane dane
     * zostały przez nas przetworzone.
     */
    tcp_recved(tpcb, p->tot_len);

    /*
     * pbuf może być łańcuchem.
     */
    for (struct pbuf *q = p; q != NULL; q = q->next)
    {
        err_t write_err = tcp_write(tpcb,
                                    q->payload,
                                    q->len,
                                    TCP_WRITE_FLAG_COPY);

        if (write_err != ERR_OK)
        {
            pbuf_free(p);
            return write_err;
        }
    }

    /*
     * Poproś lwIP o rozpoczęcie wysyłania
     * zakolejkowanych danych.
     */
    tcp_output(tpcb);

    pbuf_free(p);

    return ERR_OK;
}


static err_t tcp_echo_accept(void *arg,
                             struct tcp_pcb *newpcb,
                             err_t err)
{
    (void)arg;

    if ((err != ERR_OK) || (newpcb == NULL))
    {
        return ERR_VAL;
    }

    /*
     * Dla nowego połączenia ustawiamy callback RX.
     */
    tcp_recv(newpcb, tcp_echo_receive);

    return ERR_OK;
}


uint8_t tcp_echo_init(void)
{
    struct tcp_pcb *pcb = tcp_new();

    if (pcb == NULL)
    {
        return 0U;
    }

    err_t err = tcp_bind(pcb,
                         IP_ADDR_ANY,
                         TCP_ECHO_PORT);

    if (err != ERR_OK)
    {
        tcp_close(pcb);
        return 0U;
    }

    /*
     * tcp_listen() przekształca PCB
     * w PCB nasłuchujące.
     */
    tcp_echo_listen_pcb = tcp_listen(pcb);

    if (tcp_echo_listen_pcb == NULL)
    {
        return 0U;
    }

    tcp_accept(tcp_echo_listen_pcb,
               tcp_echo_accept);

    return 1U;
}