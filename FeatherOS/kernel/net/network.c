/* FeatherOS - Network Stack Implementation
 * Sprint 19: Network Stack (Basic)
 */

#include <network.h>
#include <kernel.h>
#include <datastructures.h>
#include <sync.h>
#include <timer.h>

/*============================================================================
 * GLOBAL STATE
 *============================================================================*/

static net_device_t *net_devices = NULL;
static int net_dev_count = 0;
static network_stats_t net_stats = {0};

/* Receive and transmit queues */
static sk_buff_t *rx_queue_head = NULL;
static sk_buff_t *rx_queue_tail = NULL;
static int rx_queue_len = 0;
static spinlock_t rx_queue_lock;

static sk_buff_t *tx_queue_head = NULL;
static sk_buff_t *tx_queue_tail = NULL;
static int tx_queue_len = 0;
static spinlock_t tx_queue_lock;

/* ARP cache */
static arp_cache_entry_t arp_cache[ARP_CACHE_SIZE];
static spinlock_t arp_cache_lock;

/* Loopback device */
static net_device_t loopback_dev;
static uint8_t loopback_rx_buf[2048];
static int loopback_rx_len = 0;

/*============================================================================
 * NETWORK DEVICE REGISTRATION
 *============================================================================*/

int netdev_register(net_device_t *dev) {
    if (!dev || net_dev_count >= MAX_NET_DEVICES) {
        return -1;
    }
    
    /* Add to device list */
    dev->next = net_devices;
    net_devices = dev;
    net_dev_count++;
    
    console_print("net: Registered device %s (MAC: %02x:%02x:%02x:%02x:%02x:%02x)\n",
                  dev->name,
                  dev->mac.addr[0], dev->mac.addr[1], dev->mac.addr[2],
                  dev->mac.addr[3], dev->mac.addr[4], dev->mac.addr[5]);
    
    return 0;
}

int netdev_unregister(net_device_t *dev) {
    if (!dev) return -1;
    
    net_device_t **prev = &net_devices;
    net_device_t *cur = net_devices;
    
    while (cur) {
        if (cur == dev) {
            *prev = cur->next;
            net_dev_count--;
            return 0;
        }
        prev = &cur->next;
        cur = cur->next;
    }
    
    return -1;
}

net_device_t *netdev_get_by_name(const char *name) {
    net_device_t *dev = net_devices;
    
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
        dev = dev->next;
    }
    
    return NULL;
}

net_device_t *netdev_get_first(void) {
    return net_devices;
}

/*============================================================================
 * SK_BUFF OPERATIONS
 *============================================================================*/

sk_buff_t *skb_alloc(uint32_t size) {
    sk_buff_t *skb = (sk_buff_t *)kmalloc(sizeof(sk_buff_t));
    if (!skb) return NULL;
    
    /* Allocate data buffer */
    uint32_t total_size = size + sizeof(sk_buff_t) + 64; /* Extra for headers */
    uint8_t *buf = (uint8_t *)kmalloc(total_size);
    if (!buf) {
        kfree(skb);
        return NULL;
    }
    
    /* Initialize sk_buff */
    memset(skb, 0, sizeof(sk_buff_t));
    
    skb->head = buf;
    skb->data = buf + 64;  /* Leave room for headers */
    skb->tail = skb->data;
    skb->end = buf + total_size;
    skb->truesize = total_size;
    skb->len = 0;
    
    net_stats.skb_allocated++;
    
    return skb;
}

void skb_free(sk_buff_t *skb) {
    if (!skb) return;
    
    if (skb->head) {
        kfree(skb->head);
    }
    kfree(skb);
    net_stats.skb_freed++;
}

sk_buff_t *skb_clone(sk_buff_t *skb) {
    if (!skb) return NULL;
    
    sk_buff_t *clone = skb_alloc(skb->len);
    if (!clone) return NULL;
    
    /* Copy data */
    memcpy(clone->data, skb->data, skb->len);
    clone->len = skb->len;
    clone->protocol = skb->protocol;
    clone->dev = skb->dev;
    
    return clone;
}

uint8_t *skb_pull(sk_buff_t *skb, uint32_t len) {
    if (!skb || skb->len < len) return NULL;
    
    skb->data += len;
    skb->len -= len;
    
    return skb->data;
}

uint8_t *skb_push(sk_buff_t *skb, uint32_t len) {
    if (!skb) return NULL;
    
    if (skb->data - skb->head < (int)len) {
        /* Need to reallocate */
        return NULL;
    }
    
    skb->data -= len;
    skb->len += len;
    
    return skb->data;
}

uint8_t *skb_put(sk_buff_t *skb, uint32_t len) {
    if (!skb) return NULL;
    
    uint8_t *ret = skb->tail;
    
    skb->tail += len;
    skb->len += len;
    
    return ret;
}

void skb_trim(sk_buff_t *skb, uint32_t len) {
    if (!skb) return;
    
    if (len < skb->len) {
        skb->len = len;
        skb->tail = skb->data + len;
    }
}

/*============================================================================
 * ETHERNET OPERATIONS
 *============================================================================*/

int eth_tx(net_device_t *dev, sk_buff_t *skb, mac_addr_t *dest, uint16_t type) {
    if (!dev || !skb) return -1;
    
    /* Make room for header */
    uint8_t *header = skb_push(skb, sizeof(eth_header_t));
    if (!header) return -1;
    
    /* Build header */
    eth_header_t *eth = (eth_header_t *)header;
    memcpy(&eth->dest, dest, sizeof(mac_addr_t));
    memcpy(&eth->src, &dev->mac, sizeof(mac_addr_t));
    eth->type = type;
    
    skb->protocol = type;
    
    /* Update stats */
    dev->tx_packets++;
    dev->tx_bytes += skb->len;
    net_stats.tx_packets++;
    net_stats.tx_bytes += skb->len;
    
    /* Call device transmit function */
    if (dev->tx) {
        return dev->tx(dev, skb);
    }
    
    return -1;
}

int eth_rx(sk_buff_t *skb) {
    if (!skb) return -1;
    
    /* Parse header */
    eth_header_t *eth = (eth_header_t *)skb->data;
    mac_addr_t src, dest;
    uint16_t type;
    
    memcpy(&src, &eth->src, sizeof(mac_addr_t));
    memcpy(&dest, &eth->dest, sizeof(mac_addr_t));
    type = eth->type;
    
    /* Remove header */
    skb_pull(skb, sizeof(eth_header_t));
    skb->protocol = type;
    
    /* Update stats */
    net_stats.rx_packets++;
    net_stats.rx_bytes += skb->len;
    
    /* Dispatch to protocol handler */
    switch (type) {
        case ETH_P_IP:
            console_print("net: Received IP packet\n");
            break;
        case ETH_P_ARP:
            return arp_rx(skb);
        default:
            console_print("net: Unknown EtherType 0x%04x\n", type);
            break;
    }
    
    skb_free(skb);
    return 0;
}

void eth_build_header(uint8_t *data, mac_addr_t *src, mac_addr_t *dest, uint16_t type) {
    eth_header_t *eth = (eth_header_t *)data;
    memcpy(&eth->dest, dest, sizeof(mac_addr_t));
    memcpy(&eth->src, src, sizeof(mac_addr_t));
    eth->type = type;
}

int eth_parse_header(sk_buff_t *skb, mac_addr_t *src, mac_addr_t *dest, uint16_t *type) {
    if (!skb || skb->len < sizeof(eth_header_t)) {
        return -1;
    }
    
    eth_header_t *eth = (eth_header_t *)skb->data;
    
    if (src) memcpy(src, &eth->src, sizeof(mac_addr_t));
    if (dest) memcpy(dest, &eth->dest, sizeof(mac_addr_t));
    if (type) *type = eth->type;
    
    return 0;
}

/*============================================================================
 * ARP PROTOCOL
 *============================================================================*/

void arp_init(void) {
    memset(arp_cache, 0, sizeof(arp_cache));
    console_print("arp: ARP cache initialized (%d entries)\n", ARP_CACHE_SIZE);
}

int arp_rx(sk_buff_t *skb) {
    if (!skb || skb->len < sizeof(arp_header_t)) {
        skb_free(skb);
        return -1;
    }
    
    arp_header_t *arp = (arp_header_t *)skb->data;
    
    /* Verify ARP packet */
    if (arp->hw_type != ARP_HWTYPE_ETH || 
        arp->proto != ARP_PRO_IP ||
        arp->hw_len != 6 ||
        arp->proto_len != 4) {
        skb_free(skb);
        return -1;
    }
    
    switch (arp->op) {
        case ARP_OP_REQUEST:
            net_stats.arp_requests++;
            console_print("arp: ARP request from %d.%d.%d.%d for %d.%d.%d.%d\n",
                         arp->sip_addr[0], arp->sip_addr[1],
                         arp->sip_addr[2], arp->sip_addr[3],
                         arp->tip_addr[0], arp->tip_addr[1],
                         arp->tip_addr[2], arp->tip_addr[3]);
            
            /* Add sender to cache */
            arp_add(arp->sip_addr, &arp->shw_addr);
            
            /* Would send ARP reply here */
            break;
            
        case ARP_OP_REPLY:
            net_stats.arp_replies++;
            console_print("arp: ARP reply from %d.%d.%d.%d = %02x:%02x:%02x:%02x:%02x:%02x\n",
                         arp->sip_addr[0], arp->sip_addr[1],
                         arp->sip_addr[2], arp->sip_addr[3],
                         arp->shw_addr.addr[0], arp->shw_addr.addr[1],
                         arp->shw_addr.addr[2], arp->shw_addr.addr[3],
                         arp->shw_addr.addr[4], arp->shw_addr.addr[5]);
            
            /* Add to cache */
            arp_add(arp->sip_addr, &arp->shw_addr);
            break;
            
        default:
            console_print("arp: Unknown ARP operation %d\n", arp->op);
            break;
    }
    
    skb_free(skb);
    return 0;
}

int arp_request(net_device_t *dev, uint8_t *ip_addr) {
    if (!dev || !ip_addr) return -1;
    
    sk_buff_t *skb = skb_alloc(sizeof(arp_header_t));
    if (!skb) return -1;
    
    /* Build ARP request */
    arp_header_t *arp = (arp_header_t *)skb_put(skb, sizeof(arp_header_t));
    arp->hw_type = ARP_HWTYPE_ETH;
    arp->proto = ARP_PRO_IP;
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->op = ARP_OP_REQUEST;
    memcpy(&arp->shw_addr, &dev->mac, sizeof(mac_addr_t));
    memcpy(arp->sip_addr, ip_addr, 4);
    memset(&arp->thw_addr, 0, sizeof(mac_addr_t));
    memcpy(arp->tip_addr, ip_addr, 4);
    
    /* Send via Ethernet */
    mac_addr_t broadcast = MAC_BROADCAST;
    return eth_tx(dev, skb, &broadcast, ETH_P_ARP);
}

bool arp_lookup(uint8_t *ip_addr, mac_addr_t *mac) {
    if (!ip_addr) return false;
    
    spin_lock(&arp_cache_lock);
    
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && 
            memcmp(arp_cache[i].ip_addr, ip_addr, 4) == 0) {
            if (mac) {
                memcpy(mac, &arp_cache[i].mac_addr, sizeof(mac_addr_t));
            }
            net_stats.arp_cache_hits++;
            spin_unlock(&arp_cache_lock);
            return true;
        }
    }
    
    net_stats.arp_cache_misses++;
    spin_unlock(&arp_cache_lock);
    return false;
}

void arp_add(uint8_t *ip_addr, mac_addr_t *mac) {
    if (!ip_addr || !mac) return;
    
    spin_lock(&arp_cache_lock);
    
    /* Find existing entry or empty slot */
    int free_slot = -1;
    for (int i = 0; i < ARP_CACHE_SIZE; i++) {
        if (arp_cache[i].valid && 
            memcmp(arp_cache[i].ip_addr, ip_addr, 4) == 0) {
            free_slot = i;
            break;
        }
        if (!arp_cache[i].valid && free_slot < 0) {
            free_slot = i;
        }
    }
    
    if (free_slot >= 0) {
        memcpy(arp_cache[free_slot].ip_addr, ip_addr, 4);
        memcpy(&arp_cache[free_slot].mac_addr, mac, sizeof(mac_addr_t));
        arp_cache[free_slot].expires = get_uptime_ms() + 60000; /* 60 seconds */
        arp_cache[free_slot].valid = true;
    }
    
    spin_unlock(&arp_cache_lock);
}

/*============================================================================
 * LOOPBACK DEVICE
 *============================================================================*/

static uint8_t loopback_mac[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
static uint8_t loopback_ip[4] = {127, 0, 0, 1};

int loopback_init(void) {
    /* Initialize loopback device */
    memset(&loopback_dev, 0, sizeof(net_device_t));
    strncpy(loopback_dev.name, "lo", NET_DEVICE_NAME_SIZE - 1);
    memcpy(loopback_dev.mac.addr, loopback_mac, 6);
    loopback_dev.mtu = 65535;
    loopback_dev.flags = NET_DEVICE_UP | NET_DEVICE_RUNNING | NET_DEVICE_LOOPBACK;
    loopback_dev.tx = loopback_tx;
    loopback_dev.rx = NULL;
    
    /* Register loopback device */
    int ret = netdev_register(&loopback_dev);
    if (ret == 0) {
        console_print("loopback: Loopback device initialized\n");
        console_print("loopback: IP address 127.0.0.1\n");
        
        /* Add loopback to ARP cache */
        mac_addr_t mac;
        memcpy(mac.addr, loopback_mac, 6);
        arp_add(loopback_ip, &mac);
    }
    
    return ret;
}

int loopback_tx(net_device_t *dev, sk_buff_t *skb) {
    if (!dev || !skb) return -1;
    
    /* Copy to loopback buffer for receive */
    loopback_rx_len = (int)skb->len;
    if (loopback_rx_len > (int)sizeof(loopback_rx_buf)) {
        loopback_rx_len = (int)sizeof(loopback_rx_buf);
    }
    memcpy(loopback_rx_buf, skb->data, loopback_rx_len);
    
    /* Deliver to receive path */
    return loopback_rx(skb);
}

int loopback_rx(sk_buff_t *skb) {
    if (!skb) return -1;
    
    /* Just free - loopback is just for testing */
    loopback_dev.rx_packets++;
    loopback_dev.rx_bytes += skb->len;
    
    skb_free(skb);
    return 0;
}

/*============================================================================
 * SEND/RECEIVE QUEUES
 *============================================================================*/

void net_rx_queue_init(void) {
    rx_queue_head = NULL;
    rx_queue_tail = NULL;
    rx_queue_len = 0;
    spin_lock_init(&rx_queue_lock);
}

void net_tx_queue_init(void) {
    tx_queue_head = NULL;
    tx_queue_tail = NULL;
    tx_queue_len = 0;
    spin_lock_init(&tx_queue_lock);
}

int net_rx_enqueue(sk_buff_t *skb) {
    if (!skb) return -1;
    
    spin_lock(&rx_queue_lock);
    
    if (rx_queue_len >= NET_RX_QUEUE_SIZE) {
        spin_unlock(&rx_queue_lock);
        skb_free(skb);
        return -1;
    }
    
    skb->queue_next = NULL;
    skb->queue_prev = rx_queue_tail;
    
    if (rx_queue_tail) {
        rx_queue_tail->queue_next = skb;
    } else {
        rx_queue_head = skb;
    }
    rx_queue_tail = skb;
    rx_queue_len++;
    
    spin_unlock(&rx_queue_lock);
    return 0;
}

sk_buff_t *net_rx_dequeue(void) {
    spin_lock(&rx_queue_lock);
    
    if (!rx_queue_head) {
        spin_unlock(&rx_queue_lock);
        return NULL;
    }
    
    sk_buff_t *skb = rx_queue_head;
    rx_queue_head = skb->queue_next;
    
    if (rx_queue_head) {
        rx_queue_head->queue_prev = NULL;
    } else {
        rx_queue_tail = NULL;
    }
    rx_queue_len--;
    
    spin_unlock(&rx_queue_lock);
    return skb;
}

int net_tx_enqueue(sk_buff_t *skb) {
    if (!skb) return -1;
    
    spin_lock(&tx_queue_lock);
    
    if (tx_queue_len >= NET_TX_QUEUE_SIZE) {
        spin_unlock(&tx_queue_lock);
        skb_free(skb);
        return -1;
    }
    
    skb->queue_next = NULL;
    skb->queue_prev = tx_queue_tail;
    
    if (tx_queue_tail) {
        tx_queue_tail->queue_next = skb;
    } else {
        tx_queue_head = skb;
    }
    tx_queue_tail = skb;
    tx_queue_len++;
    
    spin_unlock(&tx_queue_lock);
    return 0;
}

sk_buff_t *net_tx_dequeue(void) {
    spin_lock(&tx_queue_lock);
    
    if (!tx_queue_head) {
        spin_unlock(&tx_queue_lock);
        return NULL;
    }
    
    sk_buff_t *skb = tx_queue_head;
    tx_queue_head = skb->queue_next;
    
    if (tx_queue_head) {
        tx_queue_head->queue_prev = NULL;
    } else {
        tx_queue_tail = NULL;
    }
    tx_queue_len--;
    
    spin_unlock(&tx_queue_lock);
    return skb;
}

void net_rx_poll(void) {
    sk_buff_t *skb = net_rx_dequeue();
    while (skb) {
        eth_rx(skb);
        skb = net_rx_dequeue();
    }
}

void net_tx_poll(void) {
    sk_buff_t *skb = net_tx_dequeue();
    while (skb) {
        if (skb->dev && skb->dev->tx) {
            skb->dev->tx(skb->dev, skb);
        } else {
            skb_free(skb);
        }
        skb = net_tx_dequeue();
    }
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

network_stats_t *network_get_stats(void) {
    return &net_stats;
}

void network_print_stats(void) {
    console_print("\n=== Network Statistics ===\n");
    console_print("SKBs allocated: %lu\n", net_stats.skb_allocated);
    console_print("SKBs freed: %lu\n", net_stats.skb_freed);
    console_print("RX packets: %lu (%lu bytes)\n", 
                  net_stats.rx_packets, net_stats.rx_bytes);
    console_print("TX packets: %lu (%lu bytes)\n",
                  net_stats.tx_packets, net_stats.tx_bytes);
    console_print("ARP requests: %lu\n", net_stats.arp_requests);
    console_print("ARP replies: %lu\n", net_stats.arp_replies);
    console_print("ARP cache hits: %lu\n", net_stats.arp_cache_hits);
    console_print("ARP cache misses: %lu\n", net_stats.arp_cache_misses);
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int network_init(void) {
    console_print("Initializing network stack...\n");
    
    /* Initialize queues */
    net_rx_queue_init();
    net_tx_queue_init();
    
    /* Initialize ARP */
    arp_init();
    
    /* Initialize loopback device */
    loopback_init();
    
    console_print("Network stack initialized\n");
    
    return 0;
}

/*============================================================================
 * PING TEST (Loopback)
 *============================================================================*/

int network_ping_loopback(void) {
    console_print("net: Testing loopback ping...\n");
    
    /* Create ICMP-like echo packet for loopback test */
    sk_buff_t *skb = skb_alloc(64);
    if (!skb) return -1;
    
    /* Fill with pattern */
    uint8_t *data = skb_put(skb, 64);
    for (int i = 0; i < 64; i++) {
        data[i] = i & 0xFF;
    }
    
    /* Send via loopback */
    net_device_t *lo = netdev_get_by_name("lo");
    if (!lo) {
        skb_free(skb);
        return -1;
    }
    
    /* Test ARP lookup */
    mac_addr_t mac;
    if (arp_lookup(loopback_ip, &mac)) {
        console_print("net: ARP lookup for 127.0.0.1: %02x:%02x:%02x:%02x:%02x:%02x\n",
                      mac.addr[0], mac.addr[1], mac.addr[2],
                      mac.addr[3], mac.addr[4], mac.addr[5]);
    }
    
    /* Send packet */
    int ret = lo->tx(lo, skb);
    
    if (ret == 0) {
        console_print("net: Loopback ping successful!\n");
        return 0;
    }
    
    return ret;
}
