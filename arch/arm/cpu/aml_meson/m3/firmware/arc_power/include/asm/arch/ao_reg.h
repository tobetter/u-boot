#ifndef __ALWAYS_ON_REG_H_
#define __ALWAYS_ON_REG_H_

#define IO_AOBUS_BASE 0xc8100000

#define P_AO_RTI_STATUS_REG0         (IO_AOBUS_BASE | (0x00 << 10) | (0x00 << 2))
#define P_AO_RTI_STATUS_REG1         (IO_AOBUS_BASE | (0x00 << 10) | (0x01 << 2))
#define P_AO_RTI_STATUS_REG2         (IO_AOBUS_BASE | (0x00 << 10) | (0x02 << 2))

#define P_AO_RTI_PWR_CNTL_REG0       (IO_AOBUS_BASE | (0x00 << 10) | (0x04 << 2))
#define P_AO_RTI_PIN_MUX_REG         (IO_AOBUS_BASE | (0x00 << 10) | (0x05 << 2))

#define P_AO_WD_GPIO_REG             (IO_AOBUS_BASE | (0x00 << 10) | (0x06 << 2))
#define P_AO_REMAP_REG0              (IO_AOBUS_BASE | (0x00 << 10) | (0x07 << 2))
#define P_AO_REMAP_REG1              (IO_AOBUS_BASE | (0x00 << 10) | (0x08 << 2))
#define P_AO_GPIO_O_EN_N             (IO_AOBUS_BASE | (0x00 << 10) | (0x09 << 2))
#define P_AO_GPIO_I                  (IO_AOBUS_BASE | (0x00 << 10) | (0x0A << 2))

#define P_AO_RTI_PULL_UP_REG         (IO_AOBUS_BASE | (0x00 << 10) | (0x0B << 2))
#define P_AO_RTI_JTAG_CODNFIG_REG    (IO_AOBUS_BASE | (0x00 << 10) | (0x0C << 2))
#define P_AO_RTI_WD_MARK             (IO_AOBUS_BASE | (0x00 << 10) | (0x0D << 2))
#define P_AO_RTI_GEN_CNTL_REG0       (IO_AOBUS_BASE | (0x00 << 10) | (0x10 << 2))

#define P_AO_WATCHDOG_REG            (IO_AOBUS_BASE | (0x00 << 10) | (0x11 << 2))
#define P_AO_WATCHDOG_RESET          (IO_AOBUS_BASE | (0x00 << 10) | (0x12 << 2))

#define P_AO_TIMER_REG               (IO_AOBUS_BASE | (0x00 << 10) | (0x13 << 2))
#define P_AO_TIMERA_REG              (IO_AOBUS_BASE | (0x00 << 10) | (0x14 << 2))
#define P_AO_TIMERE_REG              (IO_AOBUS_BASE | (0x00 << 10) | (0x15 << 2))

#define P_AO_AHB2DDR_CNTL            (IO_AOBUS_BASE | (0x00 << 10) | (0x18 << 2))

#define P_AO_IRQ_MASK_FIQ_SEL        (IO_AOBUS_BASE | (0x00 << 10) | (0x20 << 2))
#define P_AO_IRQ_GPIO_REG            (IO_AOBUS_BASE | (0x00 << 10) | (0x21 << 2))
#define P_AO_IRQ_STAT                (IO_AOBUS_BASE | (0x00 << 10) | (0x22 << 2))
#define P_AO_IRQ_STAT_CLR            (IO_AOBUS_BASE | (0x00 << 10) | (0x23 << 2))

#define P_AO_DEBUG_REG0              (IO_AOBUS_BASE | (0x00 << 10) | (0x28 << 2))
#define P_AO_DEBUG_REG1              (IO_AOBUS_BASE | (0x00 << 10) | (0x29 << 2))
#define P_AO_DEBUG_REG2              (IO_AOBUS_BASE | (0x00 << 10) | (0x2a << 2))
#define P_AO_DEBUG_REG3              (IO_AOBUS_BASE | (0x00 << 10) | (0x2b << 2))

// -------------------------------------------------------------------
// BASE #1
// -------------------------------------------------------------------
#define P_AO_IR_DEC_LDR_ACTIVE       (IO_AOBUS_BASE | (0x01 << 10) | (0x20 << 2))
#define P_AO_IR_DEC_LDR_IDLE         (IO_AOBUS_BASE | (0x01 << 10) | (0x21 << 2))
#define P_AO_IR_DEC_LDR_REPEAT       (IO_AOBUS_BASE | (0x01 << 10) | (0x22 << 2))
#define P_AO_IR_DEC_BIT_0            (IO_AOBUS_BASE | (0x01 << 10) | (0x23 << 2))
#define P_AO_IR_DEC_REG0             (IO_AOBUS_BASE | (0x01 << 10) | (0x24 << 2))
#define P_AO_IR_DEC_FRAME            (IO_AOBUS_BASE | (0x01 << 10) | (0x25 << 2))
#define P_AO_IR_DEC_STATUS           (IO_AOBUS_BASE | (0x01 << 10) | (0x26 << 2))
#define P_AO_IR_DEC_REG1             (IO_AOBUS_BASE | (0x01 << 10) | (0x27 << 2))

// ----------------------------
// UART
// ----------------------------
#define P_AO_UART_WFIFO              (IO_AOBUS_BASE | (0x01 << 10) | (0x30 << 2))
#define P_AO_UART_RFIFO              (IO_AOBUS_BASE | (0x01 << 10) | (0x31 << 2))
#define P_AO_UART_CONTROL            (IO_AOBUS_BASE | (0x01 << 10) | (0x32 << 2))
#define P_AO_UART_STATUS             (IO_AOBUS_BASE | (0x01 << 10) | (0x33 << 2))
#define P_AO_UART_MISC               (IO_AOBUS_BASE | (0x01 << 10) | (0x34 << 2))

// ----------------------------
// I2C Master (8)
// ----------------------------
#define P_AO_I2C_M_0_CONTROL_REG     (0xc8100500)
#define P_AO_I2C_M_0_SLAVE_ADDR      (0xc8100504)
#define P_AO_I2C_M_0_TOKEN_LIST0     (0xc8100508)
#define P_AO_I2C_M_0_TOKEN_LIST1     (0xc810050c)
#define P_AO_I2C_M_0_WDATA_REG0      (0xc8100510)
#define P_AO_I2C_M_0_WDATA_REG1      (0xc8100514)
#define P_AO_I2C_M_0_RDATA_REG0      (0xc8100518)
#define P_AO_I2C_M_0_RDATA_REG1      (0xc810051c)
// ----------------------------
// I2C Slave (3)
// ----------------------------
#define P_AO_I2C_S_CONTROL_REG       (IO_AOBUS_BASE | (0x01 << 10) | (0x50 << 2))
#define P_AO_I2C_S_SEND_REG          (IO_AOBUS_BASE | (0x01 << 10) | (0x51 << 2))
#define P_AO_I2C_S_RECV_REG          (IO_AOBUS_BASE | (0x01 << 10) | (0x52 << 2))
#define P_AO_I2C_S_CNTL1_REG         (IO_AOBUS_BASE | (0x01 << 10) | (0x53 << 2))

// ---------------------------
// RTC (4)
// ---------------------------
#define P_AO_RTC_ADDR0               (IO_AOBUS_BASE | (0x01 << 10) | (0xd0 << 2))
#define P_AO_RTC_ADDR1               (IO_AOBUS_BASE | (0x01 << 10) | (0xd1 << 2))
#define P_AO_RTC_ADDR2               (IO_AOBUS_BASE | (0x01 << 10) | (0xd2 << 2))
#define P_AO_RTC_ADDR3               (IO_AOBUS_BASE | (0x01 << 10) | (0xd3 << 2))
#define P_AO_RTC_ADDR4               (IO_AOBUS_BASE | (0x01 << 10) | (0xd4 << 2))
#endif

