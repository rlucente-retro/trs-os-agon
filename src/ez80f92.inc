; ==============================================================================
; ez80f92.inc - Canonical eZ80F92 Peripheral Register Equates
;
; Reference: Zilog eZ80F92 / eZ80F93 Product Specification (PS0153)
; Formatted for ez80asm (Agon Platform Assembler)
; ==============================================================================

; ------------------------------------------------------------------------------
; Programmable Reload Timers (PRT)
; ------------------------------------------------------------------------------
TMR0_CTL:       EQU $80             ; Timer 0 Control Register
TMR0_DR_L:      EQU $81             ; Timer 0 Data Register Low
TMR0_RR_L:      EQU $81             ; Timer 0 Reload Register Low
TMR0_DR_H:      EQU $82             ; Timer 0 Data Register High
TMR0_RR_H:      EQU $82             ; Timer 0 Reload Register High

TMR1_CTL:       EQU $83             ; Timer 1 Control Register
TMR1_DR_L:      EQU $84             ; Timer 1 Data Register Low
TMR1_RR_L:      EQU $84             ; Timer 1 Reload Register Low
TMR1_DR_H:      EQU $85             ; Timer 1 Data Register High
TMR1_RR_H:      EQU $85             ; Timer 1 Reload Register High

TMR2_CTL:       EQU $86             ; Timer 2 Control Register
TMR2_DR_L:      EQU $87             ; Timer 2 Data Register Low
TMR2_RR_L:      EQU $87             ; Timer 2 Reload Register Low
TMR2_DR_H:      EQU $88             ; Timer 2 Data Register High
TMR2_RR_H:      EQU $88             ; Timer 2 Reload Register High

TMR3_CTL:       EQU $89             ; Timer 3 Control Register
TMR3_DR_L:      EQU $8A             ; Timer 3 Data Register Low
TMR3_RR_L:      EQU $8A             ; Timer 3 Reload Register Low
TMR3_DR_H:      EQU $8B             ; Timer 3 Data Register High
TMR3_RR_H:      EQU $8B             ; Timer 3 Reload Register High

TMR4_CTL:       EQU $8C             ; Timer 4 Control Register
TMR4_DR_L:      EQU $8D             ; Timer 4 Data Register Low
TMR4_RR_L:      EQU $8D             ; Timer 4 Reload Register Low
TMR4_DR_H:      EQU $8E             ; Timer 4 Data Register High
TMR4_RR_H:      EQU $8E             ; Timer 4 Reload Register High

TMR5_CTL:       EQU $8F             ; Timer 5 Control Register
TMR5_DR_L:      EQU $90             ; Timer 5 Data Register Low
TMR5_RR_L:      EQU $90             ; Timer 5 Reload Register Low
TMR5_DR_H:      EQU $91             ; Timer 5 Data Register High
TMR5_RR_H:      EQU $91             ; Timer 5 Reload Register High

TMR_ISS:        EQU $92             ; Timer Input Source Select Register

; ------------------------------------------------------------------------------
; Watchdog Timer (WDT)
; ------------------------------------------------------------------------------
WDT_CTL:        EQU $93             ; Watchdog Timer Control Register
WDT_RR:         EQU $94             ; Watchdog Timer Reset Register

; ------------------------------------------------------------------------------
; General-Purpose Input/Output (GPIO) Ports
; (Note: Port A is not bonded out on the eZ80F92 100-pin LQFP package)
; ------------------------------------------------------------------------------
PB_DR:          EQU $9A             ; Port B Data Register
PB_DDR:         EQU $9B             ; Port B Data Direction Register
PB_ALT1:        EQU $9C             ; Port B Alternate Register 1
PB_ALT2:        EQU $9D             ; Port B Alternate Register 2

PC_DR:          EQU $9E             ; Port C Data Register
PC_DDR:         EQU $9F             ; Port C Data Direction Register
PC_ALT1:        EQU $A0             ; Port C Alternate Register 1
PC_ALT2:        EQU $A1             ; Port C Alternate Register 2

PD_DR:          EQU $A2             ; Port D Data Register
PD_DDR:         EQU $A3             ; Port D Data Direction Register
PD_ALT1:        EQU $A4             ; Port D Alternate Register 1
PD_ALT2:        EQU $A5             ; Port D Alternate Register 2

; ------------------------------------------------------------------------------
; Chip Select / Memory Bus Control Registers
; ------------------------------------------------------------------------------
CS0_LBR:        EQU $A8             ; CS0 Low Boundary Register
CS0_UBR:        EQU $A9             ; CS0 High Boundary Register
CS0_CTL:        EQU $AA             ; CS0 Control Register
CS0_BMC:        EQU $F0             ; CS0 Bus Mode Control Register

CS1_LBR:        EQU $AB             ; CS1 Low Boundary Register
CS1_UBR:        EQU $AC             ; CS1 High Boundary Register
CS1_CTL:        EQU $AD             ; CS1 Control Register
CS1_BMC:        EQU $F1             ; CS1 Bus Mode Control Register

CS2_LBR:        EQU $AE             ; CS2 Low Boundary Register
CS2_UBR:        EQU $AF             ; CS2 High Boundary Register
CS2_CTL:        EQU $B0             ; CS2 Control Register
CS2_BMC:        EQU $F2             ; CS2 Bus Mode Control Register

CS3_LBR:        EQU $B1             ; CS3 Low Boundary Register
CS3_UBR:        EQU $B2             ; CS3 High Boundary Register
CS3_CTL:        EQU $B3             ; CS3 Control Register
CS3_BMC:        EQU $F3             ; CS3 Bus Mode Control Register

; ------------------------------------------------------------------------------
; On-Chip SRAM Control Registers
; ------------------------------------------------------------------------------
RAM_CTL:        EQU $B4             ; On-Chip SRAM Control Register
RAM_ADDR_U:     EQU $B5             ; On-Chip SRAM Upper Address Register

; ------------------------------------------------------------------------------
; Serial Peripheral Interface (SPI)
; ------------------------------------------------------------------------------
SPI_BRG_L:      EQU $B8             ; SPI Baud Rate Generator Register Low
SPI_BRG_H:      EQU $B9             ; SPI Baud Rate Generator Register High
SPI_CTL:        EQU $BA             ; SPI Control Register
SPI_SR:         EQU $BB             ; SPI Status Register
SPI_RBR:        EQU $BC             ; SPI Receive Buffer Register
SPI_TSR:        EQU $BC             ; SPI Transmit Shift Register

; ------------------------------------------------------------------------------
; Universal Asynchronous Receiver/Transmitter 0 (UART0)
; Connected to ESP32 / VDP on Agon Light
; ------------------------------------------------------------------------------
UART0_RBR:      EQU $C0             ; UART0 Receive Buffer Register (DLAB=0)
UART0_THR:      EQU $C0             ; UART0 Transmitter Holding Register (DLAB=0)
UART0_BRG_L:    EQU $C0             ; UART0 Baud Rate Generator Low (DLAB=1)
UART0_IER:      EQU $C1             ; UART0 Interrupt Enable Register (DLAB=0)
UART0_BRG_H:    EQU $C1             ; UART0 Baud Rate Generator High (DLAB=1)
UART0_IIR:      EQU $C2             ; UART0 Interrupt Identification Register
UART0_FCR:      EQU $C2             ; UART0 FIFO Control Register
UART0_LCR:      EQU $C3             ; UART0 Line Control Register
UART0_MCR:      EQU $C4             ; UART0 Modem Control Register
UART0_LSR:      EQU $C5             ; UART0 Line Status Register
UART0_MSR:      EQU $C6             ; UART0 Modem Status Register
UART0_SPR:      EQU $C7             ; UART0 Scratchpad Register

; ------------------------------------------------------------------------------
; Universal Asynchronous Receiver/Transmitter 1 (UART1)
; Used for MIDI or Secondary Serial / TRS-NET on Agon Light
; ------------------------------------------------------------------------------
UART1_RBR:      EQU $D0             ; UART1 Receive Buffer Register (DLAB=0)
UART1_THR:      EQU $D0             ; UART1 Transmitter Holding Register (DLAB=0)
UART1_BRG_L:    EQU $D0             ; UART1 Baud Rate Generator Low (DLAB=1)
UART1_IER:      EQU $D1             ; UART1 Interrupt Enable Register (DLAB=0)
UART1_BRG_H:    EQU $D1             ; UART1 Baud Rate Generator High (DLAB=1)
UART1_IIR:      EQU $D2             ; UART1 Interrupt Identification Register
UART1_FCR:      EQU $D2             ; UART1 FIFO Control Register
UART1_LCR:      EQU $D3             ; UART1 Line Control Register
UART1_MCR:      EQU $D4             ; UART1 Modem Control Register
UART1_LSR:      EQU $D5             ; UART1 Line Status Register
UART1_MSR:      EQU $D6             ; UART1 Modem Status Register
UART1_SPR:      EQU $D7             ; UART1 Scratchpad Register

; ------------------------------------------------------------------------------
; Inter-Integrated Circuit (I2C)
; ------------------------------------------------------------------------------
I2C_SAR:        EQU $C8             ; I2C Slave Address Register
I2C_XSAR:       EQU $C9             ; I2C Extended Slave Address Register
I2C_DR:         EQU $CA             ; I2C Data Register
I2C_CTL:        EQU $CB             ; I2C Control Register
I2C_SR:         EQU $CC             ; I2C Status Register
I2C_CCR:        EQU $CD             ; I2C Clock Control Register
I2C_SRR:        EQU $CE             ; I2C Software Reset Register

; ------------------------------------------------------------------------------
; Real-Time Clock (RTC)
; ------------------------------------------------------------------------------
RTC_SEC:        EQU $E0             ; RTC Seconds
RTC_MIN:        EQU $E1             ; RTC Minutes
RTC_HRS:        EQU $E2             ; RTC Hours
RTC_DOW:        EQU $E3             ; RTC Day of Week
RTC_DOM:        EQU $E4             ; RTC Day of Month
RTC_MON:        EQU $E5             ; RTC Month
RTC_YR:         EQU $E6             ; RTC Year
RTC_CEN:        EQU $E7             ; RTC Century
RTC_CTRL:       EQU $ED             ; RTC Control Register

; ------------------------------------------------------------------------------
; On-Chip Flash Memory Control Registers
; ------------------------------------------------------------------------------
FLASH_KEY:      EQU $F5             ; Flash Key Register
FLASH_DATA:     EQU $F6             ; Flash Data Register
FLASH_ADDR_U:   EQU $F7             ; Flash Upper Address Register
FLASH_CTRL:     EQU $F8             ; Flash Control Register
FLASH_FDIV:     EQU $F9             ; Flash Frequency Divider Register
FLASH_PROT:     EQU $FA             ; Flash Write Protect Register
FLASH_IRQ:      EQU $FB             ; Flash Interrupt Control Register
