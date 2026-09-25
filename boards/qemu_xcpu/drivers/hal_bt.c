/* Copyright (C) 2016 RDA Technologies Limited and/or its affiliates("RDA").
 * All rights reserved.
 *
 * RDABT 8809 Bluetooth HAL Driver Implementation
 */

#include "cs_types.h"
#include "global_macros.h"
#include "cfg_regs.h"
#include "hal_cfg_regs.h"
#include "hal_ispi.h"
#include "hal_i2c.h"
#include "hal_uart.h"
#include "timer.h"
#include "hal_bt.h"
#include "hal_pmd.h"
#include "bnep.h"
#include "FreeRTOS.h"
#include "task.h"

#include "extern.h"

extern void os_log_printf(const char *fmt, ...);
extern void hal_SysSetupAuxClock(BOOL enable);
extern void rdabt_mgr_generate_local_key(void);
extern void rdabt_mgr_calculate_dkkey(const uint8_t *peer_pub_key);
hal_bt_status_t g_bt_status = {0};
char g_test_result_msg[64] = {0};

static BOOL g_btPowered = FALSE;
static hal_i2c_bus_t g_bt_bus = HAL_I2C_BUS_2;

// PMU LDO Setting Register 0x02 bit 1 = btEnable
#define RDA_ADDR_LDO_SETTINGS        0x02
#define RDA_PMU_BT_ENABLE_BIT        (1 << 1)
#define CFG_REGS_BT_POWER_ON_BIT     (1 << 5)
#define CFG_REGS_BT_FEED_THROUGH_BIT (1 << 4)

// Official RDA8809E (R18 Core) Bluetooth RF Initialization Table (from RDA SDK rdabt_8809e_init.c)
static const uint16 rdabt_rf_init_18[][2] =
{
    {0x3F, 0x0000}, // page 0
    {0x01, 0x1fff}, // Padrv_gain_tb_en=1
    {0x06, 0x161c}, // Padrv_op_gain=11;Padrv_ibit_psk<3:0>=000;Padrv_input_range_psk<1:0>=11
    {0x07, 0x040d},
    {0x08, 0x8326}, // padrv_lo_tune_psk[5:0]=10000;Lna_notch_en=1
    {0x09, 0x04b5}, // rmx_imgrej=1
    {0x0B, 0x230f}, // Filter_cal_mode =1 (R18 value)
    {0x0C, 0x85e8}, // filter_bpmode<2:0>=101
    {0x0E, 0x0920}, // Tx_cal_polarity=1
    {0x0F, 0x8db3}, // adc_iq_swap=1
    {0x10, 0x1400}, // tx_sys_cal_sel<1:0>=10
    {0x12, 0x560c}, // Filter_cal_mode=0;padrv_ibit<3:0>=000;padrv_input_range<1:0>=00
    {0x14, 0x4ecc},
    {0x18, 0x0812}, // pll_refmulti2_en=1;
    {0x19, 0x10c8}, // pll_adcclk_en=1
    {0x1E, 0x3024}, // Pll_lpf_gain_tx<1:0>=00;Pll_pfd_res_tx<5:0>=100100
    // TX GAIN
    {0x23, 0x7777}, // PSK
    {0x24, 0x2368},
    {0x27, 0x5555}, // GFSK
    {0x28, 0x1358}, // (R18 value)
    {0x32, 0x0200}, // tx_dsp_reset_delay=2
    // AGC_by_xudonglin
    {0x3F, 0x0001}, // page 1
    {0x00, 0x020f},
    {0x01, 0xf9cf},
    {0x02, 0xfc2f},
    {0x03, 0xf92f},
    {0x04, 0xfa2f},
    {0x05, 0xfc2f},
    {0x06, 0xfb3f},
    {0x07, 0x7fff},
    {0x0A, 0x0018}, // thermo_pll_vcoibit_8<3:0>=1000
    // apc;padrv_gain
    {0x18, 0xffff},
    {0x19, 0xffff},
    {0x1A, 0xffff},
    {0x1B, 0xffff},
    {0x1C, 0xffff},
    {0x1D, 0xffff},
    {0x1E, 0xffff},
    {0x1F, 0xffff}, // padrv_gain; improve ACPR
    {0x22, 0x0e93},
    {0x25, 0x03e1},
    {0x26, 0x47a5}, // set voltage=1.2v
    {0x27, 0x0108}, // osc_stable_timer
    {0x28, 0x6800},
    {0x2D, 0x006a},
    {0x2F, 0x1100},
    {0x32, 0x88f9}, // TM=001;DN=1111
    {0x3F, 0x0000}, // page 0
};

// Official RDA8809E DC Calibration Table (from SDK rdabt_DC_write_r18)
static const uint16 rdabt_dccal_18[][2] =
{
    {0x30, 0x0129},
    {0x30, 0x012b},
};

// Official RDA8809E RF Calibration PSKey 0x26 (from SDK rdabt_pskey_rf_18)
static const uint32 rdabt_pskey_rf_18[] =
{
    0x0000000c, // CHIP_PS PSKEY: Total number -----------------
    0x003f0000, // PSKEY: Page 0
    0x0041000b, // TX_DC_OFFSET+/_
    0x00430fc6, // 43H=BFC6
    0x0044048f, // 44H=0580;
    0x004C400A,
    0x00694075, // TX_DC_OFFSET+/_
    0x006b10c0, //
    0x003f0001, // PSKEY: Page 1
    0x00453000, //
    0x0047f13b, // 20120512
    0x00490008, //
    0x003f0000, // PSKEY: Page 0
    0x10000000, // PSKEY: Flag
};

// RF Operating Setting PSKey 0x24 (VCO/PLL filter tuning for baseband)
static const uint8 rdabt_pskey_rf_setting[] = {0x00,0x0a,0x00,0x0c,0x40,0x30,0xb5,0x30,0xb5,0x30,0xba,0xba};

// System Configuration PSKey 0x15
static const uint8 rdabt_pskey_sys_config[] = {0x00,0x20,0x08,0x00};

// Audio PCM Configuration PSKey 0x17
static const uint8 rdabt_pskey_pcm_config[] = {0x07,0xc0,0x98,0x90};

// Baseband Core Register Setup (from SDK rda_pskey_18)
static const uint32 rda_pskey_18[][2] =
{
    {0x80000470, 0xfe8dfbff}, // enable EDR
    {0x80000474, 0x83793998}, // enable Simple Pairing (bit 19) + disable 3m esco ev4 ev5
    {0x40200010, 0x00007b7f}, // host wakeup / GPIO (keep RTS bit 10 cleared)
    {0x800004d0, 0x007803fd}, // set buffer size
    {0x800004d4, 0x00080006},
    {0x800004d8, 0x007803fd},
    {0x800004dc, 0x00080004},
    {0x800004e0, 0x00010078},
};

// Baseband ROM Bug Traps (SDK rdabt_8809e_init.c rda_trap_18)
static const uint32 rda_trap_18[219][2] = {
    {0x40180004, 0x0000f5ac},
    {0x40180024, 0x00001cf0},
    {0x40180008, 0x0000f6d0},
    {0x40180028, 0x00001cf0},
    {0x4018000c, 0x000156dc},
    {0x4018002c, 0x0002e958},
    {0x40180010, 0x00015318},
    {0x40180030, 0x0001a2cc},
    {0x80000000, 0xea000044},
    {0x80000100, 0xe51ff004},
    {0x80000104, 0x00021b98},
    {0x80000108, 0xe51ff004},
    {0x8000010c, 0x00021cc4},
    {0x80000110, 0xe51ff004},
    {0x80000114, 0x0000175c},
    {0x80000118, 0xe59f0024},
    {0x8000011c, 0xebfffff7},
    {0x80000120, 0xe3a00015},
    {0x80000124, 0xe1a01004},
    {0x80000128, 0xebfffff8},
    {0x8000012c, 0xe1a01004},
    {0x80000130, 0xe3a00018},
    {0x80000134, 0xebfffff5},
    {0x80000138, 0xe59f0008},
    {0x8000013c, 0xebfffff1},
    {0x80000140, 0xe8bd8010},
    {0x80000144, 0x80000148},
    {0x80000148, 0x00000000},
    {0x40180014, 0x00004754},
    {0x40180034, 0x00031e14},
    {0x80000004, 0xea001cC8},
    {0x40180018, 0x0001e164},
    {0x40180038, 0x00031e18},
    {0x80000008, 0xea001c99},
    {0x4018001c, 0x0001e048},
    {0x4018003c, 0x00031e1c},
    {0x80007200, 0xe51ff004},
    {0x80007204, 0x0002b808},
    {0x80007208, 0xe51ff004},
    {0x8000720c, 0x0001bfe8},
    {0x80007210, 0xe51ff004},
    {0x80007214, 0x0001db00},
    {0x80007218, 0xe51ff004},
    {0x8000721c, 0x0001dda4},
    {0x80007220, 0xe51ff004},
    {0x80007224, 0x000087e8},
    {0x80007228, 0xe51ff004},
    {0x8000722c, 0x0001dff8},
    {0x80007230, 0xe51ff004},
    {0x80007234, 0x00024128},
    {0x80007238, 0xe51ff004},
    {0x8000723c, 0x0001e648},
    {0x80007240, 0xe92d4008},
    {0x80007244, 0xe3a008c5},
    {0x80007248, 0xebffffec},
    {0x8000724c, 0xe59f101c},
    {0x80007250, 0xe3c00e40},
    {0x80007254, 0xe5d11002},
    {0x80007258, 0xe2800004},
    {0x8000725c, 0xe08101a0},
    {0x80007260, 0xe2400064},
    {0x80007264, 0xe1a00c00},
    {0x80007268, 0xe1a00c40},
    {0x8000726c, 0xe8bd8008},
    {0x80007270, 0x800027d4},
    {0x80007274, 0xe92d4070},
    {0x80007278, 0xe59f609c},
    {0x8000727c, 0xe1a05000},
    {0x80007280, 0xe5d64008},
    {0x80007284, 0xe1a00004},
    {0x80007288, 0xebffffde},
    {0x8000728c, 0xe3500000},
    {0x80007290, 0x08bd8070},
    {0x80007294, 0xe2041007},
    {0x80007298, 0xe59f0080},
    {0x8000729c, 0xe3a02001},
    {0x800072a0, 0xe7d001a4},
    {0x800072a4, 0xe1100112},
    {0x800072a8, 0x08bd8070},
    {0x800072ac, 0xe1d600da},
    {0x800072b0, 0xe59f206c},
    {0x800072b4, 0xe1d610db},
    {0x800072b8, 0xe1d23dd3},
    {0x800072bc, 0xe59f2064},
    {0x800072c0, 0xe1530000},
    {0x800072c4, 0xba000004},
    {0x800072c8, 0xe281300a},
    {0x800072cc, 0xe1500003},
    {0x800072d0, 0xda000004},
    {0x800072d4, 0xe3710046},
    {0x800072d8, 0xaa000002},
    {0x800072dc, 0xe7d23004},
    {0x800072e0, 0xe2833050},
    {0x800072e4, 0xe7c23004},
    {0x800072e8, 0xe7d22004},
    {0x800072ec, 0xe3520070},
    {0x800072f0, 0x81a01004},
    {0x800072f4, 0x81a00005},
    {0x800072f8, 0x88bd4070},
    {0x800072fc, 0x8affffc3},
    {0x80007300, 0xe2811004},
    {0x80007304, 0xe1500001},
    {0x80007308, 0xb1a01004},
    {0x8000730c, 0xb1a00005},
    {0x80007310, 0xb8bd4070},
    {0x80007314, 0xbaffffbf},
    {0x80007318, 0xe8bd8070},
    {0x8000731c, 0x800013c4},
    {0x80007320, 0x80003108},
    {0x80007324, 0x8000044c},
    {0x80007328, 0x80001373},
    {0x8000732c, 0xe92d4038},
    {0x80007330, 0xe1a04000},
    {0x80007334, 0xebffffb9},
    {0x80007338, 0xe3500000},
    {0x8000733c, 0x0a000021},
    {0x80007340, 0xe1a00004},
    {0x80007344, 0xebffffb7},
    {0x80007348, 0xe3500000},
    {0x8000734c, 0x0a00001d},
    {0x80007350, 0xe59f5074},
    {0x80007354, 0xe5d50000},
    {0x80007358, 0xe3500000},
    {0x8000735c, 0x1a000019},
    {0x80007360, 0xe5d40000},
    {0x80007364, 0xe3500000},
    {0x80007368, 0x0a000016},
    {0x8000736c, 0xebffffaf},
    {0x80007370, 0xe3500000},
    {0x80007374, 0x1a000013},
    {0x80007378, 0xe3a00470},
    {0x8000737c, 0xe5900030},
    {0x80007380, 0xe1a00900},
    {0x80007384, 0xe1a01e20},
    {0x80007388, 0xe5d40002},
    {0x8000738c, 0xe3500001},
    {0x80007390, 0x1a000001},
    {0x80007394, 0xe3510010},
    {0x80007398, 0x3a000001},
    {0x8000739c, 0xe3500000},
    {0x800073a0, 0x1a000008},
    {0x800073a4, 0xebffffa5},
    {0x800073a8, 0xe5c5000a},
    {0x800073ac, 0xeb000007},
    {0x800073b0, 0xe3a00002},
    {0x800073b4, 0xebffff9f},
    {0x800073b8, 0xe5c50008},
    {0x800073bc, 0xe5854004},
    {0x800073c0, 0xe3a00001},
    {0x800073c4, 0xe5c50000},
    {0x800073c8, 0xe8bd8038},
    {0x800073cc, 0x800013c4},
    {0x800073d0, 0xe59f3014},
    {0x800073d4, 0xe1d310db},
    {0x800073d8, 0xe0811081},
    {0x800073dc, 0xe0810000},
    {0x800073e0, 0xe1a00140},
    {0x800073e4, 0xe5c3000b},
    {0x800073e8, 0xe1a0f00e},
    {0x800073ec, 0x800013c4},
    {0x8000000c, 0xea001cf7},
    {0x800073f0, 0xe59f0014},
    {0x800073f4, 0xe5d00000},
    {0x800073f8, 0xE3500030},
    {0x800073fc, 0xC8bd8ff8},
    {0x80007400, 0xe59f0008},
    {0x80007404, 0xe3a0eb76},
    {0x80007408, 0xe28effc3},
    {0x8000740C, 0x80001370},
    {0x80007410, 0x80001373},
    {0x40180020, 0x0001db08},
    {0x40180040, 0x00031e20},
    {0x80000010, 0xea000050},
    {0x80000158, 0xe3530015},
    {0x8000015c, 0x13a03c19},
    {0x80000160, 0x11dc11ba},
    {0x80000164, 0x1283f0b4},
    {0x80000168, 0xe3a01c1a},
    {0x8000016c, 0xe281f010},
    {0x40180100, 0x000019b0},
    {0x40180120, 0x00031e24},
    {0x4018010c, 0x0001dec0},
    {0x4018012c, 0x00031e2c},
    {0x80000018, 0xea000065},
    {0x800001b4, 0x81a000a1},
    {0x800001b8, 0xe51ff004},
    {0x800001bc, 0x0001dec4},
    {0x40180118, 0x0001df40},
    {0x40180138, 0x0001f7c4},
    {0x800001cc, 0xe1a00886},
    {0x800001d0, 0xe3500000},
    {0x800001d4, 0xe51ff004},
    {0x800001d8, 0x0001d3e8},
    {0x80000020, 0xea000069},
    {0x40180114, 0x0001d3d8},
    {0x40180134, 0x00031e34},
    {0x80000014, 0xea000055},
    {0x40180108, 0x0001d158},
    {0x40180128, 0x00031e28},
    {0x80000170, 0xe3805480},
    {0x80000174, 0xe3a00470},
    {0x80000178, 0xe5900050},
    {0x8000017c, 0xe1a00900},
    {0x80000180, 0xe1a00e20},
    {0x80000184, 0xe3500002},
    {0x80000188, 0x0a000001},
    {0x8000018c, 0xe51ff004},
    {0x80000190, 0x0001d15c},
    {0x80000194, 0xe8bd81f0},
    {0x80000198, 0xeb000002},
    {0x8000019c, 0xe1d402ba},
    {0x800001a0, 0xe51ff004},
    {0x800001a4, 0x000097d0},
    {0x800001a8, 0xe51ff004},
    {0x800001ac, 0x00006bbc},
    {0x8000001c, 0xea00005d},
    {0x40180110, 0x000097cc},
    {0x40180130, 0x00031e30},
    {0x40180000, 0x0000ffff},
};

void hal_BtInit(void)
{
    g_btPowered = FALSE;
    os_log_printf("[RDABT] RDABT 8809 Driver initialized (RF: 0x%02X, Core: 0x%02X)\n",
                  RDABT_I2C_RF_ADDR, RDABT_I2C_CORE_ADDR);
}

BOOL hal_BtPowerOn(void)
{
    if (g_btPowered)
    {
        return TRUE;
    }

    // 1. Enable 26MHz AUX clock to Bluetooth subsystem
    hal_SysSetupAuxClock(TRUE);

    // 2. Route 32.768kHz reference clock on PWL1 pin / internal routing
    hwp_configRegs->Alt_mux_select = (hwp_configRegs->Alt_mux_select & ~CFG_REGS_PWL1_MASK) | CFG_REGS_PWL1_CLK_32K;

    // 3. Power on VCAM in PMU LDO Active 1 (bit 8 VCAM_OFF = 0)
    // On RDA8809, PMD_LDO_CAM supplies power to I2C1 I/O pads
    UINT16 act1 = hal_PmuRead(PMU_REG_LDO_ACTIVE1);
    act1 &= ~PMU_ACT1_VCAM_OFF;
    hal_PmuWrite(PMU_REG_LDO_ACTIVE1, act1);

    // 4. Enable BT and VCAM power rails in PMU LDO Settings (Reg 0x02)
    UINT16 ldoReg = hal_PmuRead(PMU_REG_LDO_SETTINGS);
    ldoReg |= RDA_PMU_BT_ENABLE_BIT | PMU_LDO_EN_VCAM;
    hal_PmuWrite(PMU_REG_LDO_SETTINGS, ldoReg);

    // 5. Assert BT_POWER_ON (bit 5) in Config Regs IO_Drive2_Select
    // Ensure BT_FEED_THROUGH (bit 4) is 0 so internal UART routing is active
    hwp_configRegs->IO_Drive2_Select &= ~CFG_REGS_BT_FEED_THROUGH_BIT;
    hwp_configRegs->IO_Drive2_Select |= CFG_REGS_BT_POWER_ON_BIT;

    // 6. Ensure Alt_mux_select routes UART1 to internal BT core
    // Clear bit 12 (CFG_REGS_I2C1_UART1) so UART1 is NOT hijacked by I2C1
    hwp_configRegs->Alt_mux_select &= ~CFG_REGS_I2C1_MASK;
    // Clear bit 7 (CFG_REGS_UART2_UART2)
    hwp_configRegs->Alt_mux_select &= ~CFG_REGS_UART2_MASK;

    // 7. Open I2C Bus 2 (where BT Core and RF are physically located)
    g_bt_bus = HAL_I2C_BUS_2;
    hal_I2cOpen(HAL_I2C_BUS_2, HAL_I2C_SPEED_100K);

    // 8. Stabilization delay for clocks, internal digital core PLL, and ROM bootloader
    timer_delay_ms(500);

    g_btPowered = TRUE;
    os_log_printf("[RDABT] Bluetooth Power-On: PMU_LDO=0x%04X, ACT1=0x%04X, IO_Drive2=0x%08X, AltMux=0x%08X\n",
                  ldoReg, act1, hwp_configRegs->IO_Drive2_Select, hwp_configRegs->Alt_mux_select);
    return TRUE;
}

static bt_remote_dev_t s_discovered_devs[BT_MAX_DISCOVERED_DEVICES] = {0};
static uint8_t s_discovered_count = 0;
static BOOL s_is_scanning = FALSE;

void hal_BtPowerOff(void)
{
    // 1. Deassert BT_POWER_ON in Config Regs
    hwp_configRegs->IO_Drive2_Select &= ~CFG_REGS_BT_POWER_ON_BIT;

    // 2. Disable BT power rail in PMU LDO Settings (Reg 0x02)
    UINT16 ldoReg = hal_PmuRead(PMU_REG_LDO_SETTINGS);
    ldoReg &= ~RDA_PMU_BT_ENABLE_BIT;
    hal_PmuWrite(PMU_REG_LDO_SETTINGS, ldoReg);

    // 3. Disable 26MHz AUX clock
    hal_SysSetupAuxClock(FALSE);

    g_btPowered = FALSE;
    g_bt_status.powered = FALSE;
    g_bt_status.hci_ready = FALSE;
    g_bt_status.connected = FALSE;
    g_bt_status.paired = FALSE;
    g_bt_status.conn_handle = 0;
    s_is_scanning = FALSE;
    s_discovered_count = 0;
    memset(s_discovered_devs, 0, sizeof(s_discovered_devs));
    os_log_printf("[RDABT] Bluetooth Powered Off\n");
}

static uint8_t s_bt_rx_buf[2048];
static uint16_t s_bt_rx_len = 0;

BOOL hal_BtIsPowered(void)
{
    return g_btPowered;
}

BOOL hal_BtProbe(hal_bt_status_t *status)
{
    if (!status) return FALSE;

    status->powered = g_btPowered;
    status->rf_detected = FALSE;
    status->core_detected = FALSE;
    status->chip_id = 0;
    status->rf_reg0 = 0;
    status->hci_ready = FALSE;
    for (int i = 0; i < 6; i++) status->bd_addr[i] = 0;

    if (!g_btPowered)
    {
        hal_BtPowerOn();
    }

    os_log_printf("[RDABT] ================= BLUETOOTH PROBE ================\n");
    os_log_printf("[RDABT] Power Rail  : ACTIVE (PMU LDO bit1=1, IO_Drive2 bit5=1)\n");

    // 1. Probe Bus 2 directly (RDA8809 BT Core and RF are hardwired to I2C Bus 2)
    g_bt_bus = HAL_I2C_BUS_2;
    hal_I2cOpen(HAL_I2C_BUS_2, HAL_I2C_SPEED_100K);

    uint32_t core_chip_id = 0;
    uint16_t rf_val = 0;
    bool c_ok = hal_I2cReadReg32Core(HAL_I2C_BUS_2, RDABT_I2C_CORE_ADDR, 0x40200028, &core_chip_id);
    bool r_ok = hal_I2cReadReg16(HAL_I2C_BUS_2, RDABT_I2C_RF_ADDR, 0x00, &rf_val);

    status->core_detected = c_ok;
    status->rf_detected = r_ok;
    status->chip_id = core_chip_id;
    status->rf_reg0 = rf_val;

    os_log_printf("[RDABT] Target probe on Bus 2: Core=%s (0x%08X), RF=%s (0x%04X)\n",
                  c_ok ? "ACK" : "NACK", core_chip_id, r_ok ? "ACK" : "NACK", rf_val);

    // Determine chip identity name from official RDA SDK mapping
    const char *chip_name = "Unknown RDA BT";
    if (core_chip_id == 0x11005990)
    {
        chip_name = "RDA8809 (R17 Core)";
    }
    else if (core_chip_id == 0x12005990 || core_chip_id == 0x120159ff || core_chip_id == 0x12015990)
    {
        chip_name = "RDA8809E (R18 Core)";
    }
    else if ((core_chip_id & 0xFFFF) == 0x5876)
    {
        chip_name = "RDA5876 (R12 Core)";
    }
    else if ((core_chip_id & 0xFFFF) == 0x587f)
    {
        chip_name = "RDA5876P (R16 Core)";
    }

    os_log_printf("[RDABT] Core (0x15) : %s | ChipID: 0x%08X (%s)\n",
                  status->core_detected ? "ACK" : "NACK/TIMEOUT",
                  core_chip_id, status->core_detected ? chip_name : "N/A");
    os_log_printf("[RDABT] RF   (0x16) : %s | Reg0: 0x%04X\n",
                  status->rf_detected ? "ACK" : "NACK/TIMEOUT", rf_val);
    os_log_printf("[RDABT] ===================================================\n");

    return (status->rf_detected || status->core_detected);
}

void hal_BtDumpRegisters(void)
{
    if (!g_btPowered) hal_BtPowerOn();

    os_log_printf("[RDABT_DUMP] ----- Core Registers (I2C 0x15 on Bus %d) -----\n", g_bt_bus + 1);
    uint32_t addrs[] = { 0x40200000, 0x40200004, 0x40200010, 0x40200020, 0x40200028 };
    for (int i = 0; i < 5; i++)
    {
        uint32_t val = 0;
        if (hal_I2cReadReg32Core(g_bt_bus, RDABT_I2C_CORE_ADDR, addrs[i], &val))
        {
            os_log_printf("[RDABT_DUMP] Core [0x%08X] = 0x%08X\n", addrs[i], val);
        }
        else
        {
            os_log_printf("[RDABT_DUMP] Core [0x%08X] = NACK\n", addrs[i]);
        }
    }

    os_log_printf("[RDABT_DUMP] ----- RF Page 0 Registers (I2C 0x16 on Bus %d) -----\n", g_bt_bus + 1);
    // Switch to page 0
    hal_I2cWriteReg16(g_bt_bus, RDABT_I2C_RF_ADDR, 0x3F, 0x0000);
    for (uint8_t reg = 0x00; reg <= 0x0A; reg++)
    {
        uint16_t val = 0;
        if (hal_I2cReadReg16(g_bt_bus, RDABT_I2C_RF_ADDR, reg, &val))
        {
            os_log_printf("[RDABT_DUMP] RF [0x%02X] = 0x%04X\n", reg, val);
        }
    }
}

static char s_bt_local_name[32] = "VeebhaOS";

const char* hal_BtGetLocalName(void)
{
    return s_bt_local_name;
}

BOOL hal_BtSetLocalName(const char *name)
{
    if (!name || name[0] == '\0') return FALSE;
    strncpy(s_bt_local_name, name, sizeof(s_bt_local_name) - 1);
    s_bt_local_name[sizeof(s_bt_local_name) - 1] = '\0';

    if (hal_BtIsPowered() && g_bt_status.hci_ready) {
        HAL_UART_ID_T port = HAL_UART_1;
        uint8_t name_cmd[4 + 248] = {0};
        name_cmd[0] = 0x01;
        name_cmd[1] = 0x13;
        name_cmd[2] = 0x0C;
        name_cmd[3] = 248; // 0xF8
        for (int k = 0; s_bt_local_name[k] != '\0' && k < 240; k++) {
            name_cmd[4 + k] = (uint8_t)s_bt_local_name[k];
        }
        hal_UartFifoFlush(port);
        timer_delay_ms(10);
        hal_UartWrite(port, name_cmd, sizeof(name_cmd));
        uint8_t rx_buf[16] = {0};
        int rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
        os_log_printf("[RDABT_HCI] Set Device Name (\"%s\"): Status 0x%02X\n",
                      s_bt_local_name, (rx_len >= 7) ? rx_buf[6] : 0xFF);
        return (rx_len >= 7 && rx_buf[6] == 0x00);
    }
    return TRUE;
}

static BOOL hal_BtVendorWriteMemory(HAL_UART_ID_T port, uint32_t addr, const uint32_t *data, uint8_t len, uint8_t mem_type)
{
    uint8_t pkt[256];
    pkt[0] = 0x01; // HCI Command Packet
    pkt[1] = 0x02; // Opcode 0xFD02 LSB (Vendor Memory Write)
    pkt[2] = 0xFD; // Opcode 0xFD02 MSB
    pkt[3] = (uint8_t)(len * 4 + 6); // Param length
    pkt[4] = mem_type;
    pkt[5] = len;
    pkt[6] = (uint8_t)(addr & 0xFF);
    pkt[7] = (uint8_t)((addr >> 8) & 0xFF);
    pkt[8] = (uint8_t)((addr >> 16) & 0xFF);
    pkt[9] = (uint8_t)((addr >> 24) & 0xFF);
    for (uint8_t i = 0; i < len; i++)
    {
        pkt[10 + i * 4 + 0] = (uint8_t)(data[i] & 0xFF);
        pkt[10 + i * 4 + 1] = (uint8_t)((data[i] >> 8) & 0xFF);
        pkt[10 + i * 4 + 2] = (uint8_t)((data[i] >> 16) & 0xFF);
        pkt[10 + i * 4 + 3] = (uint8_t)((data[i] >> 24) & 0xFF);
    }
    hal_UartWrite(port, pkt, 4 + pkt[3]);
    return TRUE;
}

static BOOL hal_BtVendorWritePskey(HAL_UART_ID_T port, uint8_t id, const uint8_t *data, uint8_t len)
{
    uint8_t pkt[256];
    pkt[0] = 0x01; // HCI Command Packet
    pkt[1] = 0x05; // Opcode 0xFD05 LSB (Vendor PSKey Write)
    pkt[2] = 0xFD; // Opcode 0xFD05 MSB
    pkt[3] = (uint8_t)(len + 1); // Param length
    pkt[4] = id;
    for (uint8_t i = 0; i < len; i++)
    {
        pkt[5 + i] = data[i];
    }
    hal_UartFifoFlush(port);
    timer_delay_ms(2);
    hal_UartWrite(port, pkt, 4 + pkt[3]);
    uint8_t rx[32] = {0};
    uint32_t rx_len = hal_UartRead(port, rx, sizeof(rx), 100);
    return (rx_len >= 7 && rx[0] == 0x04 && rx[1] == 0x0E && rx[6] == 0x00);
}

BOOL hal_BtInitRf(void)
{
    if (!g_btPowered)
    {
        hal_BtPowerOn();
    }

    // 1. Write RF initial sequence for RDA8809E (R18 Core)
    int count = sizeof(rdabt_rf_init_18) / sizeof(rdabt_rf_init_18[0]);
    int writes = 0;

    for (int i = 0; i < count; i++)
    {
        uint8_t reg = (uint8_t)rdabt_rf_init_18[i][0];
        uint16_t val = rdabt_rf_init_18[i][1];

        if (hal_I2cWriteReg16(g_bt_bus, RDABT_I2C_RF_ADDR, reg, val))
        {
            writes++;
        }
        timer_delay_ms(1);
    }

    // 2. Perform RF DC calibration sequence (SDK: rdabt_DC_write_r18)
    int dc_writes = 0;
    int dc_count = sizeof(rdabt_dccal_18) / sizeof(rdabt_dccal_18[0]);
    for (int i = 0; i < dc_count; i++)
    {
        if (hal_I2cWriteReg16(g_bt_bus, RDABT_I2C_RF_ADDR, (uint8_t)rdabt_dccal_18[i][0], rdabt_dccal_18[i][1]))
        {
            dc_writes++;
        }
        timer_delay_ms(55);
    }

    // 3. Disable BT-side RTS flow control in BT Core registers (SDK: rdabt_common_baudrate_flow_ctrl_disable_RTS)
    // Core register 0x40200004 bit 10 cleared: 0xFB7C
    // Core register 0x40200010 bit 10 cleared: 0x7BFF
    hal_I2cWriteReg32Core(g_bt_bus, RDABT_I2C_CORE_ADDR, 0x40200004, 0x0000FB7C);
    hal_I2cWriteReg32Core(g_bt_bus, RDABT_I2C_CORE_ADDR, 0x40200010, 0x00007BFF);

    os_log_printf("[RDABT] RF Initialization on Bus %d: %d/%d regs + %d/%d DC-cal programmed via 16-bit I2C\n",
                  g_bt_bus + 1, writes, count, dc_writes, dc_count);
    os_log_printf("[RDABT] Flow control disabled in BT core: 0x40200004=0xFB7C, 0x40200010=0x7BFF\n");
    return (writes > 0);
}

BOOL hal_BtTestHci(UINT8 *out_bd_addr)
{
    HAL_UART_CFG_T ucfg = {
        .rate = HAL_UART_BAUD_115200,
        .data = HAL_UART_8_DATA_BITS,
        .stop = HAL_UART_1_STOP_BIT,
        .parity = HAL_UART_NO_PARITY,
        .rx_trigger = 1,
        .tx_trigger = 0,
        .auto_flow_ctrl = FALSE
    };

    // Standard HCI Reset packet: Type=0x01 (Command), Opcode=0x0C03 (Reset), ParamLen=0x00
    const uint8_t hci_reset_cmd[] = { 0x01, 0x03, 0x0C, 0x00 };
    // Standard HCI Read BD_ADDR: Type=0x01 (Command), Opcode=0x1009 (Read BD_ADDR), ParamLen=0x00
    const uint8_t hci_read_bd[] = { 0x01, 0x09, 0x10, 0x00 };

    BOOL hci_ok = FALSE;
    HAL_UART_ID_T port = HAL_UART_1;

    // Test ports in sequence: Primary is UART1 (internal BT interconnect), secondary is UART2
    HAL_UART_ID_T test_ports[2] = { HAL_UART_1, HAL_UART_2 };

    uint8_t rx_buf[64] = {0};
    uint32_t rx_len = 0;

    for (int p = 0; p < 2; p++)
    {
        port = test_ports[p];

        if (port == HAL_UART_1)
        {
            // Ensure bit 12 of Alt_mux_select is 0 for UART1
            hwp_configRegs->Alt_mux_select &= ~CFG_REGS_I2C1_MASK;
        }

        hal_UartOpen(port, &ucfg);
        hal_UartSetRts(port, TRUE);
        hal_UartFifoFlush(port);

        // Allow lines and controller ROM UART transceiver to stabilize
        timer_delay_ms(150);

        for (int retry = 0; retry < 15; retry++)
        {
            hal_UartFifoFlush(port);
            timer_delay_ms(60);

            UINT32 tx_len = hal_UartWrite(port, hci_reset_cmd, sizeof(hci_reset_cmd));
            if ((retry % 5) == 0 || retry < 3)
            {
                os_log_printf("[RDABT_HCI] Sent HCI Reset on UART%d (try %d/15)...\n", port, retry + 1);
            }

            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);

            if (rx_len > 0)
            {
                os_log_printf("[RDABT_HCI] UART%d Received %u bytes on try %d: ", port, rx_len, retry + 1);
                for (uint32_t i = 0; i < rx_len && i < 16; i++) {
                    os_log_printf("%02X ", rx_buf[i]);
                }
                os_log_printf("\n");

                // Search for HCI Command Complete Event: 0x04 0x0E ... Status 0x00
                for (uint32_t i = 0; i + 6 < rx_len; i++)
                {
                    if (rx_buf[i] == 0x04 && rx_buf[i+1] == 0x0E && rx_buf[i+6] == 0x00)
                    {
                        hci_ok = TRUE;
                        os_log_printf("[RDABT_HCI] Controller ready on UART%d (try %d)!\n", port, retry + 1);
                        break;
                    }
                }
                if (hci_ok) break;

                // If Command Disallowed (0x12), controller is still completing internal startup
                if (rx_len >= 4 && rx_buf[0] == 0x04 && rx_buf[1] == 0x0F && rx_buf[3] == 0x12)
                {
                    os_log_printf("[RDABT_HCI] Controller booting (0x12: Command Disallowed), waiting...\n");
                    timer_delay_ms(150);
                }
            }
        }

        if (hci_ok) break;

        hal_UartClose(port);
    }

        if (hci_ok)
        {
            os_log_printf("[RDABT_HCI] UART%d HCI Command Complete verified! (Status: 0x00)\n", port);

            // 1. Program Baseband & RF PSKeys (matching SDK RDABT_core_Intialization_r18)
            os_log_printf("[RDABT_HCI] Programming Baseband RF PSKeys and configuration...\n");

            // SPI2 clock enable before PSKey 0x26
            uint32_t spi2_en = 0x0004f39c;
            hal_BtVendorWriteMemory(port, 0x40240000, &spi2_en, 1, 0);
            hal_I2cWriteReg32Core(g_bt_bus, RDABT_I2C_CORE_ADDR, 0x40240000, spi2_en);
            timer_delay_ms(5);

            // PSKey 0x26: RF calibration table (14 words = 56 bytes)
            BOOL ps26_ok = hal_BtVendorWritePskey(port, 0x26, (const uint8_t*)rdabt_pskey_rf_18, sizeof(rdabt_pskey_rf_18));
            os_log_printf("[RDABT_HCI] PSKey 0x26 (RF Calib 56B): %s\n", ps26_ok ? "ACK" : "SENT");
            timer_delay_ms(5);

            // SPI2 clock disable after PSKey 0x26
            uint32_t spi2_dis = 0x0000f29c;
            hal_BtVendorWriteMemory(port, 0x40240000, &spi2_dis, 1, 0);
            hal_I2cWriteReg32Core(g_bt_bus, RDABT_I2C_CORE_ADDR, 0x40240000, spi2_dis);
            timer_delay_ms(5);

            // PSKey 0x15: System Config (4 bytes)
            BOOL ps15_ok = hal_BtVendorWritePskey(port, 0x15, rdabt_pskey_sys_config, sizeof(rdabt_pskey_sys_config));
            os_log_printf("[RDABT_HCI] PSKey 0x15 (Sys Config): %s\n", ps15_ok ? "ACK" : "SENT");
            timer_delay_ms(5);

            // PSKey 0x24: RF Operating Setting (12 bytes)
            BOOL ps24_ok = hal_BtVendorWritePskey(port, 0x24, rdabt_pskey_rf_setting, sizeof(rdabt_pskey_rf_setting));
            os_log_printf("[RDABT_HCI] PSKey 0x24 (RF Setting): %s\n", ps24_ok ? "ACK" : "SENT");
            timer_delay_ms(5);

            // PSKey 0x17: Audio PCM Config (4 bytes)
            BOOL ps17_ok = hal_BtVendorWritePskey(port, 0x17, rdabt_pskey_pcm_config, sizeof(rdabt_pskey_pcm_config));
            os_log_printf("[RDABT_HCI] PSKey 0x17 (PCM Config): %s\n", ps17_ok ? "ACK" : "SENT");
            timer_delay_ms(5);

            // Baseband Core Register Setup (rda_pskey_18)
            for (int i = 0; i < sizeof(rda_pskey_18) / sizeof(rda_pskey_18[0]); i++)
            {
                hal_BtVendorWriteMemory(port, rda_pskey_18[i][0], &rda_pskey_18[i][1], 1, 0);
                timer_delay_ms(1);
            }

            // Baseband ROM Bug Traps & Patches (SDK rdabt_8809e_init.c rda_trap_18)
            os_log_printf("[RDABT_HCI] Programming Baseband ROM bug traps (%u entries)...\n",
                          sizeof(rda_trap_18) / sizeof(rda_trap_18[0]));
            for (int i = 0; i < sizeof(rda_trap_18) / sizeof(rda_trap_18[0]); i++)
            {
                hal_BtVendorWriteMemory(port, rda_trap_18[i][0], &rda_trap_18[i][1], 1, 0);
                timer_delay_ms(1);
            }
            os_log_printf("[RDABT_HCI] Baseband ROM traps activated!\n");

            // 2. Read BD_ADDR
            timer_delay_ms(50);
            hal_UartFifoFlush(port);
            timer_delay_ms(20);
            UINT32 tx_bd = hal_UartWrite(port, hci_read_bd, sizeof(hci_read_bd));
            os_log_printf("[RDABT_HCI] Sent HCI Read BD_ADDR on UART%d (%u/4 bytes)\n", port, tx_bd);

            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 300);

            os_log_printf("[RDABT_HCI] UART%d BD_ADDR Received: %u bytes\n", port, rx_len);
            if (rx_len > 0)
            {
                os_log_printf("[RDABT_HCI] BD_ADDR RX Bytes: ");
                for (uint32_t i = 0; i < rx_len && i < 16; i++) {
                    os_log_printf("%02X ", rx_buf[i]);
                }
                os_log_printf("\n");
            }

            // Search for BD_ADDR complete event packet format:
            // 04 0E 0A 01 09 10 00 [6 bytes BD_ADDR: LAP(3), UAP(1), NAP(2)]
            for (uint32_t i = 0; i + 12 < rx_len; i++)
            {
                if (rx_buf[i] == 0x04 && rx_buf[i+1] == 0x0E && rx_buf[i+3] == 0x01 &&
                    rx_buf[i+4] == 0x09 && rx_buf[i+5] == 0x10 && rx_buf[i+6] == 0x00)
                {
                    if (out_bd_addr)
                    {
                        for (int k = 0; k < 6; k++) out_bd_addr[k] = rx_buf[i + 7 + k];
                    }
                    os_log_printf("[RDABT_HCI] SUCCESS! Bluetooth MAC Address (BD_ADDR): %02X:%02X:%02X:%02X:%02X:%02X\n",
                                  rx_buf[i+12], rx_buf[i+11], rx_buf[i+10], rx_buf[i+9], rx_buf[i+8], rx_buf[i+7]);
                    break;
                }
            }

            // 3. Set Event Mask (HCI_Set_Event_Mask, Opcode 0x0C01, ParamLen 8): Official RDA SDK Mask
            const uint8_t event_mask_cmd[] = { 0x01, 0x01, 0x0C, 0x08, 0xFF, 0xFF, 0xFF, 0xFF, 0x1D, 0xBF, 0xFF, 0xFF };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, event_mask_cmd, sizeof(event_mask_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Event Mask: Status 0x%02X\n", (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 4. Connection Accept Timeout: 0x1FA0 (5.06 seconds, Opcode 0x0C16)
            const uint8_t conn_timeout_cmd[] = { 0x01, 0x16, 0x0C, 0x02, 0xA0, 0x1F };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, conn_timeout_cmd, sizeof(conn_timeout_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Connection Accept Timeout (5s): Status 0x%02X\n", (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 5. Page Timeout: 0x2000 (5.12 seconds, Opcode 0x0C18)
            const uint8_t page_timeout_cmd[] = { 0x01, 0x18, 0x0C, 0x02, 0x00, 0x20 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, page_timeout_cmd, sizeof(page_timeout_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Page Timeout (5.12s): Status 0x%02X\n", (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 6. Authentication Enable: 0x00 (Opcode 0x0C20)
            const uint8_t auth_cmd[] = { 0x01, 0x20, 0x0C, 0x01, 0x00 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, auth_cmd, sizeof(auth_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Authentication Mode (0x00): Status 0x%02X\n", (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 7. Set Local Device Name (HCI_Change_Local_Name, Opcode 0x0C13, ParamLen 248)
            const char *dev_name = s_bt_local_name;
            uint8_t name_cmd[4 + 248] = {0};
            name_cmd[0] = 0x01;
            name_cmd[1] = 0x13;
            name_cmd[2] = 0x0C;
            name_cmd[3] = 248; // 0xF8
            for (int k = 0; dev_name[k] != '\0' && k < 240; k++) {
                name_cmd[4 + k] = (uint8_t)dev_name[k];
            }
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, name_cmd, sizeof(name_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Device Name (\"%s\"): Status 0x%02X\n",
                          dev_name, (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 8. Set Class of Device (HCI_Write_Class_of_Device, Opcode 0x0C24, ParamLen 3)
            // 0x020114 = Networking / Handheld Computer / PDA (Computer icon on Android & Desktop)
            const uint8_t cod_cmd[] = { 0x01, 0x24, 0x0C, 0x03, 0x14, 0x01, 0x02 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, cod_cmd, sizeof(cod_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Class of Device (0x020114 Handheld Computer/PANU): Status 0x%02X\n",
                          (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 9. Set Inquiry Scan Activity (Interval=640ms/0x0400, Window=22.5ms/0x0024)
            // Balanced duty cycle allows Page Scan (connection) to execute without RF starvation
            const uint8_t inq_act_cmd[] = { 0x01, 0x1E, 0x0C, 0x04, 0x00, 0x04, 0x24, 0x00 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, inq_act_cmd, sizeof(inq_act_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Inquiry Scan Activity (640ms/22.5ms): Status 0x%02X\n",
                          (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 10. Set Page Scan Activity (Interval=640ms/0x0400, Window=40ms/0x0040)
            const uint8_t page_act_cmd[] = { 0x01, 0x1C, 0x0C, 0x04, 0x00, 0x04, 0x40, 0x00 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, page_act_cmd, sizeof(page_act_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Page Scan Activity (640ms/40ms): Status 0x%02X\n",
                          (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 7. Set Inquiry Scan Type (Interlaced = 0x01)
            const uint8_t inq_type_cmd[] = { 0x01, 0x43, 0x0C, 0x01, 0x01 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, inq_type_cmd, sizeof(inq_type_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Inquiry Scan Type (Interlaced): Status 0x%02X\n",
                          (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 8. Set Page Scan Type (Interlaced = 0x01)
            const uint8_t page_type_cmd[] = { 0x01, 0x47, 0x0C, 0x01, 0x01 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, page_type_cmd, sizeof(page_type_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Set Page Scan Type (Interlaced): Status 0x%02X\n",
                          (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 9. Write Extended Inquiry Response (EIR, Opcode 0x0C52, ParamLen 241)
            // Allows smartphones and PCs to display the device name instantly during inquiry
            uint8_t eir_cmd[4 + 241] = {0};
            eir_cmd[0] = 0x01;
            eir_cmd[1] = 0x52;
            eir_cmd[2] = 0x0C;
            eir_cmd[3] = 241;
            eir_cmd[4] = 0x00; // FEC_Required = FALSE
            // Structure 1: Complete Local Name (Type 0x09)
            int name_len = 0;
            while (dev_name[name_len] != '\0' && name_len < 30) name_len++;
            eir_cmd[5] = (uint8_t)(name_len + 1);
            eir_cmd[6] = 0x09; // Complete Local Name
            for (int k = 0; k < name_len; k++) {
                eir_cmd[7 + k] = (uint8_t)dev_name[k];
            }
            // Structure 2: Tx Power Level (Type 0x0A)
            int eir_idx = 7 + name_len;
            eir_cmd[eir_idx] = 0x02;
            eir_cmd[eir_idx + 1] = 0x0A;
            eir_cmd[eir_idx + 2] = 0x00; // 0 dBm
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, eir_cmd, sizeof(eir_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Write EIR (Complete Name): Status 0x%02X\n",
                          (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 10. Write Simple Pairing Mode (Opcode 0x0C56, Mode=0x01 Enabled)
            const uint8_t sp_cmd[] = { 0x01, 0x56, 0x0C, 0x01, 0x01 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, sp_cmd, sizeof(sp_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Simple Pairing Mode: Status 0x%02X (%s)\n",
                          (rx_len >= 7) ? rx_buf[6] : 0xFF,
                          (rx_len >= 7 && rx_buf[6] == 0x00) ? "Enabled" : "HW/ROM Default Active");

            // Write Local Public Key for P-192 ECDH Diffie-Hellman Simple Pairing
            rdabt_mgr_generate_local_key();
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            os_log_printf("[RDABT_HCI] Write Local Public Key: Status 0x%02X\n", (rx_len >= 7) ? rx_buf[6] : 0xFF);

            // 11. Enable Inquiry Scan & Page Scan (HCI_Write_Scan_Enable, Opcode 0x0C1A, ParamLen 1, Value 0x03)
            // Value 0x03 = Inquiry Scan (Discoverable) + Page Scan (Connectable)
            const uint8_t scan_cmd[] = { 0x01, 0x1A, 0x0C, 0x01, 0x03 };
            hal_UartFifoFlush(port);
            timer_delay_ms(10);
            hal_UartWrite(port, scan_cmd, sizeof(scan_cmd));
            for (int k = 0; k < sizeof(rx_buf); k++) rx_buf[k] = 0;
            rx_len = hal_UartRead(port, rx_buf, sizeof(rx_buf), 200);
            if (rx_len >= 7 && rx_buf[6] == 0x00)
            {
                os_log_printf("[RDABT_HCI] ===================================================\n");
                os_log_printf("[RDABT_HCI] *** BLUETOOTH DISCOVERY FULLY ACTIVE & BROADCASTING ***\n");
                os_log_printf("[RDABT_HCI] Device Name : \"%s\"\n", dev_name);
                os_log_printf("[RDABT_HCI] MAC Address : %02X:%02X:%02X:%02X:%02X:%02X\n",
                              out_bd_addr ? out_bd_addr[5] : 0, out_bd_addr ? out_bd_addr[4] : 0,
                              out_bd_addr ? out_bd_addr[3] : 0, out_bd_addr ? out_bd_addr[2] : 0,
                              out_bd_addr ? out_bd_addr[1] : 0, out_bd_addr ? out_bd_addr[0] : 0);
                os_log_printf("[RDABT_HCI] Discovery: 640ms/22.5ms | Connection: 640ms/40ms Interlaced\n");
                os_log_printf("[RDABT_HCI] Search for \"%s\" on your Android phone or PC now!\n", dev_name);
                os_log_printf("[RDABT_HCI] ===================================================\n");
            }
            else
            {
                os_log_printf("[RDABT_HCI] Scan Enable failed (Status: 0x%02X)\n",
                              (rx_len >= 7) ? rx_buf[6] : 0xFF);
            }

            // Keep UART1 open and active so controller maintains discoverability
    }

    if (!hci_ok)
    {
        os_log_printf("[RDABT_HCI] HCI controller not yet ready or baud/flowcontrol handshake pending.\n");
    }

    // Reset polling buffer and drain any UART1 residues
    timer_delay_ms(10);
    hal_UartFifoFlush(HAL_UART_1);
    s_bt_rx_len = 0;

    return hci_ok;
}

uint32_t hal_BtSendPacket(const uint8_t *pkt, uint32_t len)
{
    if (!pkt || len == 0) return 0;

    if (pkt[0] == 0x01 && len >= 4) // HCI Command packet
    {
        uint16_t opcode = (uint16_t)(pkt[1] | (pkt[2] << 8));
        uint8_t p_len = pkt[3];
        os_log_printf("[BT_TX_CMD] Opcode 0x%04X, Len %u: ", opcode, p_len);
        uint32_t dlen = (len < 16) ? len : 16;
        for (uint32_t i = 0; i < dlen; i++) {
            os_log_printf("%02X ", pkt[i]);
        }
        if (len > 16) os_log_printf("...");
        os_log_printf("\n");
    }
    else if (pkt[0] == 0x02 && len >= 5) // HCI ACL Data packet
    {
        uint16_t handle = (uint16_t)(pkt[1] | ((pkt[2] & 0x0F) << 8));
        uint16_t acl_len = (uint16_t)(pkt[3] | (pkt[4] << 8));
        uint16_t l2cap_len = (len >= 7) ? (uint16_t)(pkt[5] | (pkt[6] << 8)) : 0;
        uint16_t cid = (len >= 9) ? (uint16_t)(pkt[7] | (pkt[8] << 8)) : 0;
        os_log_printf("[BT_TX_ACL] Handle 0x%04X, Len %u (L2CAP: Len %u, CID 0x%04X): ",
                      handle, acl_len, l2cap_len, cid);
        uint32_t dlen = (len < 18) ? len : 18;
        for (uint32_t i = 0; i < dlen; i++) {
            os_log_printf("%02X ", pkt[i]);
        }
        if (len > 18) os_log_printf("...");
        os_log_printf("\n");
    }

    return hal_UartWrite(HAL_UART_1, pkt, len);
}

#define BT_MAX_PAIRED_DEVICES 4
static hal_bt_paired_dev_t s_paired_devices[BT_MAX_PAIRED_DEVICES] = {0};
static BOOL s_pairing_in_progress = FALSE;
static uint32_t s_pairing_start_ms = 0;

void hal_BtSetPairingInProgress(BOOL in_progress)
{
    s_pairing_in_progress = in_progress;
    if (in_progress)
    {
        s_pairing_start_ms = timer_get_ms();
    }
}

BOOL hal_BtIsPairingInProgress(void)
{
    if (s_pairing_in_progress)
    {
        if ((timer_get_ms() - s_pairing_start_ms) > 15000)
        {
            s_pairing_in_progress = FALSE;
            return FALSE;
        }
        return TRUE;
    }
    return FALSE;
}

static void hal_BtFormatPinMsg(uint32_t pin)
{
    g_test_result_msg[0] = '[';
    g_test_result_msg[1] = 'P';
    g_test_result_msg[2] = 'I';
    g_test_result_msg[3] = 'N';
    g_test_result_msg[4] = ']';
    g_test_result_msg[5] = ' ';
    uint32_t p = pin % 1000000;
    g_test_result_msg[6]  = '0' + ((p / 100000) % 10);
    g_test_result_msg[7]  = '0' + ((p / 10000) % 10);
    g_test_result_msg[8]  = '0' + ((p / 1000) % 10);
    g_test_result_msg[9]  = '0' + ((p / 100) % 10);
    g_test_result_msg[10] = '0' + ((p / 10) % 10);
    g_test_result_msg[11] = '0' + (p % 10);
    g_test_result_msg[12] = '\0';
}

static void hal_BtFormatPairMsg(const UINT8 *peer)
{
    const char hex[] = "0123456789ABCDEF";
    g_test_result_msg[0] = '[';
    g_test_result_msg[1] = 'P';
    g_test_result_msg[2] = 'A';
    g_test_result_msg[3] = 'I';
    g_test_result_msg[4] = 'R';
    g_test_result_msg[5] = ']';
    g_test_result_msg[6] = ' ';
    g_test_result_msg[7] = hex[(peer[5] >> 4) & 0xF];
    g_test_result_msg[8] = hex[peer[5] & 0xF];
    g_test_result_msg[9] = ':';
    g_test_result_msg[10] = hex[(peer[4] >> 4) & 0xF];
    g_test_result_msg[11] = hex[peer[4] & 0xF];
    g_test_result_msg[12] = ':';
    g_test_result_msg[13] = hex[(peer[3] >> 4) & 0xF];
    g_test_result_msg[14] = hex[peer[3] & 0xF];
    g_test_result_msg[15] = ' ';
    g_test_result_msg[16] = 'O';
    g_test_result_msg[17] = 'K';
    g_test_result_msg[18] = '\0';
}

static const UINT8* hal_BtFindLinkKey(const UINT8 *bd_addr)
{
    for (int i = 0; i < BT_MAX_PAIRED_DEVICES; i++)
    {
        if (s_paired_devices[i].valid)
        {
            BOOL match = TRUE;
            for (int k = 0; k < 6; k++) {
                if (s_paired_devices[i].bd_addr[k] != bd_addr[k]) {
                    match = FALSE;
                    break;
                }
            }
            if (match) return s_paired_devices[i].link_key;
        }
    }
    return NULL;
}

static void hal_BtSaveLinkKey(const UINT8 *bd_addr, const UINT8 *link_key, UINT8 key_type)
{
    int slot = -1;
    for (int i = 0; i < BT_MAX_PAIRED_DEVICES; i++)
    {
        if (s_paired_devices[i].valid)
        {
            BOOL match = TRUE;
            for (int k = 0; k < 6; k++) {
                if (s_paired_devices[i].bd_addr[k] != bd_addr[k]) {
                    match = FALSE;
                    break;
                }
            }
            if (match) { slot = i; break; }
        }
        else if (slot == -1)
        {
            slot = i;
        }
    }
    if (slot == -1) slot = 0;

    for (int k = 0; k < 6; k++) s_paired_devices[slot].bd_addr[k] = bd_addr[k];
    for (int k = 0; k < 16; k++) s_paired_devices[slot].link_key[k] = link_key[k];
    s_paired_devices[slot].key_type = key_type;
    s_paired_devices[slot].valid = TRUE;
}

static void hal_BtDeleteLinkKey(const UINT8 *bd_addr)
{
    if (!bd_addr) return;
    for (int i = 0; i < BT_MAX_PAIRED_DEVICES; i++)
    {
        if (s_paired_devices[i].valid)
        {
            BOOL match = TRUE;
            for (int k = 0; k < 6; k++) {
                if (s_paired_devices[i].bd_addr[k] != bd_addr[k]) {
                    match = FALSE;
                    break;
                }
            }
            if (match)
            {
                s_paired_devices[i].valid = FALSE;
                os_log_printf("[RDABT_PAIR] Stored link key invalidated for device %02X:%02X:%02X:%02X:%02X:%02X\n",
                              bd_addr[5], bd_addr[4], bd_addr[3], bd_addr[2], bd_addr[1], bd_addr[0]);
                break;
            }
        }
    }
}

BOOL hal_BtAcceptConnection(const UINT8 *bd_addr, UINT8 role)
{
    uint8_t cmd[4 + 7] = {
        0x01,       // HCI Command packet
        0x09, 0x04, // Opcode 0x0409 (HCI_Accept_Connection_Request)
        0x07,       // Param length: 7
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
        role        // 0x01: Slave role
    };
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtRejectConnection(const UINT8 *bd_addr, UINT8 reason)
{
    uint8_t cmd[4 + 7] = {
        0x01,       // HCI Command packet
        0x0A, 0x04, // Opcode 0x040A (HCI_Reject_Connection_Request)
        0x07,       // Param length: 7
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
        reason ? reason : 0x13 // Reason: 0x13 (User terminated)
    };
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

static BOOL s_pending_conn_req = FALSE;
static UINT8 s_pending_conn_addr[6] = {0};
static char  s_pending_conn_name[32] = {0};

BOOL hal_BtHasPendingConnection(void)
{
    return s_pending_conn_req;
}

BOOL hal_BtGetPendingConnectionInfo(char *name, UINT16 name_sz, char *mac, UINT16 mac_sz)
{
    if (!s_pending_conn_req) return FALSE;
    if (name && name_sz > 0) {
        if (s_pending_conn_name[0] != '\0') {
            strncpy(name, s_pending_conn_name, name_sz - 1);
            name[name_sz - 1] = '\0';
        } else {
            snprintf(name, name_sz, "Dev %02X:%02X:%02X",
                     s_pending_conn_addr[2], s_pending_conn_addr[1], s_pending_conn_addr[0]);
        }
    }
    if (mac && mac_sz >= 18) {
        snprintf(mac, mac_sz, "%02X:%02X:%02X:%02X:%02X:%02X",
                 s_pending_conn_addr[5], s_pending_conn_addr[4], s_pending_conn_addr[3],
                 s_pending_conn_addr[2], s_pending_conn_addr[1], s_pending_conn_addr[0]);
    }
    return TRUE;
}

void hal_BtAcceptPendingConnection(BOOL accept)
{
    if (!s_pending_conn_req) return;
    s_pending_conn_req = FALSE;
    if (accept) {
        os_log_printf("[RDABT_PAIR] User APPROVED incoming connection request from %02X:%02X:%02X:%02X:%02X:%02X\n",
                      s_pending_conn_addr[5], s_pending_conn_addr[4], s_pending_conn_addr[3],
                      s_pending_conn_addr[2], s_pending_conn_addr[1], s_pending_conn_addr[0]);

        int slot = -1;
        for (int i = 0; i < BT_MAX_PAIRED_DEVICES; i++) {
            if (s_paired_devices[i].valid && memcmp(s_paired_devices[i].bd_addr, s_pending_conn_addr, 6) == 0) {
                slot = i;
                break;
            } else if (!s_paired_devices[i].valid && slot == -1) {
                slot = i;
            }
        }
        if (slot == -1) slot = 0;
        memcpy(s_paired_devices[slot].bd_addr, s_pending_conn_addr, 6);
        s_paired_devices[slot].valid = TRUE;
        if (s_pending_conn_name[0] != '\0') {
            strncpy(s_paired_devices[slot].name, s_pending_conn_name, sizeof(s_paired_devices[slot].name) - 1);
            s_paired_devices[slot].name[sizeof(s_paired_devices[slot].name) - 1] = '\0';
        }

        hal_BtAcceptConnection(s_pending_conn_addr, 0x01);
    } else {
        os_log_printf("[RDABT_PAIR] User REJECTED incoming connection request from %02X:%02X:%02X:%02X:%02X:%02X\n",
                      s_pending_conn_addr[5], s_pending_conn_addr[4], s_pending_conn_addr[3],
                      s_pending_conn_addr[2], s_pending_conn_addr[1], s_pending_conn_addr[0]);
        hal_BtRejectConnection(s_pending_conn_addr, 0x13);
    }
}

BOOL hal_BtLinkKeyNegativeReply(const UINT8 *bd_addr)
{
    uint8_t cmd[4 + 6] = {
        0x01,       // HCI Command packet
        0x0C, 0x04, // Opcode 0x040C (HCI_Link_Key_Request_Negative_Reply)
        0x06,       // Param length: 6
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]
    };
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtLinkKeyReply(const UINT8 *bd_addr, const UINT8 *link_key)
{
    uint8_t cmd[4 + 22] = {
        0x01,       // HCI Command packet
        0x0B, 0x04, // Opcode 0x040B (HCI_Link_Key_Request_Reply)
        0x16,       // Param length: 22
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]
    };
    for (int i = 0; i < 16; i++) {
        cmd[10 + i] = link_key[i];
    }
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtPinCodeReply(const UINT8 *bd_addr, const char *pin)
{
    uint8_t cmd[4 + 23] = {0};
    cmd[0] = 0x01;       // HCI Command packet
    cmd[1] = 0x0D; cmd[2] = 0x04; // Opcode 0x040D (HCI_PIN_Code_Request_Reply)
    cmd[3] = 0x17;       // Param length: 23
    for (int i = 0; i < 6; i++) {
        cmd[4 + i] = bd_addr[i];
    }
    uint8_t pin_len = 0;
    while (pin[pin_len] != '\0' && pin_len < 16) {
        cmd[11 + pin_len] = (uint8_t)pin[pin_len];
        pin_len++;
    }
    cmd[10] = pin_len; // PIN_Code_Length
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtIoCapabilityReply(const UINT8 *bd_addr, UINT8 io_cap, UINT8 oob, UINT8 auth)
{
    uint8_t cmd[4 + 9] = {
        0x01,       // HCI Command packet
        0x2B, 0x04, // Opcode 0x042B (HCI_IO_Capability_Request_Reply)
        0x09,       // Param length: 9
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
        io_cap,     // 0x03: NoInputNoOutput (Just Works)
        oob,        // 0x00: OOB data not present
        auth        // 0x00: MITM Not Required, General Bonding
    };
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtUserConfirmationReply(const UINT8 *bd_addr)
{
    uint8_t cmd[4 + 6] = {
        0x01,       // HCI Command packet
        0x2C, 0x04, // Opcode 0x042C (HCI_User_Confirmation_Request_Reply)
        0x06,       // Param length: 6
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]
    };
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtUserPasskeyReply(const UINT8 *bd_addr, UINT32 passkey)
{
    uint8_t cmd[4 + 10] = {
        0x01,       // HCI Command packet
        0x2E, 0x04, // Opcode 0x042E (HCI_User_Passkey_Request_Reply)
        0x0A,       // Param length: 10
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
        (uint8_t)(passkey & 0xFF),
        (uint8_t)((passkey >> 8) & 0xFF),
        (uint8_t)((passkey >> 16) & 0xFF),
        (uint8_t)((passkey >> 24) & 0xFF)
    };
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

static void hal_BtAddOrUpdateDiscovered(const UINT8 *bd_addr, const char *name, INT8 rssi, UINT32 cod, UINT8 rep_mode, UINT16 clk_off)
{
    int slot = -1;
    for (int i = 0; i < s_discovered_count; i++)
    {
        if (memcmp(s_discovered_devs[i].bd_addr, bd_addr, 6) == 0)
        {
            slot = i;
            break;
        }
    }
    if (slot == -1)
    {
        if (s_discovered_count < BT_MAX_DISCOVERED_DEVICES)
        {
            slot = s_discovered_count++;
            memset(&s_discovered_devs[slot], 0, sizeof(bt_remote_dev_t));
            memcpy(s_discovered_devs[slot].bd_addr, bd_addr, 6);
        }
        else
        {
            return;
        }
    }

    if (name && name[0] != '\0')
    {
        strncpy(s_discovered_devs[slot].name, name, sizeof(s_discovered_devs[slot].name) - 1);
        s_discovered_devs[slot].name[sizeof(s_discovered_devs[slot].name) - 1] = '\0';
    }
    if (rssi != 0)
    {
        s_discovered_devs[slot].rssi = rssi;
    }
    if (cod != 0)
    {
        s_discovered_devs[slot].cod = cod;
    }
    if (rep_mode != 0)
    {
        s_discovered_devs[slot].rep_mode = rep_mode;
    }
    if (clk_off != 0)
    {
        s_discovered_devs[slot].clock_offset = clk_off;
    }
    s_discovered_devs[slot].is_paired = hal_BtIsPairedWith(bd_addr);
    if (g_bt_status.connected && memcmp(g_bt_status.remote_bd_addr, bd_addr, 6) == 0)
    {
        s_discovered_devs[slot].is_connected = TRUE;
    }
    else
    {
        s_discovered_devs[slot].is_connected = FALSE;
    }
}

BOOL hal_BtStartInquiry(UINT8 duration_sec)
{
    if (!g_btPowered || !g_bt_status.hci_ready || g_bt_status.connected) return FALSE;

    uint8_t inq_len = (duration_sec > 0) ? (uint8_t)((duration_sec * 100) / 128) : 8;
    if (inq_len == 0) inq_len = 1;

    uint8_t cmd[4 + 5] = {
        0x01,       // HCI Command packet
        0x01, 0x04, // Opcode 0x0401 (HCI_Inquiry)
        0x05,       // Param length: 5
        0x33, 0x8B, 0x9E, // GIAC LAP
        inq_len,    // Duration
        0x00        // Num responses (0 = unlimited)
    };
    os_log_printf("[RDABT_INQ] Starting Inquiry (duration=%us, len=%u)...\n", duration_sec, inq_len);
    s_is_scanning = TRUE;
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtCancelInquiry(void)
{
    if (!g_btPowered || !g_bt_status.hci_ready) return FALSE;

    uint8_t cmd[4] = {
        0x01,
        0x02, 0x04, // Opcode 0x0402 (HCI_Inquiry_Cancel)
        0x00
    };
    os_log_printf("[RDABT_INQ] Canceling Inquiry...\n");
    s_is_scanning = FALSE;
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

static BOOL s_name_req_pending = FALSE;
static UINT8 s_name_req_addr[6] = {0};
static BOOL s_retry_connect_pending = FALSE;
static UINT8 s_retry_connect_addr[6] = {0};
static uint32_t s_retry_connect_time = 0;
static uint32_t s_acl_connect_time = 0;

BOOL hal_BtCancelRemoteNameRequest(const UINT8 *bd_addr)
{
    if (!g_btPowered || !g_bt_status.hci_ready || !bd_addr) return FALSE;

    uint8_t cmd[4 + 6] = {
        0x01,                   // HCI Command
        0x1A, 0x04,             // Opcode 0x041A (HCI_Remote_Name_Request_Cancel)
        0x06,                   // Param len: 6
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]
    };
    os_log_printf("[RDABT_HCI] Canceling Remote Name Request for %02X:%02X:%02X:%02X:%02X:%02X...\n",
                  bd_addr[5], bd_addr[4], bd_addr[3], bd_addr[2], bd_addr[1], bd_addr[0]);
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtConnect(const UINT8 *bd_addr)
{
    if (!g_btPowered || !g_bt_status.hci_ready || !bd_addr) return FALSE;

    memcpy(s_retry_connect_addr, bd_addr, 6);

    if (s_is_scanning)
    {
        hal_BtCancelInquiry();
        uint32_t t0 = timer_get_ms();
        while (s_is_scanning && (timer_get_ms() - t0) < 500)
        {
            hal_BtPollEvents();
            timer_delay_ms(10);
        }
    }

    if (s_name_req_pending)
    {
        os_log_printf("[RDABT_CONN] Remote name request is in progress, waiting up to 2.5s for completion...\n");
        uint32_t t0 = timer_get_ms();
        while (s_name_req_pending && (timer_get_ms() - t0) < 2500)
        {
            hal_BtPollEvents();
            timer_delay_ms(15);
        }
        if (s_name_req_pending)
        {
            os_log_printf("[RDABT_CONN] Remote name request timed out; canceling before connect...\n");
            hal_BtCancelRemoteNameRequest(s_name_req_addr);
            t0 = timer_get_ms();
            while (s_name_req_pending && (timer_get_ms() - t0) < 500)
            {
                hal_BtPollEvents();
                timer_delay_ms(15);
            }
            s_name_req_pending = FALSE;
        }
    }

    // Small delay to ensure baseband radio is completely idle
    timer_delay_ms(20);
    hal_BtPollEvents();

    uint8_t rep_mode = 0x02;   // Default to R2 (2.56s page scan interval)
    uint16_t clk_off = 0x0000; // Default: Clock offset invalid

    // Check if we have discovered device info for rep_mode & clock_offset
    for (int i = 0; i < s_discovered_count; i++)
    {
        if (memcmp(s_discovered_devs[i].bd_addr, bd_addr, 6) == 0)
        {
            if (s_discovered_devs[i].rep_mode <= 0x02 && s_discovered_devs[i].rep_mode != 0)
            {
                rep_mode = s_discovered_devs[i].rep_mode;
            }
            if (s_discovered_devs[i].clock_offset != 0)
            {
                clk_off = s_discovered_devs[i].clock_offset | 0x8000; // Bit 15 = Valid
            }
            break;
        }
    }

    uint8_t cmd[4 + 13] = {
        0x01,
        0x05, 0x04, // Opcode 0x0405 (HCI_Create_Connection)
        0x0D,       // Param len: 13
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
        0x18, 0xCC, // Packet Type: DM1, DH1, DM3, DH3, DM5, DH5
        rep_mode,   // Repetition mode
        0x00,       // Reserved
        (uint8_t)(clk_off & 0xFF), (uint8_t)((clk_off >> 8) & 0xFF),
        0x01        // Allow role switch
    };
    os_log_printf("[RDABT_CONN] Initiating ACL connection to %02X:%02X:%02X:%02X:%02X:%02X (R%u, ClkOff=0x%04X)...\n",
                  bd_addr[5], bd_addr[4], bd_addr[3], bd_addr[2], bd_addr[1], bd_addr[0], rep_mode, clk_off);
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtDisconnect(UINT16 handle)
{
    if (!g_btPowered || !g_bt_status.hci_ready) return FALSE;

    uint8_t cmd[4 + 3] = {
        0x01,
        0x06, 0x04, // Opcode 0x0406 (HCI_Disconnect)
        0x03,
        (uint8_t)(handle & 0xFF), (uint8_t)((handle >> 8) & 0x0F),
        0x13        // Reason: Remote User Terminated Connection
    };
    os_log_printf("[RDABT_CONN] Disconnecting Handle 0x%04X...\n", handle);
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtRequestRemoteName(const UINT8 *bd_addr, uint8_t rep_mode, uint16_t clk_off)
{
    if (!g_btPowered || !g_bt_status.hci_ready || !bd_addr) return FALSE;
    if (g_bt_status.connected) return FALSE;

    uint8_t cmd[4 + 10] = {
        0x01,                   // HCI Command
        0x19, 0x04,             // Opcode 0x0419 (HCI_Remote_Name_Request)
        0x0A,                   // Param len: 10
        bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5],
        rep_mode ? rep_mode : 0x01, // Page Scan Repetition Mode (R1=0x01, R2=0x02)
        0x00,                   // Reserved (Page Scan Period Mode)
        (uint8_t)(clk_off & 0xFF), (uint8_t)((clk_off >> 8) & 0xFF)
    };
    os_log_printf("[RDABT_HCI] Requesting Remote Name for %02X:%02X:%02X:%02X:%02X:%02X (rep_mode=0x%02X, clk=0x%04X)...\n",
                  bd_addr[5], bd_addr[4], bd_addr[3], bd_addr[2], bd_addr[1], bd_addr[0], cmd[10], clk_off);
    s_name_req_pending = TRUE;
    memcpy(s_name_req_addr, bd_addr, 6);
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtSetEncryption(UINT16 handle, uint8_t enable)
{
    if (!g_btPowered || !g_bt_status.hci_ready) return FALSE;

    uint8_t cmd[4 + 3] = {
        0x01,                   // HCI Command
        0x13, 0x04,             // Opcode 0x0413 (HCI_Set_Connection_Encryption)
        0x03,                   // Param len: 3
        (uint8_t)(handle & 0xFF), (uint8_t)((handle >> 8) & 0x0F),
        enable                  // 0x01 = Enable, 0x00 = Disable
    };
    os_log_printf("[RDABT_PAIR] Sending HCI_Set_Connection_Encryption (Handle 0x%04X, Enable=%u)...\n",
                  handle, enable);
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtRequestAuthentication(UINT16 handle)
{
    if (!g_btPowered || !g_bt_status.hci_ready) return FALSE;

    uint8_t cmd[4 + 2] = {
        0x01,                   // HCI Command
        0x11, 0x04,             // Opcode 0x0411 (HCI_Authentication_Requested)
        0x02,                   // Param len: 2
        (uint8_t)(handle & 0xFF), (uint8_t)((handle >> 8) & 0x0F)
    };
    os_log_printf("[RDABT_PAIR] Sending HCI_Authentication_Requested (Handle 0x%04X)...\n",
                  handle);
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

BOOL hal_BtMakeDiscoverable(BOOL discoverable)
{
    if (!g_btPowered || !g_bt_status.hci_ready) return FALSE;

    uint8_t scan_mode = discoverable ? 0x03 : 0x02;
    uint8_t cmd[4 + 1] = {
        0x01,
        0x1A, 0x0C, // Opcode 0x0C1A (HCI_Write_Scan_Enable)
        0x01,
        scan_mode
    };
    os_log_printf("[RDABT] Write Scan Enable: 0x%02X (%s)\n",
                  scan_mode, discoverable ? "Inquiry + Page Scan" : "Page Scan Only");
    hal_BtSendPacket(cmd, sizeof(cmd));
    return TRUE;
}

UINT8 hal_BtGetDiscoveredCount(void)
{
    return s_discovered_count;
}

const bt_remote_dev_t* hal_BtGetDiscoveredDevice(UINT8 index)
{
    if (index < s_discovered_count)
    {
        if (g_bt_status.connected && memcmp(g_bt_status.remote_bd_addr, s_discovered_devs[index].bd_addr, 6) == 0)
            s_discovered_devs[index].is_connected = TRUE;
        else
            s_discovered_devs[index].is_connected = FALSE;
        s_discovered_devs[index].is_paired = hal_BtIsPairedWith(s_discovered_devs[index].bd_addr);
        return &s_discovered_devs[index];
    }
    return NULL;
}

BOOL hal_BtIsScanning(void)
{
    return s_is_scanning;
}

void hal_BtClearDiscoveredDevices(void)
{
    memset(s_discovered_devs, 0, sizeof(s_discovered_devs));
    s_discovered_count = 0;
}

BOOL hal_BtIsPairedWith(const UINT8 *bd_addr)
{
    for (int i = 0; i < BT_MAX_PAIRED_DEVICES; i++)
    {
        if (s_paired_devices[i].valid)
        {
            if (memcmp(s_paired_devices[i].bd_addr, bd_addr, 6) == 0)
                return TRUE;
        }
    }
    return FALSE;
}

UINT8 hal_BtGetPairedCount(void)
{
    UINT8 cnt = 0;
    for (int i = 0; i < BT_MAX_PAIRED_DEVICES; i++)
    {
        if (s_paired_devices[i].valid) cnt++;
    }
    return cnt;
}

const hal_bt_paired_dev_t* hal_BtGetPairedDevice(UINT8 index)
{
    UINT8 cur = 0;
    for (int i = 0; i < BT_MAX_PAIRED_DEVICES; i++)
    {
        if (s_paired_devices[i].valid)
        {
            if (cur == index) return &s_paired_devices[i];
            cur++;
        }
    }
    return NULL;
}

// =============================================================================
// Diffie-Hellman P-192 ECDH Hardware Interface (Vendor Opcodes 0xFE20 / 0xFE21)
// =============================================================================
int MGR_Write_DHKey(uint8_t type, const uint8_t *key_x, const uint8_t *key_y)
{
    if (type != 0)
    {
        // HCI_Write_Local_Key (Opcode 0xFE20, len 48)
        uint8_t cmd[4 + 48];
        cmd[0] = 0x01; // HCI Command
        cmd[1] = 0x20; // Opcode low
        cmd[2] = 0xFE; // Opcode high (0xFE20)
        cmd[3] = 48;   // Param len = 48 bytes
        memcpy(&cmd[4], key_x, 24);
        memcpy(&cmd[28], key_y, 24);

        os_log_printf("[RDABT_PAIR] Sending HCI_Write_Local_Key (0xFE20, 48B local public key)...\n");
        hal_UartFifoFlush(HAL_UART_1);
        timer_delay_ms(5);
        hal_BtSendPacket(cmd, sizeof(cmd));
    }
    else
    {
        // HCI_Write_Peer_Key (Opcode 0xFE21, len 24)
        uint8_t cmd[4 + 24];
        cmd[0] = 0x01; // HCI Command
        cmd[1] = 0x21; // Opcode low
        cmd[2] = 0xFE; // Opcode high (0xFE21)
        cmd[3] = 24;   // Param len = 24 bytes
        memcpy(&cmd[4], key_x, 24);

        os_log_printf("[RDABT_PAIR] Sending HCI_Write_Peer_Key (0xFE21, 24B DHKey): ");
        for (int i = 0; i < 24; i++) os_log_printf("%02X ", key_x[i]);
        os_log_printf("\n");
        hal_BtSendPacket(cmd, sizeof(cmd));
    }
    return 0;
}

static uint8_t s_peer_auth = 0x03;

extern void bnep_accept_incoming_connection(uint16_t acl_handle, uint16_t remote_scid);
extern void bnep_handle_l2cap_disc_req(uint16_t dcid, uint16_t scid);

static uint16_t s_sdp_remote_scid = 0;
static uint16_t s_sdp_handle = 0;
#define SDP_LOCAL_CID 0x0050

static const uint8_t s_sdp_panu_record[] = {
    // Attr 0x0000: ServiceRecordHandle
    0x09, 0x00, 0x00,
    0x0A, 0x00, 0x01, 0x00, 0x01,

    // Attr 0x0001: ServiceClassIDList (PANU Client Only)
    0x09, 0x00, 0x01,
    0x35, 0x03,
    0x19, 0x11, 0x15,               // UUID: 0x1115 (PANU)

    // Attr 0x0004: ProtocolDescriptorList
    0x09, 0x00, 0x04,
    0x35, 0x18,
        0x35, 0x06,
            0x19, 0x01, 0x00,       // UUID: 0x0100 (L2CAP)
            0x09, 0x00, 0x0F,       // uint16: 0x000F (PSM BNEP)
        0x35, 0x0E,
            0x19, 0x00, 0x0F,       // UUID: 0x000F (BNEP)
            0x09, 0x01, 0x00,       // uint16: 0x0100 (BNEP Version 1.0)
            0x35, 0x06,
                0x09, 0x08, 0x00,   // uint16: 0x0800 (IPv4)
                0x09, 0x08, 0x06,   // uint16: 0x0806 (ARP)

    // Attr 0x0005: BrowseGroupList
    0x09, 0x00, 0x05,
    0x35, 0x03,
    0x19, 0x10, 0x02,               // UUID: 0x1002 (PublicBrowseRoot)

    // Attr 0x0006: LanguageBaseAttributeIDList
    0x09, 0x00, 0x06,
    0x35, 0x09,
        0x09, 0x65, 0x6E,           // 'en'
        0x09, 0x00, 0x6A,           // UTF-8
        0x09, 0x01, 0x00,           // Base 0x0100

    // Attr 0x0009: BluetoothProfileDescriptorList
    0x09, 0x00, 0x09,
    0x35, 0x08,
        0x35, 0x06,
            0x19, 0x11, 0x15,       // UUID: 0x1115 (PANU)
            0x09, 0x01, 0x00,       // uint16: 0x0100 (Version 1.0)

    // Attr 0x0100: ServiceName
    0x09, 0x01, 0x00,
    0x25, 0x04,
    'P', 'A', 'N', 'U'
};

static const uint8_t s_sdp_pnp_record[] = {
    // Attr 0x0000: ServiceRecordHandle (uint32: 0x00010002)
    0x09, 0x00, 0x00,
    0x0A, 0x00, 0x01, 0x00, 0x02,

    // Attr 0x0001: ServiceClassIDList (UUID 0x1200: PnPInformation)
    0x09, 0x00, 0x01,
    0x35, 0x03,
    0x19, 0x12, 0x00,

    // Attr 0x0200: SpecificationID (uint16: 0x0103)
    0x09, 0x02, 0x00,
    0x09, 0x01, 0x03,

    // Attr 0x0201: VendorID (uint16: 0x005D - RDA Microelectronics)
    0x09, 0x02, 0x01,
    0x09, 0x00, 0x5D,

    // Attr 0x0202: ProductID (uint16: 0x8809)
    0x09, 0x02, 0x02,
    0x09, 0x88, 0x09,

    // Attr 0x0203: Version (uint16: 0x0100)
    0x09, 0x02, 0x03,
    0x09, 0x01, 0x00,

    // Attr 0x0204: PrimaryRecord (bool: true)
    0x09, 0x02, 0x04,
    0x28, 0x01,

    // Attr 0x0205: VendorIDSource (uint16: 0x0001 - Bluetooth SIG)
    0x09, 0x02, 0x05,
    0x09, 0x00, 0x01
};

static void rdabt_handle_sdp_data(uint16_t handle, uint16_t scid, const uint8_t *data, uint16_t len)
{
    if (len < 5) return;
    uint8_t pdu_id = data[0];
    uint8_t tid_hi = data[1];
    uint8_t tid_lo = data[2];

    os_log_printf("[RDABT_SDP] Handling SDP PDU 0x%02X (TID 0x%02X%02X, len %u)...\n", pdu_id, tid_hi, tid_lo, len);

    if (pdu_id == 0x06) // SDP_ServiceSearchAttributeRequest
    {
        // Extract requested UUID from ServiceSearchPattern sequence
        uint16_t req_uuid = 0;
        if (len >= 10 && data[5] == 0x35)
        {
            if (data[7] == 0x19) // 16-bit UUID
            {
                req_uuid = (uint16_t)((data[8] << 8) | data[9]);
            }
            else if (data[7] == 0x1A && len >= 12) // 32-bit UUID
            {
                req_uuid = (uint16_t)((data[10] << 8) | data[11]);
            }
            else if (data[7] == 0x1C && len >= 24) // 128-bit UUID (Bluetooth Base)
            {
                req_uuid = (uint16_t)((data[10] << 8) | data[11]);
            }
        }
        os_log_printf("[RDABT_SDP] SearchAttributeRequest: Pattern UUID=0x%04X\n", req_uuid);

        const uint8_t *rec_ptr = NULL;
        uint16_t rec_len = 0;

        if (req_uuid == 0x1200) // PnPInformation (Device ID)
        {
            rec_ptr = s_sdp_pnp_record;
            rec_len = sizeof(s_sdp_pnp_record);
        }
        else if (req_uuid == 0x1115 || req_uuid == 0x112E ||
                 req_uuid == 0x1002 || req_uuid == 0x0100 || req_uuid == 0x0000)
        {
            rec_ptr = s_sdp_panu_record;
            rec_len = sizeof(s_sdp_panu_record);
        }

        if (rec_ptr != NULL)
        {
            uint16_t inner_len = rec_len;
            uint16_t outer_len = inner_len + 2;
            uint16_t list_byte_count = outer_len + 2;

            uint16_t sdp_param_len = 2 + list_byte_count + 1;
            uint16_t sdp_pdu_len = 5 + sdp_param_len;
            uint16_t l2cap_pdu_len = 4 + sdp_pdu_len;

            uint8_t resp[256];
            uint16_t idx = 0;

            // HCI ACL Header (5 bytes)
            resp[idx++] = 0x02;
            resp[idx++] = (uint8_t)(handle & 0xFF);
            resp[idx++] = (uint8_t)(((handle >> 8) & 0x0F) | 0x20);
            resp[idx++] = (uint8_t)(l2cap_pdu_len & 0xFF);
            resp[idx++] = (uint8_t)(l2cap_pdu_len >> 8);

            // L2CAP Header (4 bytes)
            resp[idx++] = (uint8_t)(sdp_pdu_len & 0xFF);
            resp[idx++] = (uint8_t)(sdp_pdu_len >> 8);
            resp[idx++] = (uint8_t)(scid & 0xFF);
            resp[idx++] = (uint8_t)(scid >> 8);

            // SDP Header (5 bytes)
            resp[idx++] = 0x07; // SDP_ServiceSearchAttributeResponse
            resp[idx++] = tid_hi;
            resp[idx++] = tid_lo;
            resp[idx++] = (uint8_t)(sdp_param_len >> 8);
            resp[idx++] = (uint8_t)(sdp_param_len & 0xFF);

            // AttributeListsByteCount (2 bytes)
            resp[idx++] = (uint8_t)(list_byte_count >> 8);
            resp[idx++] = (uint8_t)(list_byte_count & 0xFF);

            // Outer Data Element Sequence of records (2 bytes)
            resp[idx++] = 0x35;
            resp[idx++] = (uint8_t)outer_len;

            // Inner Data Element Sequence of attributes (2 bytes)
            resp[idx++] = 0x35;
            resp[idx++] = (uint8_t)inner_len;

            // Attributes payload
            memcpy(&resp[idx], rec_ptr, rec_len);
            idx += rec_len;

            // Continuation State (1 byte: 0x00)
            resp[idx++] = 0x00;

            os_log_printf("[RDABT_SDP] Replying SDP_ServiceSearchAttributeResponse (%u bytes) for UUID 0x%04X\n", idx, req_uuid);
            hal_BtSendPacket(resp, idx);
        }
        else
        {
            // Empty match response (AttributeListsByteCount = 0)
            uint16_t sdp_param_len = 3; // 2 bytes count (0) + 1 byte cont
            uint16_t sdp_pdu_len = 5 + sdp_param_len;
            uint16_t l2cap_pdu_len = 4 + sdp_pdu_len;

            uint8_t resp[32];
            uint16_t idx = 0;

            resp[idx++] = 0x02;
            resp[idx++] = (uint8_t)(handle & 0xFF);
            resp[idx++] = (uint8_t)(((handle >> 8) & 0x0F) | 0x20);
            resp[idx++] = (uint8_t)(l2cap_pdu_len & 0xFF);
            resp[idx++] = (uint8_t)(l2cap_pdu_len >> 8);

            resp[idx++] = (uint8_t)(sdp_pdu_len & 0xFF);
            resp[idx++] = (uint8_t)(sdp_pdu_len >> 8);
            resp[idx++] = (uint8_t)(scid & 0xFF);
            resp[idx++] = (uint8_t)(scid >> 8);

            resp[idx++] = 0x07; // SDP_ServiceSearchAttributeResponse
            resp[idx++] = tid_hi;
            resp[idx++] = tid_lo;
            resp[idx++] = (uint8_t)(sdp_param_len >> 8);
            resp[idx++] = (uint8_t)(sdp_param_len & 0xFF);

            resp[idx++] = 0x00; // AttributeListsByteCount = 0
            resp[idx++] = 0x00;
            resp[idx++] = 0x00; // Continuation State = 0

            os_log_printf("[RDABT_SDP] UUID 0x%04X not found -> Replying empty SDP response\n", req_uuid);
            hal_BtSendPacket(resp, idx);
        }
    }
    else if (pdu_id == 0x02) // SDP_ServiceSearchRequest
    {
        uint16_t req_uuid = 0;
        if (len >= 10 && data[5] == 0x35 && data[7] == 0x19)
        {
            req_uuid = (uint16_t)((data[8] << 8) | data[9]);
        }
        uint32_t srv_handle = (req_uuid == 0x1200) ? 0x00010002 : 0x00010001;

        uint8_t resp[] = {
            0x02,
            (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
            18, 0x00,                                   // ACL Length = 18
            14, 0x00,                                   // L2CAP Length = 14
            (uint8_t)(scid & 0xFF), (uint8_t)(scid >> 8),
            0x03,                                       // SDP_ServiceSearchResponse
            tid_hi, tid_lo,
            0x00, 0x09,                                 // Parameter Length: 9
            0x00, 0x01,                                 // TotalServiceRecordCount = 1
            0x00, 0x01,                                 // CurrentServiceRecordCount = 1
            (uint8_t)(srv_handle >> 24), (uint8_t)(srv_handle >> 16),
            (uint8_t)(srv_handle >> 8), (uint8_t)(srv_handle & 0xFF),
            0x00                                        // ContinuationState = 0
        };
        os_log_printf("[RDABT_SDP] Replying SDP_ServiceSearchResponse (Handle: 0x%08X)\n", srv_handle);
        hal_BtSendPacket(resp, sizeof(resp));
    }
    else if (pdu_id == 0x04) // SDP_ServiceAttributeRequest
    {
        uint32_t rec_handle = 0;
        if (len >= 9) {
            rec_handle = ((uint32_t)data[5] << 24) | ((uint32_t)data[6] << 16) |
                         ((uint32_t)data[7] << 8) | (uint32_t)data[8];
        }

        const uint8_t *rec_ptr = (rec_handle == 0x00010002) ? s_sdp_pnp_record : s_sdp_panu_record;
        uint16_t rec_len = (rec_handle == 0x00010002) ? sizeof(s_sdp_pnp_record) : sizeof(s_sdp_panu_record);

        uint16_t list_byte_count = rec_len + 2;
        uint16_t sdp_param_len = 2 + list_byte_count + 1;
        uint16_t sdp_pdu_len = 5 + sdp_param_len;
        uint16_t l2cap_pdu_len = 4 + sdp_pdu_len;

        uint8_t resp[256];
        uint16_t idx = 0;

        resp[idx++] = 0x02;
        resp[idx++] = (uint8_t)(handle & 0xFF);
        resp[idx++] = (uint8_t)(((handle >> 8) & 0x0F) | 0x20);
        resp[idx++] = (uint8_t)(l2cap_pdu_len & 0xFF);
        resp[idx++] = (uint8_t)(l2cap_pdu_len >> 8);

        resp[idx++] = (uint8_t)(sdp_pdu_len & 0xFF);
        resp[idx++] = (uint8_t)(sdp_pdu_len >> 8);
        resp[idx++] = (uint8_t)(scid & 0xFF);
        resp[idx++] = (uint8_t)(scid >> 8);

        resp[idx++] = 0x05; // SDP_ServiceAttributeResponse
        resp[idx++] = tid_hi;
        resp[idx++] = tid_lo;
        resp[idx++] = (uint8_t)(sdp_param_len >> 8);
        resp[idx++] = (uint8_t)(sdp_param_len & 0xFF);

        resp[idx++] = (uint8_t)(list_byte_count >> 8);
        resp[idx++] = (uint8_t)(list_byte_count & 0xFF);

        resp[idx++] = 0x35;
        resp[idx++] = (uint8_t)rec_len;
        memcpy(&resp[idx], rec_ptr, rec_len);
        idx += rec_len;

        resp[idx++] = 0x00;

        os_log_printf("[RDABT_SDP] Replying SDP_ServiceAttributeResponse (%u bytes, Record Handle 0x%08X)\n", idx, rec_handle);
        hal_BtSendPacket(resp, idx);
    }
}

static volatile uint32_t s_bt_poll_busy = 0;

void hal_BtPollEvents(void)
{
    if (!g_btPowered) return;

    taskENTER_CRITICAL();
    if (s_bt_poll_busy)
    {
        taskEXIT_CRITICAL();
        return;
    }
    s_bt_poll_busy = 1;
    taskEXIT_CRITICAL();

    // 1. Fast drain all available bytes from UART into s_bt_rx_buf without blocking logs
    while (hal_UartRxAvailable(HAL_UART_1) && s_bt_rx_len < sizeof(s_bt_rx_buf))
    {
        uint32_t bytes_read = hal_UartRead(HAL_UART_1, &s_bt_rx_buf[s_bt_rx_len],
                                           sizeof(s_bt_rx_buf) - s_bt_rx_len, 0);
        if (bytes_read == 0) break;
        s_bt_rx_len += (uint16_t)bytes_read;
    }

    // Drop any leading bytes that cannot start a valid HCI packet (0x04 for Event, 0x02 for ACL)
    // or spurious/glitched event headers (0x04 followed by 0x00, or duplicate 0x04)
    uint16_t skip = 0;
    while (skip < s_bt_rx_len)
    {
        uint8_t b = s_bt_rx_buf[skip];
        if (b == 0x04 || b == 0x02)
        {
            if (b == 0x04 && (skip + 1 < s_bt_rx_len) && s_bt_rx_buf[skip + 1] == 0x00)
            {
                skip += 2;
                continue;
            }
            if (b == 0x04 && (skip + 2 < s_bt_rx_len) && s_bt_rx_buf[skip + 1] == 0x04 && s_bt_rx_buf[skip + 2] != 0x0A)
            {
                skip += 1;
                continue;
            }
            break;
        }
        skip++;
    }
    if (skip > 0)
    {
        os_log_printf("[RDABT_UART] Skipped %u leading desync bytes (first: 0x%02X)\n", skip, s_bt_rx_buf[0]);
        uint16_t rem = s_bt_rx_len - skip;
        if (rem > 0)
        {
            for (uint16_t i = 0; i < rem; i++) {
                s_bt_rx_buf[i] = s_bt_rx_buf[skip + i];
            }
        }
        s_bt_rx_len = rem;
    }

    // Process packets while buffer has enough data
    while (s_bt_rx_len >= 3)
    {
        uint8_t pkt_type = s_bt_rx_buf[0];

        // 1. HCI Event Packet (Type 0x04)
        if (pkt_type == 0x04)
        {
            // Ensure at least 3 bytes for event header
            if (s_bt_rx_len < 3)
            {
                uint32_t start_ms = timer_get_ms();
                while (s_bt_rx_len < 3 && (timer_get_ms() - start_ms) < 20)
                {
                    uint32_t br = hal_UartRead(HAL_UART_1, &s_bt_rx_buf[s_bt_rx_len],
                                               3 - s_bt_rx_len, 5);
                    if (br > 0) s_bt_rx_len += (uint16_t)br;
                }
                if (s_bt_rx_len < 3) break;
            }

            uint8_t evt_code = s_bt_rx_buf[1];
            uint8_t p_len    = s_bt_rx_buf[2];
            uint16_t total_pkt_len = 3 + p_len;

            // Sanity check: drop corrupted event packets that exceed buffer
            if (total_pkt_len > sizeof(s_bt_rx_buf))
            {
                os_log_printf("[RDABT_UART] Corrupt Event length %u, dropping byte 0x%02X\n",
                              total_pkt_len, s_bt_rx_buf[0]);
                uint16_t rem = s_bt_rx_len - 1;
                for (uint16_t i = 0; i < rem; i++) {
                    s_bt_rx_buf[i] = s_bt_rx_buf[1 + i];
                }
                s_bt_rx_len = rem;
                continue;
            }

            if (s_bt_rx_len < total_pkt_len)
            {
                // Incomplete event packet: actively drain UART to complete it
                uint32_t start_ms = timer_get_ms();
                uint32_t drain_timeout = (evt_code == 0xFF) ? 1500 : 50;
                while (s_bt_rx_len < total_pkt_len && (timer_get_ms() - start_ms) < drain_timeout)
                {
                    uint32_t br = hal_UartRead(HAL_UART_1, &s_bt_rx_buf[s_bt_rx_len],
                                               total_pkt_len - s_bt_rx_len, 10);
                    if (br > 0) s_bt_rx_len += (uint16_t)br;
                }
                if (s_bt_rx_len < total_pkt_len) break;
            }

            uint8_t *p_data = &s_bt_rx_buf[3];

            os_log_printf("[BT_RX_EVT] Code 0x%02X, Len %u: ", evt_code, p_len);
            uint32_t dlen = (total_pkt_len < 16) ? total_pkt_len : 16;
            for (uint32_t i = 0; i < dlen; i++) {
                os_log_printf("%02X ", s_bt_rx_buf[i]);
            }
            if (total_pkt_len > 16) os_log_printf("...");
            os_log_printf("\n");

            switch (evt_code)
            {
                case 0x0F: // HCI_evCOMMANDSTATUS
                {
                    uint8_t status = p_data[0];
                    uint8_t num_cmd = p_data[1];
                    uint16_t opcode = p_data[2] | (p_data[3] << 8);
                    (void)num_cmd;
                    os_log_printf("[RDABT_HCI] Command Status: Opcode 0x%04X -> Status 0x%02X (%s)\n",
                                  opcode, status, (status == 0x00) ? "PENDING" : "FAILED");
                    if (opcode == 0x0405 && status != 0x00)
                    {
                        os_log_printf("[RDABT_CONN] HCI_Create_Connection REJECTED by controller: 0x%02X!\n", status);
                        if (status == 0x0C)
                        {
                            os_log_printf("[RDABT_CONN] Controller busy (0x0C) - scheduling auto-retry!\n");
                            s_retry_connect_pending = TRUE;
                            s_retry_connect_time = timer_get_ms();
                        }
                    }
                    if (opcode == 0x0419 && status != 0x00)
                    {
                        s_name_req_pending = FALSE;
                    }
                    if (opcode == 0x041A)
                    {
                        os_log_printf("[RDABT_HCI] Remote Name Request Cancel Status: 0x%02X\n", status);
                    }
                    break;
                }

                case 0x01: // HCI_evINQUIRYCOMPLETE
                {
                    uint8_t status = p_data[0];
                    s_is_scanning = FALSE;
                    os_log_printf("[RDABT_INQ] Inquiry Complete (status: 0x%02X). Total Discovered: %u\n", status, s_discovered_count);
                    for (int i = 0; i < s_discovered_count; i++)
                    {
                        if (s_discovered_devs[i].name[0] == '\0')
                        {
                            hal_BtRequestRemoteName(s_discovered_devs[i].bd_addr,
                                                    s_discovered_devs[i].rep_mode ? s_discovered_devs[i].rep_mode : 0x01,
                                                    s_discovered_devs[i].clock_offset);
                            break;
                        }
                    }
                    break;
                }

                case 0x02: // HCI_evINQUIRYRESULT
                {
                    uint8_t num_resp = p_data[0];
                    os_log_printf("[RDABT_INQ] Inquiry Result: %u responses\n", num_resp);
                    uint16_t offset = 1;
                    for (uint8_t i = 0; i < num_resp && offset + 14 <= p_len; i++)
                    {
                        const uint8_t *b = &p_data[offset];
                        uint8_t rep_mode = p_data[offset + 6];
                        uint32_t cod = (uint32_t)p_data[offset + 9] | ((uint32_t)p_data[offset + 10] << 8) | ((uint32_t)p_data[offset + 11] << 16);
                        uint16_t clk_off = (uint16_t)p_data[offset + 12] | ((uint16_t)p_data[offset + 13] << 8);

                        os_log_printf("[RDABT_INQ] Dev [%u]: %02X:%02X:%02X:%02X:%02X:%02X, CoD=0x%06X\n",
                                      i, b[5], b[4], b[3], b[2], b[1], b[0], cod);
                        hal_BtAddOrUpdateDiscovered(b, NULL, 0, cod, rep_mode, clk_off);
                        offset += 14;
                    }
                    break;
                }

                case 0x22: // HCI_evINQUIRYRESULTWITHRSSI
                {
                    uint8_t num_resp = p_data[0];
                    os_log_printf("[RDABT_INQ] Inquiry Result with RSSI: %u responses\n", num_resp);
                    uint16_t offset = 1;
                    for (uint8_t i = 0; i < num_resp && offset + 14 <= p_len; i++)
                    {
                        const uint8_t *b = &p_data[offset];
                        uint8_t rep_mode = p_data[offset + 6];
                        uint32_t cod = (uint32_t)p_data[offset + 8] | ((uint32_t)p_data[offset + 9] << 8) | ((uint32_t)p_data[offset + 10] << 16);
                        uint16_t clk_off = (uint16_t)p_data[offset + 11] | ((uint16_t)p_data[offset + 12] << 8);
                        int8_t rssi = (int8_t)p_data[offset + 13];

                        os_log_printf("[RDABT_INQ] Dev [%u]: %02X:%02X:%02X:%02X:%02X:%02X, CoD=0x%06X, RSSI=%d dBm\n",
                                      i, b[5], b[4], b[3], b[2], b[1], b[0], cod, rssi);
                        hal_BtAddOrUpdateDiscovered(b, NULL, rssi, cod, rep_mode, clk_off);
                        offset += 14;
                    }
                    break;
                }

                case 0x2F: // HCI_evEXTENDEDINQUIRYRESULT
                {
                    if (p_len >= 14)
                    {
                        const uint8_t *b = &p_data[1]; // bytes 1..6: BD_ADDR
                        uint8_t rep_mode = p_data[7];
                        uint32_t cod = (uint32_t)p_data[9] | ((uint32_t)p_data[10] << 8) | ((uint32_t)p_data[11] << 16);
                        uint16_t clk_off = (uint16_t)p_data[12] | ((uint16_t)p_data[13] << 8);
                        int8_t rssi = (p_len >= 15) ? (int8_t)p_data[14] : 0;

                        char name_buf[32] = {0};
                        if (p_len > 15)
                        {
                            uint16_t eir_idx = 15;
                            while (eir_idx < p_len)
                            {
                                uint8_t dlen = p_data[eir_idx];
                                if (dlen == 0 || eir_idx + 1 + dlen > p_len) break;
                                uint8_t dtype = p_data[eir_idx + 1];
                                if (dtype == 0x09 || dtype == 0x08) // 0x09: Complete Name, 0x08: Shortened
                                {
                                    uint8_t nlen = dlen - 1;
                                    if (nlen > sizeof(name_buf) - 1) nlen = sizeof(name_buf) - 1;
                                    memcpy(name_buf, &p_data[eir_idx + 2], nlen);
                                    name_buf[nlen] = '\0';
                                    break;
                                }
                                eir_idx += 1 + dlen;
                            }
                        }

                        os_log_printf("[RDABT_INQ] EIR Dev: %02X:%02X:%02X:%02X:%02X:%02X, RSSI=%d dBm, Name=\"%s\"\n",
                                      b[5], b[4], b[3], b[2], b[1], b[0], rssi, name_buf);
                        hal_BtAddOrUpdateDiscovered(b, name_buf[0] ? name_buf : NULL, rssi, cod, rep_mode, clk_off);
                    }
                    break;
                }

                case 0x07: // HCI_evREMOTENAMEREQUESTCOMPLETE
                {
                    s_name_req_pending = FALSE;
                    uint8_t status = p_data[0];
                    const uint8_t *b = &p_data[1];
                    if (status == 0x00 && p_len > 7)
                    {
                        char name_buf[32] = {0};
                        uint16_t nlen = p_len - 7;
                        if (nlen > sizeof(name_buf) - 1) nlen = sizeof(name_buf) - 1;
                        memcpy(name_buf, &p_data[7], nlen);
                        name_buf[nlen] = '\0';
                        os_log_printf("[RDABT_INQ] Remote Name for %02X:%02X:%02X:%02X:%02X:%02X: \"%s\"\n",
                                      b[5], b[4], b[3], b[2], b[1], b[0], name_buf);
                        hal_BtAddOrUpdateDiscovered(b, name_buf, 0, 0, 0, 0);
                    }
                    else
                    {
                        os_log_printf("[RDABT_INQ] Remote Name Request failed (status: 0x%02X)\n", status);
                    }

                    if (s_retry_connect_pending)
                    {
                        s_retry_connect_pending = FALSE;
                        os_log_printf("[RDABT_CONN] Remote name completed - retrying deferred ACL connection now!\n");
                        hal_BtConnect(s_retry_connect_addr);
                        break;
                    }

                    // Request next device name if any discovered device is still without a name
                    for (int i = 0; i < s_discovered_count; i++)
                    {
                        if (s_discovered_devs[i].name[0] == '\0')
                        {
                            hal_BtRequestRemoteName(s_discovered_devs[i].bd_addr,
                                                    s_discovered_devs[i].rep_mode ? s_discovered_devs[i].rep_mode : 0x01,
                                                    s_discovered_devs[i].clock_offset);
                            break;
                        }
                    }
                    break;
                }

                case 0x04: // HCI_evCONNECTIONREQUEST
                {
                    if (p_len < 10)
                    {
                        os_log_printf("[RDABT_PAIR] Dropping bogus Connection Request (len %u < 10)\n", p_len);
                        break;
                    }

                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];
                    uint32_t cod = p_data[6] | (p_data[7] << 8) | (p_data[8] << 16);
                    uint8_t link_type = p_data[9];

                    os_log_printf("[RDABT_PAIR] >>> Incoming Connection Request from %02X:%02X:%02X:%02X:%02X:%02X (CoD: 0x%06X, Type: 0x%02X)! <<<\n",
                                  peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0],
                                  cod, link_type);

                    if (hal_BtIsPairedWith(peer_addr))
                    {
                        os_log_printf("[RDABT_PAIR] Auto-accepting connection from previously paired peer...\n");
                        hal_BtAcceptConnection(peer_addr, 0x01);
                    }
                    else
                    {
                        memcpy(s_pending_conn_addr, peer_addr, 6);
                        s_pending_conn_name[0] = '\0';
                        for (int i = 0; i < s_discovered_count; i++) {
                            if (memcmp(s_discovered_devs[i].bd_addr, peer_addr, 6) == 0 && s_discovered_devs[i].name[0] != '\0') {
                                strncpy(s_pending_conn_name, s_discovered_devs[i].name, sizeof(s_pending_conn_name) - 1);
                                break;
                            }
                        }
                        s_pending_conn_req = TRUE;
                        os_log_printf("[RDABT_PAIR] Deferred connection request - awaiting user prompt approval in VeebhaOS UI\n");
                    }
                    break;
                }

                case 0x03: // HCI_evCONNECTIONCOMPLETE
                {
                    uint8_t status = p_data[0];
                    uint16_t handle = p_data[1] | (p_data[2] << 8);
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[3 + i];

                    if (status == 0x00)
                    {
                        g_bt_status.connected = TRUE;
                        g_bt_status.conn_handle = handle;
                        s_acl_connect_time = timer_get_ms();
                        for (int i = 0; i < 6; i++) g_bt_status.remote_bd_addr[i] = peer_addr[i];

                        BOOL found = FALSE;
                        for (int i = 0; i < s_discovered_count; i++)
                        {
                            if (memcmp(s_discovered_devs[i].bd_addr, peer_addr, 6) == 0)
                            {
                                s_discovered_devs[i].is_connected = TRUE;
                                found = TRUE;
                                break;
                            }
                        }
                        if (!found)
                        {
                            hal_BtAddOrUpdateDiscovered(peer_addr, NULL, 0, 0x5A020C, 0x01, 0);
                            if (s_discovered_count > 0) {
                                s_discovered_devs[s_discovered_count - 1].is_connected = TRUE;
                            }
                        }

                        os_log_printf("[RDABT_PAIR] ===================================================\n");
                        os_log_printf("[RDABT_PAIR] *** BASEBAND ACL LINK CONNECTED! ***\n");
                        os_log_printf("[RDABT_PAIR] Handle    : 0x%04X\n", handle);
                        os_log_printf("[RDABT_PAIR] Peer MAC  : %02X:%02X:%02X:%02X:%02X:%02X\n",
                                      peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
                        os_log_printf("[RDABT_PAIR] ===================================================\n");

                        if (hal_BtIsPairedWith(peer_addr))
                        {
                            g_bt_status.paired = TRUE;
                            hal_BtSetPairingInProgress(FALSE);
                            os_log_printf("[RDABT_PAIR] Reconnected to paired peer! Requesting authentication on Handle 0x%04X...\n", handle);
                            hal_BtRequestAuthentication(handle);
                        }
                        else
                        {
                            hal_BtSetPairingInProgress(TRUE);
                            os_log_printf("[RDABT_PAIR] Unpaired peer connected on Handle 0x%04X! Requesting authentication...\n", handle);
                            hal_BtRequestAuthentication(handle);
                        }
                    }
                    else
                    {
                        hal_BtSetPairingInProgress(FALSE);
                        const char *err_str = "Error";
                        if (status == 0x04) err_str = "Page Timeout (Device not in BT settings / unreachable)";
                        else if (status == 0x05) err_str = "Authentication Failure";
                        else if (status == 0x08) err_str = "Connection Timeout";
                        else if (status == 0x0B) err_str = "ACL Link Already Exists";
                        else if (status == 0x10) err_str = "Connection Accept Timeout";

                        os_log_printf("[RDABT_PAIR] Connection FAILED! Status: 0x%02X (%s)\n", status, err_str);
                    }
                    break;
                }

                case 0x32: // HCI_evIOCAPABILITYRESPONSE (Remote Device IO Capabilities)
                {
                    if (p_len < 9)
                    {
                        os_log_printf("[RDABT_PAIR] Bogus 0x32 event (len %u < 9)\n", p_len);
                        break;
                    }
                    hal_BtSetPairingInProgress(TRUE);
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];
                    uint8_t peer_io_cap = p_data[6];
                    uint8_t peer_oob    = p_data[7];
                    uint8_t peer_auth   = p_data[8];

                    os_log_printf("[RDABT_PAIR] Peer IO Capability: Cap=0x%02X, OOB=0x%02X, Auth=0x%02X\n",
                                  peer_io_cap, peer_oob, peer_auth);
                    break;
                }

                case 0x31: // HCI_evIOCAPABILITYREQUEST (Secure Simple Pairing)
                {
                    if (p_len < 6)
                    {
                        os_log_printf("[RDABT_PAIR] Bogus 0x31 event (len %u < 6)\n", p_len);
                        break;
                    }
                    hal_BtSetPairingInProgress(TRUE);
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];

                    os_log_printf("[RDABT_PAIR] >>> Secure Simple Pairing: IO Capability Request from %02X:%02X:%02X:%02X:%02X:%02X <<<\n",
                                  peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
                    os_log_printf("[RDABT_PAIR] Replying: IO_Cap=NoInputNoOutput (0x03), Auth=0x04 (Just Works, General Bonding)...\n");
                    hal_BtIoCapabilityReply(peer_addr, 0x03, 0x00, 0x04);
                    break;
                }

                case 0x33: // HCI_evUSERCONFIRMATIONREQUEST (SSP Numeric Comparison)
                {
                    if (p_len < 10)
                    {
                        os_log_printf("[RDABT_PAIR] Bogus 0x33 event (len %u < 10)\n", p_len);
                        break;
                    }
                    hal_BtSetPairingInProgress(TRUE);
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];
                    uint32_t val = (uint32_t)p_data[6] | ((uint32_t)p_data[7] << 8) |
                                   ((uint32_t)p_data[8] << 16) | ((uint32_t)p_data[9] << 24);

                    os_log_printf("[RDABT_PAIR] ===================================================\n");
                    os_log_printf("[RDABT_PAIR] >>> NUMERIC COMPARISON PIN: %06u <<<\n", val % 1000000);
                    os_log_printf("[RDABT_PAIR] Peer: %02X:%02X:%02X:%02X:%02X:%02X -> Auto-confirming...\n",
                                  peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
                    os_log_printf("[RDABT_PAIR] ===================================================\n");

                    hal_BtFormatPinMsg(val);
                    hal_BtUserConfirmationReply(peer_addr);
                    break;
                }

                case 0x35: // HCI_evREMOTEOOBDATAREQUEST
                {
                    if (p_len < 6) break;
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];
                    uint8_t neg_cmd[] = { 0x01, 0x33, 0x04, 0x06, peer_addr[0], peer_addr[1], peer_addr[2], peer_addr[3], peer_addr[4], peer_addr[5] };
                    hal_BtSendPacket(neg_cmd, sizeof(neg_cmd));
                    break;
                }

                case 0x34: // HCI_evUSERPASSKEYREQUEST
                {
                    if (p_len < 6) break;
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];
                    os_log_printf("[RDABT_PAIR] >>> Passkey Request from peer. Replying with 000000...\n");
                    hal_BtUserPasskeyReply(peer_addr, 0);
                    break;
                }

                case 0x36: // HCI_evSIMPLEPAIRINGCOMPLETE
                {
                    if (p_len < 7)
                    {
                        os_log_printf("[RDABT_PAIR] Bogus 0x36 event (len %u < 7)\n", p_len);
                        break;
                    }
                    uint8_t status = p_data[0];
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[1 + i];

                    if (status == 0x00)
                    {
                        g_bt_status.paired = TRUE;
                        for (int i = 0; i < s_discovered_count; i++)
                        {
                            if (memcmp(s_discovered_devs[i].bd_addr, peer_addr, 6) == 0)
                            {
                                s_discovered_devs[i].is_paired = TRUE;
                                break;
                            }
                        }
                        os_log_printf("[RDABT_PAIR] *** SIMPLE PAIRING SUCCESSFUL! Peer: %02X:%02X:%02X:%02X:%02X:%02X ***\n",
                                      peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
                        hal_BtFormatPairMsg(peer_addr);

                        // Note: Link Key Notification (0x18) will follow immediately, which saves the key and connects BNEP.
                    }
                    else
                    {
                        os_log_printf("[RDABT_PAIR] Simple Pairing Failed (Status: 0x%02X)\n", status);
                        g_bt_status.paired = FALSE;
                    }
                    break;
                }

                case 0x17: // HCI_evLINKKEYREQUEST (Legacy PIN Pairing)
                {
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];

                    os_log_printf("[RDABT_PAIR] Controller requested Link Key for %02X:%02X:%02X:%02X:%02X:%02X\n",
                                  peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);

                    const UINT8 *key = hal_BtFindLinkKey(peer_addr);
                    if (key)
                    {
                        os_log_printf("[RDABT_PAIR] Stored Link Key found! Sending Link Key Reply...\n");
                        hal_BtLinkKeyReply(peer_addr, key);
                    }
                    else
                    {
                        hal_BtSetPairingInProgress(TRUE);
                        os_log_printf("[RDABT_PAIR] No stored key -> Sending Negative Reply (requesting PIN code)...\n");
                        hal_BtLinkKeyNegativeReply(peer_addr);
                    }
                    break;
                }

                case 0x16: // HCI_evPINCODEREQUEST (Legacy PIN Pairing)
                {
                    hal_BtSetPairingInProgress(TRUE);
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];

                    os_log_printf("[RDABT_PAIR] >>> PIN CODE REQUEST from %02X:%02X:%02X:%02X:%02X:%02X! <<<\n",
                                  peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
                    os_log_printf("[RDABT_PAIR] Replying with default PIN: \"0000\"...\n");

                    hal_BtPinCodeReply(peer_addr, "0000");
                    break;
                }

                case 0x18: // HCI_evLINKKEYNOTIFICATION
                {
                    hal_BtSetPairingInProgress(FALSE);
                    UINT8 peer_addr[6];
                    for (int i = 0; i < 6; i++) peer_addr[i] = p_data[i];
                    const UINT8 *key = &p_data[6];
                    UINT8 key_type = p_data[22];

                    hal_BtSaveLinkKey(peer_addr, key, key_type);
                    g_bt_status.paired = TRUE;
                    for (int i = 0; i < s_discovered_count; i++)
                    {
                        if (memcmp(s_discovered_devs[i].bd_addr, peer_addr, 6) == 0)
                        {
                            s_discovered_devs[i].is_paired = TRUE;
                            break;
                        }
                    }

                    os_log_printf("[RDABT_PAIR] ===================================================\n");
                    os_log_printf("[RDABT_PAIR] *** PAIRING SUCCESSFUL! LINK KEY GENERATED & SAVED ***\n");
                    os_log_printf("[RDABT_PAIR] Peer MAC  : %02X:%02X:%02X:%02X:%02X:%02X\n",
                                  peer_addr[5], peer_addr[4], peer_addr[3], peer_addr[2], peer_addr[1], peer_addr[0]);
                    os_log_printf("[RDABT_PAIR] Key Type  : 0x%02X\n", key_type);
                    os_log_printf("[RDABT_PAIR] Link Key  : ");
                    for (int k = 0; k < 16; k++) os_log_printf("%02X ", key[k]);
                    os_log_printf("\n");
                    os_log_printf("[RDABT_PAIR] ===================================================\n");

                    // Update LCD test result banner
                    hal_BtFormatPairMsg(peer_addr);

                    os_log_printf("[RDABT_PAIR] Link key ready! Enabling link encryption on Handle 0x%04X...\n", g_bt_status.conn_handle);
                    hal_BtSetEncryption(g_bt_status.conn_handle, 1);
                    break;
                }

                case 0x06: // HCI_evAUTHENTICATIONCOMPLETE
                {
                    hal_BtSetPairingInProgress(FALSE);
                    uint8_t status = p_data[0];
                    uint16_t handle = p_data[1] | (p_data[2] << 8);
                    os_log_printf("[RDABT_PAIR] Authentication Complete! Handle=0x%04X, Status=0x%02X (%s)\n",
                                  handle, status, (status == 0x00) ? "SUCCESS - TRUSTED DEVICE" : "FAILED");
                    if (status == 0x00)
                    {
                        g_bt_status.paired = TRUE;
                        os_log_printf("[RDABT_PAIR] Authentication complete! Enabling link encryption on Handle 0x%04X...\n", handle);
                        hal_BtSetEncryption(handle, 1);
                    }
                    else
                    {
                        g_bt_status.paired = FALSE;
                        if (status == 0x06) // HCI_ERR_PIN_OR_KEY_MISSING explicitly rejected by peer
                        {
                            os_log_printf("[RDABT_PAIR] Peer rejected link key (PIN/Key missing) -> Deleting link key\n");
                            hal_BtDeleteLinkKey(g_bt_status.remote_bd_addr);
                        }
                    }
                    break;
                }

                case 0x08: // HCI_evENCRYPTIONCHANGE
                {
                    hal_BtSetPairingInProgress(FALSE);
                    uint8_t status = p_data[0];
                    uint16_t handle = p_data[1] | (p_data[2] << 8);
                    uint8_t enc_enabled = (p_len >= 4) ? p_data[3] : 0;
                    os_log_printf("[RDABT_PAIR] Encryption Change: Handle=0x%04X, Status=0x%02X, Enabled=0x%02X\n",
                                  handle, status, enc_enabled);
                    if (status == 0x00 && enc_enabled != 0)
                    {
                        g_bt_status.paired = TRUE;
                        os_log_printf("[RDABT_PAIR] Encrypted link active! Waiting for peer SDP / BNEP...\n");
                    }
                    break;
                }

                case 0x13: // HCI_evNUMBEROFCOMPLETEDPACKETS
                {
                    // Flow control packet completion from controller, ignore quietly
                    break;
                }

                case 0x38: // HCI_evLINKSUPERVISIONTIMEOUTCHANGED
                {
                    if (p_len >= 4)
                    {
                        uint16_t handle = p_data[0] | (p_data[1] << 8);
                        uint16_t timeout = p_data[2] | (p_data[3] << 8);
                        os_log_printf("[RDABT_EVENT] Link Supervision Timeout Changed: Handle=0x%04X, Timeout=%u slots (%u ms)\n",
                                      handle, timeout, (uint32_t)timeout * 625 / 1000);
                    }
                    break;
                }

                case 0x05: // HCI_evDISCONNECTIONCOMPLETE
                {
                    hal_BtSetPairingInProgress(FALSE);
                    uint8_t status = p_data[0];
                    uint16_t handle = p_data[1] | (p_data[2] << 8);
                    uint8_t reason = p_data[3];

                    g_bt_status.connected = FALSE;
                    s_acl_connect_time = 0;
                    for (int i = 0; i < s_discovered_count; i++)
                    {
                        s_discovered_devs[i].is_connected = FALSE;
                    }
                    bnep_disconnect();
                    os_log_printf("[RDABT_PAIR] ACL Link Disconnected! Handle=0x%04X, Reason=0x%02X\n", handle, reason);

                    // Re-enable discoverability/connectability
                    os_log_printf("[RDABT_PAIR] Re-enabling Inquiry & Page Scan for discoverability...\n");
                    uint8_t scan_cmd[] = { 0x01, 0x1A, 0x0C, 0x01, 0x03 };
                    hal_BtSendPacket(scan_cmd, sizeof(scan_cmd));
                    break;
                }

                case 0x1A: // HCI_evDISCOVERYRESULT (or other events)
                {
                    // Re-assert scan enable if dropped
                    uint8_t scan_cmd[] = { 0x01, 0x1A, 0x0C, 0x01, 0x03 };
                    hal_BtSendPacket(scan_cmd, sizeof(scan_cmd));
                    break;
                }

                case 0xFF: // Vendor-Specific Event
                {
                    uint8_t subcode = p_data[0];
                    if (subcode == 0x62 && p_len >= 49)
                    {
                        hal_BtSetPairingInProgress(TRUE);
                        // 48-byte peer public key follows the 1-byte subcode 0x62
                        const uint8_t *peer_pub_key = &p_data[1];
                        os_log_printf("[RDABT_PAIR] ===================================================\n");
                        os_log_printf("[RDABT_PAIR] >>> PEER PUBLIC KEY RECEIVED (Event 0xFF Subcode 0x62) <<<\n");
                        os_log_printf("[RDABT_PAIR] Peer Public Key (48B): ");
                        for (int i = 0; i < 16; i++) os_log_printf("%02X ", peer_pub_key[i]);
                        os_log_printf("...\n");
                        os_log_printf("[RDABT_PAIR] Computing P-192 Elliptic Curve Diffie-Hellman Key (DHKey)...\n");

                        rdabt_mgr_calculate_dkkey(peer_pub_key);

                        os_log_printf("[RDABT_PAIR] DHKey calculated & sent to controller via Opcode 0xFE21!\n");
                        os_log_printf("[RDABT_PAIR] ===================================================\n");
                    }
                    else
                    {
                        os_log_printf("[RDABT_EVENT] Vendor Event 0xFF Subcode 0x%02X (len %u)\n", subcode, p_len);
                    }
                    break;
                }

                default:
                {
                    os_log_printf("[RDABT_EVENT] Event 0x%02X (len %u): ", evt_code, p_len);
                    for (uint32_t i = 0; i < p_len && i < 16; i++) {
                        os_log_printf("%02X ", p_data[i]);
                    }
                    os_log_printf("\n");
                    break;
                }
            }

            // Shift buffer past the processed packet
            uint16_t rem = s_bt_rx_len - total_pkt_len;
            if (rem > 0)
            {
                for (uint16_t i = 0; i < rem; i++) {
                    s_bt_rx_buf[i] = s_bt_rx_buf[total_pkt_len + i];
                }
            }
            s_bt_rx_len = rem;
        }
        // 2. HCI ACL Data Packet (Type 0x02)
        else if (pkt_type == 0x02)
        {
            // Ensure at least 5 bytes for ACL header
            if (s_bt_rx_len < 5)
            {
                uint32_t start_ms = timer_get_ms();
                while (s_bt_rx_len < 5 && (timer_get_ms() - start_ms) < 20)
                {
                    uint32_t br = hal_UartRead(HAL_UART_1, &s_bt_rx_buf[s_bt_rx_len],
                                               5 - s_bt_rx_len, 5);
                    if (br > 0) s_bt_rx_len += (uint16_t)br;
                }
                if (s_bt_rx_len < 5) break;
            }

            uint16_t handle = (uint16_t)(s_bt_rx_buf[1] | ((s_bt_rx_buf[2] & 0x0F) << 8));
            uint16_t data_len = (uint16_t)(s_bt_rx_buf[3] | (s_bt_rx_buf[4] << 8));
            uint16_t total_acl_len = 5 + data_len;

            // Sanity check: prevent parser lockup if data_len is corrupt or exceeds buffer
            if (total_acl_len > sizeof(s_bt_rx_buf) || total_acl_len < 5)
            {
                os_log_printf("[RDABT_UART] Corrupt ACL length %u, dropping byte 0x%02X\n",
                              total_acl_len, s_bt_rx_buf[0]);
                uint16_t rem = s_bt_rx_len - 1;
                for (uint16_t i = 0; i < rem; i++) {
                    s_bt_rx_buf[i] = s_bt_rx_buf[1 + i];
                }
                s_bt_rx_len = rem;
                continue;
            }

            if (s_bt_rx_len < total_acl_len)
            {
                // Incomplete ACL packet: actively drain UART to complete it
                uint32_t start_ms = timer_get_ms();
                while (s_bt_rx_len < total_acl_len && (timer_get_ms() - start_ms) < 50)
                {
                    uint32_t br = hal_UartRead(HAL_UART_1, &s_bt_rx_buf[s_bt_rx_len],
                                               total_acl_len - s_bt_rx_len, 10);
                    if (br > 0) s_bt_rx_len += (uint16_t)br;
                }
                if (s_bt_rx_len < total_acl_len) break;
            }

            uint16_t l2cap_len = (data_len >= 4) ? (uint16_t)(s_bt_rx_buf[5] | (s_bt_rx_buf[6] << 8)) : 0;
            uint16_t cid       = (data_len >= 4) ? (uint16_t)(s_bt_rx_buf[7] | (s_bt_rx_buf[8] << 8)) : 0;
            os_log_printf("[BT_RX_ACL] Handle 0x%04X, Len %u (L2CAP: Len %u, CID 0x%04X): ",
                          handle, data_len, l2cap_len, cid);
            uint32_t dlen = (total_acl_len < 18) ? total_acl_len : 18;
            for (uint32_t i = 0; i < dlen; i++) {
                os_log_printf("%02X ", s_bt_rx_buf[i]);
            }
            if (total_acl_len > 18) os_log_printf("...");
            os_log_printf("\n");

            if (!g_bt_status.connected)
            {
                g_bt_status.connected = TRUE;
                g_bt_status.conn_handle = handle;
                os_log_printf("[RDABT_ACL] Recovered connected state on Handle 0x%04X from incoming ACL data\n", handle);
            }

            // Check if this is an L2CAP Signaling Command (CID 0x0001)
            if (data_len >= 8 && s_bt_rx_buf[7] == 0x01 && s_bt_rx_buf[8] == 0x00)
            {
                uint16_t l2cap_pdu_len = (uint16_t)(s_bt_rx_buf[5] | (s_bt_rx_buf[6] << 8));
                uint16_t sig_offset = 9;
                uint16_t pdu_end = 9 + l2cap_pdu_len;
                if (pdu_end > 5 + data_len) pdu_end = 5 + data_len;
                if (pdu_end > s_bt_rx_len) pdu_end = s_bt_rx_len;

                while (sig_offset + 4 <= pdu_end)
                {
                    uint8_t sig_code = s_bt_rx_buf[sig_offset];
                    uint8_t req_id   = s_bt_rx_buf[sig_offset + 1];
                    uint16_t cmd_len = (uint16_t)(s_bt_rx_buf[sig_offset + 2] | (s_bt_rx_buf[sig_offset + 3] << 8));
                    const uint8_t *cmd_payload = &s_bt_rx_buf[sig_offset + 4];
                    uint16_t next_offset = sig_offset + 4 + cmd_len;
                    uint16_t avail_len = (next_offset <= pdu_end) ? cmd_len : (pdu_end - (sig_offset + 4));

                    if (sig_code == 0x02 && avail_len >= 4) // L2CAP_CMD_CONN_REQ
                    {
                        uint16_t psm   = (uint16_t)(cmd_payload[0] | (cmd_payload[1] << 8));
                        uint16_t scid  = (uint16_t)(cmd_payload[2] | (cmd_payload[3] << 8));

                        if (psm == 0x0001) // SDP (Service Discovery Protocol)
                        {
                            s_sdp_remote_scid = scid;
                            s_sdp_handle = handle;
                            os_log_printf("[RDABT_SDP] Incoming Connection Request for SDP (SCID 0x%04X, ID %u). Accepting on DCID 0x0050...\n", scid, req_id);

                            // L2CAP Connection Response: Result 0x0000 (Success)
                            uint8_t l2cap_resp[] = {
                                0x02,
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                16, 0x00,
                                12, 0x00,
                                0x01, 0x00,                                 // CID = 0x0001 (Signaling)
                                0x03,                                       // Code: Connection Response
                                req_id,                                     // Identifier
                                0x08, 0x00,                                 // Command Length: 8
                                0x50, 0x00,                                 // Destination CID = 0x0050 (SDP)
                                (uint8_t)(scid & 0xFF), (uint8_t)(scid >> 8), // Source CID
                                0x00, 0x00,                                 // Result: 0x0000 (Success)
                                0x00, 0x00                                  // Status: 0x0000
                            };
                            hal_BtSendPacket(l2cap_resp, sizeof(l2cap_resp));

                            // Send L2CAP Configuration Request for peer SCID
                            uint8_t cfg_req[] = {
                                0x02,
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                12, 0x00,
                                8, 0x00,
                                0x01, 0x00,
                                0x04,                                       // Code: Configuration Request
                                0x20,                                       // Identifier
                                0x04, 0x00,
                                (uint8_t)(scid & 0xFF), (uint8_t)(scid >> 8),
                                0x00, 0x00
                            };
                            hal_BtSendPacket(cfg_req, sizeof(cfg_req));
                        }
                        else if (psm == 0x000F) // BNEP (Bluetooth Network Encapsulation Protocol)
                        {
                            os_log_printf("[RDABT_BNEP] Incoming Connection Request for BNEP (SCID 0x%04X, ID %u). Accepting on DCID 0x0040...\n", scid, req_id);
                            bnep_accept_incoming_connection(handle, scid);

                            uint8_t l2cap_resp[] = {
                                0x02,
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                16, 0x00,
                                12, 0x00,
                                0x01, 0x00,
                                0x03,
                                req_id,
                                0x08, 0x00,
                                0x40, 0x00,                                 // Destination CID = 0x0040 (BNEP)
                                (uint8_t)(scid & 0xFF), (uint8_t)(scid >> 8),
                                0x00, 0x00,                                 // Result: 0x0000 (Success)
                                0x00, 0x00
                            };
                            hal_BtSendPacket(l2cap_resp, sizeof(l2cap_resp));

                            static uint8_t s_bnep_in_cfg_id = 0x20;
                            uint8_t cfg_id = ++s_bnep_in_cfg_id;
                            uint8_t cfg_req[] = {
                                0x02,
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                16, 0x00,
                                12, 0x00,
                                0x01, 0x00,
                                0x04,
                                cfg_id,
                                0x08, 0x00,
                                (uint8_t)(scid & 0xFF), (uint8_t)(scid >> 8),
                                0x00, 0x00,
                                0x01, 0x02, 0x9B, 0x06                      // MTU = 1691 bytes
                            };
                            hal_BtSendPacket(cfg_req, sizeof(cfg_req));
                        }
                        else
                        {
                            os_log_printf("[RDABT_L2CAP] Connection Request for Unsupported PSM 0x%04X (SCID 0x%04X, ID %u)\n", psm, scid, req_id);
                            uint8_t l2cap_resp[] = {
                                0x02,
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                16, 0x00,
                                12, 0x00,
                                0x01, 0x00,
                                0x03,
                                req_id,
                                0x08, 0x00,
                                0x00, 0x00,
                                (uint8_t)(scid & 0xFF), (uint8_t)(scid >> 8),
                                0x02, 0x00,                                 // Result: 0x0002 (PSM Not Supported)
                                0x00, 0x00
                            };
                            hal_BtSendPacket(l2cap_resp, sizeof(l2cap_resp));
                        }
                    }
                    else if (sig_code == 0x0A && avail_len >= 2) // L2CAP_CMD_INFO_REQ (Information Request)
                    {
                        uint16_t info_type = (uint16_t)(cmd_payload[0] | (cmd_payload[1] << 8));
                        os_log_printf("[RDABT_L2CAP] Information Request: InfoType=0x%04X (ID %u)\n", info_type, req_id);

                        if (info_type == 0x0002) // Extended Features Mask
                        {
                            uint8_t info_rsp[21] = {
                                0x02,                                       // HCI ACL Packet
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                16, 0x00,                                   // ACL Length: 16
                                12, 0x00,                                   // L2CAP Length: 12
                                0x01, 0x00,                                 // CID: 0x0001 (Signaling)
                                0x0B,                                       // Code: Information Response
                                req_id,                                     // Identifier
                                0x08, 0x00,                                 // Command Length: 8
                                0x02, 0x00,                                 // InfoType: 0x0002 (Extended Features)
                                0x00, 0x00,                                 // Result: 0x0000 (Success)
                                0x80, 0x00, 0x00, 0x00                      // Mask: Bit 7 = Fixed Channels Supported
                            };
                            os_log_printf("[RDABT_L2CAP] Replying Information Response: Extended Features\n");
                            hal_BtSendPacket(info_rsp, sizeof(info_rsp));
                        }
                        else if (info_type == 0x0003) // Fixed Channels Supported
                        {
                            uint8_t fix_rsp[25] = {
                                0x02,                                       // HCI ACL Packet
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                20, 0x00,                                   // ACL Length: 20
                                16, 0x00,                                   // L2CAP Length: 16
                                0x01, 0x00,                                 // CID: 0x0001 (Signaling)
                                0x0B,                                       // Code: Information Response
                                req_id,                                     // Identifier
                                0x0C, 0x00,                                 // Command Length: 12
                                0x03, 0x00,                                 // InfoType: 0x0003 (Fixed Channels)
                                0x00, 0x00,                                 // Result: 0x0000 (Success)
                                0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 // Fixed Channels Mask (Bit 1 = CID 0x0001 Signaling)
                            };
                            os_log_printf("[RDABT_L2CAP] Replying Information Response: Fixed Channels\n");
                            hal_BtSendPacket(fix_rsp, sizeof(fix_rsp));
                        }
                        else
                        {
                            uint8_t notsup_rsp[17] = {
                                0x02,
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                12, 0x00,
                                0x08, 0x00,
                                0x01, 0x00,
                                0x0B,
                                req_id,
                                0x04, 0x00,
                                (uint8_t)(info_type & 0xFF), (uint8_t)(info_type >> 8),
                                0x01, 0x00 // Result: 0x0001 (Not Supported)
                            };
                            hal_BtSendPacket(notsup_rsp, sizeof(notsup_rsp));
                        }
                    }
                    else if (sig_code == 0x08) // L2CAP_CMD_ECHO_REQ
                    {
                        uint16_t echo_len = cmd_len;
                        os_log_printf("[RDABT_L2CAP] Echo Request (ID %u, len %u)\n", req_id, echo_len);
                        uint8_t echo_rsp[16 + 32];
                        if (echo_len > 32) echo_len = 32;
                        echo_rsp[0] = 0x02;
                        echo_rsp[1] = (uint8_t)(handle & 0xFF);
                        echo_rsp[2] = (uint8_t)(((handle >> 8) & 0x0F) | 0x20);
                        echo_rsp[3] = (uint8_t)((4 + 4 + echo_len) & 0xFF);
                        echo_rsp[4] = (uint8_t)(((4 + 4 + echo_len) >> 8) & 0xFF);
                        echo_rsp[5] = (uint8_t)((4 + echo_len) & 0xFF);
                        echo_rsp[6] = (uint8_t)(((4 + echo_len) >> 8) & 0xFF);
                        echo_rsp[7] = 0x01; echo_rsp[8] = 0x00; // CID
                        echo_rsp[9] = 0x09;                     // Echo Response
                        echo_rsp[10] = req_id;
                        echo_rsp[11] = (uint8_t)(echo_len & 0xFF);
                        echo_rsp[12] = (uint8_t)((echo_len >> 8) & 0xFF);
                        if (echo_len > 0) {
                            memcpy(&echo_rsp[13], cmd_payload, echo_len);
                        }
                        hal_BtSendPacket(echo_rsp, 13 + echo_len);
                    }
                    else if (sig_code == 0x03 && avail_len >= 6) // L2CAP_CMD_CONN_RSP
                    {
                        uint16_t dcid   = (uint16_t)(cmd_payload[0] | (cmd_payload[1] << 8));
                        uint16_t scid   = (uint16_t)(cmd_payload[2] | (cmd_payload[3] << 8));
                        uint16_t result = (uint16_t)(cmd_payload[4] | (cmd_payload[5] << 8));
                        os_log_printf("[RDABT_L2CAP] Connection Response: SCID=0x%04X, DCID=0x%04X, Result=0x%04X\n",
                                      scid, dcid, result);
                        bnep_handle_l2cap_conn_rsp(scid, dcid, result);
                    }
                    else if (sig_code == 0x04 && avail_len >= 4) // L2CAP_CMD_CFG_REQ
                    {
                        uint16_t dcid = (uint16_t)(cmd_payload[0] | (cmd_payload[1] << 8));
                        uint16_t mtu = 1691;
                        if (avail_len >= 8 && cmd_payload[4] == 0x01 && cmd_payload[5] == 0x02) {
                            mtu = (uint16_t)(cmd_payload[6] | (cmd_payload[7] << 8));
                        }
                        os_log_printf("[RDABT_L2CAP] Config Request for DCID=0x%04X (MTU=%u, ID %u)\n", dcid, mtu, req_id);
                        if (dcid == 0x0050)
                        {
                            uint8_t cfg_rsp[19] = {
                                0x02,
                                (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                                14, 0x00,                                   // ACL Length = 14
                                10, 0x00,                                   // L2CAP Length = 10
                                0x01, 0x00,                                 // CID: 0x0001 (Signaling)
                                0x05,                                       // Code: Configuration Response
                                req_id,                                     // Identifier
                                0x06, 0x00,                                 // Length: 6
                                (uint8_t)(s_sdp_remote_scid & 0xFF), (uint8_t)(s_sdp_remote_scid >> 8),
                                0x00, 0x00,                                 // Flags: 0
                                0x00, 0x00                                  // Result: 0x0000 (Success)
                            };
                            hal_BtSendPacket(cfg_rsp, sizeof(cfg_rsp));
                            os_log_printf("[RDABT_SDP] SDP L2CAP Channel Configured Successfully!\n");
                        }
                        else
                        {
                            bnep_handle_l2cap_cfg_req(dcid, req_id, mtu);
                        }
                    }
                    else if (sig_code == 0x05 && avail_len >= 6) // L2CAP_CMD_CFG_RSP
                    {
                        uint16_t scid   = (uint16_t)(cmd_payload[0] | (cmd_payload[1] << 8));
                        uint16_t result = (uint16_t)(cmd_payload[4] | (cmd_payload[5] << 8));
                        os_log_printf("[RDABT_L2CAP] Config Response: SCID=0x%04X, Result=0x%04X\n", scid, result);
                        bnep_handle_l2cap_cfg_rsp(scid, result);
                    }
                    else if (sig_code == 0x06 && avail_len >= 4) // L2CAP_CMD_DISC_REQ
                    {
                        uint16_t dcid = (uint16_t)(cmd_payload[0] | (cmd_payload[1] << 8));
                        uint16_t scid = (uint16_t)(cmd_payload[2] | (cmd_payload[3] << 8));
                        os_log_printf("[RDABT_L2CAP] Disconnection Request: DCID=0x%04X, SCID=0x%04X (ID %u)\n",
                                      dcid, scid, req_id);

                        // Replying L2CAP Disconnection Response (Code 0x07)
                        uint8_t disc_rsp[17] = {
                            0x02,
                            (uint8_t)(handle & 0xFF), (uint8_t)(((handle >> 8) & 0x0F) | 0x20),
                            12, 0x00,                                   // ACL Length = 12
                            8, 0x00,                                    // L2CAP Length = 8
                            0x01, 0x00,                                 // CID: 0x0001 (Signaling)
                            0x07,                                       // Code: Disconnection Response
                            req_id,                                     // Identifier
                            0x04, 0x00,                                 // Length: 4
                            (uint8_t)(dcid & 0xFF), (uint8_t)(dcid >> 8),
                            (uint8_t)(scid & 0xFF), (uint8_t)(scid >> 8)
                        };
                        hal_BtSendPacket(disc_rsp, sizeof(disc_rsp));

                        if (dcid == 0x0050)
                        {
                            os_log_printf("[RDABT_SDP] SDP Channel 0x0050 Closed Cleanly.\n");
                            s_sdp_remote_scid = 0;
                        }
                        else
                        {
                            bnep_handle_l2cap_disc_req(dcid, scid);
                        }
                    }

                    if (cmd_len == 0 || next_offset >= pdu_end) break;
                    sig_offset = next_offset;
                }
            }
            else if (data_len >= 4) // Dynamic L2CAP Data Channel (CID != 0x0001)
            {
                uint16_t l2cap_len = (uint16_t)(s_bt_rx_buf[5] | (s_bt_rx_buf[6] << 8));
                uint16_t cid       = (uint16_t)(s_bt_rx_buf[7] | (s_bt_rx_buf[8] << 8));
                if (cid == 0x0050)
                {
                    rdabt_handle_sdp_data(handle, s_sdp_remote_scid, &s_bt_rx_buf[9], l2cap_len);
                }
                else
                {
                    bnep_handle_l2cap_data(handle, cid, &s_bt_rx_buf[9], l2cap_len);
                }
            }

            uint16_t rem = s_bt_rx_len - total_acl_len;
            if (rem > 0)
            {
                for (uint16_t i = 0; i < rem; i++) {
                    s_bt_rx_buf[i] = s_bt_rx_buf[total_acl_len + i];
                }
            }
            s_bt_rx_len = rem;
        }
        else
        {
            // Drop 1 unsynchronized byte
            os_log_printf("[RDABT_UART] Dropping desync byte: 0x%02X\n", s_bt_rx_buf[0]);
            uint16_t rem = s_bt_rx_len - 1;
            for (uint16_t i = 0; i < rem; i++) {
                s_bt_rx_buf[i] = s_bt_rx_buf[1 + i];
            }
            s_bt_rx_len = rem;
        }
    }

    if (s_retry_connect_pending && !s_name_req_pending && (timer_get_ms() - s_retry_connect_time > 200))
    {
        s_retry_connect_pending = FALSE;
        os_log_printf("[RDABT_CONN] Auto-retrying deferred connection after delay...\n");
        hal_BtConnect(s_retry_connect_addr);
    }

    if (g_bt_status.connected && !hal_BtIsPairingInProgress() && s_acl_connect_time > 0 &&
        (timer_get_ms() - s_acl_connect_time > 3000))
    {
        extern bool connectivity_tethering_get_bt(void);
        if (connectivity_tethering_get_bt())
        {
            const bnep_conn_t *bnep_stat = bnep_get_status();
            if (bnep_stat->state == BNEP_STATE_IDLE && !bnep_is_connected() && s_sdp_remote_scid == 0)
            {
                s_acl_connect_time = 0;
                os_log_printf("[RDABT_PAIR] ACL active & stable, SDP idle - initiating BNEP PANU connection on Handle 0x%04X...\n",
                              g_bt_status.conn_handle);
                bnep_connect(g_bt_status.conn_handle, g_bt_status.remote_bd_addr);
            }
        }
        else
        {
            s_acl_connect_time = 0;
        }
    }

    s_bt_poll_busy = 0;
}

