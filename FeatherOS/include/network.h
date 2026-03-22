/* FeatherOS - Network Stack Header
 * Sprint 19: Network Stack (Basic)
 */

#ifndef FEATHEROS_NETWORK_H
#define FEATHEROS_NETWORK_H

#include <stdint.h>
#include <stdbool.h>
#include <sync.h>

/*============================================================================
 * NETWORK CONSTANTS
 *============================================================================*/

#define ETH_FRAME_MIN      60    /* Minimum Ethernet frame size */
#define ETH_FRAME_MAX      1514  /* Maximum Ethernet frame size */
#define ETH_MTU            1500  /* Maximum Transmission Unit */
#define ETH_HEADER_SIZE    14    /* Ethernet header size */

#define ETH_P_IP           0x0800  /* Internet Protocol */
#define ETH_P_ARP          0x0806  /* Address Resolution Protocol */
#define ETH_P_IPV6         0x86DD /* IPv6 */

#define MAC_ADDR_SIZE      6

/*============================================================================
 * MAC ADDRESS
 *============================================================================*/

typedef struct {
    uint8_t addr[6];
} __attribute__((packed)) mac_addr_t;

/* Broadcast MAC address */
#define MAC_BROADCAST      {{0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}}

/* Null MAC address */
#define MAC_NULL           {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00}}

/*============================================================================
 * ETHERNET HEADER (IEEE 802.3)
 *============================================================================*/

typedef struct {
    mac_addr_t dest;       /* Destination MAC */
    mac_addr_t src;        /* Source MAC */
    uint16_t type;         /* EtherType or length */
} __attribute__((packed)) eth_header_t;

/*============================================================================
 * SK_BUFF (Socket Buffer)
 *============================================================================*/

#define SKB_DATA_SIZE      2048

typedef struct sk_buff {
    struct sk_buff *next;
    struct sk_buff *prev;
    
    /* Data pointers */
    uint8_t *data;         /* Pointer to data */
    uint8_t *head;         /* Head of buffer */
    uint8_t *tail;         /* Tail of buffer */
    uint8_t *end;          /* End of buffer */
    
    /* Data length */
    uint32_t len;          /* Length of data */
    uint32_t truesize;     /* Total allocated size */
    
    /* Protocol info */
    uint16_t protocol;     /* EtherType */
    
    /* Device reference */
    struct net_device *dev;
    
    /* Queue linkage */
    struct sk_buff *queue_next;
    struct sk_buff *queue_prev;
} sk_buff_t;

/*============================================================================
 * NETWORK DEVICE
 *============================================================================*/

#define NET_DEVICE_NAME_SIZE  16
#define MAX_NET_DEVICES       8

typedef struct net_device {
    char name[NET_DEVICE_NAME_SIZE];
    
    /* MAC address */
    mac_addr_t mac;
    
    /* MTU */
    uint16_t mtu;
    
    /* Flags */
    uint32_t flags;
    #define NET_DEVICE_UP       0x01
    #define NET_DEVICE_RUNNING  0x02
    #define NET_DEVICE_LOOPBACK 0x04
    #define NET_DEVICE_PROMISC  0x08
    
    /* Statistics */
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t rx_errors;
    uint64_t tx_errors;
    
    /* Device operations */
    int (*open)(struct net_device *dev);
    int (*stop)(struct net_device *dev);
    int (*tx)(struct net_device *dev, sk_buff_t *skb);
    int (*rx)(struct net_device *dev, sk_buff_t *skb);
    
    /* Private data */
    void *priv;
    
    /* List linkage */
    struct net_device *next;
} net_device_t;

/*============================================================================
 * NETWORK INTERFACES
 *============================================================================*/

/* Initialize network stack */
int network_init(void);

/* Register a network device */
int netdev_register(net_device_t *dev);

/* Unregister a network device */
int netdev_unregister(net_device_t *dev);

/* Find device by name */
net_device_t *netdev_get_by_name(const char *name);

/* Get first available device */
net_device_t *netdev_get_first(void);

/*============================================================================
 * SK_BUFF OPERATIONS
 *============================================================================*/

/* Allocate sk_buff */
sk_buff_t *skb_alloc(uint32_t size);

/* Free sk_buff */
void skb_free(sk_buff_t *skb);

/* Clone sk_buff */
sk_buff_t *skb_clone(sk_buff_t *skb);

/* Pull data from front */
uint8_t *skb_pull(sk_buff_t *skb, uint32_t len);

/* Push data to front */
uint8_t *skb_push(sk_buff_t *skb, uint32_t len);

/* Put data to back */
uint8_t *skb_put(sk_buff_t *skb, uint32_t len);

/* Trim sk_buff to len */
void skb_trim(sk_buff_t *skb, uint32_t len);

/*============================================================================
 * ETHERNET OPERATIONS
 *============================================================================*/

/* Send Ethernet frame */
int eth_tx(net_device_t *dev, sk_buff_t *skb, mac_addr_t *dest, uint16_t type);

/* Receive Ethernet frame */
int eth_rx(sk_buff_t *skb);

/* Build Ethernet header */
void eth_build_header(uint8_t *data, mac_addr_t *src, mac_addr_t *dest, uint16_t type);

/* Parse Ethernet header */
int eth_parse_header(sk_buff_t *skb, mac_addr_t *src, mac_addr_t *dest, uint16_t *type);

/*============================================================================
 * ARP PROTOCOL
 *============================================================================*/

#define ARP_OP_REQUEST     1
#define ARP_OP_REPLY       2

#define ARP_HWTYPE_ETH     1
#define ARP_PRO_IP         0x0800

typedef struct {
    uint16_t hw_type;      /* Hardware type */
    uint16_t proto;        /* Protocol type */
    uint8_t hw_len;        /* Hardware address length */
    uint8_t proto_len;     /* Protocol address length */
    uint16_t op;           /* Operation */
    mac_addr_t shw_addr;   /* Source hardware address */
    uint8_t sip_addr[4];   /* Source protocol address */
    mac_addr_t thw_addr;   /* Target hardware address */
    uint8_t tip_addr[4];   /* Target protocol address */
} __attribute__((packed)) arp_header_t;

/* ARP cache entry */
typedef struct {
    uint8_t ip_addr[4];
    mac_addr_t mac_addr;
    uint32_t expires;
    bool valid;
} arp_cache_entry_t;

#define ARP_CACHE_SIZE 64

/* Initialize ARP */
void arp_init(void);

/* Process incoming ARP packet */
int arp_rx(sk_buff_t *skb);

/* Send ARP request */
int arp_request(net_device_t *dev, uint8_t *ip_addr);

/* Lookup ARP cache */
bool arp_lookup(uint8_t *ip_addr, mac_addr_t *mac);

/* Add entry to ARP cache */
void arp_add(uint8_t *ip_addr, mac_addr_t *mac);

/*============================================================================
 * LOOPBACK DEVICE
 *============================================================================*/

/* Initialize loopback device */
int loopback_init(void);

/* Loopback transmit */
int loopback_tx(net_device_t *dev, sk_buff_t *skb);

/* Loopback receive */
int loopback_rx(sk_buff_t *skb);

/*============================================================================
 * SEND/RECEIVE QUEUES
 *============================================================================*/

#define NET_RX_QUEUE_SIZE  256
#define NET_TX_QUEUE_SIZE  256

/* Initialize receive queue */
void net_rx_queue_init(void);

/* Initialize transmit queue */
void net_tx_queue_init(void);

/* Enqueue to receive queue */
int net_rx_enqueue(sk_buff_t *skb);

/* Dequeue from receive queue */
sk_buff_t *net_rx_dequeue(void);

/* Enqueue to transmit queue */
int net_tx_enqueue(sk_buff_t *skb);

/* Dequeue from transmit queue */
sk_buff_t *net_tx_dequeue(void);

/* Process receive queue */
void net_rx_poll(void);

/* Process transmit queue */
void net_tx_poll(void);

/*============================================================================
 * STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t skb_allocated;
    uint64_t skb_freed;
    uint64_t rx_packets;
    uint64_t tx_packets;
    uint64_t rx_bytes;
    uint64_t tx_bytes;
    uint64_t arp_requests;
    uint64_t arp_replies;
    uint64_t arp_cache_hits;
    uint64_t arp_cache_misses;
} network_stats_t;

network_stats_t *network_get_stats(void);
void network_print_stats(void);

#endif /* FEATHEROS_NETWORK_H */
