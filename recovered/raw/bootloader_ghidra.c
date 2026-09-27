/* Raw Ghidra decompilation. This is evidence, not hand-cleaned source. */
/* Program: bootloader.bin */


/* ===== __program_start @ 00000250, 8 bytes ===== */

void __program_start(void)

{
  __iar_data_init3();
  (*DAT_0000025c)();
  return;
}



/* ===== main @ 00000258, 4 bytes ===== */

void main(void)

{
  (*DAT_0000025c)();
  return;
}



/* ===== FUN_00000272 @ 00000272, 6 bytes ===== */

void FUN_00000272(undefined4 param_1)

{
  bool bVar1;
  
  bVar1 = (bool)isCurrentModePrivileged();
  if (bVar1) {
    setMainStackPointer(param_1);
  }
  return;
}



/* ===== Reset_Handler @ 00000278, 8 bytes ===== */

void Reset_Handler(void)

{
  (*DAT_00000294)();
  (*DAT_00000298)();
  return;
}



/* ===== NMI_Handler @ 00000280, 2 bytes ===== */

void NMI_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== HardFault_Handler @ 00000282, 2 bytes ===== */

void HardFault_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== MemManage_Handler @ 00000284, 2 bytes ===== */

void MemManage_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== BusFault_Handler @ 00000286, 2 bytes ===== */

void BusFault_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== UsageFault_Handler @ 00000288, 2 bytes ===== */

void UsageFault_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== SVC_Handler @ 0000028a, 2 bytes ===== */

void SVC_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== DebugMon_Handler @ 0000028c, 2 bytes ===== */

void DebugMon_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== PendSV_Handler @ 0000028e, 2 bytes ===== */

void PendSV_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== SysTick_Handler @ 00000290, 2 bytes ===== */

void SysTick_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== Default_Handler @ 00000292, 2 bytes ===== */

void Default_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== FUN_0000029c @ 0000029c, 98 bytes ===== */

longlong FUN_0000029c(uint param_1,int param_2,uint param_3,uint param_4)

{
  longlong lVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  bool bVar5;
  undefined8 uVar6;
  longlong lVar7;
  
  lVar1 = 0;
  iVar3 = 0x40;
  while (iVar4 = iVar3 + -1, 0 < iVar3) {
    uVar6 = FUN_000003aa(param_1,param_2,iVar4);
    uVar2 = (uint)((ulonglong)uVar6 >> 0x20);
    iVar3 = iVar4;
    if (param_4 < uVar2 || uVar2 - param_4 < (uint)(param_3 <= (uint)uVar6)) {
      uVar6 = FUN_000002fe(param_3,param_4,iVar4);
      bVar5 = param_1 < (uint)uVar6;
      param_1 = param_1 - (uint)uVar6;
      param_2 = (param_2 - (int)((ulonglong)uVar6 >> 0x20)) - (uint)bVar5;
      lVar7 = FUN_000002fe(1,0,iVar4);
      lVar1 = lVar7 + lVar1;
    }
  }
  return lVar1;
}



/* ===== FUN_000002fe @ 000002fe, 30 bytes ===== */

longlong FUN_000002fe(uint param_1,int param_2,uint param_3)

{
  if (0x1f < (int)param_3) {
    return (ulonglong)(param_1 << (param_3 - 0x20 & 0xff)) << 0x20;
  }
  return CONCAT44(param_2 << (param_3 & 0xff) | param_1 >> (0x20 - param_3 & 0xff),
                  param_1 << (param_3 & 0xff));
}



/* ===== FUN_0000031c @ 0000031c, 36 bytes ===== */

void FUN_0000031c(undefined4 *param_1,undefined4 *param_2,uint param_3)

{
  undefined4 uVar1;
  bool bVar2;
  
  if ((((uint)param_1 | (uint)param_2) & 3) == 0) {
    for (; 3 < param_3; param_3 = param_3 - 4) {
      uVar1 = *param_2;
      param_2 = param_2 + 1;
      *param_1 = uVar1;
      param_1 = param_1 + 1;
    }
  }
  while (bVar2 = param_3 != 0, param_3 = param_3 - 1, bVar2) {
    *(undefined1 *)param_1 = *(undefined1 *)param_2;
    param_1 = (undefined4 *)((int)param_1 + 1);
    param_2 = (undefined4 *)((int)param_2 + 1);
  }
  return;
}



/* ===== FUN_00000340 @ 00000340, 26 bytes ===== */

void FUN_00000340(undefined4 param_1)

{
  FUN_000005da(param_1,0,0,0,0,0,0x433);
  return;
}



/* ===== FUN_0000035a @ 0000035a, 24 bytes ===== */

void FUN_0000035a(void)

{
  FUN_000005da();
  return;
}



/* ===== FUN_00000372 @ 00000372, 74 bytes ===== */

uint FUN_00000372(uint param_1,uint param_2)

{
  uint uVar1;
  int iVar2;
  
  if (param_1 != 0 || (param_2 & 0x7fffffff) != 0) {
    iVar2 = ((param_2 & 0x7fffffff) >> 0x14) - 0x380;
    if (0 < iVar2) {
      uVar1 = (param_1 >> 0x1d | (param_2 & 0xfffff) << 3) + (param_2 & 0x80000000) +
              iVar2 * 0x800000;
      if (-1 < (int)(param_1 * 8)) {
        return uVar1;
      }
      uVar1 = uVar1 + 1;
      if ((param_1 & 0xfffffff) == 0) {
        uVar1 = uVar1 & 0xfffffffe;
      }
      return uVar1;
    }
  }
  return 0;
}



/* ===== FUN_000003aa @ 000003aa, 32 bytes ===== */

ulonglong FUN_000003aa(uint param_1,uint param_2,uint param_3)

{
  if (0x1f < (int)param_3) {
    return (ulonglong)(param_2 >> (param_3 - 0x20 & 0xff));
  }
  return CONCAT44(param_2 >> (param_3 & 0xff),
                  param_1 >> (param_3 & 0xff) | param_2 << (0x20 - param_3 & 0xff));
}



/* ===== __iar_data_init3 @ 000003cc, 36 bytes ===== */

void __iar_data_init3(void)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 *puVar3;
  int unaff_r7;
  
  puVar1 = puRam000003ec;
  for (puVar3 = puRam000003e8; puVar3 < puVar1; puVar3 = puVar3 + 4) {
    (*(code *)puVar3[3])(*puVar3,puVar3[1],puVar3[2]);
  }
  uVar2 = main();
  *(char *)(unaff_r7 + 0xe) = (char)uVar2;
  *(char *)((int)puVar1 + 0xf) = (char)uVar2;
  flash_erase_and_program(uVar2);
  return;
}



/* ===== thunk_EXT_FUN_1fff8000 @ 000003f0, 10 bytes ===== */

void thunk_EXT_FUN_1fff8000(void)

{
  flash_erase_and_program();
  return;
}



/* ===== thunk_FUN_000003fc @ 000003fa, 2 bytes ===== */

void thunk_FUN_000003fc(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== FUN_000003fc @ 000003fc, 2 bytes ===== */

void FUN_000003fc(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== FUN_000003fe @ 000003fe, 2 bytes ===== */

void FUN_000003fe(void)

{
  return;
}



/* ===== FUN_00000460 @ 00000460, 16 bytes ===== */

void FUN_00000460(undefined4 param_1)

{
  FUN_0000467c(param_1);
  FUN_00004c58();
  return;
}



/* ===== FUN_00000470 @ 00000470, 252 bytes ===== */

undefined8 FUN_00000470(uint param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  uint uVar9;
  bool bVar10;
  
  if ((param_1 != 0 || (param_2 & 0x7fffffff) != 0) && (param_3 != 0 || (param_4 & 0x7fffffff) != 0)
     ) {
    uVar5 = param_4 & 0xfffff | 0x100000;
    uVar4 = param_2 & 0xfffff | 0x100000;
    iVar7 = ((param_2 & 0x7fffffff) >> 0x14) - ((param_4 & 0x7fffffff) >> 0x14);
    iVar8 = iVar7 + 0x3fd;
    if (uVar5 < uVar4 || uVar4 - uVar5 < (uint)(param_3 <= param_1)) {
      iVar8 = iVar7 + 0x3fe;
    }
    else {
      bVar10 = CARRY4(param_1,param_1);
      param_1 = param_1 * 2;
      uVar4 = uVar4 * 2 + (uint)bVar10;
    }
    if (-1 < iVar8) {
      uVar3 = 0x100000;
      uVar6 = 0;
      uVar9 = 0;
      for (uVar2 = 0; uVar2 != 0 || uVar3 != 0; uVar2 = (uint)(uVar1 != 0) << 0x1f | uVar2 >> 1) {
        if (uVar5 < uVar4 || uVar4 - uVar5 < (uint)(param_3 <= param_1)) {
          bVar10 = param_1 < param_3;
          param_1 = param_1 - param_3;
          uVar4 = (uVar4 - uVar5) - (uint)bVar10;
          uVar6 = uVar6 | uVar2;
          uVar9 = uVar9 | uVar3;
        }
        uVar1 = uVar3 & 1;
        uVar3 = uVar3 >> 1;
        bVar10 = CARRY4(param_1,param_1);
        param_1 = param_1 * 2;
        uVar4 = uVar4 * 2 + (uint)bVar10;
      }
      if (param_1 != 0 || uVar4 != 0) {
        if (param_1 == param_3 && uVar4 == uVar5) {
          param_1 = 0;
          uVar4 = 0x80000000;
        }
        else if (uVar5 < uVar4 || uVar4 - uVar5 < (uint)(param_3 <= param_1)) {
          param_1 = 0xfffffffe;
          uVar4 = 0xffffffff;
        }
        else {
          param_1 = 1;
          uVar4 = 0;
        }
      }
      iVar7 = uVar9 + iVar8 * 0x100000 + ((param_2 ^ param_4) & 0x80000000);
      if ((int)uVar4 < 0) {
        bVar10 = 0xfffffffe < uVar6;
        uVar6 = uVar6 + 1;
        iVar7 = iVar7 + (uint)bVar10;
        if ((param_1 & 0x7fffffff) == 0 && uVar4 * 2 + (uint)CARRY4(param_1,param_1) == 0) {
          uVar6 = uVar6 & 0xfffffffe;
        }
      }
      return CONCAT44(iVar7,uVar6);
    }
  }
  return 0;
}



/* ===== FUN_00000560 @ 00000560, 92 bytes ===== */

uint FUN_00000560(int param_1,uint param_2,int param_3,int param_4)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  
  iVar3 = LZCOUNT(param_1);
  uVar1 = param_1 << iVar3;
  if (uVar1 == 0 && param_2 == 0) {
    return uVar1;
  }
  if (param_2 != 0) {
    uVar1 = uVar1 | (uint)(param_2 << iVar3 != 0) | param_2 >> (0x20U - iVar3 & 0xff);
  }
  iVar3 = (param_4 - iVar3) + 7;
  if (-1 < iVar3) {
    uVar2 = (uVar1 >> 8) + iVar3 * 0x800000 + param_3;
    if ((int)(uVar1 * 0x1000000) < 0) {
      uVar2 = uVar2 + 1;
      if ((uVar1 & 0x7f) == 0) {
        uVar2 = uVar2 & 0xfffffffe;
      }
      return uVar2;
    }
    return uVar2;
  }
  return 0;
}



/* ===== FUN_000005da @ 000005da, 156 bytes ===== */

ulonglong FUN_000005da(undefined4 param_1,int param_2,int param_3,int param_4,uint param_5,
                      int param_6,int param_7)

{
  uint uVar1;
  int iVar2;
  uint uVar3;
  bool bVar4;
  ulonglong uVar5;
  undefined8 uVar6;
  longlong lVar7;
  
  if (param_2 == 0) {
    iVar2 = LZCOUNT(param_1) + 0x20;
  }
  else {
    iVar2 = LZCOUNT(param_2);
  }
  uVar5 = FUN_000002fe();
  uVar3 = (uint)(uVar5 >> 0x20);
  if (((uint)uVar5 != 0 || param_3 != 0) || (uVar3 != 0 || param_4 != 0)) {
    if (param_3 != 0 || param_4 != 0) {
      uVar6 = FUN_000003aa(param_3,param_4,0x40 - iVar2);
      lVar7 = FUN_000002fe(param_3,param_4,iVar2);
      uVar5 = CONCAT44(uVar3 | (uint)((ulonglong)uVar6 >> 0x20),
                       (uint)uVar5 | (uint)uVar6 | (uint)(lVar7 != 0));
    }
    uVar3 = (uint)(uVar5 >> 0xb);
    iVar2 = (param_7 - iVar2) + 10;
    if (-1 < iVar2) {
      uVar1 = uVar3 + param_5;
      iVar2 = iVar2 * 0x100000 + (int)((uVar5 >> 0xb) >> 0x20) + param_6 +
              (uint)CARRY4(uVar3,param_5);
      if ((int)uVar5 * 0x200000 < 0) {
        bVar4 = 0xfffffffe < uVar1;
        uVar1 = uVar1 + 1;
        iVar2 = iVar2 + (uint)bVar4;
        if ((uVar5 & 0x3ff) == 0) {
          uVar1 = uVar1 & 0xfffffffe;
        }
      }
      return CONCAT44(iVar2,uVar1);
    }
    uVar5 = 0;
  }
  return uVar5;
}



/* ===== __iar_unpack_data @ 00000676, 86 bytes ===== */

undefined4 __iar_unpack_data(byte *param_1,byte *param_2,int param_3)

{
  byte *pbVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  byte *pbVar5;
  uint uVar6;
  
  pbVar5 = param_2 + param_3;
  do {
    uVar6 = (uint)*param_1;
    uVar4 = uVar6 & 7;
    pbVar1 = param_1 + 1;
    if ((*param_1 & 7) == 0) {
      pbVar1 = param_1 + 2;
      uVar4 = (uint)param_1[1];
    }
    uVar2 = (int)uVar6 >> 4;
    if (uVar2 == 0) {
      uVar2 = (uint)*pbVar1;
      pbVar1 = pbVar1 + 1;
    }
    while (uVar4 = uVar4 - 1, uVar4 != 0) {
      *param_2 = *pbVar1;
      pbVar1 = pbVar1 + 1;
      param_2 = param_2 + 1;
    }
    if ((int)(uVar6 << 0x1c) < 0) {
      param_1 = pbVar1 + 1;
      iVar3 = uVar2 + 2;
      pbVar1 = param_2 + -(uint)*pbVar1;
      while (iVar3 = iVar3 + -1, -1 < iVar3) {
        *param_2 = *pbVar1;
        param_2 = param_2 + 1;
        pbVar1 = pbVar1 + 1;
      }
    }
    else {
      while (uVar2 = uVar2 - 1, param_1 = pbVar1, -1 < (int)uVar2) {
        *param_2 = 0;
        param_2 = param_2 + 1;
      }
    }
  } while (param_2 < pbVar5);
  return 0;
}



/* ===== FUN_000006cc @ 000006cc, 118 bytes ===== */

undefined8 FUN_000006cc(int param_1,int param_2,int param_3,uint param_4)

{
  int iVar1;
  uint uVar2;
  int local_30 [4];
  
  local_30[3] = param_4;
  local_30[2] = param_3;
  local_30[1] = param_2;
  local_30[0] = param_1;
  uVar2 = 0;
  iVar1 = 0x10;
  do {
    if (param_4 <= uVar2) {
      return CONCAT44(local_30[1],local_30[0]);
    }
    if (iVar1 == 0x10) {
      FUN_0000031c(local_30,param_1 + 0xf0,0x10);
      FUN_000015e2(local_30,param_1);
      for (iVar1 = 0xf; -1 < iVar1; iVar1 = iVar1 + -1) {
        if (*(char *)(param_1 + 0xf0 + iVar1) != -1) {
          *(char *)(param_1 + 0xf0 + iVar1) = *(char *)(param_1 + 0xf0 + iVar1) + '\x01';
          break;
        }
        *(undefined1 *)(param_1 + 0xf0 + iVar1) = 0;
      }
      iVar1 = 0;
    }
    *(byte *)(param_2 + uVar2) = *(byte *)(param_3 + uVar2) ^ *(byte *)((int)local_30 + iVar1);
    uVar2 = uVar2 + 1;
    iVar1 = iVar1 + 1;
  } while( true );
}



/* ===== FUN_00000742 @ 00000742, 30 bytes ===== */

void FUN_00000742(int param_1,undefined4 param_2,undefined4 param_3)

{
  FUN_00002bf8(param_1,param_2);
  FUN_0000031c(param_1 + 0xf0,param_3,0x10);
  return;
}



/* ===== FUN_00000760 @ 00000760, 56 bytes ===== */

void FUN_00000760(int param_1,int param_2,int param_3)

{
  uint uVar1;
  uint uVar2;
  
  for (uVar1 = 0; uVar1 < 4; uVar1 = uVar1 + 1 & 0xff) {
    for (uVar2 = 0; uVar2 < 4; uVar2 = uVar2 + 1 & 0xff) {
      *(byte *)(param_2 + uVar1 * 4 + uVar2) =
           *(byte *)(param_3 + param_1 * 0x10 + uVar1 * 4 + uVar2) ^
           *(byte *)(param_2 + uVar1 * 4 + uVar2);
    }
  }
  return;
}



/* ===== FUN_00000798 @ 00000798, 608 bytes ===== */

/* WARNING: Removing unreachable block (ram,0x000009c0) */
/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00000798(void)

{
  uint *puVar1;
  uint uVar2;
  undefined1 auStack_e0 [56];
  undefined4 local_a8;
  undefined4 local_a4;
  undefined4 local_94;
  undefined4 local_90;
  undefined4 local_8c;
  undefined4 local_88;
  undefined4 local_84;
  undefined4 local_80;
  int local_7c;
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  int local_6c;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 local_60;
  undefined4 local_5c;
  undefined4 local_58;
  undefined4 local_54;
  undefined4 local_50;
  undefined4 local_4c;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;
  undefined4 local_3c;
  undefined4 local_38;
  undefined4 local_34;
  undefined4 local_30;
  undefined1 *local_28;
  undefined4 local_20;
  
  FUN_0000031c(auStack_e0,DAT_00000a04,0x38);
  FUN_000012ec(1,8);
  FUN_00002670(1,0x80,0x32);
  FUN_00002670(1,0x40,0x33);
  FUN_00004088(&local_a8);
  puVar1 = DAT_00000a08;
  *DAT_00000a08 = _DAT_0003e020;
  if (0x7ff < *puVar1) {
    *puVar1 = 1;
  }
  uVar2 = _DAT_0003e08c;
  if (_DAT_0003e08c == 0xffffffff) {
    uVar2 = 4;
  }
  if (uVar2 < 5) {
    switch(uVar2) {
    case 0:
      local_94 = 8;
      break;
    case 1:
      local_94 = 5;
      break;
    case 2:
      local_94 = 4;
      break;
    case 3:
      local_94 = 2;
      break;
    case 4:
      local_94 = 1;
      break;
    default:
      local_94 = 1;
    }
    local_a8 = 0;
    local_a4 = 0;
    local_90 = 0x3c;
    local_8c = 0x14;
    local_88 = 0x14;
    local_64 = 0;
    local_60 = 2;
    local_5c = 0;
    local_58 = 3;
    local_54 = 7;
    local_50 = 3;
    local_4c = 7;
    local_48 = 3;
    local_44 = 7;
    local_3c = 3;
    local_38 = 0;
    local_34 = 0;
    local_30 = 7;
    local_40 = 4;
    local_20 = 1;
    local_28 = auStack_e0;
    FUN_0000219c(1,0);
    FUN_0000219c(1);
    FUN_00003898(DAT_00000a0c,&local_a8);
  }
  else {
    switch(uVar2) {
    case 5:
      local_6c = 0x1f;
      local_78 = 8;
      local_74 = 8;
      break;
    case 6:
      local_6c = 0x19;
      local_78 = 6;
      local_74 = 6;
      break;
    case 7:
      local_6c = 0x13;
      local_78 = 5;
      local_74 = 5;
      break;
    case 8:
      local_6c = 0xf;
      local_78 = 4;
      local_74 = 4;
      break;
    case 9:
      local_6c = 0xc;
      local_78 = 3;
      local_74 = 3;
      break;
    default:
      local_6c = 0x1f;
      local_78 = 8;
      local_74 = 8;
    }
    local_a8 = 0;
    local_a4 = 0x300;
    local_94 = 1;
    local_90 = 0x3c;
    local_8c = 0x14;
    local_88 = 0x14;
    local_84 = 0;
    local_80 = 1;
    local_7c = local_6c + 1;
    local_70 = 0x800000;
    local_68 = 0;
    local_64 = 0;
    local_60 = 2;
    local_5c = 0;
    local_58 = 3;
    local_54 = 7;
    local_50 = 3;
    local_4c = 7;
    local_48 = 3;
    local_44 = 7;
    local_3c = 3;
    local_38 = 0;
    local_34 = 0;
    local_30 = 7;
    local_40 = 4;
    local_20 = 1;
    local_28 = auStack_e0;
    FUN_0000219c(1,0);
    FUN_0000219c(1);
    FUN_00003898(DAT_00000a0c,&local_a8);
  }
  FUN_000040fc(DAT_00000a0c,1,0x200);
  FUN_00003c60(DAT_00000a0c,1,1);
  FUN_00005e80(0x76);
  *(undefined1 *)(DAT_00000a10 + 0x76) = 0x30;
  DAT_e000e10c = 0x400000;
  FUN_00004034(DAT_00000a0c);
  return;
}



/* ===== FUN_00000a18 @ 00000a18, 94 bytes ===== */

void FUN_00000a18(void)

{
  undefined1 auStack_1c [2];
  undefined2 local_1a;
  undefined2 local_18;
  undefined2 local_a;
  
  FUN_00002868(auStack_1c);
  FUN_000025d0(0x10,0);
  FUN_000025d0(8,0);
  FUN_000025d0(4,0);
  local_1a = 2;
  local_a = 0;
  local_18 = 0;
  FUN_000022f8(2,0x2000,auStack_1c);
  FUN_000022f8(5,4,auStack_1c);
  FUN_00002558(2,0x2000);
  FUN_00002758(5,4);
  return;
}



/* ===== FUN_00000a78 @ 00000a78, 312 bytes ===== */

bool FUN_00000a78(void)

{
  int iVar1;
  undefined2 local_38;
  short local_36;
  undefined4 local_34;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_20;
  undefined4 local_1c;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  
  FUN_00004d40(700);
  FUN_00005afc(&local_30);
  FUN_00002670(0,0x1000,0x21);
  FUN_00002670(0,0x800);
  FUN_00002284(0x100000);
  local_30 = 0;
  local_2c = 0;
  local_20 = 0;
  local_10 = 0;
  local_1c = 0;
  local_18 = 0;
  local_14 = 0;
  local_c = 0x80000000;
  local_8 = 0x100;
  FUN_000058f4(DAT_00000bb0,&local_30,0);
  iVar1 = FUN_000057d4(DAT_00000bb0,0xe1000,0);
  if (iVar1 == 0) {
    local_36 = 1;
    local_34 = DAT_00000bb4;
    local_38 = 0x142;
    FUN_00002a10(&local_38);
    FUN_00005ee4((int)local_36,1);
    FUN_00005e64((int)local_36);
    FUN_00005ec0((int)local_36);
    local_36 = 2;
    local_34 = DAT_00000bb8;
    local_38 = 0x141;
    FUN_00002a10(&local_38);
    FUN_00005ee4((int)local_36,1);
    FUN_00005e64((int)local_36);
    FUN_00005ec0((int)local_36);
    local_36 = 3;
    local_34 = DAT_00000bbc;
    local_38 = 0x144;
    FUN_00002a10(&local_38);
    FUN_00005ee4((int)local_36,0xf);
    FUN_00005e64((int)local_36);
    FUN_00005ec0((int)local_36);
    FUN_00005498(DAT_00000bb0,8,1);
    FUN_00005498(DAT_00000bb0,4,1);
    FUN_00005498(DAT_00000bb0,0x20,1);
    FUN_00005498(DAT_00000bb0,1);
    FUN_00005498(DAT_00000bb0,2,1);
  }
  return iVar1 != 0;
}



/* ===== FUN_00000bc0 @ 00000bc0, 390 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00000bc0(void)

{
  char cVar1;
  uint uVar2;
  
  FUN_0000031c(DAT_00000d48 + (uint)*DAT_00000d4c,DAT_00000d54,*DAT_00000d50);
  *DAT_00000d4c = *DAT_00000d4c + (short)*DAT_00000d50;
  if (*DAT_00000d58 == '\x01') {
    if (*DAT_00000d5c < *DAT_00000d60) {
      if (*DAT_00000d64 == '\0') {
        FUN_0000031c(DAT_00000d6c + *DAT_00000d5c,DAT_00000d54,*DAT_00000d50);
        *DAT_00000d5c = *DAT_00000d5c + *DAT_00000d50;
      }
      else {
        FUN_0000031c(DAT_00000d6c + *DAT_00000d5c,DAT_00000d54 + 5,*DAT_00000d68 + -5);
        *DAT_00000d5c = *DAT_00000d68 + -5;
        *DAT_00000d64 = '\0';
      }
    }
    if (*DAT_00000d5c == *DAT_00000d60 + 1) {
      cVar1 = crc8_maxim(DAT_00000d6c,*DAT_00000d60);
      *DAT_00000d70 = cVar1;
      if (*(char *)(DAT_00000d48 + *DAT_00000d60 + 5) == *DAT_00000d70) {
        disableIRQinterrupts();
        *DAT_00000d5c = 0;
        *DAT_00000d58 = '\0';
        *DAT_00000d74 = 0;
        *DAT_00000d80 = ((uint)*DAT_00000d78 - (uint)*DAT_00000d7c) * 0x2000 + 0x20000;
        aes_update_chunk_transform(DAT_00000d6c,DAT_00000d6c,*DAT_00000d60);
        thunk_EXT_FUN_1fff8000(*DAT_00000d80,DAT_00000d6c,*DAT_00000d60);
        *DAT_00000d84 = 0;
        *DAT_00000d88 = *DAT_00000d88 + 1;
        *DAT_00000d4c = 0;
        *DAT_00000d68 = 0;
        FUN_00005e20(_DAT_00000d98,0xd8c,*DAT_00000d88);
        mcan_send_frame(0x7fe,_DAT_00000d98,8);
        delay_ms(1);
        enableIRQinterrupts();
        if (*DAT_00000d7c == 0) {
          delay_ms(0x14);
          mcan_send_frame(0x7fe,0xd9c,8);
          delay_ms(1);
          *DAT_00000da8 = 1;
        }
      }
      else {
        for (uVar2 = 0; uVar2 < 10000; uVar2 = uVar2 + 1) {
        }
        mcan_send_frame(0x7fe,0xdac,8);
        delay_ms(1);
        *DAT_00000d4c = 0;
        *DAT_00000d58 = '\0';
        *DAT_00000d74 = 0;
      }
    }
  }
  return;
}



/* ===== mcan_send_frame @ 00000db8, 44 bytes ===== */

void mcan_send_frame(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  undefined4 *puVar1;
  
  puVar1 = DAT_00000de4;
  *DAT_00000de4 = param_1;
  puVar1[3] = param_3;
  FUN_0000031c(puVar1 + 10,param_2,param_3);
  FUN_00002efc(DAT_00000de8,DAT_00000de4);
  FUN_00003270(DAT_00000de8,DAT_00000de4[9]);
  return;
}



/* ===== FUN_00000dec @ 00000dec, 22 bytes ===== */

void FUN_00000dec(uint param_1)

{
  undefined4 local_8;
  
  for (local_8 = 0; local_8 < param_1; local_8 = local_8 + 1) {
  }
  return;
}



/* ===== FUN_00000e02 @ 00000e02, 22 bytes ===== */

undefined4 FUN_00000e02(int param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if (param_1 == 0) {
    uVar1 = 0xfffffffd;
  }
  else {
    FUN_00002908(param_1);
  }
  return uVar1;
}



/* ===== FUN_00000e18 @ 00000e18, 220 bytes ===== */

undefined4 FUN_00000e18(uint *param_1)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  
  uVar1 = 0;
  if (param_1 == (uint *)0x0) {
    uVar1 = 0xfffffffd;
  }
  else {
    uVar5 = *(uint *)(DAT_00000ef4 + 0x100);
    uVar6 = *(uint *)(DAT_00000ef4 + 0x100);
    uVar7 = *(uint *)(DAT_00000ef4 + 0x100);
    uVar4 = (*(uint *)(DAT_00000ef4 + 0x100) & 0xffff) >> 8;
    uVar3 = *(uint *)(DAT_00000ef4 + 0x100) & 3;
    uVar2 = DAT_00000ef8;
    if (*(int *)(DAT_00000ef4 + 0x100) << 0x18 < 0) {
      uVar2 = *DAT_00000efc;
    }
    *param_1 = uVar2 / (uVar3 + 1);
    param_1[1] = (uVar2 / (uVar3 + 1)) * (uVar4 + 1);
    param_1[2] = ((uVar2 / (uVar3 + 1)) * (uVar4 + 1)) / ((uVar5 >> 0x1c) + 1);
    param_1[3] = ((uVar2 / (uVar3 + 1)) * (uVar4 + 1)) / (((uVar6 & 0xfffffff) >> 0x18) + 1);
    param_1[4] = ((uVar2 / (uVar3 + 1)) * (uVar4 + 1)) / (((uVar7 & 0xffffff) >> 0x14) + 1);
  }
  return uVar1;
}



/* ===== FUN_00000f00 @ 00000f00, 46 bytes ===== */

bool FUN_00000f00(uint param_1)

{
  if ((param_1 == 0) || ((param_1 | 0x29) != 0x29)) {
    thunk_FUN_000003fc(0xf30,0x50c);
  }
  return (*DAT_00000f4c & param_1) != 0;
}



/* ===== FUN_00000f50 @ 00000f50, 146 bytes ===== */

int FUN_00000f50(int param_1)

{
  int iVar1;
  
  iVar1 = 0;
  if ((param_1 != 0) && (param_1 != 1)) {
    thunk_FUN_000003fc(0xfe4,0x4c2);
  }
  if ((*DAT_00001000 & 1) == 0) {
    thunk_FUN_000003fc(0xfe4,0x4c3);
  }
  if (param_1 == 0) {
    if ((*DAT_00001004 & 7) == 5) {
      iVar1 = -6;
    }
    else {
      DAT_00001004[4] = 1;
    }
  }
  else {
    if (*(int *)(DAT_00001004 + 0xda) << 0x18 < 0) {
      iVar1 = FUN_000013c8(1,0x1000);
    }
    else {
      iVar1 = FUN_000013c8(8,0x1000);
    }
    if (iVar1 == 0) {
      DAT_00001004[4] = 0;
      iVar1 = FUN_000013c8(0x20,0x1000);
    }
  }
  return iVar1;
}



/* ===== FUN_00001008 @ 00001008, 594 bytes ===== */

undefined4 FUN_00001008(char *param_1)

{
  uint *puVar1;
  uint uVar2;
  undefined4 uVar3;
  uint uVar4;
  
  if (param_1 == (char *)0x0) {
    uVar3 = 0xfffffffd;
  }
  else {
    if (((int)((uint)(byte)param_1[4] << 0x18) < 0) && (-1 < (int)((uint)(byte)param_1[4] << 0x18)))
    {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x495);
    }
    if ((4 < (byte)((param_1[4] & 3U) + 1)) || (((byte)param_1[4] & 3) == 0xffffffff)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x496);
    }
    if ((0x96 < ((*(uint *)(param_1 + 4) & 0x1ffff) >> 8) + 1) ||
       (((*(uint *)(param_1 + 4) & 0x1ffff) >> 8) + 1 < 0x19)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x497);
    }
    if ((0x10 < (*(uint *)(param_1 + 4) >> 0x1c) + 1) || ((*(uint *)(param_1 + 4) >> 0x1c) + 1 < 2))
    {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x498);
    }
    uVar4 = DAT_00001278;
    if ((int)((uint)(byte)param_1[4] << 0x18) < 0) {
      uVar4 = *DAT_0000127c;
    }
    uVar4 = uVar4 / (((byte)param_1[4] & 3) + 1);
    uVar2 = (((*(uint *)(param_1 + 4) & 0x1ffff) >> 8) + 1) * uVar4;
    if ((DAT_00001280 < uVar4) || (uVar4 < DAT_00001278)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x49d);
    }
    if ((DAT_00001284 <= uVar2 && uVar2 - DAT_00001284 != 0) ||
       (uVar2 < (uint)((int)DAT_00001284 >> 1))) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x49e);
    }
    if ((DAT_00001288 < uVar2 / ((*(uint *)(param_1 + 4) >> 0x1c) + 1)) ||
       (uVar2 / ((*(uint *)(param_1 + 4) >> 0x1c) + 1) < DAT_0000128c)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x49f);
    }
    if ((0x10 < ((*(uint *)(param_1 + 4) & 0xfffffff) >> 0x18) + 1) ||
       (((*(uint *)(param_1 + 4) & 0xfffffff) >> 0x18) + 1 < 2)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x4a0);
    }
    if ((0x10 < ((*(uint *)(param_1 + 4) & 0xffffff) >> 0x14) + 1) ||
       (((*(uint *)(param_1 + 4) & 0xffffff) >> 0x14) + 1 < 2)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x4a1);
    }
    if ((DAT_00001288 < uVar2 / (((*(uint *)(param_1 + 4) & 0xffffff) >> 0x14) + 1)) ||
       (uVar2 / (((*(uint *)(param_1 + 4) & 0xffffff) >> 0x14) + 1) < DAT_0000128c)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x4a2);
    }
    if ((DAT_00001288 < uVar2 / (((*(uint *)(param_1 + 4) & 0xfffffff) >> 0x18) + 1)) ||
       (uVar2 / (((*(uint *)(param_1 + 4) & 0xfffffff) >> 0x18) + 1) < DAT_0000128c)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x4a3);
    }
    if ((*param_1 != '\x01') && (*param_1 != '\0')) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x4a5);
    }
    if ((*DAT_00001290 & 1) == 0) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x4a6);
    }
    puVar1 = DAT_00001294;
    *DAT_00001294 = *DAT_00001294 & 0xffffff7f | (uint)((byte)param_1[4] >> 7) << 7;
    *puVar1 = *(uint *)(param_1 + 4);
    if (*param_1 == '\0') {
      uVar3 = FUN_00000f50(1);
    }
    else {
      uVar3 = FUN_00000f50(0);
    }
  }
  return uVar3;
}



/* ===== FUN_00001298 @ 00001298, 82 bytes ===== */

undefined4 FUN_00001298(undefined1 *param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if (param_1 == (undefined1 *)0x0) {
    uVar1 = 0xfffffffd;
  }
  else {
    *(undefined4 *)(param_1 + 4) = 0;
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) & 0xffffff7f;
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) & 0xfffffffc;
    *(uint *)(param_1 + 4) = *(uint *)(param_1 + 4) & 0xfffe00ff | 0x1300;
    *(uint *)(param_1 + 4) = (*(uint *)(param_1 + 4) & 0xfffffff) + 0x10000000;
    *(uint *)(param_1 + 4) = (*(uint *)(param_1 + 4) & 0xf0ffffff) + 0x1000000;
    *(uint *)(param_1 + 4) = (*(uint *)(param_1 + 4) & 0xff0fffff) + 0x100000;
    *param_1 = 1;
  }
  return uVar1;
}



/* ===== FUN_000012ec @ 000012ec, 162 bytes ===== */

void FUN_000012ec(uint param_1,int param_2)

{
  uint uVar1;
  
  uVar1 = 0;
  if ((param_1 != 1) && (param_1 != 2)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x5ab);
  }
  if ((((((param_2 != 1) && (param_2 != 2)) && (param_2 != 3)) && ((param_2 != 4 && (param_2 != 5)))
       ) && ((param_2 != 6 && ((param_2 != 7 && (param_2 != 8)))))) &&
     ((param_2 != 9 && (param_2 != 0xd)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x5ac);
  }
  if ((*DAT_000013ac & 1) == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x5ad);
  }
  for (; param_1 != 0; param_1 = (int)param_1 >> 1) {
    if ((param_1 & 1) != 0) {
      *(ushort *)(DAT_000013b0 + 0x18) =
           *(ushort *)(DAT_000013b0 + 0x18) & ~(ushort)(0xf << (uVar1 & 0xff)) |
           (ushort)(param_2 << (uVar1 & 0xff)) & (ushort)(0xf << (uVar1 & 0xff));
    }
    uVar1 = uVar1 + 4;
  }
  return;
}



/* ===== FUN_000013b4 @ 000013b4, 20 bytes ===== */

void FUN_000013b4(undefined4 param_1,undefined4 param_2)

{
  FUN_00004314(param_1,param_2);
  FUN_00004c58();
  return;
}



/* ===== FUN_000013c8 @ 000013c8, 48 bytes ===== */

undefined4 FUN_000013c8(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint local_18;
  
  local_18 = 0;
  while( true ) {
    if (param_2 < local_18) {
      return 0xfffffff8;
    }
    iVar1 = FUN_00000f00(param_1);
    if (iVar1 == 1) break;
    local_18 = local_18 + 1;
  }
  return 0;
}



/* ===== FUN_000013f8 @ 000013f8, 144 bytes ===== */

undefined4 FUN_000013f8(int param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if ((param_1 != 0) && (param_1 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x344);
  }
  if ((*DAT_000014a4 & 1) == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x345);
  }
  if (param_1 == 0) {
    if ((*DAT_000014a8 & 7) == 3) {
      uVar1 = 0xfffffffa;
    }
    else if (*(int *)(DAT_000014a8 + 0xda) << 0x18 < 0) {
      DAT_000014a8[0xc] = 1;
    }
    else if (DAT_000014a8[4] == 0) {
      uVar1 = 0xfffffffa;
    }
    else {
      DAT_000014a8[0xc] = 1;
    }
  }
  else {
    DAT_000014a8[0xc] = 0;
    uVar1 = FUN_000013c8(8,0x1000);
  }
  return uVar1;
}



/* ===== FUN_000014ac @ 000014ac, 244 bytes ===== */

undefined4 FUN_000014ac(char *param_1)

{
  undefined4 uVar1;
  
  if (param_1 == (char *)0x0) {
    uVar1 = 0xfffffffd;
  }
  else {
    if ((*param_1 != '\x01') && (*param_1 != '\0')) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x325);
    }
    if ((((param_1[1] != '\0') && (param_1[1] != '\x10')) && (param_1[1] != ' ')) &&
       (param_1[1] != '0')) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x326);
    }
    if ((param_1[2] != '\0') && (param_1[2] != '@')) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x327);
    }
    if ((((param_1[3] != '\x01') && (param_1[3] != '\x02')) &&
        ((param_1[3] != '\x03' &&
         ((((param_1[3] != '\x04' && (param_1[3] != '\x05')) && (param_1[3] != '\x06')) &&
          ((param_1[3] != '\a' && (param_1[3] != '\b')))))))) && (param_1[3] != '\t')) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x328);
    }
    if ((*DAT_000015bc & 1) == 0) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x329);
    }
    *DAT_000015c0 = param_1[3];
    *DAT_000015c4 = param_1[1] | 0x80U | param_1[2];
    if (*param_1 == '\0') {
      uVar1 = FUN_000013f8(1);
    }
    else {
      uVar1 = FUN_000013f8(0);
    }
  }
  return uVar1;
}



/* ===== FUN_000015c8 @ 000015c8, 26 bytes ===== */

undefined4 FUN_000015c8(undefined1 *param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if (param_1 == (undefined1 *)0x0) {
    uVar1 = 0xfffffffd;
  }
  else {
    *param_1 = 1;
    param_1[2] = 0;
    param_1[1] = 0;
    param_1[3] = 5;
  }
  return uVar1;
}



/* ===== FUN_000015e2 @ 000015e2, 82 bytes ===== */

void FUN_000015e2(undefined4 param_1,undefined4 param_2)

{
  byte bVar1;
  
  FUN_00000760(0,param_1,param_2);
  for (bVar1 = 1; bVar1 < 0xe; bVar1 = bVar1 + 1) {
    FUN_00004c04(param_1);
    FUN_00004814(param_1);
    FUN_0000420c(param_1);
    FUN_00000760(bVar1,param_1,param_2);
  }
  FUN_00004c04(param_1);
  FUN_00004814(param_1);
  FUN_00000760(0xe,param_1,param_2);
  return;
}



/* ===== FUN_00001634 @ 00001634, 178 bytes ===== */

void FUN_00001634(undefined4 param_1,uint param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 *puVar1;
  uint local_10;
  undefined4 local_c;
  undefined4 local_8;
  
  local_10 = param_2;
  local_c = param_3;
  local_8 = param_4;
  FUN_000015c8(&local_8);
  FUN_00001298(&local_10);
  FUN_000013b4(DAT_000016ec,DAT_000016e8);
  local_8 = 0x5002000;
  FUN_000014ac(&local_8);
  local_10 = local_10 & 0xffffff00;
  local_c = 0x39306300;
  FUN_00001008(&local_10);
  puVar1 = DAT_000016f0;
  *DAT_000016f0 = 0x123;
  *puVar1 = 0x3210;
  FUN_000020cc(3);
  *DAT_000016f0 = 0;
  FUN_000027d0(0x3000);
  FUN_00000460(5);
  return;
}



/* ===== FUN_000016f4 @ 000016f4, 98 bytes ===== */

undefined4
FUN_000016f4(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5
            ,undefined4 param_6)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 uVar3;
  
  iVar2 = -1;
  if ((((param_1 == DAT_00001758) || (param_1 == DAT_0000175c)) || (param_1 == DAT_00001760)) ||
     (((param_1 == DAT_00001764 || (param_1 == DAT_00001768)) ||
      (uVar3 = param_4, param_1 == DAT_0000176c)))) {
    uVar3 = param_6;
    iVar2 = FUN_00001770(param_1,param_2,param_3,param_4,param_5,param_6);
  }
  uVar1 = 0;
  if (iVar2 != 0) {
    uVar1 = FUN_000018f4(param_1,param_2,param_3,param_4,param_6,uVar3);
  }
  return uVar1;
}



/* ===== FUN_00001770 @ 00001770, 330 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4
FUN_00001770(int param_1,uint param_2,uint param_3,uint *param_4,uint *param_5,float *param_6)

{
  ulonglong uVar1;
  ulonglong uVar2;
  longlong lVar3;
  uint uVar4;
  float fVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  undefined8 uVar9;
  undefined8 uVar10;
  undefined4 local_64;
  
  local_64 = 0xfffffffd;
  if (((((param_1 != DAT_000018bc) && (param_1 != DAT_000018c0)) && (param_1 != DAT_000018c4)) &&
      ((param_1 != DAT_000018c8 && (param_1 != DAT_000018cc)))) && (param_1 != _DAT_000018d0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x28d);
  }
  if (((param_4 != (uint *)0x0) && (param_5 != (uint *)0x0)) && ((param_2 != 0 && (param_3 != 0))))
  {
    uVar4 = param_2 / (param_3 << 2);
    uVar7 = uVar4 - 1;
    if ((uVar7 != 0) && (uVar7 < 0x100)) {
      uVar1 = (ulonglong)uVar4 * 4;
      uVar2 = (uVar1 & 0xffffffff) * (ulonglong)param_3;
      iVar6 = ((uint)(0xfffffffe < uVar7) * 4 + (int)(uVar1 >> 0x20)) * param_3 +
              (int)(uVar2 >> 0x20);
      lVar3 = (uVar2 & 0xffffffff) * 0x100 + CONCAT44(iVar6 * 0x100,param_2 >> 1);
      uVar4 = FUN_0000029c((int)lVar3,(int)((ulonglong)lVar3 >> 0x20),param_2,0);
      uVar8 = uVar4 - 0x80;
      if (uVar8 < 0x80) {
        *param_4 = uVar7;
        *param_5 = uVar8;
        if (param_6 != (float *)0x0) {
          lVar3 = (uVar2 & 0xffffffff) * 0x100;
          uVar9 = FUN_0000035a((int)lVar3,iVar6 * 0x100 + (int)((ulonglong)lVar3 >> 0x20));
          uVar10 = FUN_0000035a((int)((ulonglong)uVar4 * (ulonglong)param_2),
                                (0xffffff7f < uVar8) * param_2 +
                                (int)((ulonglong)uVar4 * (ulonglong)param_2 >> 0x20));
          FUN_00000470((int)uVar10,(int)((ulonglong)uVar10 >> 0x20),(int)uVar9,
                       (int)((ulonglong)uVar9 >> 0x20));
          fVar5 = (float)FUN_00000372();
          *param_6 = fVar5 - 1.0;
        }
        local_64 = 0;
      }
    }
  }
  return local_64;
}



/* ===== FUN_000018f4 @ 000018f4, 232 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_000018f4(int param_1,int param_2,uint param_3,uint *param_4,float *param_5)

{
  longlong lVar1;
  uint uVar2;
  float fVar3;
  uint uVar4;
  undefined8 uVar5;
  undefined8 uVar6;
  undefined4 local_58;
  
  local_58 = 0xfffffffd;
  if (((((param_1 != DAT_000019dc) && (param_1 != DAT_000019e0)) && (param_1 != DAT_000019e4)) &&
      ((param_1 != DAT_000019e8 && (param_1 != DAT_000019ec)))) && (param_1 != _DAT_000019f0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x1c2);
  }
  if (((param_4 != (uint *)0x0) && (param_2 != 0)) && (param_3 != 0)) {
    uVar2 = ((uint)(param_2 * 10) / (param_3 << 2) + 5) / 10;
    uVar4 = uVar2 - 1;
    if ((uVar4 != 0) && (uVar4 < 0x100)) {
      *param_4 = uVar4;
      if (param_5 != (float *)0x0) {
        lVar1 = (ulonglong)uVar2 * 4 * (ulonglong)param_3;
        uVar5 = FUN_0000035a((int)lVar1,
                             (uint)(0xfffffffe < uVar4) * 4 * param_3 +
                             (int)((ulonglong)lVar1 >> 0x20));
        uVar6 = FUN_00000340(param_2);
        FUN_00000470((int)uVar6,(int)((ulonglong)uVar6 >> 0x20),(int)uVar5,
                     (int)((ulonglong)uVar5 >> 0x20));
        fVar3 = (float)FUN_00000372();
        *param_5 = fVar3 - 1.0;
      }
      local_58 = 0;
    }
  }
  return local_58;
}



/* ===== delay_ms @ 00001a14, 54 bytes ===== */

void delay_ms(int param_1)

{
  bool bVar1;
  undefined4 local_8;
  
  while (bVar1 = param_1 != 0, param_1 = param_1 + -1,
        local_8 = ((*DAT_00001a50 >> ((*(uint *)(DAT_00001a4c + 0x20) & 0x7ffffff) >> 0x18)) + 9999)
                  / 10000, bVar1) {
    do {
      bVar1 = local_8 != 0;
      local_8 = local_8 - 1;
    } while (bVar1);
  }
  return;
}



/* ===== FUN_00001a54 @ 00001a54, 74 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00001a54(uint param_1)

{
  if (*_DAT_00001aa0 != 1) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x221);
  }
  if ((param_1 | 0x13f) != 0x13f) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x222);
  }
  _DAT_00001aa0[9] = _DAT_00001aa0[9] | param_1;
  return;
}



/* ===== FUN_00001ac0 @ 00001ac0, 60 bytes ===== */

void FUN_00001ac0(int param_1)

{
  uint uVar1;
  uint *puVar2;
  
  if ((param_1 != 0) && (param_1 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x3ca);
  }
  puVar2 = DAT_00001b1c;
  uVar1 = DAT_00001b18;
  if (param_1 == 1) {
    *DAT_00001b1c = DAT_00001b18;
    *puVar2 = ~uVar1;
  }
  else {
    DAT_00001b1c[6] = DAT_00001b1c[6] | 0x10000;
  }
  return;
}



/* ===== FUN_00001b20 @ 00001b20, 52 bytes ===== */

bool FUN_00001b20(uint param_1)

{
  if ((param_1 | 0x13f) != 0x13f) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x213);
  }
  return (*DAT_00001b70 & param_1) == param_1;
}



/* ===== FUN_00001b74 @ 00001b74, 162 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00001b74(void)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  
  uVar4 = 0;
  if (*_DAT_00001c18 != 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x402);
  }
  puVar1 = DAT_00001c38;
  uVar2 = *DAT_00001c38;
  *DAT_00001c38 = *DAT_00001c38 & 0xfff0ffff;
  puVar1[1] = (puVar1[1] & 0xfffffff8) + 1;
  if (*DAT_00001c3c == -1) {
    *DAT_00001c3c = 0;
    iVar3 = FUN_0000216c(0x100,(*DAT_00001c44 >>
                               ((*(uint *)(DAT_00001c40 + 0x20) & 0x7ffffff) >> 0x18)) / 20000);
    if (iVar3 == -8) {
      uVar4 = 0xfffffffb;
    }
    FUN_00001a54(0x10);
  }
  puVar1 = DAT_00001c38;
  DAT_00001c38[1] = DAT_00001c38[1] & 0xfffffff8;
  *DAT_00001c38 = *puVar1 & 0xfff0ffff | uVar2 & 0xf0000;
  return uVar4;
}



/* ===== FUN_00001c48 @ 00001c48, 288 bytes ===== */

undefined4 FUN_00001c48(undefined4 *param_1)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  
  uVar4 = 0;
  if ((DAT_00001d68 <= param_1) && (param_1 < (undefined4 *)((int)DAT_00001d68 + 0x7f))) {
    if (((uint)param_1 & 3) != 0) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x42f);
    }
    if (*DAT_00001d88 != 0) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x430);
    }
    if (*DAT_00001d8c != 1) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x431);
    }
    if (DAT_00001d88[1] != 0) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x432);
    }
    if ((param_1 != DAT_00001d68) &&
       ((param_1 < DAT_00001d68 + 0x10 || ((undefined4 *)((int)DAT_00001d68 + 0x7f) < param_1)))) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x433);
    }
    piVar1 = DAT_00001d8c;
    uVar2 = DAT_00001d8c[6];
    DAT_00001d8c[6] = DAT_00001d8c[6] & 0xfff0ffff;
    piVar1[7] = (piVar1[7] & 0xfffffff8U) + 1;
    *param_1 = 0;
    iVar3 = FUN_0000216c(0x100,(*DAT_00001d94 >>
                               ((*(uint *)(DAT_00001d90 + 0x20) & 0x7ffffff) >> 0x18)) / 0x32);
    if (iVar3 == -8) {
      uVar4 = 0xfffffffb;
    }
    FUN_00001a54(0x10);
    piVar1 = DAT_00001d8c;
    DAT_00001d8c[7] = DAT_00001d8c[7] & 0xfffffff8;
    DAT_00001d8c[6] = piVar1[6] & 0xfff0ffffU | uVar2 & 0xf0000;
  }
  return uVar4;
}



/* ===== FUN_00001d98 @ 00001d98, 44 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00001d98(void)

{
  if (*_DAT_00001dc4 != 1) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x3f3);
  }
  _DAT_00001dc4[7] = _DAT_00001dc4[7] | 0x20000;
  return;
}



/* ===== FUN_00001de4 @ 00001de4, 40 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00001de4(void)

{
  uint uVar1;
  uint *puVar2;
  
  if (*_DAT_00001e0c != 1) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x3e6);
  }
  uVar1 = DAT_00001e2c;
  puVar2 = (uint *)(_DAT_00001e0c + 2);
  *puVar2 = DAT_00001e2c;
  *puVar2 = ~uVar1;
  return;
}



/* ===== FUN_00001e30 @ 00001e30, 278 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00001e30(undefined4 *param_1,undefined4 param_2)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  
  uVar4 = 0;
  if (*_DAT_00001f48 != 1) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x2b2);
  }
  if (*DAT_00001f68 != 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x2b3);
  }
  if ((((undefined4 *)0x3ffff < param_1) && ((param_1 < DAT_00001f6c || (DAT_00001f70 < param_1))))
     && ((param_1 < DAT_00001f74 || ((undefined4 *)((int)DAT_00001f74 + 0xbU) < param_1)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x2b4);
  }
  if (((uint)param_1 & 3) != 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x2b5);
  }
  FUN_00001a54(0x13f);
  piVar1 = _DAT_00001f48;
  uVar2 = _DAT_00001f48[6];
  _DAT_00001f48[6] = _DAT_00001f48[6] & 0xfff0ffff;
  piVar1[7] = (piVar1[7] & 0xfffffff8U) + 1;
  *param_1 = param_2;
  iVar3 = FUN_0000216c(0x100,(*DAT_00001f7c >>
                             ((*(uint *)(DAT_00001f78 + 0x20) & 0x7ffffff) >> 0x18)) / 20000);
  if (iVar3 == -8) {
    uVar4 = 0xfffffffb;
  }
  FUN_00001a54(0x10);
  piVar1 = _DAT_00001f48;
  _DAT_00001f48[7] = _DAT_00001f48[7] & 0xfffffff8;
  _DAT_00001f48[6] = piVar1[6] & 0xfff0ffffU | uVar2 & 0xf0000;
  return uVar4;
}



/* ===== FUN_00001f80 @ 00001f80, 16 bytes ===== */

void FUN_00001f80(void)

{
  undefined4 *puVar1;
  
  puVar1 = DAT_00001f90;
  *DAT_00001f90 = 0x123;
  *puVar1 = 0x3210;
  return;
}



/* ===== FUN_00001f94 @ 00001f94, 258 bytes ===== */

undefined4 FUN_00001f94(undefined4 *param_1)

{
  int *piVar1;
  uint uVar2;
  int iVar3;
  undefined4 uVar4;
  
  uVar4 = 0;
  if (((undefined4 *)0x3ffff < param_1) && ((param_1 < DAT_00002098 || (DAT_0000209c < param_1)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x366);
  }
  if (((uint)param_1 & 3) != 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x367);
  }
  if (*DAT_000020bc != 1) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x368);
  }
  if (*DAT_000020c0 != 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x369);
  }
  FUN_00001a54(0x13f);
  piVar1 = DAT_000020bc;
  uVar2 = DAT_000020bc[6];
  DAT_000020bc[6] = DAT_000020bc[6] & 0xfff0ffff;
  piVar1[7] = (piVar1[7] & 0xfffffff8U) + 4;
  *param_1 = 0;
  iVar3 = FUN_0000216c(0x100,(*DAT_000020c8 >>
                             ((*(uint *)(DAT_000020c4 + 0x20) & 0x7ffffff) >> 0x18)) / 0x32);
  if (iVar3 == -8) {
    uVar4 = 0xfffffffb;
  }
  FUN_00001a54(0x10);
  piVar1 = DAT_000020bc;
  DAT_000020bc[7] = DAT_000020bc[7] & 0xfffffff8;
  DAT_000020bc[6] = piVar1[6] & 0xfff0ffffU | uVar2 & 0xf0000;
  return uVar4;
}



/* ===== FUN_000020cc @ 000020cc, 120 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_000020cc(uint param_1)

{
  uint uVar1;
  
  uVar1 = 0;
  if (*_DAT_00002144 != 1) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x13b);
  }
  if (0xf < param_1) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_efm.c",0x13c);
  }
  _DAT_00002144[6] = _DAT_00002144[6] & 0xfffffff0U | param_1 & 0xf;
  do {
    if ((_DAT_00002144[6] & 0xfU) == param_1) {
      return 0;
    }
    uVar1 = uVar1 + 1;
  } while (uVar1 <= (*DAT_00002168 >> ((*(uint *)(DAT_00002164 + 0x20) & 0x7ffffff) >> 0x18)) /
                    20000);
  return 0xfffffff8;
}



/* ===== FUN_0000216c @ 0000216c, 48 bytes ===== */

undefined4 FUN_0000216c(undefined4 param_1,uint param_2)

{
  int iVar1;
  uint local_18;
  
  local_18 = 0;
  do {
    iVar1 = FUN_00001b20(param_1);
    if (iVar1 == 1) {
      return 0;
    }
    local_18 = local_18 + 1;
  } while (local_18 <= param_2);
  return 0xfffffff8;
}



/* ===== FUN_0000219c @ 0000219c, 80 bytes ===== */

void FUN_0000219c(uint param_1,int param_2)

{
  if ((param_1 == 0) || ((DAT_000021ec | param_1) != DAT_000021ec)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_fcg.c",0x84);
  }
  if ((param_2 != 0) && (param_2 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_fcg.c",0x85);
  }
  if (param_2 == 1) {
    *(uint *)(DAT_0000220c + 4) = *(uint *)(DAT_0000220c + 4) & ~param_1;
  }
  else {
    *(uint *)(DAT_0000220c + 4) = *(uint *)(DAT_0000220c + 4) | param_1;
  }
  return;
}



/* ===== FUN_00002210 @ 00002210, 80 bytes ===== */

void FUN_00002210(uint param_1,int param_2)

{
  if ((param_1 == 0) || ((DAT_00002260 | param_1) != DAT_00002260)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_fcg.c",0x96);
  }
  if ((param_2 != 0) && (param_2 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_fcg.c",0x97);
  }
  if (param_2 == 1) {
    *(uint *)(DAT_00002280 + 8) = *(uint *)(DAT_00002280 + 8) & ~param_1;
  }
  else {
    *(uint *)(DAT_00002280 + 8) = *(uint *)(DAT_00002280 + 8) | param_1;
  }
  return;
}



/* ===== FUN_00002284 @ 00002284, 80 bytes ===== */

void FUN_00002284(uint param_1,int param_2)

{
  if ((param_1 == 0) || ((DAT_000022d4 | param_1) != DAT_000022d4)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_fcg.c",0xa8);
  }
  if ((param_2 != 0) && (param_2 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_fcg.c",0xa9);
  }
  if (param_2 == 1) {
    *(uint *)(DAT_000022f4 + 0xc) = *(uint *)(DAT_000022f4 + 0xc) & ~param_1;
  }
  else {
    *(uint *)(DAT_000022f4 + 0xc) = *(uint *)(DAT_000022f4 + 0xc) | param_1;
  }
  return;
}



/* ===== FUN_000022f8 @ 000022f8, 570 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_000022f8(int param_1,uint param_2,ushort *param_3)

{
  uint uVar1;
  undefined4 uVar2;
  
  uVar2 = 0;
  if (param_3 == (ushort *)0x0) {
    uVar2 = 0xfffffffd;
  }
  else {
    if ((*_DAT_00002534 & 1) == 0) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x10e);
    }
    if (((((param_1 != 0) && (param_1 != 1)) && (param_1 != 2)) &&
        ((param_1 != 3 && (param_1 != 4)))) && (param_1 != 5)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x10f);
    }
    if ((param_2 == 0) || ((param_2 & 0xffff) == 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x110);
    }
    if ((*param_3 != 0) && (*param_3 != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x111);
    }
    if ((param_3[1] != 0) && (param_3[1] != 2)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x112);
    }
    if ((param_3[2] != 0) && (param_3[2] != 4)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x113);
    }
    if (((param_3[3] != 0) && (param_3[3] != 0x10)) && (param_3[3] != 0x20)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x114);
    }
    if ((param_3[4] != 0) && (param_3[4] != 0x4000)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x115);
    }
    if ((param_3[5] != 0) && (param_3[5] != 0x40)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x116);
    }
    if ((param_3[6] != 0) && (param_3[6] != 0x80)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x117);
    }
    if ((param_3[7] != 0) && (param_3[7] != 0x2000)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x118);
    }
    if ((param_3[8] != 0) && (param_3[8] != 0x200)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x119);
    }
    if ((param_3[9] != 0) && (param_3[9] != 0x1000)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x11a);
    }
    if ((param_3[10] != 0) && (param_3[10] != 0x400)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x11b);
    }
    if ((param_3[0xb] != 0) && (param_3[0xb] != 0x8000)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x11c);
    }
    for (uVar1 = 0; uVar1 < 0x10; uVar1 = uVar1 + 1 & 0xff) {
      if ((1 << uVar1 & param_2) != 0) {
        _DAT_00002534[param_1 * 0x20 + uVar1 * 2 + 2] =
             _DAT_00002534[param_1 * 0x20 + uVar1 * 2 + 2] & 0x908 |
             (param_3[10] |
             param_3[6] | param_3[7] |
             param_3[0xb] |
             *param_3 | param_3[1] | param_3[2] | param_3[3] | param_3[5] | param_3[8] | param_3[9]
             | param_3[4]) & 0xf6f7;
      }
    }
  }
  return uVar2;
}



/* ===== FUN_00002558 @ 00002558, 82 bytes ===== */

void FUN_00002558(int param_1,uint param_2)

{
  ushort *puVar1;
  
  if (((((param_1 != 0) && (param_1 != 1)) && (param_1 != 2)) && ((param_1 != 3 && (param_1 != 4))))
     && (param_1 != 5)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x261);
  }
  if ((param_2 == 0) || ((param_2 & 0xffff) == 0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x262);
  }
  puVar1 = (ushort *)(param_1 * 0x10 + DAT_000025cc);
  *puVar1 = *puVar1 | (ushort)param_2;
  return;
}



/* ===== FUN_000025d0 @ 000025d0, 124 bytes ===== */

void FUN_000025d0(uint param_1,int param_2)

{
  if ((param_1 == 0) || ((param_1 | 0x1f) != 0x1f)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x179);
  }
  if ((param_2 != 0) && (param_2 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x17a);
  }
  if ((*DAT_0000266c & 1) == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x17b);
  }
  if (param_2 == 1) {
    DAT_0000266c[-4] = DAT_0000266c[-4] | (ushort)param_1 & 0x1f;
  }
  else {
    DAT_0000266c[-4] = DAT_0000266c[-4] & ~((ushort)param_1 & 0x1f);
  }
  return;
}



/* ===== FUN_00002670 @ 00002670, 196 bytes ===== */

void FUN_00002670(int param_1,uint param_2,uint param_3)

{
  uint uVar1;
  
  if (((((param_1 != 0) && (param_1 != 1)) && (param_1 != 2)) && ((param_1 != 3 && (param_1 != 4))))
     && (param_1 != 5)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x191);
  }
  if ((param_2 == 0) || ((param_2 & 0xffff) == 0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x192);
  }
  if ((((8 < param_3) && ((param_3 < 0xb || (0xf < param_3)))) &&
      ((param_3 < 0x15 || (0x16 < param_3)))) && ((param_3 < 0x20 || (0x39 < param_3)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x193);
  }
  if ((*DAT_00002754 & 1) == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x194);
  }
  for (uVar1 = 0; uVar1 < 0x10; uVar1 = uVar1 + 1 & 0xff) {
    if ((1 << uVar1 & param_2) != 0) {
      DAT_00002754[param_1 * 0x20 + uVar1 * 2 + 3] =
           DAT_00002754[param_1 * 0x20 + uVar1 * 2 + 3] & 0xffc0 | (ushort)param_3 & 0x3f;
    }
  }
  return;
}



/* ===== FUN_00002758 @ 00002758, 82 bytes ===== */

void FUN_00002758(int param_1,uint param_2)

{
  ushort *puVar1;
  
  if (((((param_1 != 0) && (param_1 != 1)) && (param_1 != 2)) && ((param_1 != 3 && (param_1 != 4))))
     && (param_1 != 5)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x24f);
  }
  if ((param_2 == 0) || ((param_2 & 0xffff) == 0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x250);
  }
  puVar1 = (ushort *)(param_1 * 0x10 + DAT_000027cc);
  *puVar1 = *puVar1 | (ushort)param_2;
  return;
}



/* ===== FUN_000027d0 @ 000027d0, 116 bytes ===== */

void FUN_000027d0(int param_1)

{
  if (((((param_1 != 0) && (param_1 != 0x1000)) && (param_1 != 0x2000)) &&
      ((param_1 != 0x3000 && (param_1 != 0x4000)))) &&
     ((param_1 != 0x5000 && ((param_1 != 0x6000 && (param_1 != 0x7000)))))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x1e7);
  }
  if ((*DAT_00002864 & 1) == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x1e8);
  }
  DAT_00002864[-2] = DAT_00002864[-2] & 0x8fff | (ushort)param_1 & 0x7000;
  return;
}



/* ===== FUN_00002868 @ 00002868, 38 bytes ===== */

undefined4 FUN_00002868(undefined2 *param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if (param_1 == (undefined2 *)0x0) {
    uVar1 = 0xfffffffd;
  }
  else {
    *param_1 = 0;
    param_1[1] = 0;
    param_1[3] = 0;
    param_1[0xb] = 0;
    param_1[6] = 0;
    param_1[7] = 0;
    param_1[4] = 0;
    param_1[5] = 0;
    param_1[8] = 0;
    param_1[9] = 0;
    param_1[2] = 0;
    param_1[10] = 0;
  }
  return uVar1;
}



/* ===== FUN_00002890 @ 00002890, 82 bytes ===== */

void FUN_00002890(int param_1,uint param_2)

{
  ushort *puVar1;
  
  if (((((param_1 != 0) && (param_1 != 1)) && (param_1 != 2)) && ((param_1 != 3 && (param_1 != 4))))
     && (param_1 != 5)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x284);
  }
  if ((param_2 == 0) || ((param_2 & 0xffff) == 0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_gpio.c",0x285);
  }
  puVar1 = (ushort *)(param_1 * 0x10 + DAT_00002904);
  *puVar1 = *puVar1 | (ushort)param_2;
  return;
}



/* ===== FUN_00002908 @ 00002908, 246 bytes ===== */

void FUN_00002908(uint *param_1)

{
  uint *puVar1;
  
  if ((*DAT_00002a04 & 7) < 6) {
                    /* WARNING: Could not recover jumptable at 0x00002916. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (*(code *)(&switchD_00002916::switchdataD_0000291a +
              (uint)(&switchD_00002916::switchdataD_0000291a)[*DAT_00002a04 & 7] * 2))();
    return;
  }
  puVar1 = (uint *)(DAT_00002a04 + -6);
  *puVar1 = *(uint *)(DAT_00002a04 + -6);
  param_1[1] = *param_1 >> ((*puVar1 & 0x7ffffff) >> 0x18);
  param_1[3] = *param_1 >> (((byte)*puVar1 & 0x7f) >> 4);
  param_1[6] = *param_1 >> ((*puVar1 & 0x7ffff) >> 0x10);
  param_1[5] = *param_1 >> (((ushort)*puVar1 & 0x7fff) >> 0xc);
  param_1[7] = *param_1 >> ((*puVar1 & 0x7fffff) >> 0x14);
  param_1[2] = *param_1 >> ((byte)*puVar1 & 7);
  param_1[4] = *param_1 >> (((ushort)*puVar1 & 0x7ff) >> 8);
  return;
}



/* ===== FUN_00002a10 @ 00002a10, 124 bytes ===== */

undefined4 FUN_00002a10(ushort *param_1)

{
  uint *puVar1;
  undefined4 uVar2;
  
  uVar2 = 0;
  if (param_1 == (ushort *)0x0) {
    uVar2 = 0xfffffffd;
  }
  else {
    if (0x1ff < *param_1) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_interrupts.c",0xbd);
    }
    if ((short)param_1[1] < 0x10) {
      puVar1 = (uint *)((short)param_1[1] * 4 + DAT_00002ab0);
      if ((*puVar1 & 0x1ff) == 0x1ff) {
        *puVar1 = (uint)*param_1;
        *(undefined4 *)(DAT_00002ab4 + (short)param_1[1] * 4) = *(undefined4 *)(param_1 + 2);
      }
      else if ((uint)*param_1 == (*puVar1 & 0x1ff)) {
        *(undefined4 *)(DAT_00002ab4 + (short)param_1[1] * 4) = *(undefined4 *)(param_1 + 2);
      }
      else {
        uVar2 = 0xfffffffe;
      }
    }
    else {
      uVar2 = 0xfffffffd;
    }
  }
  return uVar2;
}



/* ===== IRQ000_Handler @ 00002ab8, 14 bytes ===== */

void IRQ000_Handler(void)

{
  (*(code *)*DAT_00002ac8)();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ001_Handler @ 00002acc, 14 bytes ===== */

void IRQ001_Handler(void)

{
  (**(code **)(DAT_00002adc + 4))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ002_Handler @ 00002ae0, 14 bytes ===== */

void IRQ002_Handler(void)

{
  (**(code **)(DAT_00002af0 + 8))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ003_Handler @ 00002af4, 14 bytes ===== */

void IRQ003_Handler(void)

{
  (**(code **)(DAT_00002b04 + 0xc))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ004_Handler @ 00002b08, 14 bytes ===== */

void IRQ004_Handler(void)

{
  (**(code **)(DAT_00002b18 + 0x10))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ005_Handler @ 00002b1c, 14 bytes ===== */

void IRQ005_Handler(void)

{
  (**(code **)(DAT_00002b2c + 0x14))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ006_Handler @ 00002b30, 14 bytes ===== */

void IRQ006_Handler(void)

{
  (**(code **)(DAT_00002b40 + 0x18))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ007_Handler @ 00002b44, 14 bytes ===== */

void IRQ007_Handler(void)

{
  (**(code **)(DAT_00002b54 + 0x1c))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ008_Handler @ 00002b58, 14 bytes ===== */

void IRQ008_Handler(void)

{
  (**(code **)(DAT_00002b68 + 0x20))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ009_Handler @ 00002b6c, 14 bytes ===== */

void IRQ009_Handler(void)

{
  (**(code **)(DAT_00002b7c + 0x24))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ010_Handler @ 00002b80, 14 bytes ===== */

void IRQ010_Handler(void)

{
  (**(code **)(DAT_00002b90 + 0x28))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ011_Handler @ 00002b94, 14 bytes ===== */

void IRQ011_Handler(void)

{
  (**(code **)(DAT_00002ba4 + 0x2c))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ012_Handler @ 00002ba8, 14 bytes ===== */

void IRQ012_Handler(void)

{
  (**(code **)(DAT_00002bb8 + 0x30))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ013_Handler @ 00002bbc, 14 bytes ===== */

void IRQ013_Handler(void)

{
  (**(code **)(DAT_00002bcc + 0x34))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ014_Handler @ 00002bd0, 14 bytes ===== */

void IRQ014_Handler(void)

{
  (**(code **)(DAT_00002be0 + 0x38))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== IRQ015_Handler @ 00002be4, 14 bytes ===== */

void IRQ015_Handler(void)

{
  (**(code **)(DAT_00002bf4 + 0x3c))();
  DataSynchronizationBarrier(0xf);
  return;
}



/* ===== FUN_00002bf8 @ 00002bf8, 334 bytes ===== */

void FUN_00002bf8(int param_1,int param_2)

{
  undefined1 uVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  uint uVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  undefined4 local_14;
  
  for (uVar6 = 0; uVar6 < 8; uVar6 = uVar6 + 1) {
    *(undefined1 *)(param_1 + uVar6 * 4) = *(undefined1 *)(param_2 + uVar6 * 4);
    *(undefined1 *)(param_1 + uVar6 * 4 + 1) = *(undefined1 *)(param_2 + uVar6 * 4 + 1);
    *(undefined1 *)(param_1 + uVar6 * 4 + 2) = *(undefined1 *)(param_2 + uVar6 * 4 + 2);
    *(undefined1 *)(param_1 + uVar6 * 4 + 3) = *(undefined1 *)(param_2 + uVar6 * 4 + 3);
  }
  for (uVar6 = 8; uVar6 < 0x3c; uVar6 = uVar6 + 1) {
    iVar7 = (uVar6 - 1) * 4;
    uVar1 = *(undefined1 *)(param_1 + iVar7 + 1);
    local_14._0_2_ = CONCAT11(uVar1,*(byte *)(param_1 + iVar7));
    bVar2 = *(byte *)(param_1 + iVar7 + 2);
    local_14._0_3_ = CONCAT12(bVar2,(undefined2)local_14);
    bVar3 = *(byte *)(param_1 + iVar7 + 3);
    local_14 = CONCAT13(bVar3,(uint3)local_14);
    if ((uVar6 & 7) == 0) {
      local_14._0_2_ = CONCAT11(bVar2,uVar1);
      local_14._0_3_ = CONCAT12(bVar3,(undefined2)local_14);
      bVar4 = *(byte *)(DAT_00002d48 + ((uint3)local_14 & 0xff));
      local_14._0_2_ = CONCAT11(*(undefined1 *)(DAT_00002d48 + (uint)bVar2),bVar4);
      local_14._0_3_ = CONCAT12(*(undefined1 *)(DAT_00002d48 + (uint)bVar3),(undefined2)local_14);
      local_14 = CONCAT13(*(undefined1 *)(DAT_00002d48 + (uint)*(byte *)(param_1 + iVar7)),
                          (uint3)local_14);
      local_14 = CONCAT31(local_14._1_3_,bVar4 ^ *(byte *)(DAT_00002d4c + (uVar6 >> 3)));
    }
    uVar5 = local_14;
    if ((uVar6 & 7) == 4) {
      local_14._0_2_ =
           CONCAT11(*(undefined1 *)(DAT_00002d48 + (local_14 >> 8 & 0xff)),
                    *(undefined1 *)(DAT_00002d48 + (local_14 & 0xff)));
      local_14._0_3_ =
           CONCAT12(*(undefined1 *)(DAT_00002d48 + (uVar5 >> 0x10 & 0xff)),(undefined2)local_14);
      local_14 = CONCAT13(*(undefined1 *)(DAT_00002d48 + (uVar5 >> 0x18)),(uint3)local_14);
    }
    iVar8 = uVar6 * 4;
    iVar7 = (uVar6 - 8) * 4;
    *(byte *)(param_1 + iVar8) = *(byte *)(param_1 + iVar7) ^ (byte)local_14;
    *(byte *)(param_1 + iVar8 + 1) = *(byte *)(param_1 + iVar7 + 1) ^ local_14._1_1_;
    *(byte *)(param_1 + iVar8 + 2) = *(byte *)(param_1 + iVar7 + 2) ^ local_14._2_1_;
    *(byte *)(param_1 + iVar8 + 3) = *(byte *)(param_1 + iVar7 + 3) ^ local_14._3_1_;
  }
  return;
}



/* ===== FUN_00002d50 @ 00002d50, 122 bytes ===== */

void FUN_00002d50(uint param_1)

{
  undefined4 *puVar1;
  
  puVar1 = DAT_00002dcc;
  if ((param_1 & 1) != 0) {
    *DAT_00002dcc = 0x123;
    *puVar1 = 0x3210;
  }
  if ((param_1 & 2) != 0) {
    *(undefined4 *)(DAT_00002dd4 + 0x10) = DAT_00002dd0;
  }
  if ((param_1 & 4) != 0) {
    *DAT_00002dd8 = 0xa501;
  }
  if ((param_1 & 0x10) != 0) {
    FUN_000042fc(&DAT_0000a508);
  }
  if ((param_1 & 0x40) != 0) {
    FUN_000042fc(&DAT_0000a503);
  }
  if ((param_1 & 0x80) != 0) {
    *DAT_00002ddc = 0x77;
    DAT_00002ddc[2] = 0x77;
  }
  return;
}



/* ===== FUN_00002de0 @ 00002de0, 114 bytes ===== */

void FUN_00002de0(uint param_1)

{
  if ((param_1 & 1) != 0) {
    *DAT_00002e54 = 0;
  }
  if ((param_1 & 2) != 0) {
    *(undefined4 *)(DAT_00002e5c + 0x10) = DAT_00002e58;
  }
  if ((param_1 & 4) != 0) {
    *DAT_00002e60 = 0xa500;
  }
  if ((param_1 & 0x10) != 0) {
    FUN_000042e0(&DAT_0000a508);
  }
  if ((param_1 & 0x40) != 0) {
    FUN_000042e0(&DAT_0000a503);
  }
  if ((param_1 & 0x80) != 0) {
    *DAT_00002e64 = 0x76;
    DAT_00002e64[2] = 0x76;
  }
  return;
}



/* ===== mcan1_receive_irq @ 00002e68, 124 bytes ===== */

void mcan1_receive_irq(void)

{
  int *piVar1;
  int iVar2;
  
  iVar2 = FUN_00003820(DAT_00002ee4,1);
  if (iVar2 == 1) {
    FUN_000030cc(DAT_00002ee4,1);
    iVar2 = FUN_000035c0(DAT_00002ee4,0x40,DAT_00002ee8);
    piVar1 = DAT_00002ef0;
    if ((iVar2 == 0) && ((*DAT_00002ee8 == *DAT_00002eec || (*DAT_00002ee8 == 0x7ff)))) {
      *DAT_00002ef0 = DAT_00002ee8[3];
      *DAT_00002ef4 = *DAT_00002ef4 + *piVar1;
      FUN_0000031c(DAT_00002ef8,DAT_00002ee8 + 10,*DAT_00002ef0);
      firmware_update_frame_handler(DAT_00002ee8 + 10,*DAT_00002ef0);
      FUN_00000bc0();
    }
  }
  *(undefined4 *)(DAT_00002ee4 + 0x50) = 0xffffffff;
  FUN_00005e80(0x76);
  return;
}



/* ===== FUN_00002efc @ 00002efc, 424 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00002efc(int param_1,uint *param_2)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  byte bVar4;
  uint uVar5;
  undefined4 uVar6;
  
  uVar6 = 0xfffffffd;
  if ((param_1 != DAT_000030a4) && (param_1 != _DAT_000030a8)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6dc);
  }
  if (param_2 != (uint *)0x0) {
    if ((param_2[1] != 0) && (param_2[1] != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6de);
    }
    if (param_2[1] == 0) {
      if (0x7ff < *param_2) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6e0);
      }
    }
    else if (0x1fffffff < *param_2) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6e2);
    }
    if ((param_2[2] != 0) && (param_2[2] != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6e4);
    }
    if (0xf < param_2[3]) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6e5);
    }
    if ((param_2[4] != 0) && (param_2[4] != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6e6);
    }
    if ((param_2[5] != 0) && (param_2[5] != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6e7);
    }
    if ((param_2[6] != 0) && (param_2[6] != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6e8);
    }
    if ((param_2[7] != 0) && (param_2[7] != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6e9);
    }
    uVar5 = *(uint *)(param_1 + 0x18) & 0x800;
    if (((uVar5 != 0) || (0xff < param_2[8])) && ((uVar5 == 0 || (0xffff < param_2[8])))) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6eb);
    }
    if ((param_2[9] == 0) || ((param_2[9] & param_2[9] - 1) != 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6ec);
    }
    uVar5 = param_2[9];
    bVar2 = (byte)uVar5;
    bVar3 = (byte)(uVar5 >> 8);
    bVar4 = (byte)(uVar5 >> 0x10);
    bVar1 = (byte)(uVar5 >> 0x18);
    uVar5 = LZCOUNT((uint)(byte)((((((((bVar2 & 1) << 1 | bVar2 >> 1 & 1) << 1 | bVar2 >> 2 & 1) <<
                                     1 | bVar2 >> 3 & 1) << 1 | bVar2 >> 4 & 1) << 1 |
                                  bVar2 >> 5 & 1) << 1 | bVar2 >> 6 & 1) << 1 | bVar2 >> 7) << 0x18
                    | (uint)(byte)((((((((bVar3 & 1) << 1 | bVar3 >> 1 & 1) << 1 | bVar3 >> 2 & 1)
                                       << 1 | bVar3 >> 3 & 1) << 1 | bVar3 >> 4 & 1) << 1 |
                                    bVar3 >> 5 & 1) << 1 | bVar3 >> 6 & 1) << 1 | bVar3 >> 7) <<
                      0x10 | (uint)(byte)((((((((bVar4 & 1) << 1 | bVar4 >> 1 & 1) << 1 |
                                              bVar4 >> 2 & 1) << 1 | bVar4 >> 3 & 1) << 1 |
                                            bVar4 >> 4 & 1) << 1 | bVar4 >> 5 & 1) << 1 |
                                          bVar4 >> 6 & 1) << 1 | bVar4 >> 7) << 8 |
                    (uint)(byte)((((((((bVar1 & 1) << 1 | bVar1 >> 1 & 1) << 1 | bVar1 >> 2 & 1) <<
                                     1 | bVar1 >> 3 & 1) << 1 | bVar1 >> 4 & 1) << 1 |
                                  bVar1 >> 5 & 1) << 1 | bVar1 >> 6 & 1) << 1 | bVar1 >> 7));
    if (uVar5 < (*(uint *)(param_1 + 0xc0) & 0x3fffff) >> 0x10) {
      if ((*(uint *)(param_1 + 0xcc) & param_2[9]) == 0) {
        FUN_00003138(param_1,param_2,uVar5);
        uVar6 = 0;
      }
      else {
        uVar6 = 0xfffffffa;
      }
    }
    else {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x6ef);
    }
  }
  return uVar6;
}



/* ===== FUN_000030cc @ 000030cc, 68 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000030cc(int param_1,uint param_2)

{
  if ((param_1 != DAT_00003110) && (param_1 != _DAT_00003114)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x8e6);
  }
  if ((param_2 == 0) || ((param_2 | 0x3fffffff) != 0x3fffffff)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x8e7);
  }
  *(uint *)(param_1 + 0x50) = param_2;
  return;
}



/* ===== FUN_00003138 @ 00003138, 268 bytes ===== */

void FUN_00003138(int param_1,uint *param_2,int param_3,undefined4 param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint *puVar5;
  uint uVar6;
  uint uVar7;
  
  if (param_2[1] == 0) {
    uVar6 = (uint)(byte)param_2[4] << 0x1f | (uint)(byte)param_2[2] << 0x1d |
            (uint)(ushort)*param_2 << 0x12;
  }
  else {
    uVar6 = (uint)(byte)param_2[4] << 0x1f | (uint)(byte)param_2[1] << 0x1e |
            (uint)(byte)param_2[2] << 0x1d | *param_2;
  }
  uVar7 = (uint)(byte)param_2[8] << 0x18;
  if ((*(uint *)(param_1 + 0x18) & 0x800) != 0) {
    uVar7 = (ushort)param_2[8] & 0xff00 | uVar7;
  }
  uVar1 = param_2[7];
  uVar2 = param_2[6];
  uVar3 = param_2[5];
  uVar4 = param_2[3];
  puVar5 = (uint *)(param_3 * (uint)*(byte *)(DAT_00003248 + (*(uint *)(param_1 + 200) & 7)) +
                   (*(uint *)(param_1 + 0xc0) & 0xfffc) + DAT_00003244);
  *puVar5 = uVar6;
  puVar5 = puVar5 + 1;
  *puVar5 = uVar7 | (uint)(ushort)uVar1 << 0x17 | (uint)(ushort)uVar2 << 0x15 |
            (uint)(ushort)uVar3 << 0x14 | (uint)(ushort)uVar4 << 0x10;
  uVar6 = (uint)*(byte *)(DAT_0000324c + param_2[3]);
  if ((*(uint *)(param_1 + 0x18) & 0x100) == 0) {
    if (8 < uVar6) {
      uVar6 = 8;
    }
  }
  else if (*(byte *)(DAT_0000324c + (*(uint *)(param_1 + 200) & 7) + 8) < uVar6) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",600,param_3,param_4,param_1,param_2);
  }
  for (uVar7 = 0; uVar7 < uVar6; uVar7 = uVar7 + 4 & 0xff) {
    puVar5 = puVar5 + 1;
    *puVar5 = (uint)*(byte *)((int)param_2 + uVar7 + 0x2b) << 0x18 |
              (uint)*(byte *)((int)param_2 + uVar7 + 0x2a) << 0x10 |
              (uint)*(byte *)((int)param_2 + uVar7 + 0x29) << 8 |
              (uint)*(byte *)((int)param_2 + uVar7 + 0x28);
  }
  return;
}



/* ===== FUN_00003270 @ 00003270, 100 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00003270(int param_1,uint param_2)

{
  uint uVar1;
  
  if ((param_1 != DAT_000032d4) && (param_1 != _DAT_000032d8)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x712);
  }
  uVar1 = ((*(uint *)(param_1 + 0xc0) & 0x3fffff) >> 0x10) +
          ((*(uint *)(param_1 + 0xc0) & 0x3fffffff) >> 0x18);
  if ((uVar1 < 0x20) && ((param_2 == 0 || (((1 << uVar1) - 1U | param_2) != (1 << uVar1) - 1U)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x717);
  }
  *(uint *)(param_1 + 0xd0) = param_2;
  return;
}



/* ===== FUN_000032fc @ 000032fc, 534 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_000032fc(int param_1,int *param_2)

{
  byte bVar1;
  uint uVar2;
  uint *puVar3;
  uint uVar4;
  undefined4 uVar5;
  
  uVar5 = 0xfffffffd;
  if ((param_1 != DAT_00003514) && (param_1 != _DAT_00003518)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x479);
  }
  if (param_2 != (int *)0x0) {
    uVar5 = 0;
    if ((*param_2 != 0) && (*param_2 != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x47d);
    }
    if ((((param_2[3] != 0) && (param_2[3] != 1)) && (param_2[3] != 2)) &&
       (((param_2[3] != 3 && (param_2[3] != 4)) &&
        ((param_2[3] != 5 && ((param_2[3] != 6 && (param_2[3] != 7)))))))) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x47e);
    }
    if ((param_2[3] == 7) && (uVar2 = FUN_00003588(param_1), uVar2 <= (uint)param_2[6])) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x480);
    }
    if (*param_2 == 0) {
      if ((*(uint *)(param_1 + 0x84) & 0xffffff) >> 0x10 <= (uint)param_2[1]) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x486);
      }
      if (0x7ff < (uint)param_2[4]) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x487);
      }
      if (param_2[3] == 7) {
        uVar2 = (uint)*(byte *)(param_2 + 3) << 0x1b | (uint)*(ushort *)(param_2 + 4) << 0x10 |
                param_2[6];
      }
      else {
        if (((param_2[2] != 0) && (param_2[2] != 1)) && (param_2[2] != 2)) {
          thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x48f);
        }
        if (0x7ff < (uint)param_2[5]) {
          thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x490);
        }
        uVar2 = (uint)*(byte *)(param_2 + 2) << 0x1e | (uint)*(byte *)(param_2 + 3) << 0x1b |
                (uint)*(ushort *)(param_2 + 4) << 0x10 | param_2[5];
      }
      *(uint *)(DAT_0000353c + (*(uint *)(param_1 + 0x84) & 0xfffc) + param_2[1] * 4) = uVar2;
    }
    else {
      if ((*(uint *)(param_1 + 0x88) & 0x7fffff) >> 0x10 <= (uint)param_2[1]) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x49e);
      }
      if (0x1fffffff < (uint)param_2[4]) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x49f);
      }
      bVar1 = *(byte *)(param_2 + 3);
      uVar2 = param_2[4];
      if (param_2[3] == 7) {
        uVar4 = param_2[6];
      }
      else {
        if ((((param_2[2] != 0) && (param_2[2] != 1)) && (param_2[2] != 2)) && (param_2[2] != 3)) {
          thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x4a8);
        }
        if (0x1fffffff < (uint)param_2[5]) {
          thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x4a9);
        }
        uVar4 = param_2[5] | (uint)*(byte *)(param_2 + 2) << 0x1e;
      }
      puVar3 = (uint *)(DAT_0000353c + (*(uint *)(param_1 + 0x88) & 0xfffc) + param_2[1] * 8);
      *puVar3 = uVar2 | (uint)bVar1 << 0x1d;
      puVar3[1] = uVar4;
    }
  }
  return uVar5;
}



/* ===== FUN_00003540 @ 00003540, 72 bytes ===== */

void FUN_00003540(undefined4 param_1,int *param_2)

{
  uint uVar1;
  
  if (*param_2 != 0) {
    for (uVar1 = 0; uVar1 < (uint)param_2[2]; uVar1 = uVar1 + 1) {
      FUN_000032fc(param_1,*param_2 + uVar1 * 0x1c);
    }
  }
  if (param_2[1] != 0) {
    for (uVar1 = 0; uVar1 < (uint)param_2[3]; uVar1 = uVar1 + 1) {
      FUN_000032fc(param_1,param_2[1] + uVar1 * 0x1c);
    }
  }
  return;
}



/* ===== FUN_00003588 @ 00003588, 48 bytes ===== */

undefined8 FUN_00003588(int param_1)

{
  return CONCAT44(param_1,((*(uint *)(param_1 + 0xf0) & 0xfffc) -
                          (*(uint *)(param_1 + 0xac) & 0xfffc)) /
                          (uint)*(byte *)(DAT_000035bc + ((*(uint *)(param_1 + 0xbc) & 0x7ff) >> 8))
                 );
}



/* ===== FUN_000035c0 @ 000035c0, 554 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int FUN_000035c0(int param_1,uint param_2,uint *param_3)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  uint uVar5;
  uint local_28;
  
  puVar1 = (uint *)0x0;
  local_28 = 0;
  uVar5 = 0;
  iVar3 = -3;
  if ((param_1 != DAT_000037ec) && (param_1 != _DAT_000037f0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x757);
  }
  if (param_3 != (uint *)0x0) {
    iVar3 = 0;
    if (param_2 == 0x40) {
      if ((*(uint *)(param_1 + 0xa0) & 0x7f0000) == 0) {
        iVar3 = -3;
      }
      if ((iVar3 == 0) && ((*(uint *)(param_1 + 0xa4) & 0x7f) == 0)) {
        iVar3 = -9;
      }
      if (iVar3 == 0) {
        uVar5 = (*(uint *)(param_1 + 0xa4) & 0x3fff) >> 8;
        puVar1 = (uint *)((int)(short)(ushort)((*(uint *)(param_1 + 0xa4) << 0x12) >> 0x1a) *
                          (int)(short)(ushort)(byte)PTR_DAT_00003818[*(uint *)(param_1 + 0xbc) & 7]
                         + (*(uint *)(param_1 + 0xa0) & 0xfffc) + DAT_00003814);
        local_28 = (uint)(byte)PTR_DAT_0000381c[(*(uint *)(param_1 + 0xbc) & 7) + 8];
      }
    }
    else if (param_2 == 0x41) {
      if ((*(uint *)(param_1 + 0xb0) & 0x7f0000) == 0) {
        iVar3 = -3;
      }
      if ((iVar3 == 0) && ((*(uint *)(param_1 + 0xb4) & 0x7f) == 0)) {
        iVar3 = -9;
      }
      if (iVar3 == 0) {
        uVar5 = (*(uint *)(param_1 + 0xb4) & 0x3fff) >> 8;
        puVar1 = (uint *)((int)(short)(ushort)((*(uint *)(param_1 + 0xb4) << 0x12) >> 0x1a) *
                          (int)(short)(ushort)(byte)PTR_DAT_00003818
                                                    [(*(uint *)(param_1 + 0xbc) & 0x7f) >> 4] +
                         (*(uint *)(param_1 + 0xb0) & 0xfffc) + DAT_00003814);
        local_28 = (uint)(byte)PTR_DAT_0000381c[((*(uint *)(param_1 + 0xbc) & 0x7f) >> 4) + 8];
      }
    }
    else {
      uVar2 = FUN_00003588(param_1);
      if (param_2 < uVar2) {
        puVar1 = (uint *)(param_2 * (byte)PTR_DAT_00003818[(*(uint *)(param_1 + 0xbc) & 0x7ff) >> 8]
                         + (*(uint *)(param_1 + 0xac) & 0xfffc) + DAT_00003814);
        local_28 = (uint)(byte)PTR_DAT_0000381c[((*(uint *)(param_1 + 0xbc) & 0x7ff) >> 8) + 8];
      }
      else {
        iVar3 = -3;
      }
    }
    if (iVar3 == 0) {
      param_3[1] = (*puVar1 & 0x7fffffff) >> 0x1e;
      if (param_3[1] == 0) {
        *param_3 = (*puVar1 & 0x1fffffff) >> 0x12;
      }
      else {
        *param_3 = *puVar1 & 0x1fffffff;
      }
      param_3[2] = (*puVar1 & 0x3fffffff) >> 0x1d;
      param_3[4] = *puVar1 >> 0x1f;
      puVar1 = puVar1 + 1;
      param_3[7] = *puVar1 & 0xffff;
      param_3[3] = (*puVar1 & 0xfffff) >> 0x10;
      param_3[0x1a] = (uint)(byte)PTR_DAT_0000381c[param_3[3]];
      param_3[5] = (*puVar1 & 0x1fffff) >> 0x14;
      param_3[6] = (*puVar1 & 0x3fffff) >> 0x15;
      if (param_3[6] == 0) {
        if (8 < param_3[0x1a]) {
          param_3[0x1a] = 8;
        }
      }
      else if (local_28 < param_3[0x1a]) {
        param_3[0x1a] = local_28;
      }
      param_3[8] = (*puVar1 & 0x7fffffff) >> 0x18;
      param_3[9] = *puVar1 >> 0x1f;
      for (uVar2 = 0; uVar2 < param_3[0x1a]; uVar2 = uVar2 + 4) {
        puVar1 = puVar1 + 1;
        uVar4 = *puVar1;
        *(char *)((int)param_3 + uVar2 + 0x28) = (char)uVar4;
        *(char *)((int)param_3 + uVar2 + 0x29) = (char)(uVar4 >> 8);
        *(char *)((int)param_3 + uVar2 + 0x2a) = (char)(uVar4 >> 0x10);
        *(char *)((int)param_3 + uVar2 + 0x2b) = (char)(uVar4 >> 0x18);
      }
      if (param_2 == 0x40) {
        *(uint *)(param_1 + 0xa8) = uVar5;
      }
      else if (param_2 == 0x41) {
        *(uint *)(param_1 + 0xb8) = uVar5;
      }
      else if (param_2 < 0x20) {
        *(int *)(param_1 + 0x98) = 1 << (param_2 & 0xff);
      }
      else {
        *(int *)(param_1 + 0x9c) = 1 << (param_2 & 0x1f);
      }
    }
  }
  return iVar3;
}



/* ===== FUN_00003820 @ 00003820, 78 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_00003820(int param_1,uint param_2)

{
  if ((param_1 != DAT_00003870) && (param_1 != _DAT_00003874)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x8ce);
  }
  if ((param_2 == 0) || ((param_2 | 0x3fffffff) != 0x3fffffff)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x8cf);
  }
  return (*(uint *)(param_1 + 0x50) & param_2) != 0;
}



/* ===== FUN_00003898 @ 00003898, 924 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00003898(int param_1,int *param_2)

{
  undefined4 uVar1;
  int iVar2;
  uint local_20;
  
  uVar1 = 0xfffffffd;
  local_20 = *DAT_00003c34 / 1000;
  if ((param_1 != DAT_00003c38) && (param_1 != _DAT_00003c3c)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x295);
  }
  if (param_2 != (int *)0x0) {
    if ((((*param_2 != 0) && (*param_2 != 1)) && (*param_2 != 2)) &&
       ((*param_2 != 3 && (*param_2 != 4)))) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x298);
    }
    if (((param_2[1] != 0) && (param_2[1] != 0x100)) && (param_2[1] != 0x300)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x299);
    }
    if ((param_2[5] == 0) || (0x200 < (uint)param_2[5])) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x29a);
    }
    if (((uint)param_2[6] < 3) || (0x101 < (uint)param_2[6])) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x29b);
    }
    if (((uint)param_2[7] < 2) || (0x80 < (uint)param_2[7])) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x29c);
    }
    if ((param_2[8] == 0) || (0x80 < (uint)param_2[8])) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x29d);
    }
    if ((*(ushort *)(param_2 + 1) & 0x100) == 0x100) {
      if ((param_2[9] != 0) && (param_2[9] != 0x8000)) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2a0);
      }
      if ((((param_2[0xe] != 0) || (param_2[10] == 0)) || (0x20 < (uint)param_2[10])) &&
         (((param_2[0xe] != 0x800000 || (param_2[10] == 0)) || (2 < (uint)param_2[10])))) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2a1);
      }
      if (((uint)param_2[0xb] < 2) || (0x21 < (uint)param_2[0xb])) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2a3);
      }
      if ((param_2[0xc] == 0) || (0x10 < (uint)param_2[0xc])) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2a4);
      }
      if ((param_2[0xd] == 0) || (0x10 < (uint)param_2[0xd])) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2a5);
      }
      if (param_2[0xe] == 0x800000) {
        if (0x7f < (uint)param_2[0xf]) {
          thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2a7);
        }
        if (0x7f < (uint)param_2[0x10]) {
          thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2a8);
        }
      }
    }
    if ((param_2[2] != 0x40) && (param_2[2] != 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2ab);
    }
    if ((param_2[3] != 0) && (param_2[3] != 0x4000)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2ac);
    }
    if ((param_2[4] != 0x1000) && (param_2[4] != 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2ad);
    }
    if ((param_2[0x22] != 0) && ((uint)*(byte *)(param_2 + 0x12) < (uint)param_2[0x22])) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2b0);
    }
    if ((param_2[0x23] != 0) && ((uint)*(byte *)(param_2 + 0x13) < (uint)param_2[0x23])) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x2b4);
    }
    iVar2 = 0;
    *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) & 0xffffffef;
    while ((*(uint *)(param_1 + 0x18) & 8) == 8) {
      local_20 = local_20 - 1;
      if (local_20 == 0) {
        iVar2 = -8;
      }
    }
    if (iVar2 == 0) {
      *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) | 1;
      local_20 = *DAT_00003c34 / 1000;
      while ((*(uint *)(param_1 + 0x18) & 1) == 0) {
        local_20 = local_20 - 1;
        if (local_20 == 0) {
          iVar2 = -8;
        }
      }
    }
    if (iVar2 == 0) {
      *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) | 2;
      *(uint *)(param_1 + 0x18) =
           *(uint *)(param_1 + 0x18) & 0xffffacbf |
           (param_2[1] | param_2[2] | param_2[3] | param_2[4]) & 0x5340U;
      *(uint *)(param_1 + 0x1c) =
           (*(ushort *)(param_2 + 5) - 1) * 0x10000 | (param_2[6] + -2) * 0x100 |
           (*(byte *)(param_2 + 8) - 1) * 0x2000000 | param_2[7] - 1U;
      if ((*(ushort *)(param_2 + 1) & 0x100) == 0x100) {
        *(uint *)(param_1 + 0x18) =
             *(uint *)(param_1 + 0x18) & 0xffff7fff | *(ushort *)(param_2 + 9) & 0x8000;
        *(uint *)(param_1 + 0xc) =
             (*(ushort *)(param_2 + 10) - 1) * 0x10000 | (param_2[0xb] + -2) * 0x100 |
             param_2[0xd] - 1U | (param_2[0xc] + -1) * 0x10;
        if (param_2[0xe] == 0x800000) {
          *(uint *)(param_1 + 0xc) = *(uint *)(param_1 + 0xc) | param_2[0xe];
          *(int *)(param_1 + 0x48) = param_2[0x10] | param_2[0xf] << 8;
        }
      }
      FUN_00003fd4(param_1,*param_2);
      FUN_00003d5c(param_1,param_2 + 0x11);
      FUN_00003540(param_1,param_2 + 0x20);
      *(undefined4 *)(param_1 + 0x80) = 0x3f;
    }
    uVar1 = 0;
  }
  return uVar1;
}



/* ===== FUN_00003c60 @ 00003c60, 210 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00003c60(int param_1,uint param_2,int param_3,int param_4)

{
  uint uVar1;
  
  if ((param_1 != DAT_00003d34) && (param_1 != _DAT_00003d38)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0xa4f);
  }
  if ((param_2 == 0) || ((param_2 | 0x3fffffff) != 0x3fffffff)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0xa50);
  }
  if ((param_4 != 0) && (param_4 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0xa51);
  }
  if (param_4 == 1) {
    if ((param_3 != 1) && (param_3 != 2)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0xa54);
    }
    *(uint *)(param_1 + 0x54) = *(uint *)(param_1 + 0x54) | param_2;
    if (param_3 == 1) {
      *(uint *)(param_1 + 0x58) = *(uint *)(param_1 + 0x58) & ~param_2;
      *(uint *)(param_1 + 0x5c) = *(uint *)(param_1 + 0x5c) | 1;
    }
    else {
      *(uint *)(param_1 + 0x58) = *(uint *)(param_1 + 0x58) | param_2;
      *(uint *)(param_1 + 0x5c) = *(uint *)(param_1 + 0x5c) | 2;
    }
  }
  else {
    *(uint *)(param_1 + 0x54) = *(uint *)(param_1 + 0x54) & ~param_2;
    uVar1 = *(uint *)(param_1 + 0x58);
    if ((*(uint *)(param_1 + 0x54) | uVar1) == uVar1) {
      *(uint *)(param_1 + 0x5c) = *(uint *)(param_1 + 0x5c) & 0xfffffffe;
    }
    if ((*(uint *)(param_1 + 0x54) & uVar1) == 0) {
      *(uint *)(param_1 + 0x5c) = *(uint *)(param_1 + 0x5c) & 0xfffffffd;
    }
  }
  return;
}



/* ===== FUN_00003d5c @ 00003d5c, 588 bytes ===== */

void FUN_00003d5c(int param_1,uint *param_2)

{
  uint *puVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined4 *puVar6;
  uint uVar7;
  
  uVar2 = *param_2;
  uVar7 = param_2[10] + param_2[0xb];
  if (((uVar2 & 3) != 0) || (0x7ff < uVar2)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1b0);
  }
  if (0x80 < param_2[1]) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1b1);
  }
  if (0x40 < param_2[2]) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1b2);
  }
  if (0x40 < param_2[3]) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1b3);
  }
  if (0x40 < param_2[5]) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1b4);
  }
  if (0x40 < param_2[7]) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1b5);
  }
  if (0x20 < param_2[9]) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1b6);
  }
  if (0x20 < uVar7) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1b7);
  }
  puVar1 = (uint *)(param_1 + 0x84);
  *puVar1 = uVar2;
  *puVar1 = *puVar1 | (uint)(ushort)param_2[1] << 0x10;
  uVar2 = uVar2 + param_2[1] * 4;
  *(uint *)(param_1 + 0x88) = uVar2 | (uint)(ushort)param_2[2] << 0x10;
  uVar2 = uVar2 + param_2[2] * 8;
  *(uint *)(param_1 + 0xa0) = uVar2 | (uint)(ushort)param_2[3] << 0x10;
  iVar4 = DAT_00003fc8;
  uVar2 = param_2[3] * (uint)*(byte *)(DAT_00003fc8 + param_2[4]) + uVar2;
  *(uint *)(param_1 + 0xb0) = uVar2 | (uint)(ushort)param_2[5] << 0x10;
  iVar3 = param_2[5] * (uint)*(byte *)(iVar4 + param_2[6]) + uVar2;
  *(int *)(param_1 + 0xac) = iVar3;
  uVar2 = param_2[7] * (uint)*(byte *)(iVar4 + param_2[8]) + iVar3;
  *(uint *)(param_1 + 0xf0) = uVar2 | (uint)(ushort)param_2[9] << 0x10;
  uVar2 = uVar2 + param_2[9] * 8;
  *(uint *)(param_1 + 0xc0) =
       (uint)(byte)param_2[0xb] << 0x18 | (uint)(ushort)param_2[10] << 0x10 | uVar2;
  iVar4 = uVar7 * *(byte *)(iVar4 + param_2[0xd]) + uVar2;
  param_2[0xe] = iVar4 - *param_2;
  puVar6 = (undefined4 *)(iVar4 + DAT_00003fcc);
  if (DAT_00003fd0 < puVar6) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1dd);
  }
  if (param_2[3] != 0) {
    if (7 < param_2[4]) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1e1);
    }
    *(uint *)(param_1 + 0xbc) = *(uint *)(param_1 + 0xbc) & 0xfffffff8 | (byte)param_2[4] & 7;
  }
  if (param_2[5] != 0) {
    if (7 < param_2[6]) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1e5);
    }
    *(uint *)(param_1 + 0xbc) = *(uint *)(param_1 + 0xbc) & 0xffffff8f | ((byte)param_2[6] & 7) << 4
    ;
  }
  if (param_2[7] != 0) {
    if (7 < param_2[8]) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1e9);
    }
    *(uint *)(param_1 + 0xbc) = *(uint *)(param_1 + 0xbc) & 0xfffff8ff | ((byte)param_2[8] & 7) << 8
    ;
  }
  if (uVar7 != 0) {
    if (7 < param_2[0xd]) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0x1ef);
    }
    *(uint *)(param_1 + 200) = param_2[0xd];
    if (param_2[0xb] != 0) {
      if ((param_2[0xc] != 0) && (param_2[0xc] != 0x40000000)) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",499);
      }
      *(uint *)(param_1 + 0xc0) = *(uint *)(param_1 + 0xc0) & 0xbfffffff | param_2[0xc] & 0x40000000
      ;
    }
  }
  for (puVar5 = (undefined4 *)(*param_2 + DAT_00003fcc); puVar5 < puVar6; puVar5 = puVar5 + 1) {
    *puVar5 = 0;
  }
  return;
}



/* ===== FUN_00003fd4 @ 00003fd4, 96 bytes ===== */

void FUN_00003fd4(int param_1,int param_2)

{
  *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) & 0xffffff5b;
  *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) & 0xffffffef;
  if (param_2 == 1) {
    *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) | 4;
  }
  else if (param_2 == 2) {
    *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) | 0x20;
  }
  else if (param_2 == 3) {
    *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) | 0xa0;
    *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) | 0x10;
  }
  else if (param_2 == 4) {
    *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) | 0x80;
    *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) | 0x10;
  }
  return;
}



/* ===== FUN_00004034 @ 00004034, 42 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00004034(int param_1)

{
  if ((param_1 != DAT_00004060) && (param_1 != _DAT_00004064)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",899);
  }
  *(uint *)(param_1 + 0x18) = *(uint *)(param_1 + 0x18) & 0xfffffffe;
  return;
}



/* ===== FUN_00004088 @ 00004088, 116 bytes ===== */

undefined4 FUN_00004088(undefined4 *param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0xfffffffd;
  if (param_1 != (undefined4 *)0x0) {
    *param_1 = 0;
    param_1[1] = 0x300;
    param_1[2] = 0;
    param_1[3] = 0;
    param_1[4] = 0;
    param_1[5] = 1;
    param_1[6] = 0x40;
    param_1[7] = 0x10;
    param_1[8] = 0x10;
    param_1[9] = 0;
    param_1[10] = 1;
    param_1[0xb] = 8;
    param_1[0xc] = 2;
    param_1[0xd] = 2;
    param_1[0xe] = 0;
    param_1[0xf] = 0;
    param_1[0x10] = 0;
    param_1[0x11] = 0;
    param_1[0x12] = 0;
    param_1[0x13] = 0;
    param_1[0x14] = 0;
    param_1[0x15] = 0;
    param_1[0x16] = 0;
    param_1[0x17] = 0;
    param_1[0x18] = 0;
    param_1[0x19] = 0;
    param_1[0x1a] = 0;
    param_1[0x1b] = 0;
    param_1[0x1c] = 0;
    param_1[0x1d] = 0;
    param_1[0x1e] = 0;
    param_1[0x20] = 0;
    param_1[0x21] = 0;
    param_1[0x22] = 0;
    param_1[0x23] = 0;
    uVar1 = 0;
  }
  return uVar1;
}



/* ===== FUN_000040fc @ 000040fc, 230 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000040fc(int param_1,uint param_2,uint param_3,int param_4)

{
  uint uVar1;
  
  if ((param_1 != DAT_000041e4) && (param_1 != _DAT_000041e8)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0xa8b);
  }
  uVar1 = ((*(uint *)(param_1 + 0xc0) & 0x3fffff) >> 0x10) +
          ((*(uint *)(param_1 + 0xc0) & 0x3fffffff) >> 0x18);
  if ((uVar1 < 0x20) && ((param_2 == 0 || (((1 << uVar1) - 1U | param_2) != (1 << uVar1) - 1U)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0xa90);
  }
  if ((param_3 == 0) || ((param_3 | 0x600) != 0x600)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0xa93);
  }
  if ((param_4 != 0) && (param_4 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_mcan.c",0xa94);
  }
  if (param_4 == 1) {
    if ((param_3 & 0x200) != 0) {
      *(uint *)(param_1 + 0xe0) = *(uint *)(param_1 + 0xe0) | param_2;
    }
    if ((param_3 & 0x400) != 0) {
      *(uint *)(param_1 + 0xe4) = *(uint *)(param_1 + 0xe4) | param_2;
    }
  }
  else {
    if ((param_3 & 0x200) != 0) {
      *(uint *)(param_1 + 0xe0) = *(uint *)(param_1 + 0xe0) & ~param_2;
    }
    if ((param_3 & 0x400) != 0) {
      *(uint *)(param_1 + 0xe4) = *(uint *)(param_1 + 0xe4) & ~param_2;
    }
  }
  return;
}



/* ===== FUN_0000420c @ 0000420c, 212 bytes ===== */

void FUN_0000420c(int param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  uint uVar4;
  
  for (uVar4 = 0; uVar4 < 4; uVar4 = uVar4 + 1 & 0xff) {
    bVar3 = *(byte *)(param_1 + uVar4 * 4);
    bVar1 = *(byte *)(param_1 + uVar4 * 4) ^ *(byte *)(param_1 + uVar4 * 4 + 1) ^
            *(byte *)(param_1 + uVar4 * 4 + 2) ^ *(byte *)(param_1 + uVar4 * 4 + 3);
    bVar2 = FUN_000071d0(*(byte *)(param_1 + uVar4 * 4) ^ *(byte *)(param_1 + uVar4 * 4 + 1));
    *(byte *)(param_1 + uVar4 * 4) = *(byte *)(param_1 + uVar4 * 4) ^ bVar2 ^ bVar1;
    bVar2 = FUN_000071d0(*(byte *)(param_1 + uVar4 * 4 + 1) ^ *(byte *)(param_1 + uVar4 * 4 + 2));
    *(byte *)(param_1 + uVar4 * 4 + 1) = *(byte *)(param_1 + uVar4 * 4 + 1) ^ bVar2 ^ bVar1;
    bVar2 = FUN_000071d0(*(byte *)(param_1 + uVar4 * 4 + 2) ^ *(byte *)(param_1 + uVar4 * 4 + 3));
    *(byte *)(param_1 + uVar4 * 4 + 2) = *(byte *)(param_1 + uVar4 * 4 + 2) ^ bVar2 ^ bVar1;
    bVar3 = FUN_000071d0(*(byte *)(param_1 + uVar4 * 4 + 3) ^ bVar3);
    *(byte *)(param_1 + uVar4 * 4 + 3) = *(byte *)(param_1 + uVar4 * 4 + 3) ^ bVar3 ^ bVar1;
  }
  return;
}



/* ===== FUN_000042e0 @ 000042e0, 18 bytes ===== */

void FUN_000042e0(ushort param_1)

{
  *(ushort *)(DAT_000042f8 + 0x3fe) = *DAT_000042f4 & ~param_1 | 0xa500;
  return;
}



/* ===== FUN_000042fc @ 000042fc, 14 bytes ===== */

void FUN_000042fc(ushort param_1)

{
  *(ushort *)(DAT_00004310 + 0x3fe) = *DAT_0000430c | param_1;
  return;
}



/* ===== FUN_00004314 @ 00004314, 820 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_00004314(uint param_1,uint param_2)

{
  bool bVar1;
  int *piVar2;
  byte *pbVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  
  bVar1 = false;
  iVar4 = *_DAT_00004648;
  iVar5 = _DAT_00004648[1];
  iVar6 = _DAT_00004648[2];
  iVar7 = _DAT_00004648[3];
  if (((((param_2 & 0x7000000) != 0) && ((param_2 & 0x7000000) != 0x1000000)) &&
      ((param_2 & 0x7000000) != 0x2000000)) &&
     ((((param_2 & 0x7000000) != 0x3000000 && ((param_2 & 0x7000000) != 0x4000000)) &&
      (((param_2 & 0x7000000) != 0x5000000 && ((param_2 & 0x7000000) != 0x6000000)))))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x24b);
  }
  if ((((param_2 & 0x70) != 0) && ((param_2 & 0x70) != 0x10)) &&
     (((param_2 & 0x70) != 0x20 &&
      (((((param_2 & 0x70) != 0x30 && ((param_2 & 0x70) != 0x40)) && ((param_2 & 0x70) != 0x50)) &&
       ((param_2 & 0x70) != 0x60)))))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x24c);
  }
  if (((((param_2 & 0x70000) != 0) && ((param_2 & 0x70000) != 0x10000)) &&
      (((param_2 & 0x70000) != 0x20000 &&
       (((param_2 & 0x70000) != 0x30000 && ((param_2 & 0x70000) != 0x40000)))))) &&
     (((param_2 & 0x70000) != 0x50000 && ((param_2 & 0x70000) != 0x60000)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x24d);
  }
  if (((((param_2 & 0x700000) != 0) && ((param_2 & 0x700000) != 0x100000)) &&
      ((param_2 & 0x700000) != 0x200000)) &&
     ((((param_2 & 0x700000) != 0x300000 && ((param_2 & 0x700000) != 0x400000)) &&
      (((param_2 & 0x700000) != 0x500000 && ((param_2 & 0x700000) != 0x600000)))))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x24e);
  }
  if (((((param_2 & 7) != 0) && ((param_2 & 7) != 1)) &&
      (((param_2 & 7) != 2 &&
       ((((param_2 & 7) != 3 && ((param_2 & 7) != 4)) && ((param_2 & 7) != 5)))))) &&
     ((param_2 & 7) != 6)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x24f);
  }
  if ((((param_2 & 0x700) != 0) && ((param_2 & 0x700) != 0x100)) &&
     (((((param_2 & 0x700) != 0x200 &&
        (((param_2 & 0x700) != 0x300 && ((param_2 & 0x700) != 0x400)))) &&
       ((param_2 & 0x700) != 0x500)) && ((param_2 & 0x700) != 0x600)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x250);
  }
  if ((((((param_2 & 0x7000) != 0) && ((param_2 & 0x7000) != 0x1000)) &&
       ((param_2 & 0x7000) != 0x2000)) &&
      (((param_2 & 0x7000) != 0x3000 && ((param_2 & 0x7000) != 0x4000)))) &&
     (((param_2 & 0x7000) != 0x5000 && ((param_2 & 0x7000) != 0x6000)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x251);
  }
  if ((DAT_00004668 & param_1) == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x252);
  }
  if ((*DAT_0000466c & 1) == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x253);
  }
  if ((*DAT_00004670 & 7) == 5) {
    bVar1 = true;
    if ((_DAT_00004648[4] & 1U) == 0) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x25a);
    }
    iVar8 = DAT_00004674;
    piVar2 = _DAT_00004648;
    *_DAT_00004648 = DAT_00004674;
    iVar8 = iVar8 >> 0xb;
    piVar2[1] = iVar8;
    piVar2[2] = iVar8;
    piVar2[3] = iVar8;
    FUN_00000dec((*DAT_00004678 >> ((*(uint *)(DAT_00004670 + -6) & 0x7ffffff) >> 0x18)) / 50000);
  }
  pbVar3 = DAT_00004670;
  *(uint *)(DAT_00004670 + -6) = *(uint *)(DAT_00004670 + -6) & ~param_1 | param_2 & param_1;
  FUN_00000dec((*DAT_00004678 >> ((*(uint *)(pbVar3 + -6) & 0x7ffffff) >> 0x18)) / 50000);
  piVar2 = _DAT_00004648;
  if (bVar1) {
    *_DAT_00004648 = iVar4;
    piVar2[1] = iVar5;
    piVar2[2] = iVar6;
    piVar2[3] = iVar7;
    FUN_00000dec((*DAT_00004678 >> ((*(uint *)(DAT_00004670 + -6) & 0x7ffffff) >> 0x18)) / 50000);
  }
  return CONCAT44(iVar6,iVar7);
}



/* ===== FUN_0000467c @ 0000467c, 270 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined8 FUN_0000467c(int param_1)

{
  bool bVar1;
  int *piVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  
  bVar1 = false;
  iVar3 = *_DAT_0000478c;
  iVar4 = _DAT_0000478c[1];
  iVar5 = _DAT_0000478c[2];
  iVar6 = _DAT_0000478c[3];
  if (((((param_1 != 0) && (param_1 != 1)) && (param_1 != 2)) && ((param_1 != 3 && (param_1 != 4))))
     && (param_1 != 5)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x1e1);
  }
  if ((*DAT_000047ac & 1) == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x1e2);
  }
  if (((*DAT_000047b0 & 7) == 5) || (param_1 == 5)) {
    bVar1 = true;
    if ((_DAT_0000478c[4] & 1U) == 0) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_clk.c",0x1ea);
    }
    iVar7 = DAT_000047b4;
    piVar2 = _DAT_0000478c;
    *_DAT_0000478c = DAT_000047b4;
    iVar7 = iVar7 >> 0xb;
    piVar2[1] = iVar7;
    piVar2[2] = iVar7;
    piVar2[3] = iVar7;
    FUN_00000dec((*DAT_000047b8 >> ((*(uint *)(DAT_000047b0 + -6) & 0x7ffffff) >> 0x18)) / 50000);
  }
  *DAT_000047b0 = (byte)param_1;
  FUN_00000dec((*DAT_000047b8 >> ((*(uint *)(DAT_000047b0 + -6) & 0x7ffffff) >> 0x18)) / 50000);
  piVar2 = _DAT_0000478c;
  if (bVar1) {
    *_DAT_0000478c = iVar3;
    piVar2[1] = iVar4;
    piVar2[2] = iVar5;
    piVar2[3] = iVar6;
    FUN_00000dec((*DAT_000047b8 >> ((*(uint *)(DAT_000047b0 + -6) & 0x7ffffff) >> 0x18)) / 50000);
  }
  return CONCAT44(iVar5,iVar6);
}



/* ===== configure_swd_access @ 000047bc, 54 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void configure_swd_access(int param_1)

{
  undefined4 *puVar1;
  
  load_persistent_boot_record();
  if (_DAT_000047f4[4] != param_1) {
    _DAT_000047f4[4] = param_1;
    puVar1 = _DAT_000047f4;
    *_DAT_000047f4 = 1;
    puVar1[1] = 0;
    thunk_EXT_FUN_1fff8000(0x1e000,puVar1,0x14);
  }
  debug_write("Disable The SWD Port! \r\n",0x19);
  return;
}



/* ===== FUN_00004814 @ 00004814, 50 bytes ===== */

void FUN_00004814(int param_1)

{
  undefined1 uVar1;
  
  uVar1 = *(undefined1 *)(param_1 + 1);
  *(undefined1 *)(param_1 + 1) = *(undefined1 *)(param_1 + 5);
  *(undefined1 *)(param_1 + 5) = *(undefined1 *)(param_1 + 9);
  *(undefined1 *)(param_1 + 9) = *(undefined1 *)(param_1 + 0xd);
  *(undefined1 *)(param_1 + 0xd) = uVar1;
  uVar1 = *(undefined1 *)(param_1 + 2);
  *(undefined1 *)(param_1 + 2) = *(undefined1 *)(param_1 + 10);
  *(undefined1 *)(param_1 + 10) = uVar1;
  uVar1 = *(undefined1 *)(param_1 + 6);
  *(undefined1 *)(param_1 + 6) = *(undefined1 *)(param_1 + 0xe);
  *(undefined1 *)(param_1 + 0xe) = uVar1;
  uVar1 = *(undefined1 *)(param_1 + 3);
  *(undefined1 *)(param_1 + 3) = *(undefined1 *)(param_1 + 0xf);
  *(undefined1 *)(param_1 + 0xf) = *(undefined1 *)(param_1 + 0xb);
  *(undefined1 *)(param_1 + 0xb) = *(undefined1 *)(param_1 + 7);
  *(undefined1 *)(param_1 + 7) = uVar1;
  return;
}



/* ===== FUN_00004848 @ 00004848, 98 bytes ===== */

undefined4
FUN_00004848(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5
            ,undefined4 param_6)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 uVar3;
  
  iVar2 = -1;
  if ((((param_1 == DAT_000048ac) || (param_1 == DAT_000048b0)) || (param_1 == DAT_000048b4)) ||
     (((param_1 == DAT_000048b8 || (param_1 == DAT_000048bc)) ||
      (uVar3 = param_4, param_1 == DAT_000048c0)))) {
    uVar3 = param_6;
    iVar2 = FUN_000048c4(param_1,param_2,param_3,param_4,param_5,param_6);
  }
  uVar1 = 0;
  if (iVar2 != 0) {
    uVar1 = FUN_00004aa4(param_1,param_2,param_3,param_4,param_6,uVar3);
  }
  return uVar1;
}



/* ===== FUN_000048c4 @ 000048c4, 418 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4
FUN_000048c4(int param_1,uint param_2,uint param_3,uint *param_4,uint *param_5,float *param_6)

{
  longlong lVar1;
  ulonglong uVar2;
  ulonglong uVar3;
  float fVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  bool bVar9;
  undefined8 uVar10;
  undefined8 uVar11;
  undefined4 local_7c;
  undefined4 local_74;
  undefined4 local_70;
  undefined4 uStack_6c;
  undefined4 uStack_68;
  undefined4 uStack_64;
  undefined4 local_60;
  int iStack_5c;
  
  local_70 = *DAT_00004a68;
  uStack_6c = DAT_00004a68[1];
  uStack_68 = DAT_00004a68[2];
  uStack_64 = DAT_00004a68[3];
  local_74 = 0xfffffffd;
  if ((((param_1 != DAT_00004a6c) && (param_1 != DAT_00004a70)) && (param_1 != DAT_00004a74)) &&
     (((param_1 != DAT_00004a78 && (param_1 != DAT_00004a7c)) && (param_1 != _DAT_00004a80)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x2d4);
  }
  bVar9 = param_4 == (uint *)0x0;
  do {
    if (bVar9) {
      return 0xfffffffd;
    }
    bVar9 = true;
  } while (((param_5 == (uint *)0x0) || (bVar9 = true, param_2 == 0)) ||
          (bVar9 = true, param_3 == 0));
  uVar6 = *(uint *)(param_1 + 0x14) & 0xe00000;
  if ((((uVar6 != 0) && (uVar6 != 0x200000)) && (uVar6 != 0x600000)) &&
     ((uVar6 != 0xa00000 && (uVar6 != 0xc00000)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x2db);
  }
  uVar7 = (uint)*(ushort *)((int)&local_70 + (uVar6 >> 0x15) * 2);
  uVar6 = param_2 / (param_3 * uVar7 * 2);
  uVar8 = uVar6 - 1;
  if (uVar8 < 0x100) {
    lVar1 = (ulonglong)uVar7 * 2;
    uVar2 = lVar1 * (ulonglong)uVar6;
    uVar3 = (uVar2 & 0xffffffff) * (ulonglong)param_3;
    iVar5 = ((int)lVar1 * (uint)(0xfffffffe < uVar8) + (int)(uVar2 >> 0x20)) * param_3 +
            (int)(uVar3 >> 0x20);
    lVar1 = (uVar3 & 0xffffffff) * 0x100 + CONCAT44(iVar5 * 0x100,param_2 >> 1);
    local_7c = (undefined4)((ulonglong)lVar1 >> 0x20);
    uVar6 = FUN_0000029c((int)lVar1,local_7c,param_2,0);
    uVar7 = uVar6 - 0x80;
    if (uVar7 < 0x80) {
      *param_4 = uVar8;
      *param_5 = uVar7;
      if (param_6 != (float *)0x0) {
        lVar1 = (uVar3 & 0xffffffff) * 0x100;
        local_60 = (undefined4)((ulonglong)uVar6 * (ulonglong)param_2);
        iStack_5c = (0xffffff7f < uVar7) * param_2 +
                    (int)((ulonglong)uVar6 * (ulonglong)param_2 >> 0x20);
        uVar10 = FUN_0000035a((int)lVar1,iVar5 * 0x100 + (int)((ulonglong)lVar1 >> 0x20));
        uVar11 = FUN_0000035a(local_60,iStack_5c);
        FUN_00000470((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),(int)uVar10,
                     (int)((ulonglong)uVar10 >> 0x20));
        fVar4 = (float)FUN_00000372();
        *param_6 = fVar4 - 1.0;
      }
      local_74 = 0;
    }
  }
  return local_74;
}



/* ===== FUN_00004aa4 @ 00004aa4, 298 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00004aa4(int param_1,int param_2,uint param_3,uint *param_4,float *param_5)

{
  longlong lVar1;
  ulonglong uVar2;
  longlong lVar3;
  float fVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  undefined8 uVar8;
  undefined8 uVar9;
  undefined4 local_68;
  undefined4 local_64;
  undefined4 uStack_60;
  undefined4 uStack_5c;
  undefined4 uStack_58;
  
  local_64 = *DAT_00004bd0;
  uStack_60 = DAT_00004bd0[1];
  uStack_5c = DAT_00004bd0[2];
  uStack_58 = DAT_00004bd0[3];
  local_68 = 0xfffffffd;
  if ((((param_1 != DAT_00004bd4) && (param_1 != DAT_00004bd8)) && (param_1 != DAT_00004bdc)) &&
     (param_1 != _DAT_00004be0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x1fb);
  }
  if (((param_4 != (uint *)0x0) && (param_2 != 0)) && (param_3 != 0)) {
    uVar5 = *(uint *)(param_1 + 0x14) & 0xe00000;
    if (((uVar5 != 0) && (uVar5 != 0x200000)) &&
       ((uVar5 != 0x600000 && ((uVar5 != 0xa00000 && (uVar5 != 0xc00000)))))) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x206);
    }
    uVar6 = (uint)*(ushort *)((int)&local_64 + (uVar5 >> 0x15) * 2);
    uVar5 = ((uint)(param_2 * 10) / (param_3 * uVar6 * 2) + 5) / 10;
    uVar7 = uVar5 - 1;
    if (uVar7 < 0x100) {
      *param_4 = uVar7;
      if (param_5 != (float *)0x0) {
        lVar1 = (ulonglong)uVar6 * 2;
        uVar2 = lVar1 * (ulonglong)uVar5;
        lVar3 = (uVar2 & 0xffffffff) * (ulonglong)param_3;
        uVar8 = FUN_0000035a((int)lVar3,
                             ((int)lVar1 * (uint)(0xfffffffe < uVar7) + (int)(uVar2 >> 0x20)) *
                             param_3 + (int)((ulonglong)lVar3 >> 0x20));
        uVar9 = FUN_00000340(param_2);
        FUN_00000470((int)uVar9,(int)((ulonglong)uVar9 >> 0x20),(int)uVar8,
                     (int)((ulonglong)uVar8 >> 0x20));
        fVar4 = (float)FUN_00000372();
        *param_5 = fVar4 - 1.0;
      }
      local_68 = 0;
    }
  }
  return local_68;
}



/* ===== FUN_00004c04 @ 00004c04, 46 bytes ===== */

void FUN_00004c04(int param_1)

{
  uint uVar1;
  uint uVar2;
  
  for (uVar1 = 0; uVar1 < 4; uVar1 = uVar1 + 1 & 0xff) {
    for (uVar2 = 0; uVar2 < 4; uVar2 = uVar2 + 1 & 0xff) {
      *(undefined1 *)(param_1 + uVar2 * 4 + uVar1) =
           *(undefined1 *)(DAT_00004c34 + (uint)*(byte *)(param_1 + uVar2 * 4 + uVar1));
    }
  }
  return;
}



/* ===== FUN_00004c38 @ 00004c38, 6 bytes ===== */

undefined4 FUN_00004c38(void)

{
  return *DAT_00004c40;
}



/* ===== FUN_00004c44 @ 00004c44, 18 bytes ===== */

void FUN_00004c44(void)

{
  DAT_e000e010 = DAT_e000e010 & 0xfffffffd;
  return;
}



/* ===== FUN_00004c58 @ 00004c58, 160 bytes ===== */

void FUN_00004c58(void)

{
  if ((*DAT_00004d00 & 1) == 0) {
    *DAT_00004d08 = DAT_00004d0c;
  }
  else {
    *DAT_00004d08 = DAT_00004d04;
  }
  if ((*DAT_00004d10 & 7) < 6) {
                    /* WARNING: Could not recover jumptable at 0x00004c7e. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    (*(code *)(&switchD_00004c7e::switchdataD_00004c82 +
              (uint)(&switchD_00004c7e::switchdataD_00004c82)[*DAT_00004d10 & 7] * 2))();
    return;
  }
  return;
}



/* ===== SystemInit @ 00004d20, 28 bytes ===== */

void SystemInit(void)

{
  *DAT_00004d3c = *DAT_00004d3c | 0xf00000;
  FUN_00004c58();
  DAT_00004d3c[-0x20] = 0;
  return;
}



/* ===== FUN_00004d40 @ 00004d40, 130 bytes ===== */

/* WARNING: Removing unreachable block (ram,0x00004d7a) */
/* WARNING: Removing unreachable block (ram,0x00004d84) */
/* WARNING: Removing unreachable block (ram,0x00004d6a) */
/* WARNING: Removing unreachable block (ram,0x00004d60) */

undefined8 FUN_00004d40(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  int local_28;
  undefined4 local_24;
  undefined4 local_20;
  undefined4 local_1c;
  
  local_28 = param_1;
  local_24 = param_2;
  local_20 = param_3;
  local_1c = param_4;
  FUN_00002210(0x1000);
  local_28 = 0x500;
  local_24 = 0x30;
  local_20 = 0;
  local_1c = CONCAT22(local_1c._2_2_,(short)((param_1 + 7U) / 8) + -3);
  FUN_00004f00(DAT_00004dc4,0,&local_28);
  FUN_00004e64(DAT_00004dc4,0,1);
  FUN_00004dc8(DAT_00004dc4,0,1);
  return CONCAT44(local_24,local_28);
}



/* ===== FUN_00004dc8 @ 00004dc8, 114 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00004dc8(int param_1,uint param_2,int param_3)

{
  if ((param_1 != DAT_00004e3c) && (param_1 != _DAT_00004e40)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0x210);
  }
  if ((param_2 != 0) && (param_2 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0x211);
  }
  if ((param_3 != 0) && (param_3 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0x212);
  }
  if (param_3 == 1) {
    *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) | 0x4000 << ((param_2 & 0xf) << 4);
  }
  else {
    *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) & ~(0x4000 << ((param_2 & 0xf) << 4));
  }
  return;
}



/* ===== FUN_00004e64 @ 00004e64, 114 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00004e64(int param_1,uint param_2,int param_3)

{
  if ((param_1 != DAT_00004ed8) && (param_1 != _DAT_00004edc)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0x1de);
  }
  if ((param_2 != 0) && (param_2 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0x1df);
  }
  if ((param_3 != 0) && (param_3 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0x1e0);
  }
  if (param_3 == 1) {
    *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) | 0x1000 << ((param_2 & 0xf) << 4);
  }
  else {
    *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) & ~(0x1000 << ((param_2 & 0xf) << 4));
  }
  return;
}



/* ===== FUN_00004f00 @ 00004f00, 284 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00004f00(int param_1,uint param_2,uint *param_3)

{
  undefined4 uVar1;
  
  uVar1 = 0;
  if (param_3 == (uint *)0x0) {
    uVar1 = 0xfffffffd;
  }
  else {
    if ((param_1 != DAT_0000501c) && (param_1 != _DAT_00005020)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0xbd);
    }
    if ((param_2 != 0) && (param_2 != 1)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0xbe);
    }
    if ((((*param_3 != 0) && (*param_3 != 0x200)) && (*param_3 != 0x100)) && (*param_3 != 0x500)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0xbf);
    }
    if ((((param_3[1] != 0) && (param_3[1] != 0x10)) &&
        ((param_3[1] != 0x20 && ((param_3[1] != 0x30 && (param_3[1] != 0x40)))))) &&
       ((param_3[1] != 0x50 &&
        ((((param_3[1] != 0x60 && (param_3[1] != 0x70)) && (param_3[1] != 0x80)) &&
         ((param_3[1] != 0x90 && (param_3[1] != 0xa0)))))))) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0xc0);
    }
    if ((param_3[2] != 0) && ((undefined *)param_3[2] != &UNK_00008002)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_tmr0.c",0xc1);
    }
    *(undefined4 *)(param_1 + param_2 * 4) = 0;
    *(uint *)(param_1 + 8 + param_2 * 4) = (uint)(ushort)param_3[3];
    *(uint *)(param_1 + 0x10) =
         *(uint *)(param_1 + 0x10) & ~(0x87f2 << ((param_2 & 0xf) << 4)) |
         (*param_3 | param_3[1] | param_3[2]) << ((param_2 & 0xf) << 4) &
         0x87f2 << ((param_2 & 0xf) << 4);
  }
  return uVar1;
}



/* ===== FUN_00005044 @ 00005044, 98 bytes ===== */

undefined4
FUN_00005044(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4,undefined4 param_5
            ,undefined4 param_6)

{
  undefined4 uVar1;
  int iVar2;
  undefined4 uVar3;
  
  iVar2 = -1;
  if ((((param_1 == DAT_000050a8) || (param_1 == DAT_000050ac)) || (param_1 == DAT_000050b0)) ||
     (((param_1 == DAT_000050b4 || (param_1 == DAT_000050b8)) ||
      (uVar3 = param_4, param_1 == DAT_000050bc)))) {
    uVar3 = param_6;
    iVar2 = FUN_000050c0(param_1,param_2,param_3,param_4,param_5,param_6);
  }
  uVar1 = 0;
  if (iVar2 != 0) {
    uVar1 = FUN_00005284(param_1,param_2,param_3,param_4,param_6,uVar3);
  }
  return uVar1;
}



/* ===== FUN_000050c0 @ 000050c0, 396 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4
FUN_000050c0(int param_1,uint param_2,uint param_3,uint *param_4,uint *param_5,float *param_6)

{
  int iVar1;
  ulonglong uVar2;
  ulonglong uVar3;
  longlong lVar4;
  uint uVar5;
  uint uVar6;
  float fVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  bool bVar11;
  undefined8 uVar12;
  undefined8 uVar13;
  undefined4 local_68;
  
  local_68 = 0xfffffffd;
  if (((((param_1 != DAT_0000524c) && (param_1 != DAT_00005250)) && (param_1 != DAT_00005254)) &&
      ((param_1 != DAT_00005258 && (param_1 != DAT_0000525c)))) && (param_1 != _DAT_00005260)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x23d);
  }
  bVar11 = param_4 == (uint *)0x0;
  do {
    if (bVar11) {
      return 0xfffffffd;
    }
    bVar11 = true;
  } while (((param_5 == (uint *)0x0) || (bVar11 = true, param_2 == 0)) ||
          (bVar11 = true, param_3 == 0));
  uVar8 = (uint)((*(uint *)(param_1 + 0xc) & 0x8000) != 0);
  uVar5 = param_2 / ((2 - uVar8) * param_3 * 8);
  uVar6 = uVar5 - 1;
  if (uVar6 < 0x100) {
    uVar2 = (ulonglong)(2 - uVar8) * 8;
    uVar3 = (uVar2 & 0xffffffff) * (ulonglong)uVar5;
    lVar4 = (uVar3 & 0xffffffff) * (ulonglong)param_3;
    uVar10 = (uint)lVar4;
    uVar5 = (((int)uVar2 * (uint)(0xfffffffe < uVar6) +
             ((uint)(2 < uVar8) * -8 + (int)(uVar2 >> 0x20)) * uVar5 + (int)(uVar3 >> 0x20)) *
             param_3 + (int)((ulonglong)lVar4 >> 0x20)) * 0x100 | uVar10 >> 0x18;
    uVar10 = uVar10 * 0x100;
    uVar8 = (param_2 >> 1) + uVar10;
    iVar1 = uVar5 + CARRY4(param_2 >> 1,uVar10);
    if (iVar1 == 0) {
      uVar8 = uVar8 / param_2;
    }
    else {
      uVar8 = FUN_0000029c(uVar8,iVar1,param_2,0);
    }
    uVar9 = uVar8 - 0x80;
    if (uVar9 < 0x80) {
      *param_4 = uVar6;
      *param_5 = uVar9;
      if (param_6 != (float *)0x0) {
        uVar12 = FUN_0000035a(uVar10,uVar5);
        uVar13 = FUN_0000035a((int)((ulonglong)uVar8 * (ulonglong)param_2),
                              (0xffffff7f < uVar9) * param_2 +
                              (int)((ulonglong)uVar8 * (ulonglong)param_2 >> 0x20));
        FUN_00000470((int)uVar13,(int)((ulonglong)uVar13 >> 0x20),(int)uVar12,
                     (int)((ulonglong)uVar12 >> 0x20));
        fVar7 = (float)FUN_00000372();
        *param_6 = fVar7 - 1.0;
      }
      local_68 = 0;
    }
  }
  return local_68;
}



/* ===== FUN_00005284 @ 00005284, 278 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_00005284(int param_1,int param_2,uint param_3,uint *param_4,float *param_5)

{
  ulonglong uVar1;
  ulonglong uVar2;
  longlong lVar3;
  uint uVar4;
  uint uVar5;
  float fVar6;
  uint uVar7;
  undefined8 uVar8;
  undefined8 uVar9;
  undefined4 local_58;
  
  local_58 = 0xfffffffd;
  if (((((param_1 != DAT_0000539c) && (param_1 != DAT_000053a0)) && (param_1 != DAT_000053a4)) &&
      ((param_1 != DAT_000053a8 && (param_1 != DAT_000053ac)))) && (param_1 != _DAT_000053b0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x188);
  }
  if (((param_4 != (uint *)0x0) && (param_2 != 0)) && (param_3 != 0)) {
    uVar4 = (uint)((*(uint *)(param_1 + 0xc) & 0x8000) != 0);
    uVar5 = ((uint)(param_2 * 10) / ((2 - uVar4) * param_3 * 8) + 5) / 10;
    uVar7 = uVar5 - 1;
    if (uVar7 < 0x100) {
      *param_4 = uVar7;
      if (param_5 != (float *)0x0) {
        uVar1 = (ulonglong)(2 - uVar4) * 8;
        uVar2 = (uVar1 & 0xffffffff) * (ulonglong)uVar5;
        lVar3 = (uVar2 & 0xffffffff) * (ulonglong)param_3;
        uVar8 = FUN_0000035a((int)lVar3,
                             ((int)uVar1 * (uint)(0xfffffffe < uVar7) +
                             ((uint)(2 < uVar4) * -8 + (int)(uVar1 >> 0x20)) * uVar5 +
                             (int)(uVar2 >> 0x20)) * param_3 + (int)((ulonglong)lVar3 >> 0x20));
        uVar9 = FUN_00000340(param_2);
        FUN_00000470((int)uVar9,(int)((ulonglong)uVar9 >> 0x20),(int)uVar8,
                     (int)((ulonglong)uVar8 >> 0x20));
        fVar6 = (float)FUN_00000372();
        *param_5 = fVar6 - 1.0;
      }
      local_58 = 0;
    }
  }
  return local_58;
}



/* ===== FUN_000053d4 @ 000053d4, 134 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_000053d4(int param_1,uint param_2)

{
  if (((((param_1 != DAT_0000545c) && (param_1 != DAT_00005460)) && (param_1 != DAT_00005464)) &&
      ((param_1 != DAT_00005468 && (param_1 != DAT_0000546c)))) && (param_1 != _DAT_00005470)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x5ba);
  }
  if ((param_2 == 0) || ((DAT_00005494 | param_2) != DAT_00005494)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x5bb);
  }
  if ((param_2 & 0xb) != 0) {
    *(uint *)(param_1 + 0xc) = *(uint *)(param_1 + 0xc) | (param_2 & 0xb) << 0x10;
  }
  if ((param_2 & 0x100) != 0) {
    *(uint *)(param_1 + 0xc) = *(uint *)(param_1 + 0xc) | 0x100000;
  }
  if ((param_2 & 0x800) != 0) {
    *(uint *)(param_1 + 0xc) = *(uint *)(param_1 + 0xc) | 0x2000000;
  }
  return;
}



/* ===== FUN_00005498 @ 00005498, 290 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00005498(int param_1,uint param_2,int param_3)

{
  uint uVar1;
  
  if (((((param_1 != DAT_000055bc) && (param_1 != DAT_000055c0)) && (param_1 != DAT_000055c4)) &&
      ((param_1 != DAT_000055c8 && (param_1 != DAT_000055cc)))) && (param_1 != _DAT_000055d0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x58d);
  }
  if ((param_3 != 0) && (param_3 != 1)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x58e);
  }
  if ((param_2 == 0) || ((DAT_000055f4 | param_2) != DAT_000055f4)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x58f);
  }
  if (((param_1 != DAT_000055c4) && (param_1 != _DAT_000055d0)) &&
     ((param_1 == DAT_000055c4 || ((param_1 == _DAT_000055d0 || ((DAT_000055f8 & param_2) != 0))))))
  {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x590);
  }
  if (((((param_1 != DAT_000055bc) && (param_1 != DAT_000055c0)) && (param_1 != DAT_000055c8)) &&
      (param_1 != DAT_000055cc)) &&
     (((param_1 == DAT_000055bc || (param_1 == DAT_000055c0)) ||
      ((param_1 == DAT_000055c8 || ((param_1 == DAT_000055cc || ((param_2 & 3) != 0)))))))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x591);
  }
  uVar1 = param_2 & 0xffff;
  if (uVar1 != 0) {
    if (param_3 == 1) {
      *(uint *)(param_1 + 0xc) = *(uint *)(param_1 + 0xc) | uVar1;
    }
    else {
      *(uint *)(param_1 + 0xc) = *(uint *)(param_1 + 0xc) & ~uVar1;
    }
  }
  uVar1 = (DAT_000055f8 & param_2) >> 0x10;
  if (uVar1 != 0) {
    if (param_3 == 1) {
      *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) | uVar1;
    }
    else {
      *(uint *)(param_1 + 0x10) = *(uint *)(param_1 + 0x10) & ~uVar1;
    }
  }
  return;
}



/* ===== FUN_000055fc @ 000055fc, 20 bytes ===== */

undefined8 FUN_000055fc(undefined4 param_1)

{
  return CONCAT44(param_1,*DAT_00005614 >> ((*(uint *)(DAT_00005610 + 0x20) & 0x7f) >> 4));
}



/* ===== FUN_00005618 @ 00005618, 100 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

bool FUN_00005618(uint *param_1,uint param_2)

{
  if (((((param_1 != DAT_0000567c) && (param_1 != DAT_00005680)) && (param_1 != DAT_00005684)) &&
      ((param_1 != DAT_00005688 && (param_1 != DAT_0000568c)))) && (param_1 != _DAT_00005690)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x5a9);
  }
  if ((param_2 == 0) || ((DAT_000056b4 | param_2) != DAT_000056b4)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x5aa);
  }
  return (*param_1 & param_2) != 0;
}



/* ===== FUN_000056b8 @ 000056b8, 112 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_000056b8(int param_1)

{
  uint uVar1;
  uint uVar2;
  
  if ((((param_1 != DAT_00005728) && (param_1 != DAT_0000572c)) && (param_1 != DAT_00005730)) &&
     (((param_1 != DAT_00005734 && (param_1 != DAT_00005738)) && (param_1 != _DAT_0000573c)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x37e);
  }
  uVar1 = FUN_000055fc(param_1);
  if ((*(uint *)(param_1 + 0x18) & 0x10) == 0x10) {
    uVar2 = 0x80 << (*(uint *)(param_1 + 0x18) & 3);
  }
  else {
    uVar2 = 1 << ((*(uint *)(param_1 + 0x18) & 3) << 1);
  }
  return uVar1 / uVar2;
}



/* ===== FUN_00005760 @ 00005760, 60 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined2 FUN_00005760(int param_1)

{
  if ((((param_1 != DAT_0000579c) && (param_1 != DAT_000057a0)) && (param_1 != DAT_000057a4)) &&
     (((param_1 != DAT_000057a8 && (param_1 != DAT_000057ac)) && (param_1 != _DAT_000057b0)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x6d8);
  }
  return *(undefined2 *)(param_1 + 6);
}



/* ===== FUN_000057d4 @ 000057d4, 230 bytes ===== */

int FUN_000057d4(int param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  int iVar2;
  uint local_28;
  uint local_24;
  
  local_24 = 0;
  local_28 = 0xffff;
  if (param_2 == 0) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x711,param_3,param_4,param_1,0);
  }
  if (((((param_1 != DAT_000058dc) && (param_1 != DAT_000058e0)) && (param_1 != DAT_000058e4)) &&
      ((param_1 != DAT_000058e8 && (param_1 != DAT_000058ec)))) && (param_1 != DAT_000058f0)) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x712);
  }
  uVar1 = FUN_000056b8(param_1);
  if ((*(uint *)(param_1 + 0xc) & 0x1000000) == 0) {
    if ((*(uint *)(param_1 + 0x14) & 0x20) == 0) {
      iVar2 = FUN_00005044(param_1,uVar1,param_2,&local_24,&local_28,param_3);
    }
    else {
      iVar2 = FUN_00004848(param_1,uVar1,param_2,&local_24,&local_28,param_3);
    }
  }
  else {
    iVar2 = FUN_000016f4(param_1,uVar1,param_2,&local_24,&local_28,param_3);
  }
  if ((iVar2 == 0) &&
     (*(uint *)(param_1 + 8) = *(uint *)(param_1 + 8) & 0xffff00ff | (local_24 & 0xff) << 8,
     local_28 < 0x80)) {
    *(uint *)(param_1 + 0xc) = *(uint *)(param_1 + 0xc) | 0x20000000;
    *(uint *)(param_1 + 8) = *(uint *)(param_1 + 8) & 0xffffff80 | local_28 & 0x7f;
  }
  return iVar2;
}



/* ===== FUN_000058f4 @ 000058f4, 464 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_000058f4(int param_1,uint *param_2,undefined4 param_3)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  undefined4 uVar4;
  
  uVar4 = 0xfffffffd;
  if (param_2 != (uint *)0x0) {
    if ((((param_1 != DAT_00005ac4) && (param_1 != DAT_00005ac8)) && (param_1 != DAT_00005acc)) &&
       (((param_1 != DAT_00005ad0 && (param_1 != DAT_00005ad4)) && (param_1 != _DAT_00005ad8)))) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x47c);
    }
    if ((*param_2 != 0x1000) && (*param_2 != 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x47d);
    }
    if ((param_2[2] != 0x800) && (param_2[2] != 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x47e);
    }
    if (((param_2[6] != 0x600) && (param_2[6] != 0x400)) && (param_2[6] != 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x47f);
    }
    if ((param_2[4] != 0) && (param_2[4] != 0x1000)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x480);
    }
    if ((param_2[5] != 0) && (param_2[5] != 0x2000)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x481);
    }
    if ((param_2[7] != 0x8000) && (param_2[7] != 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x482);
    }
    if ((param_2[8] != 0x10000000) && (param_2[8] != 0)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x483);
    }
    if ((param_2[9] != 0) && (param_2[9] != 0x80000000)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x484);
    }
    if ((((param_2[10] != 0) && (param_2[10] != 0x200)) && (param_2[10] != 0x100)) &&
       (param_2[10] != 0x300)) {
      thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x485);
    }
    uVar1 = *param_2;
    uVar2 = param_2[2];
    uVar3 = param_2[5];
    if (param_2[10] == 0x200) {
      uVar4 = 0x200;
    }
    else {
      uVar4 = 0;
    }
    *(uint *)(param_1 + 0xc) = param_2[6] | param_2[4] | param_2[8] | param_2[7] | param_2[9];
    *(uint *)(param_1 + 0x10) = uVar1 | 0x600 | uVar2 | uVar3;
    *(undefined4 *)(param_1 + 0x14) = uVar4;
    if (*param_2 == 0) {
      if (0x13 < param_2[1]) {
        thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x493);
      }
      *(uint *)(param_1 + 0x18) = param_2[1];
      uVar4 = FUN_000057d4(param_1,param_2[3],param_3);
    }
    else {
      uVar4 = 0;
    }
  }
  return uVar4;
}



/* ===== FUN_00005afc @ 00005afc, 46 bytes ===== */

undefined4 FUN_00005afc(undefined4 *param_1)

{
  undefined4 uVar1;
  
  uVar1 = 0xfffffffd;
  if (param_1 != (undefined4 *)0x0) {
    *param_1 = 0;
    param_1[1] = 0;
    param_1[2] = 0;
    param_1[3] = 0x2580;
    param_1[4] = 0;
    param_1[5] = 0;
    param_1[6] = 0;
    param_1[7] = 0;
    param_1[8] = 0;
    param_1[9] = 0x80000000;
    param_1[10] = 0x100;
    uVar1 = 0;
  }
  return uVar1;
}



/* ===== FUN_00005b2c @ 00005b2c, 84 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00005b2c(int param_1,uint param_2)

{
  if ((((param_1 != DAT_00005b80) && (param_1 != DAT_00005b84)) && (param_1 != DAT_00005b88)) &&
     (((param_1 != DAT_00005b8c && (param_1 != DAT_00005b90)) && (param_1 != _DAT_00005b94)))) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x6e7);
  }
  if (0x1ff < param_2) {
    thunk_FUN_000003fc("..\\driver\\src\\hc32_ll_usart.c",0x6e8);
  }
  *(short *)(param_1 + 4) = (short)param_2;
  return;
}



/* ===== debug_write @ 00005bb8, 48 bytes ===== */

void debug_write(int param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  
  for (uVar2 = 0; uVar2 < param_2; uVar2 = uVar2 + 1) {
    FUN_00005b2c(DAT_00005be8,*(undefined1 *)(param_1 + uVar2));
    do {
      iVar1 = FUN_00005618(DAT_00005be8,0x80);
    } while (iVar1 == 0);
  }
  return;
}



/* ===== debug_uart_error_irq @ 00005bec, 64 bytes ===== */

void debug_uart_error_irq(void)

{
  int iVar1;
  
  iVar1 = FUN_00005618(DAT_00005c2c,2);
  if (iVar1 == 1) {
    FUN_000053d4(DAT_00005c2c,2);
  }
  iVar1 = FUN_00005618(DAT_00005c2c,1);
  if (iVar1 == 1) {
    FUN_000053d4(DAT_00005c2c,1);
  }
  iVar1 = FUN_00005618(DAT_00005c2c,8);
  if (iVar1 == 1) {
    FUN_000053d4(DAT_00005c2c,8);
  }
  return;
}



/* ===== debug_uart_receive_irq @ 00005c30, 296 bytes ===== */

void debug_uart_receive_irq(void)

{
  ushort uVar1;
  undefined4 *puVar2;
  undefined1 uVar3;
  char cVar4;
  int iVar5;
  uint uVar6;
  
  iVar5 = FUN_00005618(DAT_00005d58,0x20);
  if (iVar5 == 1) {
    uVar3 = FUN_00005760(DAT_00005d58);
    *(undefined1 *)(DAT_00005d5c + (uint)*DAT_00005d60) = uVar3;
    *DAT_00005d60 = *DAT_00005d60 + 1;
    *DAT_00005d64 = 0;
    if (*DAT_00005d68 == '\x01') {
      if ((int)(uint)*DAT_00005d6c < (int)*DAT_00005d70) {
        uVar1 = *DAT_00005d6c;
        *DAT_00005d6c = *DAT_00005d6c + 1;
        *(undefined1 *)(DAT_00005d74 + (uint)uVar1) = uVar3;
      }
      else if ((uint)*DAT_00005d6c == *DAT_00005d70) {
        cVar4 = crc8_maxim(DAT_00005d74,*DAT_00005d70);
        *DAT_00005d78 = cVar4;
        if (*(char *)(DAT_00005d5c + *DAT_00005d70 + 5) == *DAT_00005d78) {
          *DAT_00005d6c = 0;
          *DAT_00005d68 = '\0';
          *DAT_00005d7c = 0;
          aes_update_chunk_transform(DAT_00005d74,DAT_00005d74,*DAT_00005d70);
          thunk_EXT_FUN_1fff8000(*DAT_00005d80,DAT_00005d74,*DAT_00005d70);
          *DAT_00005d80 = *DAT_00005d80 + 0x2000;
          *DAT_00005d84 = *DAT_00005d84 + 1;
          for (uVar6 = 0; uVar6 < 0x10000; uVar6 = uVar6 + 1) {
          }
          debug_printf("Pack %d pass:",*DAT_00005d84);
          *DAT_00005d98 = 1;
          debug_status_callback();
          if (*DAT_00005d9c == 0) {
            debug_printf("\r\n   Update firmware completed!!!\r\n\r\n\r\n");
            *DAT_00005dc8 = 1;
          }
        }
        else {
          debug_printf("Pack %d fail:",*DAT_00005d84);
          debug_error_callback();
        }
      }
    }
    else {
      debug_command_handler(DAT_00005d5c,*DAT_00005d60 - 1);
    }
    puVar2 = DAT_00005ddc;
    *DAT_00005ddc = 0;
    puVar2[4] = puVar2[4] | 1;
  }
  return;
}



/* ===== FUN_00005de0 @ 00005de0, 24 bytes ===== */

void FUN_00005de0(void)

{
  int iVar1;
  
  iVar1 = DAT_00005df8;
  *(uint *)(DAT_00005df8 + 0x10) = *(uint *)(DAT_00005df8 + 0x10) & 0xfffffffe;
  FUN_000053d4(DAT_00005dfc,iVar1 >> 0x16);
  return;
}



/* ===== debug_printf @ 00005e00, 22 bytes ===== */

void debug_printf(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_00005f2c(param_1,&uStack_c,DAT_00005e1c,DAT_00005e18);
  return;
}



/* ===== FUN_00005e20 @ 00005e20, 34 bytes ===== */

undefined4 FUN_00005e20(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uVar1;
  undefined4 uStack_10;
  undefined4 local_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_10 = param_1;
  local_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  uVar1 = FUN_00005f2c(param_2,&uStack_8,&uStack_10,DAT_00005e44);
  FUN_00006382(0,&uStack_10);
  return uVar1;
}



/* ===== FUN_00005e48 @ 00005e48, 22 bytes ===== */

void FUN_00005e48(uint param_1)

{
  if (-1 < (int)param_1) {
    *(int *)(DAT_00005e60 + (param_1 >> 5) * 4) = 1 << (param_1 & 0x1f);
  }
  return;
}



/* ===== FUN_00005e64 @ 00005e64, 22 bytes ===== */

void FUN_00005e64(uint param_1)

{
  if (-1 < (int)param_1) {
    *(int *)(DAT_00005e7c + (param_1 >> 5) * 4) = 1 << (param_1 & 0x1f);
  }
  return;
}



/* ===== FUN_00005e80 @ 00005e80, 22 bytes ===== */

void FUN_00005e80(uint param_1)

{
  if (-1 < (int)param_1) {
    *(int *)(DAT_00005e98 + (param_1 >> 5) * 4) = 1 << (param_1 & 0x1f);
  }
  return;
}



/* ===== FUN_00005e9c @ 00005e9c, 30 bytes ===== */

void FUN_00005e9c(uint param_1)

{
  if (-1 < (int)param_1) {
    *(int *)(DAT_00005ebc + (param_1 >> 5) * 4) = 1 << (param_1 & 0x1f);
    DataSynchronizationBarrier(0xf);
    InstructionSynchronizationBarrier(0xf);
  }
  return;
}



/* ===== FUN_00005ec0 @ 00005ec0, 34 bytes ===== */

void FUN_00005ec0(uint param_1)

{
  if (-1 < (int)param_1) {
    *(int *)(&DAT_e000e100 + (param_1 >> 5) * 4) = 1 << (param_1 & 0x1f);
  }
  return;
}



/* ===== FUN_00005ee4 @ 00005ee4, 32 bytes ===== */

void FUN_00005ee4(uint param_1,uint param_2)

{
  if ((int)param_1 < 0) {
    *(char *)(DAT_00005f08 + ((param_1 & 0xf) - 4)) = (char)((param_2 & 0xf) << 4);
  }
  else {
    *(char *)(DAT_00005f04 + param_1) = (char)((param_2 & 0xf) << 4);
  }
  return;
}



/* ===== thunk_FUN_00005f14 @ 00005f0c, 2 bytes ===== */

void thunk_FUN_00005f14(undefined4 *param_1,undefined4 *param_2,int param_3)

{
  undefined4 uVar1;
  
  for (; param_3 != 0; param_3 = param_3 + -4) {
    uVar1 = *param_1;
    param_1 = param_1 + 1;
    *param_2 = uVar1;
    param_2 = param_2 + 1;
  }
  return;
}



/* ===== FUN_00005f14 @ 00005f14, 12 bytes ===== */

void FUN_00005f14(undefined4 *param_1,undefined4 *param_2,int param_3)

{
  undefined4 uVar1;
  
  for (; param_3 != 0; param_3 = param_3 + -4) {
    uVar1 = *param_1;
    param_1 = param_1 + 1;
    *param_2 = uVar1;
    param_2 = param_2 + 1;
  }
  return;
}



/* ===== __iar_zero_init @ 00005f1c, 14 bytes ===== */

void __iar_zero_init(undefined4 param_1,undefined4 *param_2,int param_3)

{
  for (; param_3 != 0; param_3 = param_3 + -4) {
    *param_2 = 0;
    param_2 = param_2 + 1;
  }
  return;
}



/* ===== FUN_00005f2c @ 00005f2c, 984 bytes ===== */

int FUN_00005f2c(byte *param_1,uint *param_2,undefined4 param_3,code *param_4)

{
  byte bVar1;
  uint uVar2;
  int *piVar3;
  undefined2 *puVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  int extraout_r2;
  uint uVar8;
  byte *pbVar9;
  int iVar10;
  uint *puVar11;
  int iVar12;
  uint uVar13;
  uint uVar14;
  int iVar15;
  undefined1 *puVar16;
  bool bVar17;
  longlong lVar18;
  undefined1 local_68;
  undefined1 local_67;
  int local_48;
  byte local_44 [4];
  int *local_40;
  char *local_3c;
  byte *pbStack_34;
  uint *puStack_30;
  undefined4 local_2c;
  code *pcStack_28;
  
  pcStack_28 = param_4;
  local_2c = param_3;
  puStack_30 = param_2;
  pbStack_34 = param_1;
  iVar10 = 0;
  do {
    while( true ) {
      bVar1 = *param_1;
      if (bVar1 == 0) {
        return iVar10;
      }
      if (bVar1 == 0x25) break;
      (*pcStack_28)(bVar1,local_2c);
      param_1 = param_1 + 1;
      iVar10 = iVar10 + 1;
    }
    uVar8 = 0;
    uVar14 = 0;
    uVar13 = 0;
    while( true ) {
      pbVar9 = param_1 + 1;
      uVar2 = 1 << (*pbVar9 - 0x20 & 0xff);
      if ((uVar2 & DAT_00006304) == 0) break;
      uVar8 = uVar8 | uVar2;
      param_1 = pbVar9;
    }
    if (*pbVar9 == 0x2a) {
      uVar14 = *param_2;
      puVar11 = param_2 + 1;
      if ((int)uVar14 < 0) {
        uVar8 = uVar8 | 0x2000;
        uVar14 = -uVar14;
      }
      uVar8 = uVar8 | 2;
      pbVar9 = param_1 + 2;
    }
    else {
      for (; puVar11 = param_2, *pbVar9 - 0x30 < 10; pbVar9 = pbVar9 + 1) {
        uVar8 = uVar8 | 2;
        uVar14 = (uint)*pbVar9 + uVar14 * 10 + -0x30;
      }
    }
    param_1 = pbVar9;
    param_2 = puVar11;
    if (*pbVar9 == 0x2e) {
      param_1 = pbVar9 + 1;
      uVar8 = uVar8 | 4;
      if (*param_1 == 0x2a) {
        param_2 = puVar11 + 1;
        uVar13 = *puVar11;
        param_1 = pbVar9 + 2;
      }
      else {
        for (; *param_1 - 0x30 < 10; param_1 = param_1 + 1) {
          uVar13 = (uint)*param_1 + uVar13 * 10 + -0x30;
        }
      }
    }
    bVar1 = *param_1;
    if (bVar1 == 0x6c) {
      uVar8 = uVar8 | 0x100000;
LAB_00006016:
      if (param_1[1] == bVar1) {
        uVar8 = uVar8 + 0x100000;
        param_1 = param_1 + 1;
      }
LAB_00006022:
      param_1 = param_1 + 1;
    }
    else {
      if (bVar1 < 0x6d) {
        if (bVar1 != 0x4c) {
          if (bVar1 == 0x68) {
            uVar8 = uVar8 | 0x300000;
            goto LAB_00006016;
          }
          if (bVar1 != 0x6a) goto LAB_00006024;
          uVar8 = uVar8 | 0x200000;
        }
        goto LAB_00006022;
      }
      if ((bVar1 == 0x74) || (bVar1 == 0x7a)) goto LAB_00006022;
    }
LAB_00006024:
    bVar1 = *param_1;
    if (bVar1 == 0x6e) {
      uVar14 = (uVar8 & 0x7fffff) >> 0x14;
      if (uVar14 == 2) {
        piVar3 = (int *)*param_2;
        param_2 = param_2 + 1;
        *piVar3 = iVar10;
        piVar3[1] = iVar10 >> 0x1f;
      }
      else if (uVar14 == 3) {
        puVar4 = (undefined2 *)*param_2;
        param_2 = param_2 + 1;
        *puVar4 = (short)iVar10;
      }
      else {
        piVar3 = (int *)*param_2;
        param_2 = param_2 + 1;
        if (uVar14 == 4) {
          *(char *)piVar3 = (char)iVar10;
        }
        else {
          *piVar3 = iVar10;
        }
      }
    }
    else {
      if (bVar1 < 0x6f) {
        if (bVar1 != 99) {
          if (bVar1 < 100) {
            if (bVar1 == 0) {
              return iVar10;
            }
            if (bVar1 == 0x58) {
LAB_0000617c:
              local_48 = 0x10;
              goto LAB_0000619a;
            }
          }
          else if ((bVar1 == 100) || (bVar1 == 0x69)) {
            uVar2 = (uVar8 & 0x7fffff) >> 0x14;
            local_48 = 10;
            if (uVar2 == 2) {
              puVar11 = (uint *)((uint)((int)param_2 + 7) & 0xfffffff8);
              param_2 = puVar11 + 2;
              uVar5 = *puVar11;
              uVar7 = puVar11[1];
            }
            else {
              uVar5 = *param_2;
              param_2 = param_2 + 1;
              if (uVar2 == 3) {
                uVar5 = (uint)(short)uVar5;
              }
              uVar7 = (int)uVar5 >> 0x1f;
              if (uVar2 == 4) {
                uVar5 = (uint)(char)uVar5;
                uVar7 = (int)uVar5 >> 0x1f;
              }
            }
            if ((int)uVar7 < 0) {
              bVar17 = uVar5 != 0;
              uVar5 = -uVar5;
              uVar7 = -(uint)bVar17 - uVar7;
              local_44[0] = 0x2d;
            }
            else if ((int)(uVar8 << 0x14) < 0) {
              local_44[0] = 0x2b;
            }
            else {
              iVar12 = 0;
              if ((uVar8 & 1) == 0) goto LAB_00006220;
              local_44[0] = 0x20;
            }
            iVar12 = 1;
            goto LAB_00006220;
          }
LAB_0000605e:
          (*pcStack_28)(bVar1,local_2c);
          iVar10 = iVar10 + 1;
          goto LAB_00006300;
        }
        local_68 = (char)*param_2;
        local_67 = 0;
        iVar12 = 1;
        puVar16 = &local_68;
LAB_000060ae:
        param_2 = param_2 + 1;
        iVar6 = 0;
        if ((int)(uVar8 << 0x1d) < 0) {
          for (; (iVar6 < (int)uVar13 && ((iVar6 < iVar12 || (puVar16[iVar6] != '\0'))));
              iVar6 = iVar6 + 1) {
          }
        }
        else {
          for (; (iVar6 < iVar12 || (puVar16[iVar6] != '\0')); iVar6 = iVar6 + 1) {
          }
        }
        iVar15 = uVar14 - iVar6;
        iVar12 = FUN_00006354(iVar15,uVar8,local_2c,pcStack_28);
        iVar10 = iVar12 + iVar10 + iVar6;
        while (bVar17 = iVar6 != 0, iVar6 = iVar6 + -1, bVar17) {
          (*pcStack_28)(*puVar16,local_2c);
          puVar16 = puVar16 + 1;
        }
      }
      else {
        if (bVar1 == 0x73) {
          puVar16 = (undefined1 *)*param_2;
          iVar12 = -1;
          goto LAB_000060ae;
        }
        if (bVar1 < 0x74) {
          if (bVar1 == 0x6f) {
            local_48 = 8;
          }
          else {
            if (bVar1 != 0x70) goto LAB_0000605e;
            uVar8 = uVar8 | 4;
            uVar13 = 8;
            local_48 = 0x10;
          }
        }
        else {
          if (bVar1 != 0x75) {
            if (bVar1 == 0x78) goto LAB_0000617c;
            goto LAB_0000605e;
          }
          local_48 = 10;
        }
LAB_0000619a:
        uVar2 = (uVar8 & 0x7fffff) >> 0x14;
        if (uVar2 == 2) {
          puVar11 = (uint *)((uint)((int)param_2 + 7) & 0xfffffff8);
          param_2 = puVar11 + 2;
          uVar5 = *puVar11;
          uVar7 = puVar11[1];
        }
        else {
          uVar5 = *param_2;
          param_2 = param_2 + 1;
          uVar7 = 0;
          if (uVar2 == 3) {
            uVar5 = uVar5 & 0xffff;
          }
          if (uVar2 == 4) {
            uVar5 = uVar5 & 0xff;
          }
        }
        iVar12 = 0;
        if ((int)(uVar8 << 0x1c) < 0) {
          if (*param_1 == 0x70) {
            local_44[0] = 0x40;
            iVar12 = 1;
          }
          else if ((local_48 == 0x10) && (uVar5 != 0 || uVar7 != 0)) {
            local_44[0] = 0x30;
            local_44[1] = *param_1;
            iVar12 = 2;
          }
          if ((local_48 == 8) && ((uVar5 != 0 || uVar7 != 0 || ((int)(uVar8 << 0x1d) < 0)))) {
            local_44[0] = 0x30;
            iVar12 = 1;
            uVar13 = uVar13 - 1;
          }
        }
LAB_00006220:
        lVar18 = CONCAT44(uVar7,uVar5);
        if (*param_1 == 0x58) {
          local_3c = "0123456789ABCDEF";
        }
        else {
          local_3c = "0123456789abcdef";
        }
        local_40 = &local_48;
        while( true ) {
          if (lVar18 == 0) break;
          lVar18 = FUN_0000029c((int)lVar18,(int)((ulonglong)lVar18 >> 0x20),local_48,0);
          local_40 = (int *)((int)local_40 + -1);
          *(char *)local_40 = local_3c[extraout_r2];
        }
        pbVar9 = local_44 + (-4 - (int)local_40);
        if ((int)(uVar8 << 0x1d) < 0) {
          uVar8 = uVar8 & 0xfffeffff;
        }
        else {
          uVar13 = 1;
        }
        if ((int)pbVar9 < (int)uVar13) {
          local_48 = uVar13 - (int)pbVar9;
        }
        else {
          local_48 = 0;
        }
        iVar15 = uVar14 - (int)(pbVar9 + iVar12 + local_48);
        if (-1 < (int)(uVar8 << 0xf)) {
          iVar6 = FUN_00006354(iVar15,uVar8,local_2c,pcStack_28);
          iVar10 = iVar10 + iVar6;
        }
        for (iVar6 = 0; iVar6 < iVar12; iVar6 = iVar6 + 1) {
          (*pcStack_28)(local_44[iVar6],local_2c);
          iVar10 = iVar10 + 1;
        }
        if ((int)(uVar8 << 0xf) < 0) {
          iVar12 = FUN_00006354(iVar15,uVar8,local_2c,pcStack_28);
          iVar10 = iVar10 + iVar12;
        }
        while (iVar12 = local_48 + -1, bVar17 = 0 < local_48, local_48 = iVar12, bVar17) {
          (*pcStack_28)(0x30,local_2c);
          iVar10 = iVar10 + 1;
        }
        while (0 < (int)pbVar9) {
          iVar12 = *local_40;
          local_40 = (int *)((int)local_40 + 1);
          (*pcStack_28)((char)iVar12,local_2c);
          iVar10 = iVar10 + 1;
          pbVar9 = pbVar9 + -1;
        }
      }
      iVar12 = FUN_00006330(iVar15,uVar8,local_2c,pcStack_28);
      iVar10 = iVar10 + iVar12;
    }
LAB_00006300:
    param_1 = param_1 + 1;
  } while( true );
}



/* ===== FUN_00006330 @ 00006330, 36 bytes ===== */

int FUN_00006330(int param_1,int param_2,undefined4 param_3,code *param_4)

{
  int iVar1;
  
  iVar1 = 0;
  if (param_2 << 0x12 < 0) {
    while (param_1 = param_1 + -1, -1 < param_1) {
      (*param_4)(0x20,param_3);
      iVar1 = iVar1 + 1;
    }
  }
  return iVar1;
}



/* ===== FUN_00006354 @ 00006354, 46 bytes ===== */

int FUN_00006354(int param_1,int param_2,undefined4 param_3,code *param_4)

{
  int iVar1;
  undefined4 uVar2;
  
  iVar1 = 0;
  if (param_2 << 0xf < 0) {
    uVar2 = 0x30;
  }
  else {
    uVar2 = 0x20;
  }
  if (-1 < param_2 << 0x12) {
    while (param_1 = param_1 + -1, -1 < param_1) {
      (*param_4)(uVar2,param_3);
      iVar1 = iVar1 + 1;
    }
  }
  return iVar1;
}



/* ===== FUN_00006382 @ 00006382, 10 bytes ===== */

void FUN_00006382(undefined1 param_1,undefined4 *param_2)

{
  undefined1 *puVar1;
  
  puVar1 = (undefined1 *)*param_2;
  *param_2 = puVar1 + 1;
  *puVar1 = param_1;
  return;
}



/* ===== FUN_0000638c @ 0000638c, 44 bytes ===== */

bool FUN_0000638c(int param_1,int param_2,uint param_3)

{
  uint uVar1;
  
  uVar1 = 0;
  while( true ) {
    if ((int)param_3 <= (int)uVar1) {
      return uVar1 == param_3;
    }
    if (*(char *)(param_1 + uVar1) != *(char *)(param_2 + uVar1)) break;
    uVar1 = uVar1 + 1 & 0xff;
  }
  return false;
}



/* ===== boot_hardware_init @ 000063b8, 76 bytes ===== */

void boot_hardware_init(void)

{
  undefined4 *puVar1;
  
  FUN_00002d50(0x46);
  *DAT_00006404 = 0x77;
  DAT_00006404[2] = 0x77;
  puVar1 = DAT_00006404;
  DAT_00006404[-1] = 0x11000000;
  *puVar1 = 0x76;
  DAT_00006404[2] = 0x76;
  FUN_00001634();
  FUN_00004c58();
  FUN_00000a78();
  FUN_00000a18();
  FUN_00000e02(DAT_00006408);
  FUN_00000e18(DAT_0000640c);
  FUN_00000798();
  return;
}



/* ===== clear_boot_request_record @ 00006410, 28 bytes ===== */

void clear_boot_request_record(void)

{
  undefined4 *puVar1;
  
  load_persistent_boot_record();
  puVar1 = DAT_0000642c;
  *DAT_0000642c = 0;
  puVar1[1] = 0;
  thunk_EXT_FUN_1fff8000(0x1e000,puVar1,0x14);
  return;
}



/* ===== erase_flash_sector_at @ 00006430, 82 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void erase_flash_sector_at(uint param_1)

{
  int iVar1;
  
  if (param_1 < 0x40000) {
    disableIRQinterrupts();
    FUN_00001f80();
    FUN_00001ac0(1);
    do {
      iVar1 = FUN_00001b20(0x100);
    } while (iVar1 != 1);
    *DAT_00006484 = 1 << (param_1 >> 0xd & 0xff);
    FUN_00001f94(param_1);
    FUN_00001ac0(0);
    *_DAT_00006488 = 0;
    enableIRQinterrupts();
    debug_write("Code has been deleted!  \r\n",0x1a);
  }
  return;
}



/* ===== debug_putchar @ 000064a8, 24 bytes ===== */

void debug_putchar(byte param_1)

{
  do {
  } while ((*DAT_000064c0 & 0x80) == 0);
  *(ushort *)(DAT_000064c0 + 1) = (ushort)param_1;
  return;
}



/* ===== debug_command_handler @ 000064c4, 248 bytes ===== */

void debug_command_handler(char *param_1,int param_2,undefined4 param_3,undefined4 param_4)

{
  int *piVar1;
  int iVar2;
  int local_18;
  undefined4 uStack_14;
  undefined4 uStack_10;
  
  if (((*DAT_000065bc == '\0') && (param_2 == 0)) && (*param_1 == 'X')) {
    *DAT_000065c0 = 1;
  }
  if (((param_2 == 2) && (*param_1 == '#')) && (param_1[2] == '#')) {
    *DAT_000065c4 = (ushort)(byte)param_1[1];
    *DAT_000065c8 = '\x01';
  }
  piVar1 = DAT_000065d4;
  if (((param_2 == 2) && (*param_1 == 'U')) && ((param_1[1] == 'f' && (param_1[2] == -0x56)))) {
    local_18 = CONCAT31((int3)*(undefined4 *)(DAT_000065cc + 8),param_1[1]);
    uStack_14._1_3_ = (undefined3)*(undefined4 *)(DAT_000065cc + 0xc);
    uStack_14 = CONCAT31(uStack_14._1_3_,(char)((uint)*(undefined4 *)(DAT_000065cc + 8) >> 0x18));
    uStack_10._1_3_ = (undefined3)((uint)param_4 >> 8);
    uStack_10._0_1_ = (undefined1)((uint)*(undefined4 *)(DAT_000065cc + 0xc) >> 0x18);
    debug_write(&local_18,9);
    *DAT_000065d0 = 0;
  }
  else {
    local_18 = param_2;
    uStack_14 = param_3;
    uStack_10 = param_4;
    if ((param_2 == 4) && (*DAT_000065c8 == '\x01')) {
      *DAT_000065d4 = (uint)(byte)param_1[3] + (uint)(byte)param_1[4] * 0x100;
      if ((*piVar1 < 0x2001) && (0 < *piVar1)) {
        *DAT_000065d8 = 1;
      }
      else {
        debug_error_callback();
      }
    }
    else if ((param_2 == 0x26) && (*DAT_000065c8 == '\0')) {
      iVar2 = FUN_0000638c(param_1,DAT_000065dc,0x27);
      if (iVar2 == 0) {
        iVar2 = FUN_0000638c(param_1,DAT_000065f4,0x27);
        if (iVar2 != 0) {
          configure_swd_access(0);
          *DAT_000065f0 = 1;
          debug_write("Enable SWD!  \r\n",0xf);
        }
      }
      else {
        configure_swd_access(1);
        debug_write("Disable SWD! \r\n",0xf);
        *DAT_000065f0 = 1;
      }
    }
  }
  return;
}



/* ===== firmware_update_frame_handler @ 00006608, 378 bytes ===== */

void firmware_update_frame_handler(char *param_1,int param_2)

{
  undefined1 *puVar1;
  int *piVar2;
  int iVar3;
  
  if (((((param_2 == 8) && (*param_1 == 'U')) && (param_1[1] == '\x01')) &&
      ((param_1[2] == '\x02' && (param_1[3] == -0x56)))) &&
     ((uint)*(ushort *)(param_1 + 4) == *DAT_00006784)) {
    *DAT_00006788 = 1;
    param_2 = 0;
  }
  puVar1 = DAT_0000678c;
  if (((((param_2 == 8) && (*param_1 == -0x12)) &&
       ((param_1[1] == '\0' &&
        (((param_1[2] == '\0' && (param_1[3] == '\x11')) && (param_1[4] == '\0')))))) &&
      ((param_1[5] == '\0' && (param_1[6] == '\0')))) && (param_1[7] == '\0')) {
    *DAT_0000678c = 0x11;
    puVar1[1] = *(char *)(DAT_00006790 + 0xc) + '0';
    puVar1[2] = (char)((ushort)*(undefined2 *)(DAT_00006790 + 0xc) >> 8) + '0';
    puVar1[3] = 0xee;
    mcan_send_frame(0x7fe,puVar1,4);
    param_2 = 0;
  }
  puVar1 = DAT_0000678c;
  if (((param_2 == 8) && (*param_1 == 'U')) &&
     ((param_1[1] == '\0' && ((param_1[2] == '\0' && (param_1[3] == -0x56)))))) {
    *DAT_0000678c = 0x55;
    puVar1[1] = (char)*DAT_00006784;
    puVar1[2] = 0;
    puVar1[3] = 0xaa;
    mcan_send_frame(0x7ff,puVar1,4);
    param_2 = 0;
  }
  if (((*param_1 == '#') && (param_1[2] == '#')) && (*DAT_00006794 == '\0')) {
    *DAT_00006798 = (ushort)(byte)param_1[1];
    if ((*DAT_0000679c == 0x20000) && (*DAT_000067a0 == 0)) {
      *DAT_000067a0 = (ushort)(byte)param_1[1];
    }
    *DAT_00006794 = '\x01';
    *DAT_000067a4 = 0;
  }
  piVar2 = DAT_000067ac;
  if ((*DAT_00006794 == '\x01') && (*DAT_000067a8 == '\0')) {
    *DAT_000067ac = (uint)(byte)param_1[3] + (uint)(byte)param_1[4] * 0x100;
    if ((0x2000 < *piVar2) || (*piVar2 < 1)) {
      *DAT_000067a4 = 0;
      return;
    }
    *DAT_000067a8 = '\x01';
    *DAT_000067b0 = 1;
  }
  if ((param_2 == 8) && (*DAT_00006794 == '\0')) {
    iVar3 = FUN_0000638c(param_1,DAT_000067b4,8);
    if (iVar3 == 0) {
      iVar3 = FUN_0000638c(param_1,DAT_000067bc,8);
      if (iVar3 != 0) {
        configure_swd_access(0);
        *DAT_000067b8 = 1;
      }
    }
    else {
      configure_swd_access(1);
      *DAT_000067b8 = 1;
    }
  }
  return;
}



/* ===== prepare_device_key_material @ 000067c0, 42 bytes ===== */

void prepare_device_key_material(void)

{
  int iVar1;
  uint uVar2;
  undefined1 *puVar3;
  
  puVar3 = DAT_000067ec;
  for (uVar2 = 0; iVar1 = DAT_000067f0, uVar2 < 0xc; uVar2 = uVar2 + 1 & 0xff) {
    *(undefined1 *)(DAT_000067f0 + uVar2) = *puVar3;
    puVar3 = puVar3 + 1;
  }
  *(undefined1 *)(DAT_000067f0 + 0xc) = 1;
  *(undefined1 *)(iVar1 + 0xd) = 2;
  *(undefined1 *)(iVar1 + 0xe) = 3;
  *(undefined1 *)(iVar1 + 0xf) = 4;
  return;
}



/* ===== jump_to_application @ 000067f4, 24 bytes ===== */

void jump_to_application(undefined4 *param_1)

{
  *DAT_0000680c = param_1[1];
  FUN_00000272(*param_1);
  (*(code *)*DAT_0000680c)();
  return;
}



/* ===== flash_region_is_programmed @ 00006810, 34 bytes ===== */

undefined4 flash_region_is_programmed(char *param_1)

{
  byte bVar1;
  
  bVar1 = 0;
  while( true ) {
    if (0xf < bVar1) {
      return 0xff;
    }
    if (*param_1 != -1) break;
    bVar1 = bVar1 + 1;
    param_1 = param_1 + 1;
  }
  return 0;
}



/* ===== load_persistent_boot_record @ 00006834, 14 bytes ===== */

void load_persistent_boot_record(void)

{
  FUN_0000031c(DAT_00006848,DAT_00006844,0x14);
  return;
}



/* ===== main @ 0000684c, 648 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void main(void)

{
  char *pcVar1;
  ushort *puVar2;
  char cVar3;
  int iVar4;
  uint uVar5;
  
  boot_hardware_init();
  if (*(int *)(DAT_00006ad4 + 8) == 1) {
    FUN_00002758(2,0x2000);
    delay_ms(0x14);
    FUN_000025d0(1,0);
    FUN_000025d0(2,0);
  }
  iVar4 = flash_region_is_programmed(FUN_00012000,0x14);
  if (iVar4 == 0) {
    iVar4 = authenticate_device_and_print_serial();
    if (iVar4 != 0) {
      authenticate_device_and_print_serial();
      FUN_00012000();
      configure_swd_access(1);
    }
    erase_flash_sector_at(FUN_00012000);
  }
  cVar3 = authenticate_device_key();
  pcVar1 = DAT_00006ad8;
  *DAT_00006ad8 = cVar3;
  if (*pcVar1 != '\0') {
    uVar5 = 0;
    do {
      do {
        FUN_00002890(2,0x2000);
        delay_ms(0x14);
        uVar5 = uVar5 + 1 & 0xffff;
      } while (uVar5 != (uVar5 / 0x32) * 0x32);
      debug_write("The key verification failed. This is a duplicate! \r\n",0x34);
    } while( true );
  }
  FUN_00002558(2,0x2000);
  update_persistent_device_id(DAT_00006b14);
  puVar2 = DAT_00006b18;
  *DAT_00006b18 = *DAT_00006ad4;
  if (*puVar2 == 0) {
    *DAT_00006b34 = 0;
    *DAT_00006b1c = 0;
  }
  else {
    *DAT_00006b1c = 1000;
    FUN_00002758(2,0x2000);
    if (*DAT_00006b18 == 0xffffffff) {
      store_upgrade_complete_record();
    }
    debug_printf("Enter Bootloader!\r\n");
  }
  while (*DAT_00006b38 == '\0') {
    if (*DAT_00006b1c <= *DAT_00006b34 || *DAT_00006b50 != '\0') {
      if ((_DAT_00020004 & 0xff000000) == 0) {
        if ((*(int *)(DAT_00006ad4 + 2) == 0) && (*(int *)DAT_00006ad4 == 1)) {
          disableIRQinterrupts();
          clear_boot_request_record();
          FUN_00004c44();
          for (iVar4 = 0; iVar4 < DAT_00006b54; iVar4 = iVar4 + 1) {
          }
          FUN_00005e9c(1);
          FUN_00005e9c(2);
          FUN_00005e9c(3);
          FUN_00005e9c(0x76);
          jump_to_application(0x20000);
          enableIRQinterrupts();
        }
        else if ((*(int *)(DAT_00006ad4 + 2) == 1) && (*(int *)DAT_00006ad4 == 0)) {
          disableIRQinterrupts();
          FUN_00004c44();
          for (iVar4 = 0; iVar4 < DAT_00006b54; iVar4 = iVar4 + 1) {
          }
          FUN_00005e9c(1);
          FUN_00005e9c(2);
          FUN_00005e9c(3);
          FUN_00005e9c(0x76);
          jump_to_application(0x20000);
          enableIRQinterrupts();
        }
        else if ((*(int *)(DAT_00006ad4 + 2) == 0) && (*(int *)DAT_00006ad4 == 0)) {
          debug_printf("Upgrade failed,please retry!\r\n");
          delay_ms(1000);
        }
      }
      else {
        debug_printf(" Firmware cannot be executed!\r\n");
        delay_ms(1000);
      }
    }
    uVar5 = FUN_00004c38();
    if (*DAT_00006b98 + 10U < uVar5) {
      iVar4 = FUN_00004c38();
      *DAT_00006b98 = iVar4;
      *DAT_00006b34 = *DAT_00006b34 + 1;
      *DAT_00006b9c = *DAT_00006b9c + 1;
      FUN_000003fe(0);
    }
    if ((*DAT_00006ba0 != 0) && (0x96 < *DAT_00006b9c)) {
      FUN_00005e48(0x76);
      FUN_00000798();
      FUN_00005e48(0x76);
      delay_ms(10);
      mcan_send_frame(0x7fe,"TimERROR",8);
      *DAT_00006bb0 = 0;
      *DAT_00006bb4 = 0;
      *DAT_00006bb8 = 0;
      *DAT_00006b9c = 0;
    }
    delay_ms(10);
    *DAT_00006b34 = *DAT_00006b34 + 1;
    FUN_000003fe(0);
  }
  mcan_send_frame(0x7fe,"Aupgrade",8);
  store_upgrade_complete_record();
  delay_ms(10);
  DataSynchronizationBarrier(0xf);
  *DAT_00006b48 = (*DAT_00006b48 & 0x700 | DAT_00006b4c) + 4;
  DataSynchronizationBarrier(0xf);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== aes_update_chunk_transform @ 00006bbc, 46 bytes ===== */

void aes_update_chunk_transform(undefined4 param_1,undefined4 param_2,undefined4 param_3)

{
  if (*DAT_00006bec == '\x01') {
    FUN_00000742(DAT_00006bf8,DAT_00006bf4,DAT_00006bf0);
    *DAT_00006bec = '\0';
  }
  FUN_000006cc(DAT_00006bf8,param_1,param_2,param_3);
  return;
}



/* ===== debug_error_callback @ 00006bfc, 16 bytes ===== */

void debug_error_callback(void)

{
  debug_printf("ERROR\r\n");
  *DAT_00006c14 = 0;
  return;
}



/* ===== debug_status_callback @ 00006c18, 16 bytes ===== */

void debug_status_callback(void)

{
  debug_printf(&DAT_00006c28);
  *DAT_00006c30 = 0;
  return;
}



/* ===== store_upgrade_complete_record @ 00006c34, 28 bytes ===== */

void store_upgrade_complete_record(void)

{
  undefined4 *puVar1;
  
  load_persistent_boot_record();
  puVar1 = DAT_00006c50;
  *DAT_00006c50 = 1;
  puVar1[1] = 0;
  thunk_EXT_FUN_1fff8000(0x1e000,puVar1,0x14);
  return;
}



/* ===== reverse_serial_hex_group @ 00006c54, 72 bytes ===== */

undefined4
reverse_serial_hex_group(int param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  uint uVar1;
  undefined1 auStack_e [2];
  undefined4 local_c;
  
  local_c = param_4;
  for (uVar1 = 0; uVar1 < 4; uVar1 = uVar1 + 1 & 0xff) {
    auStack_e[(4 - uVar1) * 2 + 1] = *(undefined1 *)(param_1 + uVar1 * 2 + 1);
    auStack_e[(4 - uVar1) * 2] = *(undefined1 *)(param_1 + uVar1 * 2);
  }
  for (uVar1 = 0; uVar1 < 8; uVar1 = uVar1 + 1 & 0xff) {
    *(undefined1 *)(param_1 + uVar1) = *(undefined1 *)((int)&local_c + uVar1);
  }
  return 1;
}



/* ===== update_persistent_device_id @ 00006c9c, 60 bytes ===== */

void update_persistent_device_id(byte *param_1)

{
  int iVar1;
  
  if (((uint)param_1[3] << 0x18 | (uint)param_1[2] << 0x10 | (uint)param_1[1] << 8 | (uint)*param_1)
      != *(uint *)(DAT_00006cd8 + 8)) {
    load_persistent_boot_record();
    iVar1 = DAT_00006cdc;
    *(undefined4 *)(DAT_00006cdc + 8) = *(undefined4 *)param_1;
    thunk_EXT_FUN_1fff8000(0x1e000,iVar1,0x14);
  }
  return;
}



/* ===== authenticate_device_and_print_serial @ 00006ce0, 620 bytes ===== */

undefined4 authenticate_device_and_print_serial(void)

{
  undefined4 uVar1;
  int iVar2;
  undefined1 uStack_188;
  undefined1 auStack_187 [11];
  undefined1 uStack_17c;
  undefined1 auStack_17b [11];
  undefined1 uStack_170;
  undefined1 auStack_16f [11];
  undefined1 uStack_164;
  undefined1 auStack_163 [11];
  uint local_158;
  uint uStack_154;
  uint local_150;
  uint uStack_14c;
  undefined1 auStack_148 [32];
  byte local_128;
  byte local_127;
  byte local_126;
  byte local_125;
  byte local_124;
  byte local_123;
  byte local_122;
  byte local_121;
  byte local_120;
  byte local_11f;
  byte local_11e;
  byte local_11d;
  byte local_11c;
  byte local_11b;
  byte local_11a;
  byte local_119;
  undefined1 auStack_118 [260];
  
  iVar2 = 0;
  FUN_0000031c(auStack_148,&DAT_00006f4c,0x20);
  prepare_device_key_material();
  FUN_00000742(auStack_118,auStack_148,DAT_00006f6c);
  FUN_000006cc(auStack_118,&local_128,DAT_00006f70,0x10);
  uStack_154 = (uint)local_121 << 0x18 | (uint)local_122 << 0x10 | (uint)local_123 << 8 |
               (uint)local_124;
  local_158 = (uint)local_125 << 0x18 | (uint)local_126 << 0x10 | (uint)local_127 << 8 |
              (uint)local_128;
  uStack_14c = (uint)local_119 << 0x18 | (uint)local_11a << 0x10 | (uint)local_11b << 8 |
               (uint)local_11c;
  local_150 = (uint)local_11d << 0x18 | (uint)local_11e << 0x10 | (uint)local_11f << 8 |
              (uint)local_120;
  if (local_158 == *DAT_00006f74 && uStack_154 == DAT_00006f74[1]) {
    if (local_150 != DAT_00006f74[2] || uStack_14c != DAT_00006f74[3]) goto LAB_00006e1e;
    debug_write("Key 0 is matched!\r\n",0x13);
LAB_00006e86:
    FUN_00005e20(&uStack_164,&DAT_00006fd4,*(undefined4 *)(DAT_00006fd0 + iVar2 * 0x100));
    FUN_00005e20(&uStack_170,&DAT_00006fd4,*(undefined4 *)(DAT_00006fd0 + iVar2 * 0x100 + 4));
    FUN_00005e20(&uStack_17c,&DAT_00006fd4,*(undefined4 *)(DAT_00006fd0 + iVar2 * 0x100 + 8));
    FUN_00005e20(&uStack_188,&DAT_00006fd4,*(undefined4 *)(DAT_00006fd0 + iVar2 * 0x100 + 0xc));
    reverse_serial_hex_group(auStack_163);
    reverse_serial_hex_group(auStack_16f);
    reverse_serial_hex_group(auStack_17b);
    reverse_serial_hex_group(auStack_187);
    debug_write("The Driver SN is :",0x12);
    debug_write(auStack_163,8);
    debug_write(&DAT_00006ff0,1);
    debug_write(auStack_16f,8);
    debug_write(&DAT_00006ff0,1);
    debug_write(auStack_17b,8);
    debug_write(&DAT_00006ff0,1);
    debug_write(auStack_187,8);
    debug_write(&DAT_00006ff4,2);
    debug_write(DAT_00006fd0 + 0x10,0x30);
    debug_write(&DAT_00006ff4,2);
    uVar1 = 0;
  }
  else {
LAB_00006e1e:
    if (local_158 == DAT_00006f74[0x10] && uStack_154 == DAT_00006f74[0x11]) {
      if (local_150 == DAT_00006f74[0x12] && uStack_14c == DAT_00006f74[0x13]) {
        debug_write("Key 1 is matched!\r\n",0x13);
        iVar2 = 1;
        goto LAB_00006e86;
      }
    }
    if (local_158 == DAT_00006f74[0x20] && uStack_154 == DAT_00006f74[0x21]) {
      if (local_150 == DAT_00006f74[0x22] && uStack_14c == DAT_00006f74[0x23]) {
        debug_write("Key 2 is matched!\r\n",0x13);
        iVar2 = 1;
        goto LAB_00006e86;
      }
    }
    debug_write("Key 0&1&2 do not match!\r\n",0x17);
    uVar1 = 1;
  }
  return uVar1;
}



/* ===== authenticate_device_key @ 00006ff8, 370 bytes ===== */

undefined4 authenticate_device_key(void)

{
  undefined4 uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  undefined1 auStack_148 [32];
  byte local_128;
  byte local_127;
  byte local_126;
  byte local_125;
  byte local_124;
  byte local_123;
  byte local_122;
  byte local_121;
  byte local_120;
  byte local_11f;
  byte local_11e;
  byte local_11d;
  byte local_11c;
  byte local_11b;
  byte local_11a;
  byte local_119;
  undefined1 auStack_118 [260];
  
  FUN_0000031c(auStack_148,&DAT_0000716c,0x20);
  prepare_device_key_material();
  FUN_00000742(auStack_118,auStack_148,DAT_0000718c);
  FUN_000006cc(auStack_118,&local_128,DAT_00007190,0x10);
  uVar2 = (uint)local_121 << 0x18 | (uint)local_122 << 0x10 | (uint)local_123 << 8 | (uint)local_124
  ;
  uVar3 = (uint)local_125 << 0x18 | (uint)local_126 << 0x10 | (uint)local_127 << 8 | (uint)local_128
  ;
  uVar4 = (uint)local_119 << 0x18 | (uint)local_11a << 0x10 | (uint)local_11b << 8 | (uint)local_11c
  ;
  uVar5 = (uint)local_120 | (uint)local_11d << 0x18 | (uint)local_11e << 0x10 | (uint)local_11f << 8
  ;
  if ((uVar3 == *DAT_00007194 && uVar2 == DAT_00007194[1]) &&
     (uVar5 == DAT_00007194[2] && uVar4 == DAT_00007194[3])) {
    uVar1 = 0;
  }
  else if ((uVar3 == DAT_00007194[0x10] && uVar2 == DAT_00007194[0x11]) &&
          (uVar5 == DAT_00007194[0x12] && uVar4 == DAT_00007194[0x13])) {
    uVar1 = 0;
  }
  else if ((uVar3 == DAT_00007194[0x20] && uVar2 == DAT_00007194[0x21]) &&
          (uVar5 == DAT_00007194[0x22] && uVar4 == DAT_00007194[0x23])) {
    uVar1 = 0;
  }
  else {
    uVar1 = 1;
  }
  return uVar1;
}



/* ===== crc8_maxim @ 00007198, 56 bytes ===== */

void crc8_maxim(byte *param_1,int param_2)

{
  uint uVar1;
  byte *pbVar2;
  byte bVar3;
  uint uVar4;
  bool bVar5;
  
  uVar1 = 0;
  while (bVar5 = param_2 != 0, param_2 = param_2 + -1, bVar5) {
    pbVar2 = param_1 + 1;
    uVar1 = uVar1 ^ *param_1;
    for (bVar3 = 0; param_1 = pbVar2, bVar3 < 8; bVar3 = bVar3 + 1) {
      uVar4 = uVar1 & 1;
      uVar1 = (int)uVar1 >> 1;
      if (uVar4 != 0) {
        uVar1 = uVar1 ^ 0x8c;
      }
    }
  }
  return;
}



/* ===== FUN_000071d0 @ 000071d0, 20 bytes ===== */

uint FUN_000071d0(uint param_1)

{
  return ((param_1 >> 7) * 0x1b ^ param_1 << 1) & 0xff;
}



/* ===== FUN_00012000 @ 00012000, 1 bytes ===== */

/* WARNING: Control flow encountered bad instruction data */

void FUN_00012000(void)

{
                    /* WARNING: Bad instruction - Truncating control flow here */
  halt_baddata();
}



/* ===== flash_erase_and_program @ 1fff8000, 156 bytes ===== */

void flash_erase_and_program(uint param_1,int param_2,uint param_3)

{
  int iVar1;
  uint uVar2;
  
  if (param_1 < 0x40000) {
    disableIRQinterrupts();
    thunk_FUN_00001f80();
    thunk_FUN_00001ac0(1);
    do {
      iVar1 = thunk_FUN_00001b20(0x100);
    } while (iVar1 != 1);
    *DAT_1fff8230 = 1 << (param_1 >> 0xd & 0xff);
    thunk_FUN_00001f94(param_1);
    for (uVar2 = 0; uVar2 < (param_3 & 0x3ffff) >> 2; uVar2 = uVar2 + 1 & 0xffff) {
      thunk_FUN_00001e30(param_1,CONCAT13(*(undefined1 *)(param_2 + uVar2 * 4 + 3),
                                          CONCAT12(*(undefined1 *)(param_2 + uVar2 * 4 + 2),
                                                   CONCAT11(*(undefined1 *)(param_2 + uVar2 * 4 + 1)
                                                            ,*(undefined1 *)(param_2 + uVar2 * 4))))
                        );
      param_1 = param_1 + 4;
    }
    thunk_FUN_00001ac0(0);
    *DAT_1fff8234 = 0;
    enableIRQinterrupts();
  }
  return;
}



/* ===== flash_program_and_verify @ 1fff809c, 402 bytes ===== */

undefined4 flash_program_and_verify(int *param_1,undefined4 param_2,int param_3,uint param_4)

{
  int iVar1;
  uint uVar2;
  int *piVar3;
  uint uVar4;
  
  uVar4 = (param_4 & 0x3ffff) >> 2;
  do {
    iVar1 = thunk_FUN_00001b20(0x100);
  } while (iVar1 != 1);
  thunk_FUN_00001f80();
  thunk_FUN_00001ac0(1);
  thunk_FUN_00001de4();
  thunk_FUN_00001b74();
  *(uint *)(DAT_1fff8234 + 0x1c) = (*(uint *)(DAT_1fff8234 + 0x1c) & 0xfffffff8) + 1;
  piVar3 = param_1;
  for (uVar2 = 0; uVar2 < uVar4; uVar2 = uVar2 + 1 & 0xffff) {
    if (param_1 + 0x10 < piVar3) {
      *(uint *)(DAT_1fff8234 + 0x1c) = *(uint *)(DAT_1fff8234 + 0x1c) & 0xfffffff8;
      do {
        iVar1 = thunk_FUN_00001b20(0x100);
      } while (iVar1 != 1);
      *(undefined4 *)(DAT_1fff8234 + 0x24) = 0x10;
      thunk_FUN_00001ac0(0);
      thunk_FUN_00001d98();
      thunk_FUN_00002de0(1);
      return 2;
    }
    *piVar3 = CONCAT13(*(undefined1 *)(param_3 + uVar2 * 4 + 3),
                       CONCAT12(*(undefined1 *)(param_3 + uVar2 * 4 + 2),
                                CONCAT11(*(undefined1 *)(param_3 + uVar2 * 4 + 1),
                                         *(undefined1 *)(param_3 + uVar2 * 4))));
    do {
    } while ((*(uint *)(DAT_1fff8234 + 0x20) & 0x100) == 0);
    piVar3 = piVar3 + 1;
  }
  *(uint *)(DAT_1fff8234 + 0x1c) = *(uint *)(DAT_1fff8234 + 0x1c) & 0xfffffff8;
  do {
    iVar1 = thunk_FUN_00001b20(0x100);
  } while (iVar1 != 1);
  *(undefined4 *)(DAT_1fff8234 + 0x24) = 0x10;
  uVar2 = 0;
  while( true ) {
    if (uVar4 <= uVar2) {
      thunk_FUN_00001c48(param_2);
      *(undefined4 *)(DAT_1fff8234 + 0x24) = 0x10;
      thunk_FUN_00001ac0(0);
      thunk_FUN_00001d98();
      thunk_FUN_00002de0(1);
      return 0;
    }
    if (*param_1 !=
        CONCAT13(*(undefined1 *)(param_3 + uVar2 * 4 + 3),
                 CONCAT12(*(undefined1 *)(param_3 + uVar2 * 4 + 2),
                          CONCAT11(*(undefined1 *)(param_3 + uVar2 * 4 + 1),
                                   *(undefined1 *)(param_3 + uVar2 * 4))))) break;
    param_1 = param_1 + 1;
    uVar2 = uVar2 + 1 & 0xffff;
  }
  *(uint *)(DAT_1fff8234 + 0x1c) = *(uint *)(DAT_1fff8234 + 0x1c) & 0xfffffff8;
  do {
    iVar1 = thunk_FUN_00001b20(0x100);
  } while (iVar1 != 1);
  *(undefined4 *)(DAT_1fff8234 + 0x24) = 0x10;
  thunk_FUN_00001ac0(0);
  thunk_FUN_00001d98();
  thunk_FUN_00002de0(1);
  return 1;
}



/* ===== thunk_FUN_00001f80 @ 1fff8238, 10 bytes ===== */

void thunk_FUN_00001f80(void)

{
  FUN_00001f80();
  return;
}



/* ===== thunk_FUN_00001ac0 @ 1fff8242, 10 bytes ===== */

void thunk_FUN_00001ac0(void)

{
  FUN_00001ac0();
  return;
}



/* ===== thunk_FUN_00001b20 @ 1fff824c, 10 bytes ===== */

void thunk_FUN_00001b20(void)

{
  FUN_00001b20();
  return;
}



/* ===== thunk_FUN_00001f94 @ 1fff8256, 10 bytes ===== */

void thunk_FUN_00001f94(void)

{
  FUN_00001f94();
  return;
}



/* ===== thunk_FUN_00001e30 @ 1fff8260, 10 bytes ===== */

void thunk_FUN_00001e30(void)

{
  FUN_00001e30();
  return;
}



/* ===== thunk_FUN_00001de4 @ 1fff826a, 10 bytes ===== */

void thunk_FUN_00001de4(void)

{
  FUN_00001de4();
  return;
}



/* ===== thunk_FUN_00001b74 @ 1fff8274, 10 bytes ===== */

void thunk_FUN_00001b74(void)

{
  FUN_00001b74();
  return;
}



/* ===== thunk_FUN_00001d98 @ 1fff827e, 10 bytes ===== */

void thunk_FUN_00001d98(void)

{
  FUN_00001d98();
  return;
}



/* ===== thunk_FUN_00002de0 @ 1fff8288, 10 bytes ===== */

void thunk_FUN_00002de0(void)

{
  FUN_00002de0();
  return;
}



/* ===== thunk_FUN_00001c48 @ 1fff8292, 10 bytes ===== */

void thunk_FUN_00001c48(void)

{
  FUN_00001c48();
  return;
}


