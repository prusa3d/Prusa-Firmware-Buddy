/*
 * Telnet server example is based on single "user" thread
 * which listens for new connections and accept it.
 *
 * Only one connection is allowed for Telnet server
 */

#include <stdbool.h>
#include <stdarg.h> /* Required for printf */
#include "lwesp/lwesp.h"
#include "lwesp/lwesp_cli.h"
#include "cli/cli.h"
#include "cli/cli_input.h"

static lwesp_netconn_p client;
static bool close_conn = false;

static void telnet_cli_exit(cli_printf cliprintf, int argc, char** argv);

static const cli_command_t
telnet_commands[] = {
    { "exit",           "Close/Exit the terminal",                  telnet_cli_exit },
    { "close",          "Close/Exit the terminal",                  telnet_cli_exit },
};

/**
 * \brief           Telnet CLI to terminate the telnet connection
 * \param[in]       cliprintf: Pointer to CLI printf function
 * \param[in]       argc: Number fo arguments in argv
 * \param[in]       argv: Pointer to the commands arguments
 */
static void
telnet_cli_exit(cli_printf cliprintf, int argc, char** argv) {
    close_conn = true;
}

/**
 * \brief           Telnet CLI printf, used for CLI commands
 * \param[in]       fmt: Format for the printf
 */
static void
telnet_cli_printf(const char* fmt, ...) {
    static char tempStr[128];
    va_list argptr;
    int len = 1;

    memset(&tempStr, 0x00, sizeof(tempStr) );
    va_start(argptr, fmt);
    len = vsprintf(tempStr, fmt, argptr);
    va_end(argptr);

    if (len > 0 && len < 128) {
        lwesp_netconn_write(client, (uint8_t*)tempStr, len);
    }
}

/**
 * \brief           Telnet client config (disable ECHO and LINEMOD)
 * \param[in]       nc: Netconn handle used to write data to
 * \return          \ref lwespOK on success, member of \ref lwespr_t enumeration otherwise
 */
static lwespr_t
telnet_client_config(lwesp_netconn_p nc) {
    lwespr_t res;
    uint8_t cfg_data[12];

    /* do echo 'I will echo your chars' (RFC 857) */
    cfg_data[0] = 0xFF;
    cfg_data[1] = 0xFD;
    cfg_data[2] = 0x01;
    /* will echo */
    cfg_data[3] = 0xFF;
    cfg_data[4] = 0xFB;
    cfg_data[5] = 0x01;
    /* will SGA */
    cfg_data[6] = 0xFF;
    cfg_data[7] = 0xFB;
    cfg_data[8] = 0x03;
    /* don't LINEMODE 'Send each char as you get it' (RFC 1184) */
    cfg_data[9] = 0xFF;
    cfg_data[10] = 0xFE;
    cfg_data[11] = 0x22;

    res = lwesp_netconn_write(nc, cfg_data, sizeof(cfg_data));
    if (res != lwespOK) {
        return res;
    }

    return lwesp_netconn_flush(nc);
}

/**
 * \brief           Telnet command sequence check
 * \param[in]       ch: input byte from telnet
 * \ref             true when command sequence is active, else false
 */
static bool
telnet_command_sequence_check(char ch) {
    static uint32_t telnet_command_sequence = 0;
    bool command_sequence_found = false;

    if (!telnet_command_sequence && ch == 0xff) {
        command_sequence_found = true;
        telnet_command_sequence = 1;
        printf("AIC   ");
    } else if (telnet_command_sequence == 1) {
        command_sequence_found = true;
        telnet_command_sequence = 2;
        if (ch == 251) {
            printf("%-8s ", "WILL");
        } else if (ch == 252) {
            printf("%-8s ", "WON'T");
        } else if (ch == 253) {
            printf("%-8s ", "DO");
        } else if (ch == 254) {
            printf("%-8s ", "DON'T");
        } else {
            printf("%-8s ", "UNKNOWN");
        }
    } else if (telnet_command_sequence == 2) {
        command_sequence_found = true;
        telnet_command_sequence = 0;
        switch (ch) {
            case 0 :
                printf("Binary Transmission 0x%02x-%d\r\n", ch, ch);
                break;
            case 1 :
                printf("Echo 0x%02x-%d\r\n", ch, ch);
                break;
            case 2 :
                printf("Reconnection 0x%02x-%d\r\n", ch, ch);
                break;
            case 3 :
                printf("Suppress Go Ahead 0x%02x-%d\r\n", ch, ch);
                break;
            case 4 :
                printf("Approx Message Size Negotiation 0x%02x-%d\r\n", ch, ch);
                break;
            case 5 :
                printf("Status 0x%02x-%d\r\n", ch, ch);
                break;
            case 6 :
                printf("Timing Mark 0x%02x-%d\r\n", ch, ch);
                break;
            case 7 :
                printf("Remote Controlled Trans and Echo 0x%02x-%d\r\n", ch, ch);
                break;
            case 8 :
                printf("Output Line Width 0x%02x-%d\r\n", ch, ch);
                break;
            case 9 :
                printf("Output Page Size 0x%02x-%d\r\n", ch, ch);
                break;
            case 10:
                printf("Output Carriage-Return Disposition 0x%02x-%d\r\n", ch, ch);
                break;
            case 11:
                printf("Output Horizontal Tab Stops 0x%02x-%d\r\n", ch, ch);
                break;
            case 12:
                printf("Output Horizontal Tab Disposition 0x%02x-%d\r\n", ch, ch);
                break;
            case 13:
                printf("Output Formfeed Disposition 0x%02x-%d\r\n", ch, ch);
                break;
            case 14:
                printf("Output Vertical Tabstops 0x%02x-%d\r\n", ch, ch);
                break;
            case 15:
                printf("Output Vertical Tab Disposition 0x%02x-%d\r\n", ch, ch);
                break;
            case 16:
                printf("Output Linefeed Disposition 0x%02x-%d\r\n", ch, ch);
                break;
            case 17:
                printf("Extended ASCII 0x%02x-%d\r\n", ch, ch);
                break;
            case 18:
                printf("Logout 0x%02x-%d\r\n", ch, ch);
                break;
            case 19:
                printf("Byte Macro 0x%02x-%d\r\n", ch, ch);
                break;
            case 20:
                printf("Data Entry Terminal 0x%02x-%d\r\n", ch, ch);
                break;
            case 21:
                printf("SUPDUP 0x%02x-%d\r\n", ch, ch);
                break;
            case 22:
                printf("SUPDUP Output 0x%02x-%d\r\n", ch, ch);
                break;
            case 23:
                printf("Send Location 0x%02x-%d\r\n", ch, ch);
                break;
            case 24:
                printf("Terminal Type 0x%02x-%d\r\n", ch, ch);
                break;
            case 25:
                printf("End of Record 0x%02x-%d\r\n", ch, ch);
                break;
            case 26:
                printf("TACACS User Identification 0x%02x-%d\r\n", ch, ch);
                break;
            case 27:
                printf("Output Marking 0x%02x-%d\r\n", ch, ch);
                break;
            case 28:
                printf("Terminal Location Number 0x%02x-%d\r\n", ch, ch);
                break;
            case 29:
                printf("Telnet 3270 Regime 0x%02x-%d\r\n", ch, ch);
                break;
            case 30:
                printf("X.3 PAD 0x%02x-%d\r\n", ch, ch);
                break;
            case 31:
                printf("Negotiate About Window Size 0x%02x-%d\r\n", ch, ch);
                break;
            case 32:
                printf("Terminal Speed 0x%02x-%d\r\n", ch, ch);
                break;
            case 33:
                printf("Remote Flow Control 0x%02x-%d\r\n", ch, ch);
                break;
            case 34:
                printf("Linemode 0x%02x-%d\r\n", ch, ch);
                break;
            case 35:
                printf("X Display Location 0x%02x-%d\r\n", ch, ch);
                break;
            default:
                printf("UNKNOWN 0x%02x-%d \n\r", ch, ch);
        }
    }

    return command_sequence_found;
}

/**
 * \brief           Telnet server thread implementation
 * \param[in]       arg: User argument
 */
void
telnet_server_thread(void const* arg) {
    lwespr_t res;
    lwesp_pbuf_p pbuf;
    lwesp_netconn_p server;

    /*
     * First create a new instance of netconn
     * connection and initialize system message boxes
     * to accept clients and packet buffers
     */
    server = lwesp_netconn_new(LWESP_NETCONN_TYPE_TCP);
    if (server == NULL) {
        printf("Cannot create Telnet server\r\n");
        return;
    }
    printf("Server telnet created\r\n");

    /* Bind network connection to port 23 */
    res = lwesp_netconn_bind(server, 23);
    if (res != lwespOK) {
        printf("Telnet server cannot bind to port\r\n");
        lwesp_netconn_delete(server);
        return;
    }
    printf("Server telnet listens on port 23\r\n");

    /* Init command line interface and add telnet commands */
    cli_init();
    cli_register_commands(telnet_commands, LWESP_ARRAYSIZE(telnet_commands));
    lwesp_cli_register_commands();

    /*
     * Start listening for incoming connections
     * on previously binded port
     */
    res = lwesp_netconn_listen_with_max_conn(server, 1);
    while (1) {
        /*
         * Wait and accept new client connection
         *
         * Function will block thread until
         * new client is connected to server
         */
        res = lwesp_netconn_accept(server, &client);
        if (res != lwespOK) {
            printf("Telnet connection accept error!\r\n");
            break;
        }

        printf("Telnet new client connected.\r\n");

        /*
         * Inform telnet client that it should disable LINEMODE
         * and that we will echo for him.
         */
        res = telnet_client_config(client);
        if (res != lwespOK) {
            break;
        }

        while (1) {
            const uint8_t* in_data;
            size_t length;

            res = lwesp_netconn_receive(client, &pbuf);
            if (res == lwespCLOSED) {
                break;
            }

            in_data = lwesp_pbuf_data(pbuf);
            length = lwesp_pbuf_length(pbuf, 1);  /* Get length of received packet */

            for (size_t i = 0; i < length; ++i) {
                if (!telnet_command_sequence_check(in_data[i])) {
                    cli_in_data(telnet_cli_printf, in_data[i]);
                }
            }

            lwesp_pbuf_free(pbuf);
            lwesp_netconn_flush(client);

            if (close_conn) {
                close_conn = false;
                lwesp_netconn_close(client);      /* Close netconn connection */
                break;
            }
        }
        if (client != NULL) {
            lwesp_netconn_delete(client);         /* Delete netconn connection */
            client = NULL;
        }
    }
    if (client != NULL) {
        lwesp_netconn_delete(client);             /* Delete netconn connection */
        client = NULL;
    }

    lwesp_netconn_delete(server);                 /* Delete netconn structure */
    lwesp_sys_thread_terminate(NULL);             /* Terminate current thread */
}

