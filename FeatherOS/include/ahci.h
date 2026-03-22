/* FeatherOS - AHCI (SATA) Driver
 * Sprint 18: AHCI (SATA)
 */

#ifndef FEATHEROS_AHCI_H
#define FEATHEROS_AHCI_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 * AHCI REGISTERS
 *============================================================================*/

/* BAR5 - AHCI Base Address */
#define AHCI_BAR5_SIZE     0x4000

/* Generic Host Control Registers (offset from BAR5) */
#define AHCI_CAP          0x00  /* Host Capabilities */
#define AHCI_GHC          0x04  /* Global Host Control */
#define AHCI_IS           0x08  /* Interrupt Status */
#define AHCI_PI           0x0C  /* Ports Implemented */
#define AHCI_AHCI_VER     0x10  /* AHCI Version */
#define AHCI_CCC_CTL      0x14  /* Command Completion Coalescing Control */
#define AHCI_CCC_PORTS    0x18  /* Command Completion Coalescing Ports */
#define AHCI_EM_LOC       0x1C  /* Enclosure Management Location */
#define AHCI_EM_CTL       0x20  /* Enclosure Management Control */
#define AHCI_HBA_CAP      0x24  /* Host Capabilities Extended */
#define AHCIBIOS_OS_Handoff 0x28 /* BIOS/OS Handoff */

/*============================================================================
 * AHCI CAPABILITIES (CAP)
 *============================================================================*/

#define AHCI_CAP_NP_MASK   0x1F   /* Number of Ports (0-31) */
#define AHCI_CAP_SNC      (1 << 5)  /* Supports NCQ */
#define AHCI_CAP_SS       (1 << 6)  /* Supports Staggered Spin-up */
#define AHCI_CAP_SPM      (1 << 7)  /* Supports Port Multiplier */
#define AHCI_CAP_FBSS     (1 << 8)  /* FIS-based Switching Supported */
#define AHCI_CAP_PMD      (1 << 9)  /* PIO Multiple DRQ Block */
#define AHCI_CAP_SSC      (1 << 10) /* Slumber State Capable */
#define AHCI_CAP_NCQ       (1 << 21) /* NCQ Support */
#define AHCI_CAP_64BIT    (1 << 31) /* 64-bit Addressing */

/*============================================================================
 * GLOBAL HOST CONTROL (GHC)
 *============================================================================*/

#define AHCI_GHC_AE       (1 << 0)   /* AHCI Enable */
#define AHCI_GHC_MRSM     (1 << 2)   /* MSI Revert to Single Message */
#define AHCI_GHC_IE       (1 << 1)   /* Interrupt Enable */

/*============================================================================
 * PORT REGISTERS (per port, 0x100 bytes each)
 *============================================================================*/

#define AHCI_PORT_OFFSET   0x100
#define AHCI_PORT_SIZE     0x80

#define AHCI_PORT_CLB      0x00  /* Command List Base (32-bit aligned) */
#define AHCI_PORT_CLBU     0x04  /* Command List Base Upper (64-bit) */
#define AHCI_PORT_FB       0x08  /* FIS Base Address */
#define AHCI_PORT_FBU      0x0C  /* FIS Base Address Upper */
#define AHCI_PORT_IS       0x10  /* Interrupt Status */
#define AHCI_PORT_IE       0x14  /* Interrupt Enable */
#define AHCI_PORT_CMD      0x18  /* Command */
#define AHCI_PORT_TFD      0x20  /* Task File Data */
#define AHCI_PORT_sig      0x24  /* Signature */
#define AHCI_PORT_SSTS     0x28  /* SATA Status */
#define AHCI_PORT_SCTL     0x2C  /* SATA Control */
#define AHCI_PORT_SERR     0x30  /* SATA Error */
#define AHCI_PORT_SACT     0x34  /* SATA Active (NCQ) */
#define AHCI_PORT_CI       0x38  /* Command Issue */
#define AHCI_PORT_SNTF     0x3C  /* SATA Notification */

/* Port Command register bits */
#define AHCI_PORT_CMD_ST   (1 << 0)   /* Start */
#define AHCI_PORT_CMD_SUD  (1 << 1)   /* Spin-Up */
#define AHCI_PORT_CMD_POD  (1 << 2)   /* Power On DevSlp */
#define AHCI_PORT_CMD_CLO  (1 << 3)   /* Command List Override */
#define AHCI_PORT_CMD_FRE  (1 << 4)   /* FIS Receive Enable */
#define AHCI_PORT_CMD_CCS_MASK (0x1F << 8) /* Command Completion State */
#define AHCI_PORT_CMD_FR   (1 << 14)  /* FIS Receive Running */
#define AHCI_PORT_CMD_CR   (1 << 15)  /* Command List Running */
#define AHCI_PORT_CMD_APST (1 << 16)  /* Automatic Partial to Slumber */
#define AHCI_PORT_CMD_ATAPI (1 << 17) /* Device is ATAPI */
#define AHCI_PORT_CMD_DLAE (1 << 18)  /* Drive LED on ATAPI Enable */
#define AHCI_PORT_CMD_ALPE (1 << 19)  /* Aggressive Link Power Management Enable */
#define AHCI_PORT_CMD_APSTE (1 << 20) /* Aggressive Slumber Enable */
#define AHCI_PORT_CMD_AE   (1 << 31) /* Enable */

/* Port TFD register bits */
#define AHCI_PORT_TFD_ERR  (1 << 0)   /* Error */
#define AHCI_PORT_TFD_DRQ  (1 << 3)   /* Data Request */
#define AHCI_PORT_TFD_BSY  (1 << 7)   /* Busy */

/* SATA Status (SSTS) */
#define AHCI_PORT_SSTS_DET_MASK   0x0F  /* Device Detection */
#define AHCI_PORT_SSTS_DET_NONE  0x00   /* No device */
#define AHCI_PORT_SSTS_DET_PRES  0x01   /* Device detected */
#define AHCI_PORT_SSTS_DET_COMM  0x03   /* Communication established */
#define AHCI_PORT_SSTS_DET_OFF   0x04   /* Phy offline */
#define AHCI_PORT_SSTS_IPM_MASK  0xF00  /* Interface Power Management */
#define AHCI_PORT_SSTS_IPM_ACTIVE 0x100  /* Active */

/* SATA Error (SERR) */
#define AHCI_PORT_SERR_ERR  (1 << 0)  /* Error */
#define AHCI_PORT_SERR_NONFATAL (1 << 1)  /* Non-fatal error */
#define AHCI_PORT_SERR_FATAL (1 << 2)  /* Fatal error */

/*============================================================================
 * DEVICE SIGNATURES
 *============================================================================*/

#define AHCI_DEV_SATA     0x00000101  /* SATA drive */
#define AHCI_DEV_SATAPI    0xEB140101  /* SATA PM */
#define AHCI_DEV_SEMB     0xC33C0101  /* Enclosure management bridge */
#define AHCI_DEV_PM       0x0669101   /* Port multiplier */

/*============================================================================
 * FIS TYPES
 *============================================================================*/

#define FIS_TYPE_REG_H2D   0x27  /* Register FIS - Host to Device */
#define FIS_TYPE_REG_D2H   0x34  /* Register FIS - Device to Host */
#define FIS_TYPE_DMA_ACT   0x39  /* DMA Activate FIS - Device to Host */
#define FIS_TYPE_DMA_SETUP 0x41  /* DMA Setup FIS - Bidirectional */
#define FIS_TYPE_DATA      0x46  /* Data FIS - Bidirectional */
#define FIS_TYPE_BIST      0x58  /* BIST Activate FIS */
#define FIS_TYPE_PIO_SETUP 0x5F  /* PIO Setup FIS - Device to Host */
#define FIS_TYPE_SET_DEV_BITS 0xA1 /* Set Device Bits FIS - Device to Host */

/*============================================================================
 * COMMAND HEADER (32 bytes each)
 *============================================================================*/

typedef struct {
    uint32_t flags;       /* DW0: Flags and PRD byte count */
    uint32_t prdtl;       /* DW1: Physical Region Descriptor Table Length */
    uint32_t prdbc;       /* DW2: PRD Byte Count */
    uint32_t ctba;        /* DW3: Command Table Descriptor Base Address */
    uint32_t ctbau;       /* DW4: Command Table Descriptor Base Address Upper */
    uint32_t reserved[4]; /* DW5-8: Reserved */
} __attribute__((packed)) ahci_cmd_hdr_t;

/* Command header flags */
#define AHCI_CMD_HDR_CFL_MASK  0x1F  /* Command FIS Length (in DWORDS) */
#define AHCI_CMD_HDR_A         (1 << 5)  /* ATAPI */
#define AHCI_CMD_HDR_W         (1 << 6)  /* Write (device to host if clear) */
#define AHCI_CMD_HDR_P          (1 << 7)  /* Prefetchable */
#define AHCI_CMD_HDR_R          (1 << 8)  /* Reset */
#define AHCI_CMD_HDR_B          (1 << 9)  /* BIST */
#define AHCI_CMD_HDR_PRD_LEN_MASK 0x3FFFF  /* PRD Table Length (for NCQ) */

/*============================================================================
 * PHYSICAL REGION DESCRIPTOR (PRD)
 *============================================================================*/

typedef struct {
    uint32_t dba;         /* Data Base Address */
    uint32_t dbau;        /* Data Base Address Upper */
    uint32_t reserved;     /* Reserved */
    uint32_t dbc_i;       /* Dword Count - 1 (bit 31 = interrupt flag) */
} __attribute__((packed)) ahci_prd_t;

#define AHCI_PRD_I  (1 << 31) /* Interrupt on completion */
#define AHCI_PRD_DBC_MASK 0x3FFFFF /* Dword count - 1 (max 4MB) */

/*============================================================================
 * COMMAND TABLE (for each command)
 *============================================================================*/

typedef struct {
    uint8_t cfis[64];     /* Command FIS */
    uint8_t acmd[32];      /* ATAPI Command (12-16 bytes) */
    uint8_t reserved[32];   /* Reserved */
    /* Physical Region Descriptors follow at end of table */
} __attribute__((packed)) ahci_cmd_tbl_t;

/*============================================================================
 * FIS - Register FIS Host to Device
 *============================================================================*/

typedef struct {
    uint8_t fis_type;       /* FIS_TYPE_REG_H2D */
    uint8_t pmport_c;        /* Port multiplier port */
    uint8_t c;              /* Command (1) / Control (0) */
    uint8_t command;         /* Command */
    uint8_t featurel;        /* Feature (7:0) */
    uint8_t lba0;           /* LBA (7:0) */
    uint8_t lba1;           /* LBA (15:8) */
    uint8_t lba2;           /* LBA (23:16) */
    uint8_t device;         /* Device register */
    uint8_t lba3;           /* LBA (31:24) */
    uint8_t lba4;           /* LBA (39:32) */
    uint8_t lba5;           /* LBA (47:40) */
    uint8_t featureh;        /* Feature (15:8) */
    uint16_t count;         /* Count */
    uint8_t icc;           /* Isochronous Command Completion */
    uint8_t control;        /* Control */
    uint8_t rsvd1[4];      /* Reserved */
} __attribute__((packed)) ahci_fis_h2d_t;

/*============================================================================
 * AHCI CONTROLLER
 *============================================================================*/

typedef struct {
    /* Host capabilities */
    uint32_t cap;
    uint32_t ghc;
    uint32_t is;
    uint32_t pi;
    uint32_t ahci_ver;
    uint32_t ccc_ctl;
    uint32_t ccc_ports;
    uint32_t em_loc;
    uint32_t em_ctl;
    uint32_t hba_cap;
    uint32_t bios_handoff;
    
    /* Ports */
    uint8_t *port_base[32];
} ahci_controller_t;

/*============================================================================
 * AHCI PORT
 *============================================================================*/

typedef struct {
    uint32_t clb;      /* Command List Base */
    uint32_t clbu;     /* Command List Base Upper */
    uint32_t fb;       /* FIS Base */
    uint32_t fbu;      /* FIS Base Upper */
    uint32_t is;       /* Interrupt Status */
    uint32_t ie;       /* Interrupt Enable */
    uint32_t cmd;      /* Command */
    uint32_t rsvd01;
    uint32_t tfd;       /* Task File Data */
    uint32_t sig;      /* Signature */
    uint32_t ssts;     /* SATA Status */
    uint32_t sctl;     /* SATA Control */
    uint32_t serr;     /* SATA Error */
    uint32_t sact;     /* SATA Active */
    uint32_t ci;       /* Command Issue */
    uint32_t sntf;     /* SATA Notification */
    uint32_t fbs;     /* FIS-based Switch */
    uint32_t rsvd1[11];
    uint32_t vendor[4];
} ahci_port_t;

typedef struct {
    ahci_controller_t *controller;
    int port_num;
    ahci_port_t *port;
    uint32_t *abar;
    
    /* Device info */
    uint32_t signature;
    uint32_t sectors;
    bool present;
    bool ncq;
    bool lba48;
    bool dma;
    
    /* Command structures */
    ahci_cmd_hdr_t *cmd_list;
    uint8_t *fis_base;
    ahci_cmd_tbl_t *cmd_tbl;
    ahci_prd_t *prd_table;
} ahci_device_t;

/*============================================================================
 * AHCI INITIALIZATION
 *============================================================================*/

/* Initialize AHCI driver */
int ahci_init(void);

/* Detect AHCI controller */
int ahci_detect(void);

/* Reset AHCI controller */
int ahci_reset(void);

/* Enable AHCI */
int ahci_enable(void);

/*============================================================================
 * PORT OPERATIONS
 *============================================================================*/

/* Probe port for devices */
int ahci_probe_port(ahci_device_t *dev, int port);

/* Start port (enable DMA) */
int ahci_port_start(ahci_device_t *dev);

/* Stop port (disable DMA) */
int ahci_port_stop(ahci_device_t *dev);

/* Wait for port ready */
int ahci_port_wait_ready(ahci_device_t *dev, uint32_t timeout_ms);

/*============================================================================
 * DMA OPERATIONS
 *============================================================================*/

/* Read sectors using DMA */
int ahci_dma_read(ahci_device_t *dev, uint64_t lba, uint32_t count, void *buffer);

/* Write sectors using DMA */
int ahci_dma_write(ahci_device_t *dev, uint64_t lba, uint32_t count, const void *buffer);

/* Flush cache */
int ahci_flush_cache(ahci_device_t *dev);

/*============================================================================
 * NCQ (Native Command Queuing)
 *============================================================================*/

/* Issue NCQ read command */
int ahci_ncq_read(ahci_device_t *dev, uint64_t lba, uint32_t count, void *buffer, int tag);

/* Issue NCQ write command */
int ahci_ncq_write(ahci_device_t *dev, uint64_t lba, uint32_t count, const void *buffer, int tag);

/*============================================================================
 * HOTPLUG SUPPORT
 *============================================================================*/

/* Enable hotplug interrupts */
void ahci_enable_hotplug(int port);

/* Disable hotplug interrupts */
void ahci_disable_hotplug(int port);

/* Handle hotplug event */
void ahci_handle_hotplug(int port);

/* Poll for device presence */
bool ahci_device_present(ahci_device_t *dev);

/*============================================================================
 * INTERRUPTS
 *============================================================================*/

/* AHCI interrupt handler */
void ahci_interrupt_handler(void);

/* Clear port interrupt */
void ahci_clear_interrupt(int port);

/*============================================================================
 * STATISTICS
 *============================================================================*/

typedef struct {
    uint64_t reads;
    uint64_t writes;
    uint64_t read_bytes;
    uint64_t write_bytes;
    uint64_t dma_reads;
    uint64_t dma_writes;
    uint64_t ncq_commands;
    uint64_t hotplug_events;
    uint64_t errors;
} ahci_stats_t;

ahci_stats_t *ahci_get_stats(void);
void ahci_print_stats(void);

#endif /* FEATHEROS_AHCI_H */
