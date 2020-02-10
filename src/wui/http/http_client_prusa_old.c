// http_client_prusa.c
// author: Migi

#include "http_client_prusa_old.h"
#include "lwip.h"
#include "string.h"

#define RECV_BUFFSIZE 512
#define DEST_PORT 9000
#define TCP_POLL_INTERVAL 2

char recv_buf[RECV_BUFFSIZE];
char send_buf[100];
uint16_t msg_count = 0;
struct tcp_pcb * web_client_pcb;

struct http_client_t {
    uint8_t state;
    struct tcp_pcb * pcb;
    struct pbuf * pbuf_ptr;
};

// Private function prototypes
static err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_recv_cb(void * arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_close_connection_cb(struct tcp_pcb *tpcb, struct http_client_t * hcp);
static err_t tcp_poll_cb(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_send(struct tcp_pcb *tpcb, struct http_client_t * hcp);

void tcp_connect_to_server(const char * msg){
    ip_addr_t dest_ip;
    web_client_pcb = tcp_new();
    strncpy(send_buf, msg, strlen(msg));
    if(web_client_pcb != NULL){
        IP4_ADDR(&dest_ip, 192, 168, 1, 152);   //IP of my computer/server
        tcp_connect(web_client_pcb, &dest_ip, DEST_PORT, tcp_connected_cb);
    }
}

static err_t tcp_connected_cb(void *arg, struct tcp_pcb *tpcb, err_t err){
    struct http_client_t * hcp = NULL;

    if(err != ERR_OK){
        tcp_close_connection_cb(tpcb, hcp);
        return err;
    }

    hcp = (struct http_client_t *)mem_malloc(sizeof(struct http_client_t));
    if(hcp == NULL){
        tcp_close_connection_cb(tpcb, hcp);
        return ERR_MEM;
    }

    hcp->state = HCS_CONNECTED;
    hcp->pcb = tpcb;

    /*allocate pbuf*/
    hcp->pbuf_ptr = pbuf_alloc(PBUF_TRANSPORT, strlen(send_buf), PBUF_POOL);

    if(hcp->pbuf_ptr != NULL){
        /*copy test message to pbuf*/
        pbuf_take(hcp->pbuf_ptr, send_buf, strlen(send_buf));

        /*pass wcp structure as argument to tpcb*/
        tcp_arg(tpcb, hcp);

        /*initialize lwIP tcp_recv callback func*/
        tcp_recv(tpcb, tcp_recv_cb);

        /*initialize lwIP tcp_poll callback func*/
        tcp_poll(tpcb, tcp_poll_cb, TCP_POLL_INTERVAL);

        /*initialize lwIP tcp_sent callback func*/
        tcp_sent(tpcb, tcp_sent_cb);

        /*send test data*/
        tcp_send(tpcb, hcp);

        return ERR_OK;
    }
    return ERR_MEM;
}

static err_t tcp_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    struct http_client_t *hcp;
    err_t return_err;

    LWIP_ASSERT("arg != NULL", arg != NULL);

    hcp = (struct http_client_t*) arg;
    /*If we recieve empty frame, we close the connection*/
    if(p == NULL){
        hcp->state = HCS_CLOSING;
        if(hcp->pbuf_ptr != NULL){
            /*We have something to send before closing*/
            tcp_send(tpcb, hcp);
        } else {
            tcp_close_connection_cb(tpcb, hcp);
        }
        return_err = ERR_OK;

    /*Else if frame is not empty, but err != ERR_OK*/
    } else if(err != ERR_OK){
        if(p != NULL){
            /*free the buffer*/
            pbuf_free(p);
        }
        return_err = err;

    /*Else if all is alright and connection is established*/
    } else if(hcp->state == HCS_CONNECTED){
        msg_count++;

        //READ & PARSE & COMMAND

        /* Acknowledge data reception */
        tcp_recved(tpcb, p->tot_len);

        pbuf_free(p);
        tcp_close_connection_cb(tpcb, hcp);
        return_err = ERR_OK;

    /*Data recieved when connection is already closed*/
    } else {

        /* Acknowledge data reception */
        tcp_recved(tpcb, p->tot_len);

        pbuf_free(p);
        return_err = ERR_OK;
    }
    return return_err;
}

static void tcp_send(struct tcp_pcb *tpcb, struct http_client_t * hcp){
    struct pbuf *ptr;
    err_t write_err = ERR_OK;

    while(write_err == ERR_OK && hcp->pbuf_ptr != NULL && hcp->pbuf_ptr->len <= tcp_sndbuf(tpcb)){
        ptr = hcp->pbuf_ptr;

        write_err = tcp_write(tpcb, ptr->payload, ptr->len, 1);

        if (write_err == ERR_OK)
        {
        /* continue with next pbuf in chain (if any) */
            hcp->pbuf_ptr = ptr->next;

            if(hcp->pbuf_ptr != NULL)
            {
                /* increment reference count for es->p */
                pbuf_ref(hcp->pbuf_ptr);
            }

        /* free pbuf: will free pbufs up to hcp->p (because hcp->p has a reference count > 0) */
        pbuf_free(ptr);
        } else if(write_err == ERR_MEM){
            /* we are low on memory, try later, defer to poll */
            hcp->pbuf_ptr = ptr;
        } else {
            /* other problem ?? */
        }
    }
}

static err_t tcp_sent_cb(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct http_client_t * hcp;

  LWIP_UNUSED_ARG(len);

  hcp = (struct http_client_t *)arg;

  if(hcp->pbuf_ptr != NULL)
  {
    /* still got pbufs to send */
    tcp_send(tpcb, hcp);
  }

  return ERR_OK;
}

static void tcp_close_connection_cb(struct tcp_pcb *tpcb, struct http_client_t * hcp )
{
  /* remove callbacks */
  tcp_recv(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_poll(tpcb, NULL,0);

  if (hcp != NULL)
  {
    mem_free(hcp);
  }

  /* close tcp connection */
  tcp_close(tpcb);
}

static err_t tcp_poll_cb(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct http_client_t * hcp;

  hcp = (struct http_client_t*)arg;
  if (hcp != NULL)
  {
    if (hcp->pbuf_ptr != NULL)
    {
      /* there is a remaining pbuf (chain) , try to send data */
      tcp_send(tpcb, hcp);
    }
    else
    {
      /* no remaining pbuf (chain)  */
      if(hcp->state == HCS_CLOSING)
      {
        /* close tcp connection */
        tcp_close_connection_cb(tpcb, hcp);
      }
    }
    ret_err = ERR_OK;
  }
  else
  {
    /* nothing to be done */
    tcp_abort(tpcb);
    ret_err = ERR_ABRT;
  }
  return ret_err;
}
