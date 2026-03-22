/* FeatherOS - TCP/IP Stack Header
 * Sprint 20: TCP/IP Stack
 */

#ifndef FEATHEROS_TCPIP_H
#define FEATHEROS_TCPIP_H

#include <stdint.h>
#include <stdbool.h>
#include <network.h>

/*============================================================================
 * IP PROTOCOL CONSTANTS
 *============================================================================*/

#define IP_VERSION_4      4
#define IP_HEADER_MIN     20
#define IP_HEADER_MAX     60
#define IP_PROTO_ICMP     1
#define IP_PROTO_TCP      6
#define IP_PROTO_UDP      17

/* IP fragmentation flags */
#define IP_DF             0x4000  /* Don't fragment */
#define IP_MF             0x2000  /* More fragments */

/*============================================================================
 * IP ADDRESS
 *============================================================================*/

typedef struct {
    uint8_t addr[4];
} ip_addr_t;

/* Well-known IP addresses - use without braces cast */
#define IP_ANY_ADDR       0, 0, 0, 0
#define IP_LOOPBACK_ADDR  127, 0, 0, 1
#define IP_BROADCAST_ADDR 255, 255, 255, 255

/*============================================================================
 * IP HEADER
 *============================================================================*/

typedef struct {
    uint8_t  ver_ihl;       /* Version (4) + IHL (5) */
    uint8_t  tos;           /* Type of service */
    uint16_t total_len;     /* Total length */
    uint16_t id;            /* Identification */
    uint16_t frag_offset;   /* Fragment offset */
    uint8_t  ttl;           /* Time to live */
    uint8_t  proto;         /* Protocol */
    uint16_t checksum;      /* Header checksum */
    ip_addr_t src_addr;     /* Source address */
    ip_addr_t dest_addr;    /* Destination address */
} __attribute__((packed)) ip_header_t;

/*============================================================================
 * TCP PROTOCOL CONSTANTS
 *============================================================================*/

#define TCP_PORT_MAX      65535
#define TCP_HEADER_MIN    20
#define TCP_HEADER_MAX    60
#define TCP_WINDOW_SIZE   65535

/* TCP flags */
#define TCP_FIN          0x01
#define TCP_SYN          0x02
#define TCP_RST          0x04
#define TCP_PSH          0x08
#define TCP_ACK          0x10
#define TCP_URG          0x20
#define TCP_ECE          0x40
#define TCP_CWR          0x80

/* TCP options */
#define TCP_OPT_END       0
#define TCP_OPT_NOP       1
#define TCP_OPT_MSS       2
#define TCP_OPT_WS        3
#define TCP_OPT_SACK_PERM 4
#define TCP_OPT_SACK      5
#define TCP_OPT_TS        8

/*============================================================================
 * TCP HEADER
 *============================================================================*/

typedef struct {
    uint16_t src_port;      /* Source port */
    uint16_t dest_port;     /* Destination port */
    uint32_t seq_num;       /* Sequence number */
    uint32_t ack_num;       /* Acknowledgment number */
    uint8_t  data_offset;   /* Data offset (4 bits) + reserved (4 bits) */
    uint8_t  flags;         /* Flags */
    uint16_t window;        /* Window size */
    uint16_t checksum;      /* Checksum */
    uint16_t urgent;        /* Urgent pointer */
} __attribute__((packed)) tcp_header_t;

/*============================================================================
 * TCP STATES
 *============================================================================*/

typedef enum {
    TCP_STATE_CLOSED = 0,
    TCP_STATE_LISTEN,
    TCP_STATE_SYN_SENT,
    TCP_STATE_SYN_RECEIVED,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_CLOSE_WAIT,
    TCP_STATE_FIN_WAIT_1,
    TCP_STATE_CLOSING,
    TCP_STATE_LAST_ACK,
    TCP_STATE_FIN_WAIT_2,
    TCP_STATE_TIME_WAIT
} tcp_state_t;

/* TCP state names */
#define TCP_STATE_NAMES { \
    "CLOSED", "LISTEN", "SYN_SENT", "SYN_RECEIVED", \
    "ESTABLISHED", "CLOSE_WAIT", "FIN_WAIT_1", "CLOSING", \
    "LAST_ACK", "FIN_WAIT_2", "TIME_WAIT" \
}

/*============================================================================
 * TCP SLIDING WINDOW
 *============================================================================*/

#define TCP_SND_WND       65535
#define TCP_RCV_WND       65535
#define TCP_SND_BUF_SIZE  (64 * 1024)
#define TCP_RCV_BUF_SIZE  (64 * 1024)

typedef struct {
    uint32_t una;           /* Unacknowledged */
    uint32_t nxt;          /* Next sequence */
    uint32_t wnd;          /* Window size */
    uint32_t up;           /* Urgent pointer */
    uint32_t wl1;          /* Segment sequence for window update */
    uint32_t wl2;          /* Segment acknowledgment for window update */
} tcp_window_t;

/*============================================================================
 * UDP PROTOCOL CONSTANTS
 *============================================================================*/

#define UDP_HEADER_SIZE   8

/*============================================================================
 * UDP HEADER
 *============================================================================*/

typedef struct {
    uint16_t src_port;      /* Source port */
    uint16_t dest_port;     /* Destination port */
    uint16_t length;       /* Length */
    uint16_t checksum;      /* Checksum */
} __attribute__((packed)) udp_header_t;

/*============================================================================
 * SOCKET
 *============================================================================*/

#define SOCKET_TYPE_STREAM    1
#define SOCKET_TYPE_DGRAM     2
#define SOCKET_TYPE_RAW       3

#define SOCKET_FAMILY_INET    2
#define SOCKET_FAMILY_UNIX    1

#define SOCKET_STATE_FREE     0
#define SOCKET_STATE_ALLOC    1
#define SOCKET_STATE_BOUND    2
#define SOCKET_STATE_LISTEN   3
#define SOCKET_STATE_CONNECT  4
#define SOCKET_STATE_OPEN     5

#define MAX_SOCKETS       256

typedef struct socket {
    int fd;                 /* File descriptor */
    int type;              /* Socket type */
    int family;            /* Address family */
    int protocol;          /* Protocol */
    int state;             /* Socket state */
    
    /* Addresses */
    ip_addr_t local_addr;
    uint16_t local_port;
    ip_addr_t remote_addr;
    uint16_t remote_port;
    
    /* TCP-specific */
    tcp_state_t tcp_state;
    tcp_window_t snd_wnd;
    tcp_window_t rcv_wnd;
    uint32_t iss;          /* Initial sequence number */
    uint32_t irs;          /* Initial received sequence */
    
    /* Buffers */
    uint8_t *send_buf;
    uint32_t send_len;
    uint8_t *recv_buf;
    uint32_t recv_len;
    
    /* Connection queue (for listening sockets) */
    struct socket *accept_queue[16];
    int accept_count;
    
    /* Reference count */
    int refcount;
    
    /* List linkage */
    struct socket *next;
} socket_t;

/*============================================================================
 * ROUTING TABLE
 *============================================================================*/

#define MAX_ROUTES  32

typedef struct {
    ip_addr_t network;
    ip_addr_t netmask;
    ip_addr_t gateway;
    net_device_t *dev;
    int metric;
} route_t;

/*============================================================================
 * IP OPERATIONS
 *============================================================================*/

/* Initialize IP layer */
void ip_init(void);

/* Send IP packet */
int ip_tx(net_device_t *dev, ip_addr_t *dest, uint8_t proto, 
          void *data, uint32_t len);

/* Receive IP packet */
int ip_rx(sk_buff_t *skb);

/* Build IP header */
void ip_build_header(ip_header_t *hdr, ip_addr_t *src, ip_addr_t *dest,
                    uint8_t proto, uint16_t payload_len);

/* Calculate IP checksum */
uint16_t ip_checksum(void *data, int len);

/* Fragment packet */
int ip_fragment(net_device_t *dev, sk_buff_t *skb, uint16_t mtu);

/*============================================================================
 * TCP OPERATIONS
 *============================================================================*/

/* Initialize TCP layer */
void tcp_init(void);

/* Create TCP socket */
socket_t *tcp_socket(int type, int protocol);

/* Bind socket to address */
int tcp_bind(socket_t *sock, ip_addr_t *addr, uint16_t port);

/* Listen for connections */
int tcp_listen(socket_t *sock, int backlog);

/* Accept connection */
socket_t *tcp_accept(socket_t *sock);

/* Connect to remote host */
int tcp_connect(socket_t *sock, ip_addr_t *addr, uint16_t port);

/* Send data */
int tcp_send(socket_t *sock, const void *data, uint32_t len);

/* Receive data */
int tcp_recv(socket_t *sock, void *buf, uint32_t len);

/* Close connection */
int tcp_close(socket_t *sock);

/* Process incoming TCP segment */
int tcp_rx(sk_buff_t *skb);

/* TCP state transition */
void tcp_set_state(socket_t *sock, tcp_state_t state);

/* Generate initial sequence number */
uint32_t tcp_generate_isn(void);

/* Calculate TCP checksum */
uint16_t tcp_checksum(ip_header_t *ip, tcp_header_t *tcp, uint32_t len);

/*============================================================================
 * UDP OPERATIONS
 *============================================================================*/

/* Initialize UDP layer */
void udp_init(void);

/* Create UDP socket */
socket_t *udp_socket(int type, int protocol);

/* Send UDP datagram */
int udp_sendto(socket_t *sock, const void *data, uint32_t len,
               ip_addr_t *dest, uint16_t port);

/* Receive UDP datagram */
int udp_recvfrom(socket_t *sock, void *buf, uint32_t len,
                 ip_addr_t *src, uint16_t *port);

/* Process incoming UDP datagram */
int udp_rx(sk_buff_t *skb);

/* Calculate UDP checksum */
uint16_t udp_checksum(ip_header_t *ip, udp_header_t *udp, uint32_t len);

/*============================================================================
 * SOCKET API
 *============================================================================*/

/* Initialize socket layer */
void socket_init(void);

/* Create socket */
socket_t *socket_create(int family, int type, int protocol);

/* Bind socket */
int socket_bind(socket_t *sock, ip_addr_t *addr, uint16_t port);

/* Listen on socket */
int socket_listen(socket_t *sock, int backlog);

/* Accept connection */
socket_t *socket_accept(socket_t *sock);

/* Connect */
int socket_connect(socket_t *sock, ip_addr_t *addr, uint16_t port);

/* Send data */
int socket_send(socket_t *sock, const void *data, uint32_t len);

/* Receive data */
int socket_recv(socket_t *sock, void *buf, uint32_t len);

/* Close socket */
int socket_close(socket_t *sock);

/* Find socket by address */
socket_t *socket_lookup(ip_addr_t *local_ip, uint16_t local_port,
                        ip_addr_t *remote_ip, uint16_t remote_port,
                        int protocol);

/*============================================================================
 * ROUTING TABLE
 *============================================================================*/

/* Initialize routing table */
void route_init(void);

/* Add route */
int route_add(ip_addr_t *network, ip_addr_t *netmask, 
              ip_addr_t *gateway, net_device_t *dev, int metric);

/* Delete route */
int route_del(ip_addr_t *network, ip_addr_t *netmask);

/* Lookup route for address */
route_t *route_lookup(ip_addr_t *addr);

/* Find gateway for address */
int route_lookup_gateway(ip_addr_t *addr, ip_addr_t *gateway, net_device_t **dev);

/* Print routing table */
void route_print(void);

/*============================================================================
 * STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t ip_tx_packets;
    uint64_t ip_rx_packets;
    uint64_t ip_tx_bytes;
    uint64_t ip_rx_bytes;
    uint64_t ip_fragments;
    uint64_t ip_reassembles;
    uint64_t tcp_tx_segments;
    uint64_t tcp_rx_segments;
    uint64_t tcp_retransmits;
    uint64_t tcp_connects;
    uint64_t tcp_accepts;
    uint64_t udp_tx_datagrams;
    uint64_t udp_rx_datagrams;
    uint64_t route_lookups;
    uint64_t socket_creates;
    uint64_t socket_closes;
} tcpip_stats_t;

tcpip_stats_t *tcpip_get_stats(void);
void tcpip_print_stats(void);

/*============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

/* Compare IP addresses */
bool ip_addr_equal(ip_addr_t *a, ip_addr_t *b);

/* Check if IP is broadcast */
bool ip_addr_is_broadcast(ip_addr_t *addr);

/* Check if IP is multicast */
bool ip_addr_is_multicast(ip_addr_t *addr);

/* Convert IP to string */
void ip_addr_to_str(ip_addr_t *addr, char *buf, size_t len);

/* Parse IP from string */
int ip_addr_from_str(const char *str, ip_addr_t *addr);

/* Network byte order helpers */
#define ip_ntohs(x) (((x & 0xFF) << 8) | ((x >> 8) & 0xFF))
#define ip_htons(x) (((x & 0xFF) << 8) | ((x >> 8) & 0xFF))
#define ip_ntohl(x) (((x & 0xFF) << 24) | ((x >> 8) & 0xFF) << 16 | \
                     ((x >> 16) & 0xFF) << 8 | ((x >> 24) & 0xFF))

#endif /* FEATHEROS_TCPIP_H */
