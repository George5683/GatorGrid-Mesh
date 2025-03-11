#include "MeshNode.hpp"
#include <random>
#include <ctime>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sstream>

extern "C" {
    #include "pico/cyw43_arch.h"
    #include "pico/stdlib.h"
    
    #include "lwip/pbuf.h"
    #include "lwip/tcp.h"
    
    #include "dhcpserver.h"
    #include "dnsserver.h"

    #include "hardware/vreg.h"
    #include "hardware/clocks.h"
}

// TCP Server definitions
#define TCP_PORT 80
#define DEBUG_printf printf
#define POLL_TIME_S 5
#define HTTP_GET "GET"
#define HTTP_RESPONSE_HEADERS "HTTP/1.1 %d OK\nContent-Length: %d\nContent-Type: text/html; charset=utf-8\nConnection: close\n\n"
#define LED_TEST_BODY "<html><body><h1>Hello from Pico.</h1><p>Led is %s</p><p><a href=\"?led=%d\">Turn led %s</a></body></html>"
#define LED_PARAM "led=%d"
#define LED_TEST "/ledtest"
#define LED_GPIO 0
#define HTTP_RESPONSE_REDIRECT "HTTP/1.1 302 Redirect\nLocation: http://%s" LED_TEST "\n\n"

// TCP Server state structures
typedef struct TCP_CONNECT_STATE_T_ {
    struct tcp_pcb *pcb;
    int sent_len;
    char headers[128];
    char result[256];
    int header_len;
    int result_len;
    ip_addr_t *gw;
    void* ap_node;
} TCP_CONNECT_STATE_T;

typedef struct TCP_SERVER_T_ {
    struct tcp_pcb *server_pcb;
    bool complete;
    ip_addr_t gw;
    void* ap_node;
    dhcp_server_t dhcp_server;
    dns_server_t dns_server;
} TCP_SERVER_T;

// Forward declarations for server functions
static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, struct tcp_pcb *client_pcb, err_t close_err);
static void tcp_server_close(TCP_SERVER_T *state);
static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len);
static int test_server_content(const char *request, const char *params, char *result, size_t max_result_len);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb);
static void tcp_server_err(void *arg, err_t err);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err);
static bool tcp_server_open(void *arg, const char *ap_name);
static void key_pressed_func(void *param);

// Implementation of these functions (same as in your original code)
static err_t tcp_close_client_connection(TCP_CONNECT_STATE_T *con_state, struct tcp_pcb *client_pcb, err_t close_err) {
    if (client_pcb) {
        assert(con_state && con_state->pcb == client_pcb);
        tcp_arg(client_pcb, NULL);
        tcp_poll(client_pcb, NULL, 0);
        tcp_sent(client_pcb, NULL);
        tcp_recv(client_pcb, NULL);
        tcp_err(client_pcb, NULL);
        err_t err = tcp_close(client_pcb);
        if (err != ERR_OK) {
            DEBUG_printf("close failed %d, calling abort\n", err);
            tcp_abort(client_pcb);
            close_err = ERR_ABRT;
        }
        if (con_state) {
            free(con_state);
        }
    }
    return close_err;
}

static void tcp_server_close(TCP_SERVER_T *state) {
    if (state->server_pcb) {
        tcp_arg(state->server_pcb, NULL);
        tcp_close(state->server_pcb);
        state->server_pcb = NULL;
    }
}

static err_t tcp_server_sent(void *arg, struct tcp_pcb *pcb, u16_t len) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    DEBUG_printf("tcp_server_sent %u\n", len);
    con_state->sent_len += len;
    if (con_state->sent_len >= con_state->header_len + con_state->result_len) {
        DEBUG_printf("all done\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    return ERR_OK;
}

static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    DEBUG_printf("tcp_server_poll_fn\n");
    return tcp_close_client_connection(con_state, pcb, ERR_OK); // Just disconnect client?
}

static void tcp_server_err(void *arg, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;
    if (err != ERR_ABRT) {
        DEBUG_printf("tcp_client_err_fn %d\n", err);
        tcp_close_client_connection(con_state, con_state->pcb, err);
    }
}

static err_t tcp_server_accept(void *arg, struct tcp_pcb *client_pcb, err_t err) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    if (err != ERR_OK || client_pcb == NULL) {
        DEBUG_printf("failure in accept\n");
        return ERR_VAL;
    }
    DEBUG_printf("client connected\n");

    // Create the state for the connection
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)calloc(1, sizeof(TCP_CONNECT_STATE_T));
    if (!con_state) {
        DEBUG_printf("failed to allocate connect state\n");
        return ERR_MEM;
    }
    con_state->pcb = client_pcb; // for checking
    con_state->gw = &state->gw;
    con_state->ap_node = state->ap_node;  // Store pointer to APNode instance

    // setup connection to client
    tcp_arg(client_pcb, con_state);
    tcp_sent(client_pcb, tcp_server_sent);
    tcp_recv(client_pcb, tcp_server_recv);
    tcp_poll(client_pcb, tcp_server_poll, POLL_TIME_S * 2);
    tcp_err(client_pcb, tcp_server_err);

    return ERR_OK;
}

static void key_pressed_func(void *param) {
    assert(param);
    TCP_SERVER_T *state = (TCP_SERVER_T*)param;
    int key = getchar_timeout_us(0); // get any pending key press but don't wait
    if (key == 'd' || key == 'D') {
        cyw43_arch_lwip_begin();
        cyw43_arch_disable_ap_mode();
        cyw43_arch_lwip_end();
        state->complete = true;
    }
}

// APNode class constructor
APNode::APNode() : state(nullptr), running(false), password("password"), webpage_enabled(false) {
    snprintf(ap_name, sizeof(ap_name), "GatorGrid_Node:%08X", get_NodeID());
    
}

// MeshNode class constructor
MeshNode::MeshNode(){
    // Seed the random number generator
    std::srand(static_cast<unsigned>(time(nullptr)));

    int ID = std::rand() % 10000 + 1; // Random number between 1 and 10,000
    // set the NodeID variable
    set_NodeID(ID);
}

// MeshNode deconstructor
MeshNode::~MeshNode(){
    NodeID = 0;
}

// set node ID from the APNode class
void APNode::set_node_id(int ID){
    // calls the base class function
    set_NodeID(ID);
}

// MeshNode base class function to set NodeID
void MeshNode::set_NodeID(int ID){
    // Assign ID to NodeID
    NodeID = ID; // Random number between 1 and 10,000
}

// APNode class function for getting the node id
int APNode::get_node_id(){
    return get_NodeID();
}

// MeshNode class function for getting Node ID
int MeshNode::get_NodeID(){
    return NodeID;
}

// APNode deconstructor
APNode::~APNode(){
    // Make sure AP mode is stopped
    stop_ap_mode();
    
    // Free the state if it exists
    if (state) {
        free(state);
        state = nullptr;
    }
    
    // Deinitialize the driver
    cyw43_arch_deinit();
}

bool APNode::init_ap_mode() {
    // Allocate the state of the TCP server if not already allocated
    if (!state) {
        state = (TCP_SERVER_T*)calloc(1, sizeof(TCP_SERVER_T));
        if (!state) {
            DEBUG_printf("failed to allocate state\n");
            return false;
        }
    }
    
    // Initialize the state
    state->complete = false;
    state->server_pcb = nullptr;
    state->ap_node = this;
    
    // Initialize the driver
    if (cyw43_arch_init()) {
        DEBUG_printf("failed to initialize cyw43 driver\n");
        return false;
    }
    
    // Get notified if the user presses a key
    stdio_set_chars_available_callback(key_pressed_func, state);
    
    return true;
}

void APNode::set_ap_credentials(char name[32], const char* pwd) {
    memcpy(ap_name, name, sizeof(ap_name));
    password = pwd;
}

void APNode::poll(unsigned int timeout_ms) {
    if (!running || !state) {
        return;
    }
    
    // Check if we should stop
    if (state->complete) {
        running = false;
        return;
    }
    
    // Handle polling operations based on the architecture
    #if PICO_CYW43_ARCH_POLL
    // If using poll architecture, check for work
    cyw43_arch_poll();
    // Wait for work or timeout
    cyw43_arch_wait_for_work_until(make_timeout_time_ms(timeout_ms));
    #else
    // If using interrupt architecture, just sleep
    sleep_ms(timeout_ms);
    #endif
}

void APNode::stop_ap_mode() {
    if (!running) {
        return;
    }
    
    // Disable the AP mode
    cyw43_arch_lwip_begin();
    cyw43_arch_disable_ap_mode();
    cyw43_arch_lwip_end();
    
    // Close the TCP server
    if (state && state->server_pcb) {
        tcp_server_close(state);
    }
    
    running = false;
}

// Function to enable a webpage on the access point
void APNode::enable_webpage() {
    if (!running || !state) {
        DEBUG_printf("AP not running, cannot enable webpage\n");
        return;
    }
    
    webpage_enabled = true;
    printf("Webpage enabled on the access point\n");
}

// Function to disable a webpage on the access point
void APNode::disable_webpage() {
    if (!running || !state) {
        DEBUG_printf("AP not running, cannot disable webpage\n");
        return;
    }
    
    webpage_enabled = false;
    printf("Webpage disabled on the access point\n");
}

// Modified HTTP response handler to only process data when webpage is enabled
static int test_server_content(APNode* ap_node, const char *request, const char *params, char *result, size_t max_result_len) {
    int len = 0;
    
    // Only process webpage requests if webpage is enabled
    if (!ap_node->webpage_enabled) {
        // Return minimal response when webpage is disabled
        len = snprintf(result, max_result_len, "<html><body><h1>Data Mode Only</h1><p>Web interface is disabled.</p></body></html>");
        return len;
    }
    
    if (strncmp(request, LED_TEST, sizeof(LED_TEST) - 1) == 0) {
        // Get the state of the led
        bool value;
        cyw43_gpio_get(&cyw43_state, LED_GPIO, &value);
        int led_state = value;

        // See if the user changed it
        if (params) {
            int led_param = sscanf(params, LED_PARAM, &led_state);
            if (led_param == 1) {
                if (led_state) {
                    // Turn led on
                    cyw43_gpio_set(&cyw43_state, LED_GPIO, true);
                } else {
                    // Turn led off
                    cyw43_gpio_set(&cyw43_state, LED_GPIO, false);
                }
            }
        }
        // Generate result
        if (led_state) {
            len = snprintf(result, max_result_len, LED_TEST_BODY, "ON", 0, "OFF");
        } else {
            len = snprintf(result, max_result_len, LED_TEST_BODY, "OFF", 1, "ON");
        }
    }
    return len;
}

// Modified TCP server receive function to print received data to serial
static err_t tcp_server_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    TCP_CONNECT_STATE_T *con_state = (TCP_CONNECT_STATE_T*)arg;

    APNode* ap_node = (APNode*)con_state->ap_node;
    
    if (!p) {
        DEBUG_printf("connection closed\n");
        return tcp_close_client_connection(con_state, pcb, ERR_OK);
    }
    assert(con_state && con_state->pcb == pcb);
    if (p->tot_len > 0) {
        DEBUG_printf("tcp_server_recv %d err %d\n", p->tot_len, err);
        
        // Copy the request into the buffer
        pbuf_copy_partial(p, con_state->headers, p->tot_len > sizeof(con_state->headers) - 1 ? sizeof(con_state->headers) - 1 : p->tot_len, 0);
        
        // Print received data to serial
        printf("Received data from client: %.*s\n", p->tot_len, con_state->headers);

        // Handle GET request if webpage is enabled
        if (ap_node->webpage_enabled && strncmp(HTTP_GET, con_state->headers, sizeof(HTTP_GET) - 1) == 0) {
            char *request = con_state->headers + sizeof(HTTP_GET); // + space
            char *params = strchr(request, '?');
            if (params) {
                if (*params) {
                    char *space = strchr(request, ' ');
                    *params++ = 0;
                    if (space) {
                        *space = 0;
                    }
                } else {
                    params = NULL;
                }
            }

            // Generate content
            con_state->result_len = test_server_content(ap_node, request, params, con_state->result, sizeof(con_state->result));
            DEBUG_printf("Request: %s?%s\n", request, params);
            DEBUG_printf("Result: %d\n", con_state->result_len);

            // Check we had enough buffer space
            if (con_state->result_len > sizeof(con_state->result) - 1) {
                DEBUG_printf("Too much result data %d\n", con_state->result_len);
                return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
            }

            // Generate web page
            if (con_state->result_len > 0) {
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_HEADERS,
                    200, con_state->result_len);
                if (con_state->header_len > sizeof(con_state->headers) - 1) {
                    DEBUG_printf("Too much header data %d\n", con_state->header_len);
                    return tcp_close_client_connection(con_state, pcb, ERR_CLSD);
                }
            } else {
                // Send redirect
                con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), HTTP_RESPONSE_REDIRECT,
                    ipaddr_ntoa(con_state->gw));
                DEBUG_printf("Sending redirect %s", con_state->headers);
            }

            // Send the headers to the client
            con_state->sent_len = 0;
            err_t err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
            if (err != ERR_OK) {
                DEBUG_printf("failed to write header data %d\n", err);
                return tcp_close_client_connection(con_state, pcb, err);
            }

            // Send the body to the client
            if (con_state->result_len) {
                err = tcp_write(pcb, con_state->result, con_state->result_len, 0);
                if (err != ERR_OK) {
                    DEBUG_printf("failed to write result data %d\n", err);
                    return tcp_close_client_connection(con_state, pcb, err);
                }
            }
        } else {
            // For non-GET requests or when webpage is disabled, just acknowledge the data
            // Send a simple response back to the client
            const char *response = "Data received";
            con_state->sent_len = 0;
            con_state->header_len = snprintf(con_state->headers, sizeof(con_state->headers), 
                "HTTP/1.1 200 OK\nContent-Length: %d\nContent-Type: text/plain\nConnection: close\n\n", 
                strlen(response));
            
            err_t write_err = tcp_write(pcb, con_state->headers, con_state->header_len, 0);
            if (write_err == ERR_OK) {
                tcp_write(pcb, response, strlen(response), 0);
            }
        }
        
        tcp_recved(pcb, p->tot_len);
    }
    pbuf_free(p);
    return ERR_OK;
}

// Simplified TCP server open function
static bool tcp_server_open(void *arg, const char *ap_name) {
    TCP_SERVER_T *state = (TCP_SERVER_T*)arg;
    DEBUG_printf("starting server on port %d\n", TCP_PORT);

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (!pcb) {
        DEBUG_printf("failed to create pcb\n");
        return false;
    }

    err_t err = tcp_bind(pcb, IP_ANY_TYPE, TCP_PORT);
    if (err) {
        DEBUG_printf("failed to bind to port %d\n",TCP_PORT);
        return false;
    }

    state->server_pcb = tcp_listen_with_backlog(pcb, 1);
    if (!state->server_pcb) {
        DEBUG_printf("failed to listen\n");
        if (pcb) {
            tcp_close(pcb);
        }
        return false;
    }

    tcp_arg(state->server_pcb, state);
    tcp_accept(state->server_pcb, tcp_server_accept);

    printf("Access point '%s' started. Waiting for client data...\n", ap_name);
    return true;
}

// Modified start_ap_mode to initialize with webpage disabled by default
bool APNode::start_ap_mode() {
    if (!state) {
        DEBUG_printf("APNode not initialized\n");
        return false;
    }
    
    // Enable the AP mode on the driver
    cyw43_arch_enable_ap_mode(ap_name, password, CYW43_AUTH_WPA2_AES_PSK);
    
    // Check if an IPV6 was assigned
    #if LWIP_IPV6
    // Get the union of the IPV4 and IPV6
    #define IP(x) ((x).u_addr.ip4)
    #else
    // if no IP is established, return x
    #define IP(x) (x)
    #endif
    
    // Set the mask and IP address of the access point
    ip4_addr_t mask;
    IP(state->gw).addr = PP_HTONL(CYW43_DEFAULT_IP_AP_ADDRESS);
    IP(mask).addr = PP_HTONL(CYW43_DEFAULT_IP_MASK);
    
    #undef IP
    
    // Start the DHCP server
    dhcp_server_init(&state->dhcp_server, &state->gw, &mask);
    
    // Start the DNS server
    dns_server_init(&state->dns_server, &state->gw);
    
    // Open the TCP server
    if (!tcp_server_open(state, ap_name)) {
        DEBUG_printf("failed to open server\n");
        return false;
    }
    
    // Webpage is disabled by default
    webpage_enabled = false;
    printf("AP mode started with webpage disabled. Use enable_webpage() to enable it.\n");
    
    running = true;
    return true;
}
