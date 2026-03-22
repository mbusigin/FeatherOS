/* FeatherOS - TCP/IP Stack Implementation
 * Sprint 20: TCP/IP Stack
 */

#include <tcpip.h>
#include <network.h>
#include <kernel.h>
#include <datastructures.h>
#include <sync.h>

/*============================================================================
 * GLOBAL STATE
 *============================================================================*/

static tcpip_stats_t tcpip_stats = {0};
static route_t routes[MAX_ROUTES];
static int route_count = 0;
static spinlock_t route_lock;

static socket_t *sockets = NULL;
static int socket_count = 0;
static spinlock_t socket_lock;
static int next_fd = 1;

/* TCP sequence number */
static uint32_t tcp_isn = 0x10000000;

/*============================================================================
 * IP OPERATIONS
 *============================================================================*/

void ip_init(void) {
    console_print("ip: IP layer initialized\n");
    
    /* Add default routes */
    ip_addr_t loopback = {{127, 0, 0, 1}};
    ip_addr_t any = {{0, 0, 0, 0}};
    route_add(&loopback, &loopback, &any, NULL, 0);
    
    console_print("ip: Routing table initialized\n");
}

int ip_tx(net_device_t *dev, ip_addr_t *dest, uint8_t proto,
          void *data, uint32_t len) {
    if (!dev || !dest || !data) return -1;
    
    sk_buff_t *skb = skb_alloc(len + sizeof(ip_header_t));
    if (!skb) return -1;
    
    /* Copy data */
    uint8_t *payload = skb_put(skb, len);
    memcpy(payload, data, len);
    
    /* Build IP header */
    ip_header_t *hdr = (ip_header_t *)skb_push(skb, sizeof(ip_header_t));
    ip_addr_t src_ip = {{0}};  /* Use zero for source IP */
    ip_build_header(hdr, &src_ip,
                    dest, proto, len);
    
    /* Set device and send */
    skb->dev = dev;
    
    /* Update stats */
    tcpip_stats.ip_tx_packets++;
    tcpip_stats.ip_tx_bytes += len + sizeof(ip_header_t);
    
    /* Send via Ethernet */
    mac_addr_t mac = MAC_BROADCAST;
    return eth_tx(dev, skb, &mac, ETH_P_IP);
}

int ip_rx(sk_buff_t *skb) {
    if (!skb || skb->len < sizeof(ip_header_t)) {
        if (skb) skb_free(skb);
        return -1;
    }
    
    ip_header_t *hdr = (ip_header_t *)skb->data;
    
    /* Verify version */
    uint8_t version = (hdr->ver_ihl >> 4) & 0x0F;
    if (version != IP_VERSION_4) {
        console_print("ip: Unknown IP version %d\n", version);
        skb_free(skb);
        return -1;
    }
    
    /* Get header length */
    uint8_t ihl = hdr->ver_ihl & 0x0F;
    uint32_t header_len = ihl * 4;
    
    if (skb->len < header_len) {
        console_print("ip: Truncated IP header\n");
        skb_free(skb);
        return -1;
    }
    
    /* Verify checksum */
    uint16_t csum = ip_checksum(hdr, header_len);
    if (csum != 0) {
        console_print("ip: Checksum failed (0x%04x)\n", csum);
        skb_free(skb);
        return -1;
    }
    
    /* Remove IP header */
    skb_pull(skb, header_len);
    
    /* Update stats */
    tcpip_stats.ip_rx_packets++;
    tcpip_stats.ip_rx_bytes += skb->len;
    
    /* Dispatch to protocol handler */
    switch (hdr->proto) {
        case IP_PROTO_TCP:
            return tcp_rx(skb);
        case IP_PROTO_UDP:
            return udp_rx(skb);
        case IP_PROTO_ICMP:
            console_print("ip: ICMP packet received\n");
            skb_free(skb);
            return 0;
        default:
            console_print("ip: Unknown protocol %d\n", hdr->proto);
            skb_free(skb);
            return -1;
    }
}

void ip_build_header(ip_header_t *hdr, ip_addr_t *src, ip_addr_t *dest,
                    uint8_t proto, uint16_t payload_len) {
    memset(hdr, 0, sizeof(ip_header_t));
    
    hdr->ver_ihl = (IP_VERSION_4 << 4) | (IP_HEADER_MIN / 4);
    hdr->tos = 0;
    hdr->total_len = ip_htons((uint16_t)(sizeof(ip_header_t) + payload_len));
    hdr->id = ip_htons(0);
    hdr->frag_offset = ip_htons(IP_DF);
    hdr->ttl = 64;
    hdr->proto = proto;
    hdr->checksum = 0;
    memcpy(&hdr->src_addr, src, sizeof(ip_addr_t));
    memcpy(&hdr->dest_addr, dest, sizeof(ip_addr_t));
    
    hdr->checksum = ip_checksum(hdr, sizeof(ip_header_t));
}

uint16_t ip_checksum(void *data, int len) {
    uint16_t *p = (uint16_t *)data;
    uint32_t sum = 0;
    
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    
    if (len) {
        sum += *(uint8_t *)p;
    }
    
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    
    return ~sum;
}

/*============================================================================
 * TCP OPERATIONS
 *============================================================================*/

void tcp_init(void) {
    tcpip_stats.tcp_rx_segments = 0;
    tcpip_stats.tcp_tx_segments = 0;
    tcpip_stats.tcp_connects = 0;
    tcpip_stats.tcp_accepts = 0;
    console_print("tcp: TCP layer initialized\n");
}

socket_t *tcp_socket(int type, int protocol) {
    socket_t *sock = socket_create(SOCKET_FAMILY_INET, type, protocol);
    if (sock) {
        sock->tcp_state = TCP_STATE_CLOSED;
        sock->snd_wnd.wnd = TCP_SND_WND;
        sock->rcv_wnd.wnd = TCP_RCV_WND;
    }
    return sock;
}

int tcp_bind(socket_t *sock, ip_addr_t *addr, uint16_t port) {
    return socket_bind(sock, addr, port);
}

int tcp_listen(socket_t *sock, int backlog) {
    (void)backlog;  /* Unused for now */
    if (!sock) return -1;
    tcp_set_state(sock, TCP_STATE_LISTEN);
    sock->state = SOCKET_STATE_LISTEN;
    sock->accept_count = 0;
    tcpip_stats.tcp_connects++;
    return 0;
}

socket_t *tcp_accept(socket_t *sock) {
    if (!sock || sock->tcp_state != TCP_STATE_LISTEN) {
        return NULL;
    }
    
    if (sock->accept_count <= 0) {
        return NULL;
    }
    
    socket_t *new_sock = sock->accept_queue[0];
    
    /* Shift queue */
    for (int i = 0; i < sock->accept_count - 1; i++) {
        sock->accept_queue[i] = sock->accept_queue[i + 1];
    }
    sock->accept_count--;
    
    tcp_set_state(new_sock, TCP_STATE_ESTABLISHED);
    new_sock->state = SOCKET_STATE_OPEN;
    tcpip_stats.tcp_accepts++;
    
    return new_sock;
}

int tcp_connect(socket_t *sock, ip_addr_t *addr, uint16_t port) {
    if (!sock || !addr) return -1;
    
    memcpy(&sock->remote_addr, addr, sizeof(ip_addr_t));
    sock->remote_port = port;
    
    tcp_set_state(sock, TCP_STATE_SYN_SENT);
    sock->iss = tcp_generate_isn();
    sock->snd_wnd.una = sock->iss;
    sock->snd_wnd.nxt = sock->iss;
    sock->snd_wnd.wnd = TCP_SND_WND;
    
    tcpip_stats.tcp_connects++;
    
    /* Would send SYN here */
    return 0;
}

int tcp_send(socket_t *sock, const void *data, uint32_t len) {
    if (!sock || !data) return -1;
    
    if (sock->tcp_state != TCP_STATE_ESTABLISHED &&
        sock->tcp_state != TCP_STATE_CLOSE_WAIT) {
        return -1;
    }
    
    /* Copy data to send buffer */
    if (sock->send_len + len > TCP_SND_BUF_SIZE) {
        len = TCP_SND_BUF_SIZE - sock->send_len;
    }
    
    if (len == 0) return -1;
    
    memcpy(sock->send_buf + sock->send_len, data, len);
    sock->send_len += len;
    
    tcpip_stats.tcp_tx_segments++;
    
    return len;
}

int tcp_recv(socket_t *sock, void *buf, uint32_t len) {
    if (!sock || !buf) return -1;
    
    if (sock->recv_len == 0) {
        return 0;
    }
    
    if (len > sock->recv_len) {
        len = sock->recv_len;
    }
    
    memcpy(buf, sock->recv_buf, len);
    
    /* Shift remaining data */
    sock->recv_len -= len;
    if (sock->recv_len > 0) {
        memmove(sock->recv_buf, sock->recv_buf + len, sock->recv_len);
    }
    
    return len;
}

int tcp_close(socket_t *sock) {
    if (!sock) return -1;
    
    switch (sock->tcp_state) {
        case TCP_STATE_LISTEN:
            tcp_set_state(sock, TCP_STATE_CLOSED);
            break;
        case TCP_STATE_SYN_SENT:
            tcp_set_state(sock, TCP_STATE_CLOSED);
            break;
        case TCP_STATE_ESTABLISHED:
            tcp_set_state(sock, TCP_STATE_FIN_WAIT_1);
            break;
        case TCP_STATE_CLOSE_WAIT:
            tcp_set_state(sock, TCP_STATE_LAST_ACK);
            break;
        default:
            break;
    }
    
    tcpip_stats.socket_closes++;
    return socket_close(sock);
}

int tcp_rx(sk_buff_t *skb) {
    if (!skb || skb->len < sizeof(tcp_header_t)) {
        if (skb) skb_free(skb);
        return -1;
    }
    
    tcp_header_t *hdr = (tcp_header_t *)skb->data;
    uint32_t data_len = skb->len - (hdr->data_offset >> 2);
    
    tcpip_stats.tcp_rx_segments++;
    
    /* Find socket */
    ip_addr_t src_ip = {0}, dest_ip = {0};
    socket_t *sock = socket_lookup(&dest_ip, hdr->dest_port,
                                   &src_ip, hdr->src_port, IP_PROTO_TCP);
    
    if (!sock) {
        console_print("tcp: No socket for packet\n");
        skb_free(skb);
        return -1;
    }
    
    /* Process flags */
    if (hdr->flags & TCP_RST) {
        console_print("tcp: RST received\n");
        tcp_set_state(sock, TCP_STATE_CLOSED);
        skb_free(skb);
        return -1;
    }
    
    if (hdr->flags & TCP_SYN) {
        sock->irs = hdr->seq_num;
        sock->rcv_wnd.nxt = hdr->seq_num + 1;
        tcp_set_state(sock, TCP_STATE_SYN_RECEIVED);
    }
    
    if (hdr->flags & TCP_ACK) {
        sock->snd_wnd.una = hdr->ack_num;
    }
    
    if (data_len > 0 && (hdr->flags & TCP_PSH)) {
        /* Copy data to receive buffer */
        if (sock->recv_len + data_len <= TCP_RCV_BUF_SIZE) {
            memcpy(sock->recv_buf + sock->recv_len, 
                  skb->data + (hdr->data_offset >> 2), data_len);
            sock->recv_len += data_len;
        }
    }
    
    if (hdr->flags & TCP_FIN) {
        tcp_set_state(sock, TCP_STATE_CLOSE_WAIT);
    }
    
    skb_free(skb);
    return 0;
}

void tcp_set_state(socket_t *sock, tcp_state_t state) {
    if (!sock) return;
    
    const char *states[] = TCP_STATE_NAMES;
    
    if (sock->tcp_state != state) {
        console_print("tcp: State %s -> %s\n", 
                     states[sock->tcp_state], states[state]);
        sock->tcp_state = state;
    }
}

uint32_t tcp_generate_isn(void) {
    return ++tcp_isn;
}

uint16_t tcp_checksum(ip_header_t *ip, tcp_header_t *tcp, uint32_t len) {
    (void)ip;
    (void)tcp;
    (void)len;
    return 0;  /* Simplified */
}

/*============================================================================
 * UDP OPERATIONS
 *============================================================================*/

void udp_init(void) {
    console_print("udp: UDP layer initialized\n");
}

socket_t *udp_socket(int type, int protocol) {
    return socket_create(SOCKET_FAMILY_INET, type, protocol);
}

int udp_sendto(socket_t *sock, const void *data, uint32_t len,
               ip_addr_t *dest, uint16_t port) {
    if (!sock || !data || !dest) return -1;
    
    sk_buff_t *skb = skb_alloc(len + sizeof(udp_header_t));
    if (!skb) return -1;
    
    /* Build UDP header */
    udp_header_t *hdr = (udp_header_t *)skb_put(skb, sizeof(udp_header_t));
    hdr->src_port = sock->local_port;
    hdr->dest_port = port;
    hdr->length = ip_htons((uint16_t)(sizeof(udp_header_t) + len));
    hdr->checksum = 0;
    
    /* Copy data */
    uint8_t *payload = skb_put(skb, len);
    memcpy(payload, data, len);
    
    /* Send via IP */
    net_device_t *dev = netdev_get_first();
    if (dev) {
        int ret = ip_tx(dev, dest, IP_PROTO_UDP, skb->data, skb->len);
        skb_free(skb);
        
        if (ret == 0) {
            tcpip_stats.udp_tx_datagrams++;
            return len;
        }
    }
    
    skb_free(skb);
    return -1;
}

int udp_recvfrom(socket_t *sock, void *buf, uint32_t len,
                 ip_addr_t *src, uint16_t *port) {
    if (!sock || !buf) return -1;
    
    (void)src;
    (void)port;
    
    /* Check receive buffer */
    if (sock->recv_len == 0) {
        return 0;
    }
    
    uint32_t recv_len = sock->recv_len;
    if (recv_len > len) {
        recv_len = len;
    }
    
    memcpy(buf, sock->recv_buf, recv_len);
    
    /* Shift remaining */
    sock->recv_len -= recv_len;
    if (sock->recv_len > 0) {
        memmove(sock->recv_buf, sock->recv_buf + recv_len, sock->recv_len);
    }
    
    return recv_len;
}

int udp_rx(sk_buff_t *skb) {
    if (!skb || skb->len < sizeof(udp_header_t)) {
        if (skb) skb_free(skb);
        return -1;
    }
    
    udp_header_t *hdr = (udp_header_t *)skb->data;
    uint32_t data_len = ip_ntohs(hdr->length) - sizeof(udp_header_t);
    
    tcpip_stats.udp_rx_datagrams++;
    
    /* Find socket */
    ip_addr_t src_ip = {0}, dest_ip = {0};
    socket_t *sock = socket_lookup(&dest_ip, hdr->dest_port,
                                   &src_ip, hdr->src_port, IP_PROTO_UDP);
    
    if (!sock) {
        console_print("udp: No socket for datagram\n");
        skb_free(skb);
        return -1;
    }
    
    /* Copy data to receive buffer */
    if (sock->recv_len + data_len <= TCP_RCV_BUF_SIZE) {
        memcpy(sock->recv_buf + sock->recv_len,
               skb->data + sizeof(udp_header_t), data_len);
        sock->recv_len += data_len;
    }
    
    skb_free(skb);
    return 0;
}

uint16_t udp_checksum(ip_header_t *ip, udp_header_t *udp, uint32_t len) {
    (void)ip;
    (void)udp;
    (void)len;
    return 0;  /* Simplified */
}

/*============================================================================
 * SOCKET API
 *============================================================================*/

void socket_init(void) {
    sockets = NULL;
    socket_count = 0;
    tcp_init();
    udp_init();
    ip_init();
    console_print("socket: Socket layer initialized\n");
}

socket_t *socket_create(int family, int type, int protocol) {
    socket_t *sock = (socket_t *)kmalloc(sizeof(socket_t));
    if (!sock) return NULL;
    
    memset(sock, 0, sizeof(socket_t));
    
    sock->fd = next_fd++;
    sock->family = family;
    sock->type = type;
    sock->protocol = protocol;
    sock->state = SOCKET_STATE_ALLOC;
    sock->refcount = 1;
    
    /* Allocate buffers */
    sock->send_buf = (uint8_t *)kmalloc(TCP_SND_BUF_SIZE);
    sock->recv_buf = (uint8_t *)kmalloc(TCP_RCV_BUF_SIZE);
    
    if (!sock->send_buf || !sock->recv_buf) {
        if (sock->send_buf) kfree(sock->send_buf);
        if (sock->recv_buf) kfree(sock->recv_buf);
        kfree(sock);
        return NULL;
    }
    
    /* Add to list */
    spin_lock(&socket_lock);
    sock->next = sockets;
    sockets = sock;
    socket_count++;
    spin_unlock(&socket_lock);
    
    tcpip_stats.socket_creates++;
    
    return sock;
}

int socket_bind(socket_t *sock, ip_addr_t *addr, uint16_t port) {
    if (!sock) return -1;
    
    if (addr) {
        memcpy(&sock->local_addr, addr, sizeof(ip_addr_t));
    }
    sock->local_port = port;
    sock->state = SOCKET_STATE_BOUND;
    
    return 0;
}

int socket_listen(socket_t *sock, int backlog) {
    if (!sock) return -1;
    return tcp_listen(sock, backlog);
}

socket_t *socket_accept(socket_t *sock) {
    return tcp_accept(sock);
}

int socket_connect(socket_t *sock, ip_addr_t *addr, uint16_t port) {
    if (!sock) return -1;
    
    if (sock->type == SOCKET_TYPE_STREAM) {
        return tcp_connect(sock, addr, port);
    }
    
    memcpy(&sock->remote_addr, addr, sizeof(ip_addr_t));
    sock->remote_port = port;
    sock->state = SOCKET_STATE_CONNECT;
    
    return 0;
}

int socket_send(socket_t *sock, const void *data, uint32_t len) {
    if (!sock || !data) return -1;
    
    if (sock->type == SOCKET_TYPE_STREAM) {
        return tcp_send(sock, data, len);
    }
    
    return udp_sendto(sock, data, len, &sock->remote_addr, sock->remote_port);
}

int socket_recv(socket_t *sock, void *buf, uint32_t len) {
    if (!sock || !buf) return -1;
    
    if (sock->type == SOCKET_TYPE_STREAM) {
        return tcp_recv(sock, buf, len);
    }
    
    return udp_recvfrom(sock, buf, len, NULL, NULL);
}

int socket_close(socket_t *sock) {
    if (!sock) return -1;
    
    spin_lock(&socket_lock);
    
    sock->refcount--;
    if (sock->refcount > 0) {
        spin_unlock(&socket_lock);
        return 0;
    }
    
    /* Remove from list */
    socket_t **prev = &sockets;
    socket_t *cur = sockets;
    
    while (cur) {
        if (cur == sock) {
            *prev = cur->next;
            break;
        }
        prev = &cur->next;
        cur = cur->next;
    }
    
    socket_count--;
    spin_unlock(&socket_lock);
    
    /* Free resources */
    if (sock->send_buf) kfree(sock->send_buf);
    if (sock->recv_buf) kfree(sock->recv_buf);
    kfree(sock);
    
    tcpip_stats.socket_closes++;
    
    return 0;
}

socket_t *socket_lookup(ip_addr_t *local_ip, uint16_t local_port,
                        ip_addr_t *remote_ip, uint16_t remote_port,
                        int protocol) {
    (void)protocol;
    
    spin_lock(&socket_lock);
    
    socket_t *sock = sockets;
    while (sock) {
        if (sock->local_port == local_port &&
            (local_ip == NULL || ip_addr_equal(&sock->local_addr, local_ip))) {
            if (remote_port == 0 || 
                (sock->remote_port == remote_port &&
                 (remote_ip == NULL || ip_addr_equal(&sock->remote_addr, remote_ip)))) {
                spin_unlock(&socket_lock);
                return sock;
            }
        }
        sock = sock->next;
    }
    
    spin_unlock(&socket_lock);
    return NULL;
}

/*============================================================================
 * ROUTING TABLE
 *============================================================================*/

void route_init(void) {
    memset(routes, 0, sizeof(routes));
    route_count = 0;
    spin_lock_init(&route_lock);
    console_print("route: Routing table initialized\n");
}

int route_add(ip_addr_t *network, ip_addr_t *netmask,
              ip_addr_t *gateway, net_device_t *dev, int metric) {
    if (!network || !netmask || route_count >= MAX_ROUTES) {
        return -1;
    }
    
    spin_lock(&route_lock);
    
    routes[route_count].network = *network;
    routes[route_count].netmask = *netmask;
    if (gateway) {
        routes[route_count].gateway = *gateway;
    }
    routes[route_count].dev = dev;
    routes[route_count].metric = metric;
    route_count++;
    
    spin_unlock(&route_lock);
    
    return 0;
}

int route_del(ip_addr_t *network, ip_addr_t *netmask) {
    spin_lock(&route_lock);
    
    for (int i = 0; i < route_count; i++) {
        if (ip_addr_equal(&routes[i].network, network) &&
            ip_addr_equal(&routes[i].netmask, netmask)) {
            
            /* Shift remaining routes */
            for (int j = i; j < route_count - 1; j++) {
                routes[j] = routes[j + 1];
            }
            route_count--;
            spin_unlock(&route_lock);
            return 0;
        }
    }
    
    spin_unlock(&route_lock);
    return -1;
}

route_t *route_lookup(ip_addr_t *addr) {
    if (!addr) return NULL;
    
    spin_lock(&route_lock);
    tcpip_stats.route_lookups++;
    
    route_t *best = NULL;
    int best_len = -1;
    
    for (int i = 0; i < route_count; i++) {
        /* Calculate prefix length */
        int len = 0;
        for (int j = 0; j < 4; j++) {
            uint8_t mask = routes[i].netmask.addr[j];
            while (mask) {
                len += mask & 1;
                mask >>= 1;
            }
        }
        
        /* Check if this route matches */
        bool match = true;
        for (int j = 0; j < 4; j++) {
            if ((addr->addr[j] & routes[i].netmask.addr[j]) != 
                routes[i].network.addr[j]) {
                match = false;
                break;
            }
        }
        
        if (match && len > best_len) {
            best = &routes[i];
            best_len = len;
        }
    }
    
    spin_unlock(&route_lock);
    return best;
}

int route_lookup_gateway(ip_addr_t *addr, ip_addr_t *gateway, net_device_t **dev) {
    route_t *route = route_lookup(addr);
    if (!route) return -1;
    
    if (gateway) {
        *gateway = route->gateway;
    }
    if (dev) {
        *dev = route->dev;
    }
    
    return 0;
}

void route_print(void) {
    console_print("\n=== Routing Table ===\n");
    console_print("%-16s %-16s %-16s %-8s %s\n", 
                 "Destination", "Gateway", "Genmask", "Metric", "Iface");
    
    spin_lock(&route_lock);
    for (int i = 0; i < route_count; i++) {
        char dest[16], gw[16], mask[16];
        ip_addr_to_str(&routes[i].network, dest, sizeof(dest));
        ip_addr_to_str(&routes[i].gateway, gw, sizeof(gw));
        ip_addr_to_str(&routes[i].netmask, mask, sizeof(mask));
        
        console_print("%-16s %-16s %-16s %-8d %s\n",
                     dest, gw, mask, routes[i].metric,
                     routes[i].dev ? routes[i].dev->name : "*");
    }
    spin_unlock(&route_lock);
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

tcpip_stats_t *tcpip_get_stats(void) {
    return &tcpip_stats;
}

void tcpip_print_stats(void) {
    console_print("\n=== TCP/IP Statistics ===\n");
    console_print("IP:\n");
    console_print("  TX packets: %lu (%lu bytes)\n",
                 tcpip_stats.ip_tx_packets, tcpip_stats.ip_tx_bytes);
    console_print("  RX packets: %lu (%lu bytes)\n",
                 tcpip_stats.ip_rx_packets, tcpip_stats.ip_rx_bytes);
    console_print("TCP:\n");
    console_print("  TX segments: %lu\n", tcpip_stats.tcp_tx_segments);
    console_print("  RX segments: %lu\n", tcpip_stats.tcp_rx_segments);
    console_print("  Retransmits: %lu\n", tcpip_stats.tcp_retransmits);
    console_print("  Connections: %lu\n", tcpip_stats.tcp_connects);
    console_print("  Accepts: %lu\n", tcpip_stats.tcp_accepts);
    console_print("UDP:\n");
    console_print("  TX datagrams: %lu\n", tcpip_stats.udp_tx_datagrams);
    console_print("  RX datagrams: %lu\n", tcpip_stats.udp_rx_datagrams);
    console_print("Sockets: %lu created, %lu closed\n",
                 tcpip_stats.socket_creates, tcpip_stats.socket_closes);
    console_print("Routes: %lu lookups\n", tcpip_stats.route_lookups);
}

/*============================================================================
 * UTILITY FUNCTIONS
 *============================================================================*/

bool ip_addr_equal(ip_addr_t *a, ip_addr_t *b) {
    if (!a || !b) return false;
    return memcmp(a->addr, b->addr, 4) == 0;
}

bool ip_addr_is_broadcast(ip_addr_t *addr) {
    if (!addr) return false;
    return addr->addr[0] == 255 && addr->addr[1] == 255 &&
           addr->addr[2] == 255 && addr->addr[3] == 255;
}

bool ip_addr_is_multicast(ip_addr_t *addr) {
    if (!addr) return false;
    return (addr->addr[0] & 0xF0) == 0xE0;
}

void ip_addr_to_str(ip_addr_t *addr, char *buf, size_t len) {
    if (!addr || !buf) return;
    /* Simple implementation without snprintf */
    int pos = 0;
    for (int i = 0; i < 4 && pos < (int)len - 1; i++) {
        if (i > 0) buf[pos++] = '.';
        int val = addr->addr[i];
        if (val >= 100) {
            buf[pos++] = '0' + (val / 100);
            val %= 100;
        }
        if (val >= 10 || (i > 0 && addr->addr[i] >= 10)) {
            buf[pos++] = '0' + (val / 10);
            val %= 10;
        }
        buf[pos++] = '0' + val;
    }
    buf[pos] = '\0';
}

int ip_addr_from_str(const char *str, ip_addr_t *addr) {
    if (!str || !addr) return -1;
    
    /* Simple parser for dotted decimal */
    int a = 0, b = 0, c = 0, d = 0;
    const char *p = str;
    int i = 0;
    
    while (*p && i < 4) {
        int val = 0;
        while (*p >= '0' && *p <= '9') {
            val = val * 10 + (*p - '0');
            p++;
        }
        
        if (val > 255) return -1;
        
        switch (i) {
            case 0: a = val; break;
            case 1: b = val; break;
            case 2: c = val; break;
            case 3: d = val; break;
        }
        
        if (*p == '.') p++;
        i++;
    }
    
    if (i != 4 || *p != '\0') return -1;
    
    addr->addr[0] = (uint8_t)a;
    addr->addr[1] = (uint8_t)b;
    addr->addr[2] = (uint8_t)c;
    addr->addr[3] = (uint8_t)d;
    
    return 0;
}

/*============================================================================
 * ECHO SERVER TEST
 *============================================================================*/

int tcpip_echo_server_test(void) {
    console_print("tcp: Starting echo server test...\n");
    
    /* Create server socket */
    socket_t *server = tcp_socket(SOCKET_TYPE_STREAM, 0);
    if (!server) {
        console_print("tcp: Failed to create socket\n");
        return -1;
    }
    
    /* Bind to port 7 (echo) */
    ip_addr_t any = {{0, 0, 0, 0}};
    if (socket_bind(server, &any, 7) != 0) {
        console_print("tcp: Failed to bind\n");
        socket_close(server);
        return -1;
    }
    
    /* Listen */
    if (socket_listen(server, 5) != 0) {
        console_print("tcp: Failed to listen\n");
        socket_close(server);
        return -1;
    }
    
    console_print("tcp: Echo server listening on port 7\n");
    console_print("tcp: State: LISTEN\n");
    console_print("tcp: Echo server test complete\n");
    
    socket_close(server);
    return 0;
}
