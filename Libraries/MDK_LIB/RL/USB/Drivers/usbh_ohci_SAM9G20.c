/*------------------------------------------------------------------------------
 *      RL-ARM - USB
 *------------------------------------------------------------------------------
 *      Name:    usbh_ohci_SAM9G20.c
 *      Purpose: OHCI Hardware Specific Layer Driver for the 
 *               Atmel AT91SAM9G20 Device Series
 *      Rev.:    V4.70
 *------------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2013 KEIL - An ARM Company. All rights reserved.
 *----------------------------------------------------------------------------*/

#include <RTL.h>
#include <rl_usb.h>
#include <AT91SAM9G20.H>


/************************* OHCI Hardware Driver Configuration *****************/

// *** <<< Use Configuration Wizard in Context Menu >>> ***

// <h> USB Host OHCI Settings
//   <o0> USB Host OHCI Controller Base Address
//   <h> Root Hub ports used by OHCI Controller
//     <i> These are the ports that OHCI will use.
//     <o1.0>  Port 1
//     <o1.1>  Port 2
//     <o1.2>  Port 3
//     <o1.3>  Port 4
//     <o1.4>  Port 5
//     <o1.5>  Port 6
//     <o1.6>  Port 7
//     <o1.7>  Port 8
//     <o1.8>  Port 9
//     <o1.9>  Port 10
//     <o1.10> Port 11
//     <o1.11> Port 12
//     <o1.12> Port 13
//     <o1.13> Port 14
//     <o1.14> Port 15
//     </h>
//
//   <o2> Start address of memory used by OHCI
//     <i> This is a start address of memory that OHCI will use for descriptors 
//     <i> and USB communication data.
//
//   <o3> Size of memory used for USB data by OHCI <1-1048576>
//     <i> This is a size of memory (in bytes) that OHCI will use for  
//     <i> USB communication data (maximum sum of data at a single point in 
//     <i> time, for example for HID = size of max Configuration Descriptor + 
//     <i> size of max HID Report Descriptor).
//
//   <o4> Maximum number of Endpoint Descriptors used by OHCI <1-64>
//     <i> This is a maximum number of Endpoints that OHCI will use.
//
//   <o5> Maximum number of Transfer Descriptors used by OHCI <1-64>
//     <i> This is a maximum number of Transfers that OHCI will use.
//
//   <o6> Maximum number of Isochronous Transfer Descriptors used by OHCI <0-64>
//     <i> This is a maximum number of Isochronous Transfers that OHCI will use.
// </h>
#define USBH_OHCI_ADR               0x00500000
#define USBH_OHCI_PORTS             0x00000003
#define USBH_OHCI_MEM_ADR           0x00301000
#define USBH_OHCI_MEM_DATA_SZ       4096
#define USBH_OHCI_NUM_ED            5
#define USBH_OHCI_NUM_TD            2
#define USBH_OHCI_NUM_ITD           0

// *** <<< End of Configuration section             >>> ***


/************************** OHCI Host Controller Hardware Driver Variables ****/

#define USBH_OHCI_MEM_SZ           (256                   +  /* HCCA sz    */  \
                                    USBH_OHCI_NUM_ED * 16 +  /* EDs sz     */  \
                                    USBH_OHCI_NUM_TD * 16 +  /* TDs sz     */  \
                                    USBH_OHCI_MEM_DATA_SZ +  /* DATA sz    */  \
                                    USBH_OHCI_NUM_TD * 8  +  /* alloc ovhd */  \
                                    4                     )  /* alloc ovhd */

#define USBH_OHCI_MEM_HCCA         (USBH_OHCI_MEM_ADR)
#define USBH_OHCI_MEM_ED           (USBH_OHCI_MEM_ADR+256)
#define USBH_OHCI_MEM_TD           (USBH_OHCI_MEM_ED+(USBH_OHCI_NUM_ED<<4))
#define USBH_OHCI_MEM_ITD          (USBH_OHCI_MEM_TD+(USBH_OHCI_NUM_TD<<4))
#define USBH_OHCI_MEM_MPOOL        (USBH_OHCI_MEM_ITD+(USBH_OHCI_NUM_ITD<<5))
#define USBH_OHCI_MEM_SZ_MPOOL     (USBH_OHCI_MEM_SZ-(USBH_OHCI_MEM_MPOOL-USBH_OHCI_MEM_ADR))
#define USBH_OHCI_NUM_TDURB        (USBH_OHCI_NUM_TD+USBH_OHCI_NUM_ITD)

U32     usbh_ohci_hcca  [64]                        __attribute__((at(USBH_OHCI_MEM_ADR)));
U32     usbh_ohci_ed    [USBH_OHCI_NUM_ED<<2]       __attribute__((at(USBH_OHCI_MEM_ED)));
U32     usbh_ohci_td    [USBH_OHCI_NUM_TD<<2]       __attribute__((at(USBH_OHCI_MEM_TD)));
#if    (USBH_OHCI_NUM_ITD > 0)
U32     usbh_ohci_itd   [USBH_OHCI_NUM_ITD<<3]      __attribute__((at(USBH_OHCI_MEM_ITD)));
#endif
U32     usbh_ohci_mpool [USBH_OHCI_MEM_SZ_MPOOL>>2] __attribute__((at(USBH_OHCI_MEM_MPOOL)));
U32     usbh_ohci_tdurb [USBH_OHCI_NUM_TDURB<<1];


/************************** OHCI Host Controller Hardware Function Prototypes */

void usbh_ohci_hw_get_capabilities (USBH_HCI_CAP *cap);
void usbh_ohci_hw_delay_ms         (U32 ms);
void usbh_ohci_hw_reg_wr           (U32 reg_ofs, U32 val);
U32  usbh_ohci_hw_reg_rd           (U32 reg_ofs);
BOOL usbh_ohci_hw_pins_config      (BOOL on);
BOOL usbh_ohci_hw_init             (BOOL on);
BOOL usbh_ohci_hw_port_power       (BOOL on);
BOOL usbh_ohci_hw_irq_en           (BOOL on);

__irq void USB_IRQHandler          (void);


/************************** OHCI Host Controller Hardware Driver Structure ****/

USBH_HWD_OHCI usbh0_hwd_ohci = {        /* OHCI Host Controller Hardware Drv  */
  USBH_OHCI_PORTS,                      /* Ports (bits 0..15)                 */
  USBH_OHCI_NUM_ED,                     /* Maximum Endpoint Descriptors       */
  USBH_OHCI_NUM_TD,                     /* Maximum Transfer Descriptors       */
  USBH_OHCI_NUM_ITD,                    /* Maximum Iso Transfer Descriptors   */
  (U32 *) &usbh_ohci_hcca,              /* Pointer to HCCA memory start       */
  (U32 *) &usbh_ohci_ed,                /* Pointer to ED memory start         */
  (U32 *) &usbh_ohci_td,                /* Pointer to TD memory start         */
#if    (USBH_OHCI_NUM_ITD > 0)
  (U32 *) &usbh_ohci_itd,               /* Pointer to ITD memory start        */
#else
  NULL,
#endif
  (U32 *) &usbh_ohci_tdurb,             /* Pointer to TDURB memory start      */
  usbh_ohci_hw_get_capabilities,        /* Get driver capabilities            */
  usbh_ohci_hw_delay_ms,                /* Delay in ms                        */
  usbh_ohci_hw_reg_wr,                  /* Write register                     */
  usbh_ohci_hw_reg_rd,                  /* Read register                      */
  usbh_ohci_hw_pins_config,             /* Config/Unconfig pins               */
  usbh_ohci_hw_init,                    /* Init/Uninit Host Controller        */
  usbh_ohci_hw_port_power,              /* On/Off Port Power                  */
  usbh_ohci_hw_irq_en                   /* Enable/Disable interrupt           */
};


/************************** Imported Functions ********************************/

extern void USBH_OHCI_IRQHandler (void);


/************************** Module Functions **********************************/

/*------------------------- usbh_ohci_hw_get_capabilities ----------------------
 *
 *  Get capabilities of Host Controller Driver
 *
 *  Parameter:  cap:        Pointer to USBH_HCI_CAP structure where 
 *                          capabilities are loaded
 *  Return:
 *----------------------------------------------------------------------------*/

void usbh_ohci_hw_get_capabilities (USBH_HCI_CAP *cap) {
  cap->MultiPckt = __TRUE;
  cap->MaxDataSz = USBH_OHCI_MEM_DATA_SZ;
  cap->CtrlNAKs  = 1000;
  cap->BulkNAKs  = 1000000;
}


/*------------------------- usbh_ohci_hw_delay_ms ------------------------------
 *
 *  Delay execution (in milliseconds on 400 MHz CPU clock)
 *
 *  Parameter:  ms:         Delay in ms
 *  Return:
 *----------------------------------------------------------------------------*/

void usbh_ohci_hw_delay_ms (U32 ms) {

  ms <<= 15;
  while (ms--) {
    __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop(); __nop();
  }
}


/*------------------------- usbh_ohci_hw_reg_wr --------------------------------
 *
 *  Write hardware register.
 *
 *  Parameter:  reg:        Register offset
 *              val:        Register value 
 *  Return:
 *----------------------------------------------------------------------------*/

void usbh_ohci_hw_reg_wr (U32 reg_ofs, U32 val) {
  *((U32 *)(USBH_OHCI_ADR + reg_ofs)) = val;
}


/*------------------------- usbh_ohci_hw_reg_rd --------------------------------
 *
 *  Read hardware register.
 *
 *  Parameter:  reg:        Register offset
 *  Return:                 Register value 
 *----------------------------------------------------------------------------*/

U32 usbh_ohci_hw_reg_rd (U32 reg_ofs) {
  return (*((U32 *)(USBH_OHCI_ADR + reg_ofs)));
}


/*------------------------- usbh_ohci_hw_pins_cfg ------------------------------
 *
 *  Configurate or unconfigurate pins used by the USB Host.
 *
 *  Parameter:  on:         __TRUE = configurate, __FALSE = unconfigurate
 *  Return:                 __TRUE = Ok, __FALSE = error
 *----------------------------------------------------------------------------*/

BOOL usbh_ohci_hw_pins_config (BOOL on) {

  return (__TRUE);
}


/*------------------------- usbh_ohci_hw_init ----------------------------------
 *
 *  Initialize or uninitialize the USB Host Controller.
 *
 *  Parameter:  on:         __TRUE = initialize, __FALSE = uninitialize
 *  Return:                 __TRUE = Ok, __FALSE = error
 *----------------------------------------------------------------------------*/

BOOL usbh_ohci_hw_init (BOOL on) {
  S32 tout;

  if (on) {
    /* Initialize memory pool for data                                        */
    if (!usbh_mem_init(0, (U32 *)&usbh_ohci_mpool, sizeof(usbh_ohci_mpool)))
      return (__FALSE);

    /* Configure and enable USB Host clocks                                   */
    AT91C_BASE_PMC->PMC_PCER |= (1 << 20);        /* Enable UHP peri clock    */
    for (tout = 1100; tout >= 0; tout--) {        /* Wait max 1 second        */
      if (AT91C_BASE_PMC->PMC_PCSR & (1 << 20))   /* Wait UHP peri clock en   */
        break;
      if (!tout) 
        return (__FALSE);
      if (tout <= 100)
        usbh_ohci_hw_delay_ms (10);
    }

    /* PLLB  = 18.432 MHz * 26 / 5 = 95.8464 MHz
       UHPCK = PLLB / 2            = 47.9232 MHz
       this is 0.16% error in regards to 48MHz and acceptible error is 0.25%  */
    AT91C_BASE_CKGR->CKGR_PLLBR = 0x10190F05;
    for (tout = 1100; tout >= 0; tout--) {        /* Wait max 1 second        */
      if (AT91C_BASE_PMC->PMC_SR & (1 << 2))      /* Wait for PLLB lock       */
        break;
      if (!tout) 
        return (__FALSE);
      if (tout <= 100)
        usbh_ohci_hw_delay_ms (10);
    }

    AT91C_BASE_PMC->PMC_SCER |= (1 << 6);         /* Enable UHP clock         */
    for (tout = 1100; tout >= 0; tout--) {        /* Wait max 1 second        */
      if (AT91C_BASE_PMC->PMC_SCSR & (1 << 6))    /* Wait for UHP clock en    */
        break;
      if (!tout) 
        return (__FALSE);
      if (tout <= 100)
        usbh_ohci_hw_delay_ms (10);
    }

    AT91C_BASE_PMC->PMC_PCER |= (1 << 0);         /* Enable AIC peri clock    */
    AT91C_BASE_AIC->AIC_SVR[AT91C_ID_UHP] = (U32)USB_IRQHandler;
    AT91C_BASE_AIC->AIC_SMR[AT91C_ID_UHP] = AT91C_AIC_SRCTYPE_INT_LEVEL_SENSITIVE | 7;
  } else {
  }

  return (__TRUE);
}


/*------------------------- usbh_ohci_hw_port_power ----------------------------
 *
 *  Turn Port Power on or off with pin if not handled through OHCI.
 *
 *  Parameter:  on:         __TRUE = turn power on, __FALSE = turn power off
 *  Return:                 __TRUE = Ok, __FALSE = error
 *----------------------------------------------------------------------------*/

BOOL usbh_ohci_hw_port_power (BOOL on) {

  return (__TRUE);
}


/*------------------------- usbh_ohci_hw_irq_en --------------------------------
 *
 *  USB Host OHCI Controller interrupt enable or disable.
 *
 *  Parameter:  on:         __TRUE = enable, __FALSE = disable
 *  Return:                 __TRUE = Ok, __FALSE = error
 *----------------------------------------------------------------------------*/

BOOL usbh_ohci_hw_irq_en (BOOL on) {

  if (on) {
    AT91C_BASE_AIC->AIC_IECR |= (1 << AT91C_ID_UHP);
  } else {
    AT91C_BASE_AIC->AIC_IDCR |= (1 << AT91C_ID_UHP);
  }

  return (__TRUE);
}


/*------------------------- USB_IRQHandler -------------------------------------
 *
 *  Hardware USB Interrupt Handler Routine.
 *
 *  Parameter:
 *  Return:
 *----------------------------------------------------------------------------*/

__irq void USB_IRQHandler (void) {
  USBH_OHCI_IRQHandler();
  *AT91C_AIC_EOICR = 0;                 /* Acknowledge the UHP interrupt      */
}
