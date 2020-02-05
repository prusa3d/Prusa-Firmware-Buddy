// http_client_prusa.c
// author: Migi

#include "http_client_prusa.h"
#include "string.h"

#define RECV_BUFFSIZE 512
#define DEST_PORT 8
#define TCP_POLL_INTERVAL 2

web_client_t web_client;

char recv_buf[RECV_BUFFSIZE];
char data[100];
uint16_t msg_count = 0;
struct tcp_pcb * web_client_pcb;

struct http_client_t {
    uint8_t state;
    struct tcp_pcb * pcb;
    struct pbuf * pbuf_ptr;
};

// Private function prototypes
static err_t tcp_http_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static err_t tcp_http_client_recv(void * arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_http_client_connection_close(struct tcp_pcb *tpcb, struct http_client_t * hcp);
static err_t tcp_http_client_poll(void *arg, struct tcp_pcb *tpcb);
static err_t tcp_http_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static void tcp_http_client_send(struct tcp_pcb *tpcb, struct http_client_t * hcp);


void tcp_http_client_connect(void){
    ip_addr_t dest_ip;
    web_client_pcb = tcp_new();
    if(web_client_pcb != NULL){
        //netif get addrs
        //IP4_ADDR(&dest_ip, 1, 2, 3, 4);
        tcp_connect(web_client_pcb, &dest_ip, DEST_PORT, tcp_http_client_connected);
    }
}

static err_t tcp_http_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err){
    struct http_client_t * hcp = NULL;

    if(err != ERR_OK){
        tcp_http_client_connection_close(tpcb, hcp);
        return err;
    }

    hcp = (struct http_client_t *)mem_malloc(sizeof(struct http_client_t));
    if(hcp == NULL){
        tcp_http_client_connection_close(tpcb, hcp);
        return ERR_MEM;
    }

    hcp->state = HCS_CONNECTED;
    hcp->pcb = tpcb;

    sprintf(data, "sending tcp client message #%d", msg_count++);

    /*allocate pbuf*/
    hcp->pbuf_ptr = pbuf_alloc(PBUF_TRANSPORT, strlen(data), PBUF_POOL);

    if(hcp->pbuf_ptr != NULL){
        /*copy test message to pbuf*/
        pbuf_take(hcp->pbuf_ptr, data, strlen(data));

        /*pass wcp structure as argument to tpcb*/
        tcp_arg(tpcb, hcp);

        /*initialize lwIP tcp_recv callback func*/
        tcp_recv(tpcb, tcp_http_client_recv);

        /*initialize lwIP tcp_poll callback func*/
        tcp_poll(tpcb, tcp_http_client_poll, TCP_POLL_INTERVAL);

        /*initialize lwIP tcp_sent callback func*/
        tcp_sent(tpcb, tcp_http_client_sent);

        /*send test data*/
        tcp_http_client_send(tpcb, hcp);

        return ERR_OK;
    }
    return ERR_MEM;
}

static err_t tcp_http_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
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
            tcp_http_client_send(tpcb, hcp);
        } else {
            tcp_http_client_connection_close(tpcb, hcp);
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

        /* Acknowledge data reception */
        tcp_recved(tpcb, p->tot_len);

        pbuf_free(p);
        tcp_http_client_connection_close(tpcb, hcp);
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

static void tcp_http_client_send(struct tcp_pcb *tpcb, struct http_client_t * hcp){
    struct pbuf *ptr;
    err_t write_err;

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

static err_t tcp_http_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  struct http_client_t * hcp;

  LWIP_UNUSED_ARG(len);

  hcp = (struct http_client_t *)arg;

  if(hcp->pbuf_ptr != NULL)
  {
    /* still got pbufs to send */
    tcp_http_client_send(tpcb, hcp);
  }

  return ERR_OK;
}

static void tcp_http_client_connection_close(struct tcp_pcb *tpcb, struct http_client_t * hcp )
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

static err_t tcp_http_client_poll(void *arg, struct tcp_pcb *tpcb)
{
  err_t ret_err;
  struct http_client_t * hcp;

  hcp = (struct http_client_t*)arg;
  if (hcp != NULL)
  {
    if (hcp->pbuf_ptr != NULL)
    {
      /* there is a remaining pbuf (chain) , try to send data */
      tcp_http_client_send(tpcb, hcp);
    }
    else
    {
      /* no remaining pbuf (chain)  */
      if(hcp->state == HCS_CLOSING)
      {
        /* close tcp connection */
        tcp_http_client_connection_close(tpcb, hcp);
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

void web_client_init(void){
    memset(&web_client, 0, sizeof(web_client_t));
}

void web_client_queue_cycle(void){
    osEvent ose;
    char ch;

    if(web_client.flags & WUI_FLG_PEND_REQ){
        if(process_server_request()){
            web_client.flags &= ~WUI_FLG_PEND_REQ;
            web_client.request_len = 0;
        }
    }

    while ((ose = osMessageGet(web_client_queue_id, 0)).status == osEventMessage) {
        ch = (char)((uint8_t)(ose.value.v));
        switch (ch) {
        case '\r':
        case '\n':
            ch = 0;
            break;
        }
        if (web_client.request_len < MAX_REQUEST_LEN)
            web_client.request[web_client.request_len++] = ch;
        else {
            //TOO LONG
            web_client.request_len = 0;
        }
        if ((ch == 0) && (web_client.request_len > 1)) {
            if (process_server_request()) {
                web_client.request_len = 0;
            } else {
                web_client.flags |= WUI_FLG_PEND_REQ;
                break;
            }
        }
    }
}

int process_server_request(void){
    if(strncmp(web_client.request, "!g", 2) == 0){
        if(web_client.request_len < 5){
            return 2;
        }
        char gcode_str[MAX_MARLIN_REQUEST_LEN];
        strncpy(gcode_str, web_client.request + 3, web_client.request_len - 3);
        marlin_gcode_printf(gcode_str);
        return 1;
    }
    return 0;
}

void send_request_to_server(){

}
