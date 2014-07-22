/*----------------------------------------------------------------------------
 *      RL-ARM - FlashFS
 *----------------------------------------------------------------------------
 *      Name:    FS_FLASHPRG.C 
 *      Purpose: Flash Programming Functions - SST39x320x (16-bit Bus)
 *      Rev.:    V4.20
 *----------------------------------------------------------------------------
 *      This code is part of the RealView Run-Time Library.
 *      Copyright (c) 2004-2011 KEIL - An ARM Company. All rights reserved.
 *---------------------------------------------------------------------------*/

#include <File_Config.h>

/*----------------------------------------------------------------------------
  Embedded Flash Driver instance definition
   fl0_drv: First  Embedded Flash drive [F0:]
   fl1_drv: Second Embedded Flash drive [F1:]
 *----------------------------------------------------------------------------*/

#define __DRV_ID  fl0_drv

/* Embedded Flash Driver Interface functions */
static BOOL Init        (U32 adr, U32 clk);
static BOOL UnInit      (void);
static BOOL ProgramPage (U32 adr, U32 sz, U8 *buf);
static BOOL EraseSector (U32 adr);
static BOOL EraseChip   (void);         /* Optional function if supported    */ 

/* Embedded Flash Device Driver Control Block */
EFS_DRV __DRV_ID = {
  Init,
  UnInit,
  NULL,                                 /* =NULL, use FFS internal ReadData  */
  ProgramPage,
  EraseSector,
  EraseChip
};


#define M8(adr)  (*((volatile U8  *) (adr)))
#define M16(adr) (*((volatile U16 *) (adr)))
#define M32(adr) (*((volatile U32 *) (adr)))

/* Embedded Flash programming/erase commands   */
#define RESET       0x00F0              /* reset command                     */
#define ERASE       0x0080              /* erase command                     */
#define ERA_CHIP    0x0010              /* chip erase command                */
#define ERA_SECT    0x0030              /* sector erase command (4kB)        */
#define ERA_BLOCK   0x0050              /* block erase command  (64kB)       */
#define PROGRAM     0x00A0              /* write command                     */

#define DQ7         0x0080              /* Flash Status Register bits        */
#define DQ6         0x0040
#define DQ5         0x0020
#define DQ3         0x0008

/* Local Variables */
static U32 base_adr;

/* Local Function Prototypes */
static BOOL Q6Polling (U32 adr);


/*--------------------------- Init ------------------------------------------*/

static BOOL Init (U32 adr, U32 clk)  {
  /* Initialize flash programming functions. */
  base_adr = adr;
  return (__TRUE);
}


/*--------------------------- UnInit ----------------------------------------*/

static BOOL UnInit (void)  {
  /* Uninitialize flash programming functions. */
  base_adr = 0;
  return (__TRUE);
}


/*--------------------------- ProgramPage -----------------------------------*/

static BOOL ProgramPage (U32 adr, U32 sz, U8 *buf) {
 /* Program Page in Flash Memory. */ 

  for (  ; sz; sz -= 2, adr += 2, buf += 2)  {
    M16(base_adr | (0x5555 << 1)) = 0xAA;
    M16(base_adr | (0x2AAA << 1)) = 0x55;
    M16(base_adr | (0x5555 << 1)) = PROGRAM;
    /* 'buf' might be unaligned. */
    M16(adr) = *(__packed U16 *)buf;

    /* Wait until Programming completed */
    if (Q6Polling (adr) == __FALSE) {
      return (__FALSE);
    }
  }
  return (__TRUE);
}


/*--------------------------- EraseSector -----------------------------------*/

static BOOL EraseSector (U32 adr) {
  /*  Erase Sector in Flash Memory. */
  U32 fsreg;

  M16(base_adr | (0x5555 << 1)) = 0xAA;
  M16(base_adr | (0x2AAA << 1)) = 0x55;
  M16(base_adr | (0x5555 << 1)) = ERASE;
  M16(base_adr | (0x5555 << 1)) = 0xAA;
  M16(base_adr | (0x2AAA << 1)) = 0x55;
  M16(adr) = ERA_BLOCK;

  /* Wait for Block Erase Timeout. */
  do {
    fsreg = M16(adr);
  } while ((fsreg & DQ3) < DQ3);
  /* Wait until Erase Completed */
  return (Q6Polling (adr));
}


/*--------------------------- EraseChip -------------------------------------*/

static BOOL EraseChip (void) {
 /* Global Erase complete Flash Memory. */

  M16(base_adr | (0x5555 << 1)) = 0xAA;
  M16(base_adr | (0x2AAA << 1)) = 0x55;
  M16(base_adr | (0x5555 << 1)) = ERASE;
  M16(base_adr | (0x5555 << 1)) = 0xAA;
  M16(base_adr | (0x2AAA << 1)) = 0x55;
  M16(base_adr | (0x5555 << 1)) = ERA_CHIP;

  /* Wait until Erase Completed */
  return (Q6Polling (base_adr));
}


/*--------------------------- Q6Polling -------------------------------------*/

static BOOL Q6Polling (U32 adr) {
 /* Check if Program/Erase completed. */
  U32 dq6,dq6o;

  do {
    dq6o = M16(adr) & DQ6;
    dq6  = M16(adr) & DQ6;
  } while (dq6 != dq6o);
  return (__TRUE);
}

/*----------------------------------------------------------------------------
 * end of file
 *---------------------------------------------------------------------------*/
