/* FeatherOS - AHCI (SATA) Driver Implementation
 * Sprint 18: AHCI (SATA)
 */

#include <ahci.h>
#include <ata.h>
#include <kernel.h>
#include <datastructures.h>
#include <sync.h>
#include <timer.h>

/*============================================================================
 * AHCI DEVICE AND STATISTICS
 *============================================================================*/

static ahci_device_t ahci_devices[32] = {0};
static int ahci_device_count = 0;
static ahci_stats_t ahci_stats = {0};
static bool ahci_initialized = false;
static uint32_t *ahci_bar5 = NULL;

/*============================================================================
 * PCI CONFIGURATION
 *============================================================================*/

/* PCI configuration space access */
static inline uint32_t pci_read(uint16_t bus, uint16_t dev, uint16_t func, uint8_t offset) {
    /* Would use PCI configuration space access mechanism */
    /* For now, return default AHCI BAR5 address */
    (void)bus;
    (void)dev;
    (void)func;
    (void)offset;
    return 0xE0000000;  /* Simulated */
}

/*============================================================================
 * AHCI DETECTION
 *============================================================================*/

static uint32_t *ahci_find_and_map(void) {
    /* Search PCI bus for AHCI controller */
    /* In real implementation, would enumerate PCI devices */
    
    /* Check for standard AHCI BAR5 location */
    console_print("AHCI: Scanning for controller...\n");
    
    /* Simulated - in real HW, would search PCI */
    return (uint32_t *)0xE0000000;
}

static bool ahci_check_signature(ahci_port_t *port) {
    uint32_t sig = port->sig;
    
    switch (sig) {
        case AHCI_DEV_SATA:
            return true;
        case AHCI_DEV_SATAPI:
            console_print("AHCI: ATAPI device found\n");
            return false;
        case AHCI_DEV_SEMB:
            console_print("AHCI: Enclosure management device\n");
            return false;
        case AHCI_DEV_PM:
            console_print("AHCI: Port multiplier\n");
            return false;
        default:
            return false;
    }
}

/*============================================================================
 * AHCI CONTROLLER INITIALIZATION
 *============================================================================*/

int ahci_detect(void) {
    if (ahci_initialized) {
        return ahci_device_count;
    }
    
    /* Find and map AHCI registers */
    ahci_bar5 = ahci_find_and_map();
    if (!ahci_bar5) {
        console_print("AHCI: No controller found\n");
        return 0;
    }
    
    /* Read capabilities register */
    uint32_t cap = ahci_bar5[0];
    uint32_t pi = ahci_bar5[3];  /* Ports implemented */
    
    console_print("AHCI: Controller found\n");
    console_print("  Capabilities:\n");
    console_print("    Ports: %d\n", (cap & AHCI_CAP_NP_MASK) + 1);
    console_print("    NCQ: %s\n", (cap & AHCI_CAP_NCQ) ? "supported" : "not supported");
    console_print("    64-bit: %s\n", (cap & AHCI_CAP_64BIT) ? "yes" : "no");
    
    /* Count implemented ports */
    int num_ports = 0;
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            num_ports++;
        }
    }
    
    console_print("  Ports implemented: %d\n", num_ports);
    
    ahci_device_count = 0;
    
    /* Probe each implemented port */
    for (int i = 0; i < 32; i++) {
        if (pi & (1 << i)) {
            ahci_device_t *dev = &ahci_devices[i];
            dev->port_num = i;
            dev->port = (ahci_port_t *)((uint8_t *)ahci_bar5 + 0x100 + i * 0x80);
            dev->controller = (ahci_controller_t *)ahci_bar5;
            dev->abar = ahci_bar5;
            
            /* Check device presence */
            uint32_t ssts = dev->port->ssts;
            if ((ssts & 0x0F) == AHCI_PORT_SSTS_DET_COMM) {
                if (ahci_check_signature((ahci_port_t *)dev->port)) {
                    dev->present = true;
                    dev->signature = dev->port->sig;
                    dev->ncq = (cap & AHCI_CAP_NCQ) != 0;
                    dev->dma = true;
                    
                    /* Get capacity from device */
                    dev->sectors = 0xFFFFFFFF;  /* Would query device */
                    
                    console_print("  Port %d: SATA device present\n", i);
                    ahci_device_count++;
                }
            }
        }
    }
    
    console_print("AHCI: %d device(s) detected\n", ahci_device_count);
    
    return ahci_device_count;
}

int ahci_reset(void) {
    if (!ahci_bar5) return -1;
    
    /* Get current control */
    uint32_t ghc = ahci_bar5[1];
    
    /* Enable AHCI */
    ghc |= AHCI_GHC_AE;
    ahci_bar5[1] = ghc;
    
    /* Reset controller */
    ahci_bar5[1] |= (1 << 1);  /* HR */
    
    /* Wait for reset complete */
    for (int i = 0; i < 1000; i++) {
        if (!(ahci_bar5[1] & (1 << 1))) {
            break;
        }
        delay_ms(1);
    }
    
    /* Re-enable AHCI */
    ahci_bar5[1] |= AHCI_GHC_AE;
    
    console_print("AHCI: Controller reset complete\n");
    
    return 0;
}

int ahci_enable(void) {
    if (!ahci_bar5) return -1;
    
    /* Enable interrupts */
    ahci_bar5[1] |= AHCI_GHC_IE;
    
    console_print("AHCI: Interrupts enabled\n");
    
    return 0;
}

/*============================================================================
 * PORT OPERATIONS
 *============================================================================*/

int ahci_port_start(ahci_device_t *dev) {
    if (!dev || !dev->present) return -1;
    
    ahci_port_t *port = dev->port;
    
    /* Clear any pending interrupts */
    port->is = 0xFFFFFFFF;
    port->ie = 0;
    
    /* Stop port */
    port->cmd &= ~AHCI_PORT_CMD_ST;
    port->cmd &= ~AHCI_PORT_CMD_FRE;
    
    /* Wait for CR and FR to clear */
    for (int i = 0; i < 500; i++) {
        if (!(port->cmd & (AHCI_PORT_CMD_CR | AHCI_PORT_CMD_FR))) {
            break;
        }
        delay_ms(1);
    }
    
    /* Allocate command list and FIS receive area (must be aligned to 1KB) */
    /* In real implementation, would allocate from memory */
    dev->cmd_list = (ahci_cmd_hdr_t *)0x100000;
    dev->fis_base = (uint8_t *)0x101000;
    dev->cmd_tbl = (ahci_cmd_tbl_t *)0x102000;
    
    /* Clear memory */
    for (int i = 0; i < 1024; i++) {
        ((uint32_t *)dev->cmd_list)[i] = 0;
        ((uint32_t *)dev->fis_base)[i] = 0;
    }
    
    /* Set command list base */
    port->clb = (uint32_t)((uintptr_t)dev->cmd_list);
    port->clbu = 0;
    
    /* Set FIS receive base */
    port->fb = (uint32_t)((uintptr_t)dev->fis_base);
    port->fbu = 0;
    
    /* Start port */
    port->cmd |= AHCI_PORT_CMD_FRE;
    port->cmd |= AHCI_PORT_CMD_ST;
    
    /* Enable interrupts */
    port->ie = 0x0F;  /* DRS, PSS, DPS, UFS */
    
    console_print("AHCI: Port %d started\n", dev->port_num);
    
    return 0;
}

int ahci_port_stop(ahci_device_t *dev) {
    if (!dev || !dev->port) return -1;
    
    ahci_port_t *port = dev->port;
    
    /* Stop port */
    port->cmd &= ~AHCI_PORT_CMD_ST;
    port->cmd &= ~AHCI_PORT_CMD_FRE;
    
    /* Wait for completion */
    for (int i = 0; i < 500; i++) {
        if (!(port->cmd & (AHCI_PORT_CMD_CR | AHCI_PORT_CMD_FR))) {
            break;
        }
        delay_ms(1);
    }
    
    return 0;
}

int ahci_port_wait_ready(ahci_device_t *dev, uint32_t timeout_ms) {
    if (!dev || !dev->port) return -1;
    
    uint32_t start = get_uptime_ms();
    
    while (get_uptime_ms() - start < timeout_ms) {
        uint32_t tfd = dev->port->tfd;
        if (!(tfd & ((uint32_t)(AHCI_PORT_TFD_BSY) | (uint32_t)(AHCI_PORT_TFD_DRQ)))) {
            if (tfd & AHCI_PORT_TFD_ERR) {
                return -1;  /* Error */
            }
            return 0;  /* Ready */
        }
        delay_ms(1);
    }
    
    return -1;  /* Timeout */
}

/*============================================================================
 * DMA OPERATIONS
 *============================================================================*/

int ahci_dma_read(ahci_device_t *dev, uint64_t lba, uint32_t count, void *buffer) {
    if (!dev || !dev->present) return -1;
    
    /* Simulated DMA read */
    ahci_stats.reads++;
    ahci_stats.read_bytes += count * 512;
    ahci_stats.dma_reads++;
    
    (void)lba;
    (void)buffer;
    
    return 0;
}

int ahci_dma_write(ahci_device_t *dev, uint64_t lba, uint32_t count, const void *buffer) {
    if (!dev || !dev->present) return -1;
    
    /* Simulated DMA write */
    ahci_stats.writes++;
    ahci_stats.write_bytes += count * 512;
    ahci_stats.dma_writes++;
    
    (void)lba;
    (void)buffer;
    
    return 0;
}

int ahci_flush_cache(ahci_device_t *dev) {
    if (!dev || !dev->present) return -1;
    
    /* Would issue flush cache command */
    return 0;
}

/*============================================================================
 * NCQ OPERATIONS
 *============================================================================*/

int ahci_ncq_read(ahci_device_t *dev, uint64_t lba, uint32_t count, void *buffer, int tag) {
    if (!dev || !dev->present || !dev->ncq) return -1;
    
    ahci_stats.ncq_commands++;
    
    /* NCQ would use SACT register and tag */
    (void)tag;
    (void)lba;
    (void)count;
    (void)buffer;
    
    return 0;
}

int ahci_ncq_write(ahci_device_t *dev, uint64_t lba, uint32_t count, const void *buffer, int tag) {
    if (!dev || !dev->present || !dev->ncq) return -1;
    
    ahci_stats.ncq_commands++;
    
    (void)tag;
    (void)lba;
    (void)count;
    (void)buffer;
    
    return 0;
}

/*============================================================================
 * HOTPLUG SUPPORT
 *============================================================================*/

void ahci_enable_hotplug(int port) {
    if (!ahci_bar5 || port >= 32) return;
    
    uint32_t *port_base = ahci_bar5 + 0x100/4 + port * 0x80/4;
    port_base[5] |= 0x0F;  /* Enable interrupts */
}

void ahci_disable_hotplug(int port) {
    if (!ahci_bar5 || port >= 32) return;
    
    uint32_t *port_base = ahci_bar5 + 0x100/4 + port * 0x80/4;
    port_base[5] = 0;
}

void ahci_handle_hotplug(int port) {
    ahci_stats.hotplug_events++;
    
    if (port >= 0 && port < 32) {
        ahci_device_t *dev = &ahci_devices[port];
        
        /* Check if device appeared or disappeared */
        uint32_t ssts = dev->port->ssts;
        bool present = ((ssts & 0x0F) == AHCI_PORT_SSTS_DET_COMM);
        
        if (present && !dev->present) {
            console_print("AHCI: Device hot-plugged on port %d\n", port);
            dev->present = true;
            dev->signature = dev->port->sig;
            ahci_device_count++;
        } else if (!present && dev->present) {
            console_print("AHCI: Device hot-unplugged on port %d\n", port);
            dev->present = false;
            ahci_device_count--;
        }
    }
}

bool ahci_device_present(ahci_device_t *dev) {
    if (!dev || !dev->port) return false;
    
    uint32_t ssts = dev->port->ssts;
    return ((ssts & 0x0F) == AHCI_PORT_SSTS_DET_COMM);
}

/*============================================================================
 * INTERRUPTS
 *============================================================================*/

void ahci_interrupt_handler(void) {
    if (!ahci_bar5) return;
    
    /* Read interrupt status */
    uint32_t is = ahci_bar5[2];
    
    if (!is) return;  /* Not for us */
    
    /* Clear interrupt status */
    ahci_bar5[2] = is;
    
    /* Process each port with pending interrupt */
    for (int i = 0; i < 32; i++) {
        if (is & (1 << i)) {
            /* Clear port interrupt */
            ahci_devices[i].port->is = 0xFFFFFFFF;
        }
    }
}

void ahci_clear_interrupt(int port) {
    if (port >= 0 && port < 32 && ahci_devices[port].port) {
        ahci_devices[port].port->is = 0xFFFFFFFF;
    }
}

/*============================================================================
 * INITIALIZATION
 *============================================================================*/

int ahci_init(void) {
    if (ahci_initialized) {
        return ahci_device_count;
    }
    
    console_print("Initializing AHCI driver...\n");
    
    /* Detect controller */
    if (ahci_detect() == 0) {
        console_print("AHCI: No controller found\n");
        return 0;
    }
    
    /* Reset controller */
    if (ahci_reset() != 0) {
        console_print("AHCI: Reset failed\n");
        return -1;
    }
    
    /* Enable AHCI */
    if (ahci_enable() != 0) {
        console_print("AHCI: Enable failed\n");
        return -1;
    }
    
    /* Start ports with devices */
    for (int i = 0; i < 32; i++) {
        if (ahci_devices[i].present) {
            ahci_port_start(&ahci_devices[i]);
        }
    }
    
    ahci_initialized = true;
    
    console_print("AHCI driver initialized: %d device(s)\n", ahci_device_count);
    
    return ahci_device_count;
}

/*============================================================================
 * STATISTICS
 *============================================================================*/

ahci_stats_t *ahci_get_stats(void) {
    return &ahci_stats;
}

void ahci_print_stats(void) {
    console_print("\n=== AHCI Statistics ===\n");
    console_print("Reads: %lu (%lu bytes)\n", 
                 ahci_stats.reads, ahci_stats.read_bytes);
    console_print("Writes: %lu (%lu bytes)\n",
                 ahci_stats.writes, ahci_stats.write_bytes);
    console_print("DMA reads: %lu\n", ahci_stats.dma_reads);
    console_print("DMA writes: %lu\n", ahci_stats.dma_writes);
    console_print("NCQ commands: %lu\n", ahci_stats.ncq_commands);
    console_print("Hotplug events: %lu\n", ahci_stats.hotplug_events);
    console_print("Errors: %lu\n", ahci_stats.errors);
}
