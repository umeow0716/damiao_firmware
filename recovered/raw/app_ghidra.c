/* Raw Ghidra decompilation. This is evidence, not hand-cleaned source. */
/* Program: app_mem.bin */


/* ===== __main @ 00020250, 8 bytes ===== */

void __main(void)

{
  int iVar1;
  code *UNRECOVERED_JUMPTABLE;
  int iVar2;
  undefined4 *puVar3;
  
  __scatterload();
  __rt_entry();
  iVar1 = DAT_00020284;
  puVar3 = (undefined4 *)((int)&DAT_00020284 + DAT_00020284);
  iVar2 = DAT_00020284 + 0x20283;
  if (puVar3 == (undefined4 *)((int)&DAT_00020284 + DAT_00020288)) {
    __rt_entry();
  }
  UNRECOVERED_JUMPTABLE = *(code **)(iVar1 + 0x20290);
  if (((uint)UNRECOVERED_JUMPTABLE & 1) != 0) {
    UNRECOVERED_JUMPTABLE = (code *)(iVar2 - (int)UNRECOVERED_JUMPTABLE);
  }
                    /* WARNING: Could not recover jumptable at 0x00020282. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)
            (*puVar3,*(undefined4 *)((int)&DAT_00020288 + iVar1),
             *(undefined4 *)(__scatterload_decompress + iVar1));
  return;
}



/* ===== __scatterload @ 00020258, 44 bytes ===== */

void __scatterload(void)

{
  int iVar1;
  code *UNRECOVERED_JUMPTABLE;
  int iVar2;
  undefined4 *puVar3;
  
  iVar1 = DAT_00020284;
  puVar3 = (undefined4 *)((int)&DAT_00020284 + DAT_00020284);
  iVar2 = DAT_00020284 + 0x20283;
  if (puVar3 == (undefined4 *)((int)&DAT_00020284 + DAT_00020288)) {
    __rt_entry();
  }
  UNRECOVERED_JUMPTABLE = *(code **)(iVar1 + 0x20290);
  if (((uint)UNRECOVERED_JUMPTABLE & 1) != 0) {
    UNRECOVERED_JUMPTABLE = (code *)(iVar2 - (int)UNRECOVERED_JUMPTABLE);
  }
                    /* WARNING: Could not recover jumptable at 0x00020282. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*UNRECOVERED_JUMPTABLE)
            (*puVar3,*(undefined4 *)((int)&DAT_00020288 + iVar1),
             *(undefined4 *)(__scatterload_decompress + iVar1));
  return;
}



/* ===== __scatterload_decompress @ 0002028c, 100 bytes ===== */

void __scatterload_decompress(byte *param_1,byte *param_2,int param_3)

{
  byte bVar1;
  int iVar2;
  byte *pbVar3;
  byte *pbVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  byte *pbVar8;
  uint uVar9;
  
  bVar1 = *param_1;
  pbVar4 = param_2 + param_3;
  while( true ) {
    uVar5 = (uint)bVar1;
    uVar7 = uVar5 & 3;
    pbVar3 = param_1 + 1;
    if ((bVar1 & 3) == 0) {
      pbVar3 = param_1 + 2;
      uVar7 = (uint)param_1[1];
    }
    uVar9 = (int)uVar5 >> 4;
    param_1 = pbVar3;
    if (uVar9 == 0) {
      param_1 = pbVar3 + 1;
      uVar9 = (uint)*pbVar3;
    }
    while (uVar7 = uVar7 - 1, uVar7 != 0) {
      *param_2 = *param_1;
      param_1 = param_1 + 1;
      param_2 = param_2 + 1;
    }
    if (uVar9 != 0) {
      pbVar3 = param_1 + 1;
      bVar1 = *param_1;
      if ((uVar5 & 0xc) == 0xc) {
        param_1 = param_1 + 2;
        iVar2 = (uint)*pbVar3 * -0x100;
      }
      else {
        iVar2 = (uVar5 & 0xc) * -0x40;
        param_1 = pbVar3;
      }
      iVar6 = uVar9 + 1;
      pbVar3 = param_2;
      pbVar8 = param_2 + (iVar2 - (uint)bVar1);
      do {
        iVar6 = iVar6 + -1;
        param_2 = pbVar3 + 1;
        *pbVar3 = *pbVar8;
        pbVar3 = param_2;
        pbVar8 = pbVar8 + 1;
      } while (-1 < iVar6);
    }
    if (pbVar4 <= param_2) break;
    bVar1 = *param_1;
  }
  return;
}



/* ===== __scatterload_copy @ 000202f0, 26 bytes ===== */

void __scatterload_copy(undefined4 *param_1,undefined4 *param_2,uint param_3)

{
  uint uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  
  do {
    uVar1 = param_3;
    param_3 = uVar1 - 0x10;
    if (0xf < uVar1) {
      uVar2 = *param_1;
      uVar3 = param_1[1];
      uVar4 = param_1[2];
      uVar5 = param_1[3];
      param_1 = param_1 + 4;
      *param_2 = uVar2;
      param_2[1] = uVar3;
      param_2[2] = uVar4;
      param_2[3] = uVar5;
      param_2 = param_2 + 4;
    }
  } while (0xf < uVar1 && param_3 != 0);
  if ((param_3 & 8) != 0) {
    uVar2 = *param_1;
    uVar3 = param_1[1];
    param_1 = param_1 + 2;
    *param_2 = uVar2;
    param_2[1] = uVar3;
    param_2 = param_2 + 2;
  }
  if ((int)(uVar1 * 0x20000000) < 0) {
    *param_2 = *param_1;
  }
  return;
}



/* ===== __scatterload_zeroinit @ 0002030c, 28 bytes ===== */

void __scatterload_zeroinit(undefined4 param_1,undefined4 *param_2,uint param_3)

{
  uint uVar1;
  
  do {
    uVar1 = param_3;
    param_3 = uVar1 - 0x10;
    if (0xf < uVar1) {
      *param_2 = 0;
      param_2[1] = 0;
      param_2[2] = 0;
      param_2[3] = 0;
      param_2 = param_2 + 4;
    }
  } while (0xf < uVar1 && param_3 != 0);
  if ((param_3 & 8) != 0) {
    *param_2 = 0;
    param_2[1] = 0;
    param_2 = param_2 + 2;
  }
  if ((int)(uVar1 * 0x20000000) < 0) {
    *param_2 = 0;
  }
  return;
}



/* ===== FUN_00020328 @ 00020328, 1042 bytes ===== */

/* WARNING: Removing unreachable block (ram,0x00020bb4) */
/* WARNING: Removing unreachable block (ram,0x00020c20) */
/* WARNING: Removing unreachable block (ram,0x00020c24) */
/* WARNING: Removing unreachable block (ram,0x00020c34) */
/* WARNING: Removing unreachable block (ram,0x00020c30) */
/* WARNING: Removing unreachable block (ram,0x00020c36) */
/* WARNING: Removing unreachable block (ram,0x00020c50) */
/* WARNING: Removing unreachable block (ram,0x00020c54) */
/* WARNING: Removing unreachable block (ram,0x00020c56) */
/* WARNING: Removing unreachable block (ram,0x00020c58) */
/* WARNING: Removing unreachable block (ram,0x00020c5c) */
/* WARNING: Removing unreachable block (ram,0x00020c76) */
/* WARNING: Removing unreachable block (ram,0x00020c66) */
/* WARNING: Removing unreachable block (ram,0x00020c6a) */
/* WARNING: Removing unreachable block (ram,0x00020c7a) */
/* WARNING: Removing unreachable block (ram,0x00020c84) */
/* WARNING: Removing unreachable block (ram,0x00020c8a) */
/* WARNING: Removing unreachable block (ram,0x00020c7e) */
/* WARNING: Removing unreachable block (ram,0x00020c8c) */
/* WARNING: Removing unreachable block (ram,0x00020bae) */
/* WARNING: Removing unreachable block (ram,0x00020bb6) */
/* WARNING: Removing unreachable block (ram,0x00020bc4) */
/* WARNING: Removing unreachable block (ram,0x00020bc0) */
/* WARNING: Removing unreachable block (ram,0x00020bc8) */
/* WARNING: Removing unreachable block (ram,0x00020c70) */
/* WARNING: Removing unreachable block (ram,0x00020cb4) */
/* WARNING: Removing unreachable block (ram,0x00020cbc) */
/* WARNING: Removing unreachable block (ram,0x00020cc0) */
/* WARNING: Removing unreachable block (ram,0x00020cc2) */
/* WARNING: Removing unreachable block (ram,0x00020cca) */
/* WARNING: Removing unreachable block (ram,0x00020ccc) */
/* WARNING: Removing unreachable block (ram,0x00020ce4) */
/* WARNING: Removing unreachable block (ram,0x00020cf4) */
/* WARNING: Removing unreachable block (ram,0x00020cf0) */
/* WARNING: Removing unreachable block (ram,0x00020cf6) */

undefined4 FUN_00020328(uint *param_1,int param_2,uint *param_3)

{
  undefined1 uVar1;
  uint uVar2;
  undefined4 uVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined *puVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  undefined1 *puVar10;
  undefined1 *puVar11;
  undefined1 auStack_64 [32];
  int iStack_44;
  undefined4 uStack_40;
  int iStack_3c;
  int iStack_38;
  int iStack_34;
  int iStack_30;
  undefined1 uStack_25;
  
  if (param_2 == 0x66) {
    puVar5 = (undefined4 *)((int)param_3 + 7U & 0xfffffff8);
    uStack_40 = *puVar5;
    iStack_3c = puVar5[1];
    iVar4 = FUN_000237c0();
    if (iStack_3c < 0) {
      iStack_44 = 0x2d;
    }
    else if ((int)(*param_1 << 0x1e) < 0) {
      iStack_44 = 0x2b;
    }
    else {
      iStack_44 = (*param_1 & 4) << 3;
    }
    if ((iVar4 == 3) || (6 < iVar4)) {
      FUN_00020f0c(param_1,0x66,iVar4,iStack_44);
    }
    else {
      if ((int)((uint)(byte)*param_1 << 0x1a) < 0) {
        uVar2 = param_1[7];
      }
      else {
        uVar2 = 6;
      }
      FUN_00020996(&iStack_38,auStack_64,&uStack_40,uVar2,1);
      iVar7 = 0;
      iVar4 = iStack_34;
      if (iStack_30 == 0) {
        iVar4 = iStack_38 + uVar2 + 1;
      }
      if (-1 < (int)(uVar2 - iVar4)) {
        iVar7 = -1 - (uVar2 - iVar4);
        iVar4 = uVar2 + 1;
      }
      iVar9 = iVar4 - uVar2;
      if ((-1 < (int)((uint)(byte)*param_1 << 0x1c)) && (iVar4 <= iVar9)) {
        iVar9 = -1;
      }
      puVar11 = &uStack_25;
      uStack_25 = 0;
      puVar10 = &uStack_25 + -(int)puVar11;
      param_1[6] = (param_1[6] - (int)(puVar10 + (uint)(iStack_44 != 0) + iVar4 + (iVar9 >> 0x1f)))
                   - 1;
      if (-1 < (int)((uint)(byte)*param_1 << 0x1b)) {
        FUN_0002048c(param_1);
      }
      if (iStack_44 != 0) {
        (*(code *)param_1[1])(iStack_44,param_1[2]);
        param_1[8] = param_1[8] + 1;
      }
      if ((int)((uint)(byte)*param_1 << 0x1b) < 0) {
        FUN_0002048c(param_1);
      }
      while (iVar8 = iVar4 + -1, 0 < iVar4) {
        if ((iVar7 < 0) || (iStack_34 <= iVar7)) {
          uVar1 = 0x30;
        }
        else {
          uVar1 = auStack_64[iVar7];
        }
        (*(code *)param_1[1])(uVar1,param_1[2]);
        param_1[8] = param_1[8] + 1;
        iVar7 = iVar7 + 1;
        iVar9 = iVar9 + -1;
        iVar4 = iVar8;
        if (iVar9 == 0) {
          iVar8 = FUN_00020e0c();
          (*(code *)param_1[1])
                    (*(undefined1 *)((int)*(int **)(iVar8 + 0xc) + **(int **)(iVar8 + 0xc)),
                     param_1[2]);
          param_1[8] = param_1[8] + 1;
        }
      }
      while (0 < (int)puVar10) {
        (*(code *)param_1[1])(*puVar11,param_1[2]);
        param_1[8] = param_1[8] + 1;
        puVar10 = puVar10 + -1;
        puVar11 = puVar11 + 1;
      }
      FUN_000204b8(param_1);
    }
    return 3;
  }
  if (param_2 != 100) {
    if (param_2 != 0x78) {
      if (param_2 != 0x73) {
        return 0;
      }
      if (param_1[5] == 0) {
        FUN_000204da(param_1,*param_3,0xffffffff);
      }
      return 1;
    }
    uVar2 = *param_3;
    if ((int)((uint)(ushort)*param_1 << 0x14) < 0) {
      puVar6 = (undefined *)(DAT_000205f8 + 0x205be);
    }
    else {
      puVar6 = (undefined *)(DAT_000205f8 + 0x205d2);
    }
    iVar4 = 0;
    for (; uVar2 != 0; uVar2 = uVar2 >> 4) {
      *(undefined *)((int)param_1 + iVar4 + 0x24) = puVar6[uVar2 & 0xf];
      iVar4 = iVar4 + 1;
    }
    iVar7 = 0;
    if (((int)((uint)(byte)*param_1 << 0x1c) < 0) && (iVar4 != 0)) {
      iVar7 = 2;
      puVar6 = puVar6 + 0x11;
    }
    goto LAB_000208e2;
  }
  iVar7 = 0;
  uVar2 = *param_3;
  puVar6 = &UNK_00020594;
  if ((int)uVar2 < 0) {
    uVar2 = -uVar2;
    puVar6 = &UNK_00020598;
LAB_0002055c:
    iVar7 = 1;
  }
  else {
    if ((int)(*param_1 << 0x1e) < 0) {
      puVar6 = &UNK_0002059c;
      goto LAB_0002055c;
    }
    if ((int)(*param_1 << 0x1d) < 0) {
      puVar6 = &UNK_000205a0;
      goto LAB_0002055c;
    }
  }
  iVar4 = 0;
  for (; uVar2 != 0; uVar2 = uVar2 / 10) {
    *(byte *)((int)param_1 + iVar4 + 0x24) = (char)uVar2 + (char)(uVar2 / 10) * -10 + 0x30;
    iVar4 = iVar4 + 1;
  }
LAB_000208e2:
  if ((int)(*param_1 << 0x1a) < 0) {
    uVar2 = param_1[7];
    *param_1 = *param_1 & 0xffffffef;
  }
  else {
    uVar2 = 1;
  }
  if (iVar4 < (int)uVar2) {
    iVar9 = uVar2 - iVar4;
  }
  else {
    iVar9 = 0;
  }
  param_1[6] = param_1[6] - (iVar9 + iVar4 + iVar7);
  if (-1 < (int)((uint)(byte)*param_1 << 0x1b)) {
    FUN_0002048c(param_1);
  }
  for (iVar8 = 0; iVar8 < iVar7; iVar8 = iVar8 + 1) {
    (*(code *)param_1[1])(puVar6[iVar8],param_1[2]);
    param_1[8] = param_1[8] + 1;
  }
  if ((int)((uint)(byte)*param_1 << 0x1b) < 0) {
    FUN_0002048c(param_1);
  }
  while (0 < iVar9) {
    (*(code *)param_1[1])(0x30,param_1[2]);
    param_1[8] = param_1[8] + 1;
    iVar9 = iVar9 + -1;
  }
  while (0 < iVar4) {
    (*(code *)param_1[1])(*(byte *)((int)param_1 + iVar4 + 0x23),param_1[2]);
    param_1[8] = param_1[8] + 1;
    iVar4 = iVar4 + -1;
  }
  FUN_000204b8(param_1);
  if ((int)((uint)(byte)*param_1 << 0x18) < 0) {
    uVar3 = 2;
  }
  else {
    uVar3 = 1;
  }
  return uVar3;
}



/* ===== FUN_00020344 @ 00020344, 32 bytes ===== */

undefined8 FUN_00020344(undefined4 param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 uVar2;
  
  FUN_00027c28();
  FUN_00020860(param_1,param_2);
  iVar1 = FUN_00020e0c();
  uVar2 = FUN_00027048(0,0);
  *(undefined4 *)(iVar1 + 0xc) = uVar2;
  return CONCAT44(param_2,param_1);
}



/* ===== FUN_00020364 @ 00020364, 4 bytes ===== */

void FUN_00020364(void)

{
  return;
}



/* ===== __rt_entry @ 00020368, 32 bytes ===== */

void __rt_entry(void)

{
  undefined4 uVar1;
  undefined4 extraout_r2;
  undefined8 uVar2;
  
  uVar1 = FUN_00021088();
  FUN_00020344(uVar1,extraout_r2);
  main();
  uVar2 = FUN_000210d2();
  FUN_00020364();
  FUN_000211c0((int)uVar2,(int)((ulonglong)uVar2 >> 0x20));
  (*DAT_000203b0)();
  (*DAT_000203b4)();
  return;
}



/* ===== Reset_Handler @ 00020388, 8 bytes ===== */

void Reset_Handler(void)

{
  (*DAT_000203b0)();
  (*DAT_000203b4)();
  return;
}



/* ===== NMI_Handler @ 00020390, 2 bytes ===== */

void NMI_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== HardFault_Handler @ 00020392, 2 bytes ===== */

void HardFault_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== MemManage_Handler @ 00020394, 2 bytes ===== */

void MemManage_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== BusFault_Handler @ 00020396, 2 bytes ===== */

void BusFault_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== UsageFault_Handler @ 00020398, 2 bytes ===== */

void UsageFault_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== SVC_Handler @ 0002039a, 2 bytes ===== */

void SVC_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== DebugMon_Handler @ 0002039c, 2 bytes ===== */

void DebugMon_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== PendSV_Handler @ 0002039e, 2 bytes ===== */

void PendSV_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== SysTick_Handler @ 000203a0, 2 bytes ===== */

void SysTick_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== Default_Handler @ 000203a2, 2 bytes ===== */

void Default_Handler(void)

{
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== FUN_000203a4 @ 000203a4, 10 bytes ===== */

undefined8 FUN_000203a4(void)

{
  return CONCAT44(DAT_000203bc,DAT_000203b8);
}



/* ===== FUN_000203c8 @ 000203c8, 94 bytes ===== */

uint * FUN_000203c8(uint param_1)

{
  uint *puVar1;
  uint *puVar2;
  undefined4 *puVar3;
  int *piVar4;
  int iVar5;
  uint uVar6;
  uint *puVar7;
  
  puVar3 = (undefined4 *)FUN_00020828();
  puVar7 = (uint *)*puVar3;
  uVar6 = param_1 + 0xb & 0xfffffff8;
  if (uVar6 <= param_1) {
    return (uint *)0x0;
  }
  do {
    puVar1 = puVar7;
    for (puVar2 = (uint *)puVar7[1]; puVar2 != (uint *)0x0; puVar2 = (uint *)puVar2[1]) {
      if (uVar6 <= *puVar2) {
        if (*puVar2 < uVar6 + 8) {
          puVar1[1] = puVar2[1];
        }
        else {
          piVar4 = (int *)((int)puVar2 + uVar6);
          piVar4[1] = puVar2[1];
          *piVar4 = *puVar2 - uVar6;
          puVar1[1] = (uint)piVar4;
          *puVar2 = uVar6;
        }
        return puVar2 + 1;
      }
      puVar1 = puVar2;
    }
    iVar5 = FUN_00020838(puVar7,uVar6);
  } while (iVar5 != 0);
  return (uint *)0x0;
}



/* ===== debug_printf @ 00020474, 20 bytes ===== */

void debug_printf(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 uStack_4;
  
  uStack_c = param_2;
  uStack_8 = param_3;
  uStack_4 = param_4;
  FUN_00020de0(param_1,DAT_00020488,&uStack_c);
  return;
}



/* ===== FUN_0002048c @ 0002048c, 44 bytes ===== */

void FUN_0002048c(uint *param_1)

{
  uint uVar1;
  undefined4 uVar2;
  
  uVar1 = param_1[6];
  if ((int)(*param_1 << 0x1b) < 0) {
    uVar2 = 0x30;
  }
  else {
    uVar2 = 0x20;
  }
  if ((*param_1 & 1) != 0) {
    return;
  }
  while (uVar1 = uVar1 - 1, -1 < (int)uVar1) {
    (*(code *)param_1[1])(uVar2,param_1[2]);
    param_1[8] = param_1[8] + 1;
  }
  return;
}



/* ===== FUN_000204b8 @ 000204b8, 34 bytes ===== */

void FUN_000204b8(byte *param_1)

{
  int iVar1;
  
  iVar1 = *(int *)(param_1 + 0x18);
  if ((*param_1 & 1) == 0) {
    return;
  }
  while (iVar1 = iVar1 + -1, -1 < iVar1) {
    (**(code **)(param_1 + 4))(0x20,*(undefined4 *)(param_1 + 8));
    *(int *)(param_1 + 0x20) = *(int *)(param_1 + 0x20) + 1;
  }
  return;
}



/* ===== FUN_000204da @ 000204da, 82 bytes ===== */

void FUN_000204da(byte *param_1,undefined1 *param_2,uint param_3)

{
  uint uVar1;
  undefined1 *puVar2;
  
  if (param_3 == 1) {
    uVar1 = 1;
  }
  else {
    if ((int)((uint)*param_1 << 0x1a) < 0) {
      param_3 = *(uint *)(param_1 + 0x1c);
    }
    for (uVar1 = 0; (uVar1 < param_3 && (param_2[uVar1] != '\0')); uVar1 = uVar1 + 1) {
    }
  }
  puVar2 = param_2 + uVar1;
  *(uint *)(param_1 + 0x18) = *(int *)(param_1 + 0x18) - uVar1;
  *(uint *)(param_1 + 0x20) = *(int *)(param_1 + 0x20) + uVar1;
  FUN_0002048c(param_1);
  for (; param_2 < puVar2; param_2 = param_2 + 1) {
    (**(code **)(param_1 + 4))(*param_2,*(undefined4 *)(param_1 + 8));
  }
  FUN_000204b8(param_1);
  return;
}



/* ===== FUN_000205fc @ 000205fc, 308 bytes ===== */

uint FUN_000205fc(uint *param_1,uint *param_2)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  uint *puVar7;
  
  iVar1 = DAT_00020730;
  param_1[8] = 0;
  while (uVar2 = (*(code *)param_1[3])(param_1), uVar2 != 0) {
    if (uVar2 == 0x25) {
      uVar5 = 0;
      while (((uVar2 = (*(code *)param_1[3])(param_1), 0x1f < (int)uVar2 && (uVar2 < 0x31)) &&
             (uVar3 = (uint)*(byte *)(iVar1 + uVar2 + 0x205f0), uVar3 != 0))) {
        uVar5 = uVar5 | uVar3;
      }
      if ((int)(uVar5 << 0x1e) < 0) {
        uVar5 = uVar5 & 0xfffffffb;
      }
      param_1[7] = 0;
      iVar6 = 0;
      param_1[6] = 0;
      puVar7 = param_2;
      do {
        if (uVar2 == 0x2a) {
          param_2 = puVar7 + 1;
          param_1[iVar6 + 6] = *puVar7;
          uVar2 = (*(code *)param_1[3])(param_1);
          if (iVar6 == 1) {
            if ((int)param_1[7] < 0) {
              uVar5 = uVar5 & 0xffffffdf;
            }
            break;
          }
        }
        else {
          iVar4 = FUN_00024438(uVar2);
          if (iVar4 != 0) {
            param_1[iVar6 + 6] = uVar2 - 0x30;
            while( true ) {
              uVar2 = (*(code *)param_1[3])(param_1);
              iVar4 = FUN_00024438();
              if (iVar4 == 0) break;
              param_1[iVar6 + 6] = (uVar2 + param_1[iVar6 + 6] * 10) - 0x30;
            }
          }
          param_2 = puVar7;
          if (iVar6 == 1) break;
        }
        if (uVar2 != 0x2e) break;
        uVar2 = (*(code *)param_1[3])(param_1);
        iVar6 = iVar6 + 1;
        uVar5 = uVar5 | 0x20;
        puVar7 = param_2;
      } while (iVar6 < 2);
      if ((int)param_1[6] < 0) {
        uVar5 = uVar5 | 1;
        param_1[6] = -param_1[6];
      }
      if ((uVar5 & 1) != 0) {
        uVar5 = uVar5 & 0xffffffef;
      }
      if (uVar2 == 0) break;
      if (uVar2 - 0x41 < 0x1a) {
        uVar2 = uVar2 + 0x20;
        uVar5 = uVar5 | 0x800;
      }
      *param_1 = uVar5;
      iVar6 = FUN_00020328(param_1,uVar2,param_2);
      if (iVar6 == 0) goto LAB_00020620;
      if (iVar6 == 1) {
        param_2 = param_2 + 1;
      }
      else {
        param_2 = (uint *)(((int)param_2 + 7U & 0xfffffff8) + 8);
      }
    }
    else {
LAB_00020620:
      (*(code *)param_1[1])(uVar2,param_1[2]);
      param_1[8] = param_1[8] + 1;
    }
  }
  return param_1[8];
}



/* ===== runtime_memcpy @ 00020734, 138 bytes ===== */

undefined8 runtime_memcpy(uint *param_1,uint *param_2,uint param_3,uint param_4)

{
  bool bVar1;
  uint *puVar2;
  uint *puVar3;
  uint *puVar4;
  byte *pbVar5;
  byte bVar6;
  undefined2 uVar7;
  byte in_r12;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  bool bVar13;
  
  puVar4 = param_2;
  if (3 < param_3) {
    uVar8 = (uint)param_1 & 3;
    puVar2 = param_1;
    uVar9 = uVar8;
    if (uVar8 != 0) {
      bVar6 = (byte)*param_2;
      puVar4 = (uint *)((int)param_2 + 1);
      if (uVar8 < 3) {
        puVar4 = (uint *)((int)param_2 + 2);
        uVar9 = (uint)*(byte *)((int)param_2 + 1);
      }
      *(byte *)param_1 = bVar6;
      param_2 = puVar4;
      if (uVar8 < 2) {
        param_2 = (uint *)((int)puVar4 + 1);
        bVar6 = (byte)*puVar4;
      }
      param_3 = (param_3 + uVar8) - 4;
      puVar4 = (uint *)((int)param_1 + 1);
      if (uVar8 < 3) {
        puVar4 = (uint *)((int)param_1 + 2);
        *(byte *)((int)param_1 + 1) = (byte)uVar9;
      }
      puVar2 = puVar4;
      if (uVar8 < 2) {
        puVar2 = (uint *)((int)puVar4 + 1);
        *(byte *)puVar4 = bVar6;
      }
    }
    param_4 = (uint)param_2 & 3;
    if (param_4 == 0) {
      uVar9 = 0;
      while (uVar8 = param_3 - 0x20, 0x1f < param_3) {
        uVar9 = param_2[1];
        uVar10 = param_2[2];
        uVar11 = param_2[3];
        *puVar2 = *param_2;
        puVar2[1] = uVar9;
        puVar2[2] = uVar10;
        puVar2[3] = uVar11;
        uVar9 = param_2[4];
        uVar10 = param_2[5];
        uVar11 = param_2[6];
        uVar12 = param_2[7];
        param_2 = param_2 + 8;
        puVar2[4] = uVar9;
        puVar2[5] = uVar10;
        puVar2[6] = uVar11;
        puVar2[7] = uVar12;
        puVar2 = puVar2 + 8;
        param_3 = uVar8;
      }
      if ((uVar8 & 0x10) != 0) {
        uVar9 = *param_2;
        uVar10 = param_2[1];
        uVar11 = param_2[2];
        uVar12 = param_2[3];
        param_2 = param_2 + 4;
        *puVar2 = uVar9;
        puVar2[1] = uVar10;
        puVar2[2] = uVar11;
        puVar2[3] = uVar12;
        puVar2 = puVar2 + 4;
      }
      if ((int)(param_3 << 0x1c) < 0) {
        uVar9 = *param_2;
        uVar10 = param_2[1];
        param_2 = param_2 + 2;
        *puVar2 = uVar9;
        puVar2[1] = uVar10;
        puVar2 = puVar2 + 2;
      }
      puVar3 = puVar2;
      puVar4 = param_2;
      if ((uVar8 & 4) != 0) {
        puVar4 = param_2 + 1;
        uVar9 = *param_2;
        puVar3 = puVar2 + 1;
        *puVar2 = uVar9;
      }
      uVar7 = (undefined2)uVar9;
      if ((uVar8 & 3) != 0) {
        bVar1 = (uVar8 & 2) != 0;
        param_3 = param_3 << 0x1f;
        bVar13 = (int)param_3 < 0;
        puVar2 = puVar4;
        if (bVar1) {
          puVar2 = (uint *)((int)puVar4 + 2);
          uVar7 = (undefined2)*puVar4;
        }
        puVar4 = puVar2;
        if (bVar13) {
          puVar4 = (uint *)((int)puVar2 + 1);
          param_3 = (uint)(byte)*puVar2;
        }
        puVar2 = puVar3;
        if (bVar1) {
          puVar2 = (uint *)((int)puVar3 + 2);
          *(undefined2 *)puVar3 = uVar7;
        }
        puVar3 = puVar2;
        if (bVar13) {
          puVar3 = (uint *)((int)puVar2 + 1);
          *(byte *)puVar2 = (byte)param_3;
        }
        return CONCAT44(puVar4,puVar3);
      }
      return CONCAT44(puVar4,puVar3);
    }
    while( true ) {
      in_r12 = (byte)uVar9;
      if (param_3 < 8) break;
      puVar4 = param_2 + 1;
      param_4 = *param_2;
      param_2 = param_2 + 2;
      uVar9 = *puVar4;
      *puVar2 = param_4;
      puVar2[1] = uVar9;
      puVar2 = puVar2 + 2;
      param_3 = param_3 - 8;
    }
    param_3 = param_3 - 4;
    param_1 = puVar2;
    puVar4 = param_2;
    if (-1 < (int)param_3) {
      puVar4 = param_2 + 1;
      param_4 = *param_2;
      param_1 = puVar2 + 1;
      *puVar2 = param_4;
    }
  }
  bVar6 = (byte)param_4;
  bVar1 = (param_3 & 2) != 0;
  param_3 = param_3 << 0x1f;
  bVar13 = (int)param_3 < 0;
  if (bVar1) {
    pbVar5 = (byte *)((int)puVar4 + 1);
    bVar6 = (byte)*puVar4;
    puVar4 = (uint *)((int)puVar4 + 2);
    in_r12 = *pbVar5;
  }
  puVar2 = puVar4;
  if (bVar13) {
    puVar2 = (uint *)((int)puVar4 + 1);
    param_3 = (uint)(byte)*puVar4;
  }
  if (bVar1) {
    pbVar5 = (byte *)((int)param_1 + 1);
    *(byte *)param_1 = bVar6;
    param_1 = (uint *)((int)param_1 + 2);
    *pbVar5 = in_r12;
  }
  puVar4 = param_1;
  if (bVar13) {
    puVar4 = (uint *)((int)param_1 + 1);
    *(byte *)param_1 = (byte)param_3;
  }
  return CONCAT44(puVar2,puVar4);
}



/* ===== runtime_memcpy_aligned @ 000207be, 100 bytes ===== */

undefined8 runtime_memcpy_aligned(undefined4 *param_1,byte *param_2,uint param_3,undefined4 param_4)

{
  bool bVar1;
  uint uVar2;
  undefined4 *puVar3;
  undefined4 *puVar4;
  byte *pbVar5;
  byte *pbVar6;
  undefined2 uVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  bool bVar11;
  
  while (uVar2 = param_3 - 0x20, 0x1f < param_3) {
    uVar8 = *(undefined4 *)(param_2 + 4);
    uVar9 = *(undefined4 *)(param_2 + 8);
    uVar10 = *(undefined4 *)(param_2 + 0xc);
    *param_1 = *(undefined4 *)param_2;
    param_1[1] = uVar8;
    param_1[2] = uVar9;
    param_1[3] = uVar10;
    param_4 = *(undefined4 *)(param_2 + 0x10);
    uVar8 = *(undefined4 *)(param_2 + 0x14);
    uVar9 = *(undefined4 *)(param_2 + 0x18);
    uVar10 = *(undefined4 *)(param_2 + 0x1c);
    param_2 = param_2 + 0x20;
    param_1[4] = param_4;
    param_1[5] = uVar8;
    param_1[6] = uVar9;
    param_1[7] = uVar10;
    param_1 = param_1 + 8;
    param_3 = uVar2;
  }
  if ((uVar2 & 0x10) != 0) {
    param_4 = *(undefined4 *)param_2;
    uVar8 = *(undefined4 *)(param_2 + 4);
    uVar9 = *(undefined4 *)(param_2 + 8);
    uVar10 = *(undefined4 *)(param_2 + 0xc);
    param_2 = param_2 + 0x10;
    *param_1 = param_4;
    param_1[1] = uVar8;
    param_1[2] = uVar9;
    param_1[3] = uVar10;
    param_1 = param_1 + 4;
  }
  if ((int)(param_3 << 0x1c) < 0) {
    param_4 = *(undefined4 *)param_2;
    uVar8 = *(undefined4 *)(param_2 + 4);
    param_2 = param_2 + 8;
    *param_1 = param_4;
    param_1[1] = uVar8;
    param_1 = param_1 + 2;
  }
  puVar4 = param_1;
  pbVar5 = param_2;
  if ((uVar2 & 4) != 0) {
    pbVar5 = param_2 + 4;
    param_4 = *(undefined4 *)param_2;
    puVar4 = param_1 + 1;
    *param_1 = param_4;
  }
  uVar7 = (undefined2)param_4;
  if ((uVar2 & 3) == 0) {
    return CONCAT44(pbVar5,puVar4);
  }
  bVar1 = (uVar2 & 2) != 0;
  param_3 = param_3 << 0x1f;
  bVar11 = (int)param_3 < 0;
  pbVar6 = pbVar5;
  if (bVar1) {
    pbVar6 = pbVar5 + 2;
    uVar7 = *(undefined2 *)pbVar5;
  }
  pbVar5 = pbVar6;
  if (bVar11) {
    pbVar5 = pbVar6 + 1;
    param_3 = (uint)*pbVar6;
  }
  puVar3 = puVar4;
  if (bVar1) {
    puVar3 = (undefined4 *)((int)puVar4 + 2);
    *(undefined2 *)puVar4 = uVar7;
  }
  puVar4 = puVar3;
  if (bVar11) {
    puVar4 = (undefined4 *)((int)puVar3 + 1);
    *(char *)puVar3 = (char)param_3;
  }
  return CONCAT44(pbVar5,puVar4);
}



/* ===== FUN_00020828 @ 00020828, 4 bytes ===== */

undefined4 FUN_00020828(void)

{
  return DAT_0002082c;
}



/* ===== FUN_00020838 @ 00020838, 34 bytes ===== */

/* WARNING: Removing unreachable block (ram,0x0002084c) */

longlong FUN_00020838(void)

{
  undefined1 local_10 [4];
  
  return ZEXT48(local_10) << 0x20;
}



/* ===== FUN_00020860 @ 00020860, 94 bytes ===== */

void FUN_00020860(uint param_1,uint param_2,undefined4 param_3,uint param_4)

{
  uint *puVar1;
  uint uVar2;
  bool bVar3;
  uint local_18;
  
  local_18 = param_4;
  if ((param_2 < param_1 + 0x10) &&
     (FUN_00021078(0,&local_18,0x10), bVar3 = local_18 != param_2, param_2 = local_18, bVar3)) {
    param_1 = local_18;
  }
  puVar1 = (uint *)FUN_00020828();
  *puVar1 = param_1;
  uVar2 = param_1 + 0x10;
  FUN_000208be(param_1);
  if (param_2 == uVar2) {
    return;
  }
  FUN_00020e1c(*puVar1,uVar2,param_2 - uVar2,local_18);
  return;
}



/* ===== FUN_000208be @ 000208be, 10 bytes ===== */

void FUN_000208be(undefined4 *param_1)

{
  *param_1 = 0;
  param_1[1] = 0;
  param_1[2] = param_1;
  return;
}



/* ===== FUN_000208d6 @ 000208d6, 12 bytes ===== */

void FUN_000208d6(undefined4 param_1)

{
  undefined4 *puVar1;
  
  puVar1 = (undefined4 *)FUN_00020e14();
  *puVar1 = param_1;
  return;
}



/* ===== FUN_00020996 @ 00020996, 432 bytes ===== */

/* WARNING: Removing unreachable block (ram,0x000209e0) */

void FUN_00020996(uint *param_1,char *param_2,int *param_3,uint param_4,uint param_5)

{
  char cVar1;
  bool bVar2;
  undefined4 uVar3;
  uint uVar4;
  int iVar5;
  undefined4 uVar6;
  int iVar7;
  char extraout_r2;
  char extraout_r2_00;
  undefined4 extraout_r2_01;
  undefined4 extraout_r2_02;
  uint uVar8;
  uint uVar9;
  uint unaff_r11;
  undefined8 uVar10;
  ulonglong uVar11;
  longlong lVar12;
  undefined8 local_60;
  int local_54;
  undefined4 uStack_50;
  undefined4 uStack_4c;
  int local_48;
  undefined4 uStack_44;
  undefined4 uStack_40;
  uint local_3c;
  int local_38;
  uint *local_34;
  char *pcStack_30;
  int *piStack_2c;
  uint uStack_28;
  
  uVar11 = CONCAT44(local_60._4_4_,(int)local_60);
  local_38 = *param_3;
  uVar4 = param_3[1];
  uVar8 = (uVar4 & 0x7fffffff) >> 0x14;
  if (uVar8 == 0) {
    uVar8 = 0xffffffff;
  }
  if (local_38 == 0 && (uVar4 & 0x7fffffff) == 0) {
    uVar4 = 0;
    if (param_5 == 1) {
      uVar8 = ~param_4;
    }
    else {
      for (; (int)uVar4 < (int)param_4; uVar4 = uVar4 + 1) {
        param_2[uVar4] = '0';
      }
      uVar8 = 0;
      uVar4 = param_4;
    }
    param_2[uVar4] = '\0';
    param_1[2] = param_5;
    *param_1 = uVar8;
    param_1[1] = uVar4;
    return;
  }
  uVar8 = (int)((uVar8 - 0x3ff) * 0x4d10) >> 0x10;
  local_34 = param_1;
  pcStack_30 = param_2;
  piStack_2c = param_3;
  uStack_28 = param_4;
LAB_000209f6:
  do {
    if (param_5 == 0) {
      iVar7 = (uVar8 - param_4) + 1;
    }
    else {
      iVar7 = -param_4;
    }
    iVar5 = iVar7;
    if (iVar7 < 0) {
      iVar5 = -iVar7;
    }
    local_60 = uVar11;
    FUN_00020f8c(&local_48,iVar5,0);
    local_54 = local_48;
    uStack_50 = uStack_44;
    uStack_4c = uStack_40;
    uVar10 = FUN_0002120c(uVar4,local_38);
    local_60._4_4_ = (undefined4)((ulonglong)uVar10 >> 0x20);
    local_60._0_4_ = (int)uVar10 + -0x201f;
    if (iVar7 < 1) {
      local_54 = local_54 + -0x201f;
      uVar11 = FUN_0002172e(&local_60,&local_54,0,0);
      uVar3 = extraout_r2_02;
    }
    else {
      local_54 = local_54 + 0x201f;
      uVar11 = FUN_00021704(&local_60,&local_54,0,0);
      uVar3 = extraout_r2_01;
    }
    local_60._4_4_ = (undefined4)(uVar11 >> 0x20);
    uVar6 = local_60._4_4_;
    if ((uVar11 & 0xffff) != 0) {
      uVar3 = 0xffffffff;
      uVar6 = 0x7fffffff;
    }
    lVar12 = CONCAT44(uVar6,uVar3);
    uVar9 = param_4;
    if (param_5 != 0) {
      local_60._0_4_ = 0;
      for (uVar9 = 0;
          (uVar11 = CONCAT44(local_60._4_4_,(int)local_60), lVar12 != 0 && ((int)uVar9 < 0x11));
          uVar9 = uVar9 + 1) {
        lVar12 = FUN_00020e50();
        param_2[uVar9] = extraout_r2_00 + '0';
      }
      if (lVar12 != 0) {
        if ((int)local_60 != 0) goto LAB_00020b2a;
        param_4 = 0x11;
        param_5 = 0;
        goto LAB_000209f6;
      }
      uVar4 = uVar9;
      for (iVar7 = 0; uVar4 = uVar4 - 1, iVar7 < (int)uVar4; iVar7 = iVar7 + 1) {
        cVar1 = param_2[iVar7];
        param_2[iVar7] = param_2[uVar4];
        param_2[uVar4] = cVar1;
      }
      unaff_r11 = uVar9;
      local_3c = (uVar9 - param_4) - 1;
LAB_00020b2a:
      param_2[unaff_r11] = '\0';
      local_34[2] = param_5;
      *local_34 = local_3c;
      local_34[1] = unaff_r11;
      return;
    }
    while (uVar9 = uVar9 - 1, -1 < (int)uVar9) {
      local_60 = uVar11;
      lVar12 = FUN_00020e50();
      param_2[uVar9] = extraout_r2 + '0';
      uVar11 = local_60;
    }
    bVar2 = true;
    if (lVar12 == 0) {
      if (*param_2 == '0') {
        bVar2 = false;
        uVar8 = uVar8 - 1;
      }
    }
    else {
      bVar2 = false;
      uVar8 = uVar8 + 1;
    }
    unaff_r11 = param_4;
    local_3c = uVar8;
    if (bVar2) goto LAB_00020b2a;
  } while( true );
}



/* ===== FUN_00020de0 @ 00020de0, 32 bytes ===== */

undefined4 FUN_00020de0(undefined4 param_1,undefined4 param_2)

{
  undefined4 uVar1;
  int iVar2;
  
  uVar1 = FUN_00020ee6();
  iVar2 = FUN_00021070(param_2);
  if (iVar2 != 0) {
    return 0xffffffff;
  }
  return uVar1;
}



/* ===== FUN_00020e04 @ 00020e04, 4 bytes ===== */

undefined4 FUN_00020e04(void)

{
  return DAT_00020e08;
}



/* ===== FUN_00020e0c @ 00020e0c, 4 bytes ===== */

undefined4 FUN_00020e0c(void)

{
  return DAT_00020e10;
}



/* ===== FUN_00020e14 @ 00020e14, 4 bytes ===== */

undefined4 FUN_00020e14(void)

{
  return DAT_00020e18;
}



/* ===== FUN_00020e1c @ 00020e1c, 130 bytes ===== */

void FUN_00020e1c(int *param_1,int *param_2,int param_3)

{
  int *piVar1;
  int *piVar2;
  int *piVar3;
  
  piVar3 = (int *)param_1[1];
  for (; piVar3 != (int *)0x0 && piVar3 < param_2; piVar3 = (int *)piVar3[1]) {
    param_1 = piVar3;
  }
  piVar3 = param_2;
  if ((int *)(*param_1 + (int)param_1) != param_2) {
    piVar3 = (int *)(((int)param_2 + 3U & 0xfffffff8) + 4);
    param_3 = param_3 - ((int)piVar3 - (int)param_2);
  }
  *piVar3 = param_3;
  piVar1 = (int *)FUN_00020828();
  if (piVar3 == (int *)0xfffffffc) {
    return;
  }
  piVar2 = (int *)*piVar1;
  for (piVar1 = (int *)((int *)*piVar1)[1]; piVar1 != (int *)0x0 && piVar1 < piVar3;
      piVar1 = (int *)piVar1[1]) {
    piVar2 = piVar1;
  }
  if ((int *)(*piVar2 + (int)piVar2) == piVar3) {
    *piVar2 = *piVar2 + *piVar3;
  }
  else {
    piVar2[1] = (int)piVar3;
    piVar2 = piVar3;
  }
  if ((int *)(*piVar2 + (int)piVar2) == piVar1) {
    piVar2[1] = piVar1[1];
    *piVar2 = *piVar2 + *piVar1;
    return;
  }
  piVar2[1] = (int)piVar1;
  return;
}



/* ===== FUN_00020e50 @ 00020e50, 138 bytes ===== */

undefined8 FUN_00020e50(uint param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  
  uVar3 = param_1 >> 2 | param_2 << 0x1e;
  uVar1 = param_1 - uVar3;
  uVar2 = (param_2 - (param_2 >> 2)) - (uint)(param_1 < uVar3);
  uVar4 = uVar1 >> 4 | uVar2 * 0x10000000;
  uVar3 = uVar1 + uVar4;
  uVar2 = uVar2 + (uVar2 >> 4) + (uint)CARRY4(uVar1,uVar4);
  uVar4 = uVar3 >> 8 | uVar2 * 0x1000000;
  uVar1 = uVar3 + uVar4;
  uVar2 = uVar2 + (uVar2 >> 8) + (uint)CARRY4(uVar3,uVar4);
  uVar4 = uVar1 >> 0x10 | uVar2 * 0x10000;
  uVar3 = uVar1 + uVar4;
  uVar2 = uVar2 + (uVar2 >> 0x10) + (uint)CARRY4(uVar1,uVar4);
  uVar4 = uVar2 + CARRY4(uVar3,uVar2);
  uVar1 = uVar3 + uVar2 >> 3 | uVar4 * 0x20000000;
  uVar3 = uVar4 >> 3;
  if (-1 < (int)(((param_2 - (param_1 < 10)) -
                 (((uVar3 << 2 | (uVar4 & 7) >> 1) + uVar3 + CARRY4(uVar1,uVar1 * 4)) * 2 +
                 (uint)CARRY4(uVar1 * 5,uVar1 * 5))) - (uint)(param_1 - 10 < uVar1 * 10))) {
    return CONCAT44(uVar3 + (0xfffffffe < uVar1),uVar1 + 1);
  }
  return CONCAT44(uVar3,uVar1);
}



/* ===== FUN_00020ee6 @ 00020ee6, 32 bytes ===== */

void FUN_00020ee6(undefined4 param_1,undefined4 param_2,undefined4 param_3,undefined4 param_4)

{
  undefined1 auStack_40 [4];
  undefined4 local_3c;
  undefined4 uStack_38;
  int local_34;
  undefined4 uStack_30;
  undefined4 local_2c;
  
  local_2c = 0;
  local_34 = DAT_00020f08 + 0x20ef8;
  local_3c = param_4;
  uStack_38 = param_2;
  uStack_30 = param_1;
  FUN_000205fc(auStack_40,param_3);
  return;
}



/* ===== FUN_00020f0c @ 00020f0c, 112 bytes ===== */

void FUN_00020f0c(uint *param_1,undefined4 param_2,int param_3,int param_4)

{
  uint uVar1;
  undefined1 *puVar2;
  int iVar3;
  bool bVar4;
  
  iVar3 = 3;
  uVar1 = *param_1 & 0x800;
  if (param_3 < 7) {
    if (uVar1 == 0) {
      puVar2 = &UNK_00020f88;
    }
    else {
      puVar2 = &UNK_00020f84;
    }
  }
  else if (uVar1 == 0) {
    puVar2 = &UNK_00020f80;
  }
  else {
    puVar2 = &DAT_00020f7c;
  }
  *param_1 = *param_1 & 0xffffffef;
  uVar1 = param_1[6];
  param_1[6] = uVar1 - 3;
  if (param_4 != 0) {
    param_1[6] = uVar1 - 4;
  }
  FUN_0002048c(param_1);
  if (param_4 == 0) {
    uVar1 = param_1[8];
  }
  else {
    (*(code *)param_1[1])(param_4,param_1[2]);
    uVar1 = param_1[8] + 1;
    param_1[8] = uVar1;
  }
  param_1[8] = uVar1 + 3;
  while (bVar4 = iVar3 != 0, iVar3 = iVar3 + -1, bVar4) {
    (*(code *)param_1[1])(*puVar2,param_1[2]);
    puVar2 = puVar2 + 1;
  }
  FUN_000204b8(param_1);
  return;
}



/* ===== FUN_00020f8c @ 00020f8c, 224 bytes ===== */

void FUN_00020f8c(undefined8 *param_1,int param_2,int param_3)

{
  undefined4 *puVar1;
  undefined4 extraout_r2;
  undefined4 extraout_r2_00;
  undefined4 extraout_r2_01;
  undefined4 extraout_r2_02;
  undefined4 uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  bool bVar7;
  undefined8 uVar8;
  undefined8 uVar9;
  undefined8 local_48;
  undefined4 uStack_40;
  undefined8 local_3c;
  undefined4 uStack_34;
  undefined4 local_30;
  undefined4 uStack_2c;
  int local_28;
  
  uVar9 = *(undefined8 *)(DAT_0002106c + 0x20f9c);
  uStack_40 = *(undefined4 *)(DAT_0002106c + 0x20fa4);
  local_3c._0_4_ = *(undefined4 *)(DAT_0002106c + 0x20fa8);
  local_3c._4_4_ = *(undefined4 *)(DAT_0002106c + 0x20fac);
  uStack_34 = *(undefined4 *)(DAT_0002106c + 0x20fb0);
  uVar4 = (param_2 + 0x1b9b) / 0x37 - 0x80;
  uVar3 = (param_2 + 0x1b9b) % 0x37 - 0x1b;
  bVar7 = -1 < (int)uVar3;
  if (!bVar7) {
    uVar3 = -uVar3;
  }
  iVar5 = 0;
  iVar6 = DAT_0002106c + 0x20f20;
  for (; uVar8 = CONCAT44(local_3c._4_4_,(undefined4)local_3c), uVar3 != 0; uVar3 = (int)uVar3 >> 1)
  {
    if ((uVar3 & 1) != 0) {
      local_48 = uVar9;
      uVar9 = FUN_0002172e(&local_48,iVar6 + iVar5 * 0xc,param_3,1);
      uStack_40 = extraout_r2;
    }
    iVar5 = iVar5 + 1;
  }
  iVar6 = DAT_0002106c + 0x20f5c;
  iVar5 = 0;
  for (; local_48 = uVar9, local_3c = uVar8, uVar4 != 0; uVar4 = (int)uVar4 >> 1) {
    if ((uVar4 & 1) != 0) {
      puVar1 = (undefined4 *)(iVar6 + iVar5 * 0x10);
      local_30 = *puVar1;
      uStack_2c = puVar1[1];
      local_28 = puVar1[2];
      if (puVar1[3] + param_3 == 0) {
        local_28 = local_28 + param_3;
      }
      uVar8 = FUN_0002172e(&local_3c,&local_30,param_3,1);
      uStack_34 = extraout_r2_00;
      uVar9 = local_48;
    }
    iVar5 = iVar5 + 1;
  }
  if (bVar7) {
    uVar9 = FUN_0002172e(&local_3c,&local_48,param_3,1);
    uVar2 = extraout_r2_02;
  }
  else {
    uVar9 = FUN_00021704();
    uVar2 = extraout_r2_01;
  }
  *param_1 = uVar9;
  *(undefined4 *)(param_1 + 1) = uVar2;
  return;
}



/* ===== FUN_00021070 @ 00021070, 8 bytes ===== */

byte FUN_00021070(int param_1)

{
  return *(byte *)(param_1 + 0xc) & 0x80;
}



/* ===== FUN_00021078 @ 00021078, 24 bytes ===== */

void FUN_00021078(void)

{
  int iVar1;
  
  iVar1 = FUN_000210f0();
  if (iVar1 != 0) {
    FUN_000211c0();
    return;
  }
  return;
}



/* ===== FUN_00021088 @ 00021088, 74 bytes ===== */

void FUN_00021088(void)

{
  uint uVar1;
  undefined4 *puVar2;
  undefined4 unaff_lr;
  uint *puVar3;
  
  uVar1 = FUN_00020e04();
  *(undefined4 *)((uVar1 & 0xfffffff8) + 0x5c) = unaff_lr;
  puVar3 = (uint *)((uVar1 & 0xfffffff8) + 0x58);
  *puVar3 = uVar1;
  FUN_000203a4();
  puVar2 = (undefined4 *)*puVar3;
  *puVar2 = 0;
  puVar2[1] = 0;
  puVar2[2] = 0;
  puVar2[3] = 0;
  puVar2[4] = 0;
  puVar2[5] = 0;
  puVar2[6] = 0;
  puVar2[7] = 0;
  puVar2[8] = 0;
  puVar2[9] = 0;
  puVar2[10] = 0;
  puVar2[0xb] = 0;
  puVar2[0xc] = 0;
  puVar2[0xd] = 0;
  puVar2[0xe] = 0;
  puVar2[0xf] = 0;
  return;
}



/* ===== FUN_000210d2 @ 000210d2, 18 bytes ===== */

void FUN_000210d2(undefined4 param_1,undefined4 param_2)

{
  FUN_00020364();
  FUN_000211c0(param_1,param_2);
  (*DAT_000203b0)();
  (*DAT_000203b4)();
  return;
}



/* ===== FUN_000210f0 @ 000210f0, 22 bytes ===== */

undefined4 FUN_000210f0(int param_1)

{
  char *pcVar1;
  
  if (param_1 == 1) {
    pcVar1 = ": Heap memory corrupted";
  }
  else {
    pcVar1 = (char *)0x0;
  }
  FUN_000211cc("SIGRTMEM: Out of heap memory",pcVar1);
  return 1;
}



/* ===== FUN_00021140 @ 00021140, 126 bytes ===== */

int FUN_00021140(uint *param_1,uint *param_2)

{
  uint *puVar1;
  uint uVar2;
  uint *puVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  
  while( true ) {
    uVar4 = *param_1;
    uVar5 = *param_2;
    if (uVar4 != uVar5) break;
    if ((uVar4 + 0xfefefeff & ~uVar4 & 0x80808080) != 0) {
      return 0;
    }
    puVar1 = param_1 + 2;
    uVar4 = param_1[1];
    puVar3 = param_2 + 2;
    uVar5 = param_2[1];
    if (uVar4 != uVar5) break;
    if ((uVar4 + 0xfefefeff & ~uVar4 & 0x80808080) != 0) {
      return 0;
    }
    param_1 = param_1 + 3;
    uVar4 = *puVar1;
    param_2 = param_2 + 3;
    uVar5 = *puVar3;
    if (uVar4 != uVar5) break;
    if ((uVar4 + 0xfefefeff & ~uVar4 & 0x80808080) != 0) {
      return 0;
    }
  }
  uVar2 = uVar4 - uVar5;
  uVar2 = LZCOUNT(uVar2 * 0x1000000 | (uVar2 >> 8 & 0xff) << 0x10 | (uVar2 >> 0x10 & 0xff) << 8 |
                  uVar2 >> 0x18) & 0x18;
  uVar6 = 0x1010101 >> (0x20 - uVar2 & 0xff);
  if ((uVar4 - uVar6 & ~uVar4 & uVar6 << 7) != 0) {
    return 0;
  }
  return (uVar4 >> uVar2 & 0xff) - (uVar5 >> uVar2 & 0xff);
}



/* ===== FUN_000211c0 @ 000211c0, 8 bytes ===== */

void FUN_000211c0(void)

{
  software_bkpt(0xab);
  do {
                    /* WARNING: Do nothing block with infinite loop */
  } while( true );
}



/* ===== FUN_000211cc @ 000211cc, 50 bytes ===== */

undefined8 FUN_000211cc(char *param_1,char *param_2)

{
  char cVar1;
  
  cVar1 = '\n';
  for (; (FUN_000211fe(cVar1), param_1 != (char *)0x0 && (cVar1 = *param_1, cVar1 != '\0'));
      param_1 = param_1 + 1) {
  }
  for (; (param_2 != (char *)0x0 && (*param_2 != '\0')); param_2 = param_2 + 1) {
    FUN_000211fe();
  }
  software_bkpt(0xab);
  return CONCAT44(&stack0xfffffff8,3);
}



/* ===== FUN_000211fe @ 000211fe, 14 bytes ===== */

undefined8 FUN_000211fe(void)

{
  undefined1 local_8 [4];
  
  software_bkpt(0xab);
  return CONCAT44(local_8,3);
}



/* ===== FUN_0002120c @ 0002120c, 228 bytes ===== */

undefined8 FUN_0002120c(uint param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  
  iVar5 = param_1 << 1;
  uVar4 = param_2 << 0xb;
  uVar2 = param_1 << 0xb | param_2 >> 0x15;
  uVar1 = (param_1 & 0x7fffffff) >> 0x13;
  if (iVar5 != 0 || param_2 != 0) {
    uVar1 = uVar1 + 0x7800;
  }
  uVar1 = (uint)((param_1 & 0x80000000) != 0) << 0x1f | uVar1 >> 1;
  if (iVar5 != 0 || param_2 != 0) {
    uVar2 = uVar2 | 0x80000000;
  }
  if (iVar5 >> 0x15 != 0) {
    if (iVar5 >> 0x15 == -1) {
      uVar1 = uVar1 | 0x40000000;
    }
    return CONCAT44(uVar2,uVar1);
  }
  if ((uVar2 & 0x80000000) == 0) {
    return CONCAT44(uVar2,uVar1);
  }
  uVar3 = uVar2 & 0x7fffffff;
  if (uVar3 == 0) {
    if ((param_2 & 0x1fffff) >> 5 == 0) {
      uVar4 = param_2 << 0x1b;
      iVar5 = 0x10;
    }
    else {
      iVar5 = 0;
    }
    if (uVar4 >> 0x18 == 0) {
      uVar4 = uVar4 << 8;
      iVar5 = iVar5 + 8;
    }
    if (uVar4 >> 0x1c == 0) {
      uVar4 = uVar4 << 4;
      iVar5 = iVar5 + 4;
    }
    if (uVar4 >> 0x1e == 0) {
      uVar4 = uVar4 << 2;
      iVar5 = iVar5 + 2;
    }
    if (-1 < (int)uVar4) {
      uVar4 = uVar4 << 1;
      iVar5 = iVar5 + 1;
    }
    return CONCAT44(uVar4,(uVar1 - 0x1f) - iVar5);
  }
  if (uVar3 >> 0x10 == 0) {
    uVar3 = uVar2 << 0x10;
    iVar5 = 0x10;
  }
  else {
    iVar5 = 0;
  }
  if (uVar3 >> 0x18 == 0) {
    uVar3 = uVar3 << 8;
    iVar5 = iVar5 + 8;
  }
  if (uVar3 >> 0x1c == 0) {
    uVar3 = uVar3 << 4;
    iVar5 = iVar5 + 4;
  }
  if (uVar3 >> 0x1e == 0) {
    uVar3 = uVar3 << 2;
    iVar5 = iVar5 + 2;
  }
  if (-1 < (int)uVar3) {
    uVar3 = uVar3 << 1;
    iVar5 = iVar5 + 1;
  }
  return CONCAT44(uVar3 | uVar4 >> (0x20U - iVar5 & 0xff),(uVar1 - iVar5) + 1);
}



/* ===== FUN_000212f0 @ 000212f0, 696 bytes ===== */

ulonglong FUN_000212f0(uint param_1,uint param_2,uint param_3,uint param_4)

{
  bool bVar1;
  uint uVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  uint unaff_r4;
  uint unaff_r5;
  uint uVar9;
  uint uVar10;
  int iVar11;
  uint uVar12;
  uint uVar13;
  uint uVar14;
  uint uVar15;
  uint uVar16;
  uint uVar17;
  uint uVar18;
  uint uVar19;
  uint uVar20;
  uint uVar21;
  uint uVar22;
  bool bVar23;
  bool bVar24;
  bool bVar25;
  bool bVar26;
  bool bVar27;
  
  param_1 = param_1 ^ param_4;
  uVar2 = param_1 & 0x80000000;
  uVar17 = unaff_r5 >> 0x10;
  uVar8 = unaff_r4 >> 0x10;
  uVar22 = unaff_r5 & ~(uVar17 << 0x10);
  uVar16 = unaff_r4 & ~(uVar8 << 0x10);
  uVar10 = (uint)*(byte *)((unaff_r4 >> 0x18) + 0x21528);
  uVar3 = (uint)((param_2 & 1) != 0) << 0x1f | param_3 >> 1;
  iVar11 = ((0x800000 - (uVar8 * uVar10 + uVar10)) * uVar10 >> 0x13) + 2;
  uVar18 = 0x20000000 - ((unaff_r4 >> 0xd) * iVar11 + iVar11);
  uVar10 = uVar18 >> 0x10;
  uVar10 = uVar10 * iVar11 + ((uVar18 & ~(uVar10 << 0x10)) * iVar11 >> 0x10) >> 6;
  if ((param_3 & 1) == 0) {
    uVar2 = 0;
  }
  if ((param_3 & 1) != 0) {
    uVar2 = 0x80000000;
  }
  uVar19 = uVar10 * (param_2 >> 0x10) >> 0x10;
  uVar4 = uVar3 - uVar19 * uVar17;
  uVar18 = uVar19 * uVar22;
  bVar23 = uVar18 * 0x10000 <= uVar2;
  uVar2 = uVar2 + uVar18 * -0x10000;
  uVar18 = uVar18 >> 0x10;
  bVar1 = uVar4 - uVar18 < (uint)bVar23;
  bVar24 = uVar18 < uVar4 || bVar1;
  uVar5 = (uVar4 - uVar18) - (uint)!bVar23;
  uVar12 = uVar19 * uVar16;
  if (uVar18 < uVar4 || bVar1) {
    bVar24 = uVar12 * 0x10000 <= uVar5;
  }
  uVar5 = uVar5 + uVar12 * -0x10000;
  uVar18 = ((((param_2 >> 1) - uVar8 * uVar19) - (uint)(uVar3 < uVar19 * uVar17)) - (uVar12 >> 0x10)
           ) - (uint)!bVar24;
  uVar20 = uVar10 * (uVar18 >> 2) >> 0x10;
  uVar3 = uVar20 * uVar17;
  bVar23 = uVar3 * 0x80000 <= uVar2;
  uVar2 = uVar2 + uVar3 * -0x80000;
  uVar3 = uVar3 >> 0xd;
  bVar1 = uVar5 - uVar3 < (uint)bVar23;
  bVar24 = uVar3 < uVar5 || bVar1;
  uVar4 = (uVar5 - uVar3) - (uint)!bVar23;
  uVar12 = uVar8 * uVar20;
  if (uVar3 < uVar5 || bVar1) {
    bVar24 = uVar12 * 0x80000 <= uVar4;
  }
  uVar4 = uVar4 + uVar12 * -0x80000;
  uVar5 = uVar20 * uVar22;
  bVar25 = uVar5 * 8 <= uVar2;
  uVar3 = uVar5 >> 0x1d;
  bVar1 = uVar4 - uVar3 < (uint)bVar25;
  bVar23 = uVar3 < uVar4 || bVar1;
  uVar6 = (uVar4 - uVar3) - (uint)!bVar25;
  uVar13 = uVar20 * uVar16;
  if (uVar3 < uVar4 || bVar1) {
    bVar23 = uVar13 * 8 <= uVar6;
  }
  uVar6 = uVar6 + uVar13 * -8;
  uVar3 = ((((uVar18 - (uVar12 >> 0xd)) - (uint)!bVar24) - (uVar13 >> 0x1d)) - (uint)!bVar23) *
          0x4000000 | uVar6 >> 6;
  uVar13 = uVar10 * (uVar3 >> 0xf);
  uVar18 = uVar6 * 0x4000000 | uVar2 + uVar5 * -8 >> 6;
  uVar5 = uVar5 * -0x20000000;
  uVar21 = uVar13 >> 0x10;
  uVar4 = uVar18 - uVar21 * uVar17;
  uVar2 = uVar21 * uVar22;
  bVar23 = uVar2 * 0x10000 <= uVar5;
  uVar5 = uVar5 + uVar2 * -0x10000;
  uVar2 = uVar2 >> 0x10;
  bVar1 = uVar4 - uVar2 < (uint)bVar23;
  bVar24 = uVar2 < uVar4 || bVar1;
  uVar12 = (uVar4 - uVar2) - (uint)!bVar23;
  uVar6 = uVar21 * uVar16;
  if (uVar2 < uVar4 || bVar1) {
    bVar24 = uVar6 * 0x10000 <= uVar12;
  }
  uVar12 = uVar12 + uVar6 * -0x10000;
  uVar3 = (((uVar3 - uVar8 * uVar21) - (uint)(uVar18 < uVar21 * uVar17)) - (uVar6 >> 0x10)) -
          (uint)!bVar24;
  uVar6 = uVar10 * (uVar3 >> 2) >> 0x10;
  uVar2 = uVar6 * uVar17;
  bVar23 = uVar2 * 0x80000 <= uVar5;
  uVar5 = uVar5 + uVar2 * -0x80000;
  uVar2 = uVar2 >> 0xd;
  bVar1 = uVar12 - uVar2 < (uint)bVar23;
  bVar24 = uVar2 < uVar12 || bVar1;
  uVar18 = (uVar12 - uVar2) - (uint)!bVar23;
  uVar4 = uVar8 * uVar6;
  if (uVar2 < uVar12 || bVar1) {
    bVar24 = uVar4 * 0x80000 <= uVar18;
  }
  uVar18 = uVar18 + uVar4 * -0x80000;
  uVar12 = uVar6 * uVar22;
  bVar25 = uVar12 * 8 <= uVar5;
  uVar2 = uVar12 >> 0x1d;
  bVar1 = uVar18 - uVar2 < (uint)bVar25;
  bVar23 = uVar2 < uVar18 || bVar1;
  uVar7 = (uVar18 - uVar2) - (uint)!bVar25;
  uVar14 = uVar6 * uVar16;
  if (uVar2 < uVar18 || bVar1) {
    bVar23 = uVar14 * 8 <= uVar7;
  }
  uVar7 = uVar7 + uVar14 * -8;
  uVar9 = uVar21 * 0x400000 + uVar6 * 0x200;
  uVar18 = ((((uVar3 - (uVar4 >> 0xd)) - (uint)!bVar24) - (uVar14 >> 0x1d)) - (uint)!bVar23) *
           0x4000000 | uVar7 >> 6;
  uVar4 = uVar7 * 0x4000000 | uVar5 + uVar12 * -8 >> 6;
  uVar10 = uVar10 * (uVar18 >> 0xf);
  uVar12 = uVar12 * -0x20000000;
  uVar14 = uVar10 >> 0x10;
  uVar5 = uVar4 - uVar14 * uVar17;
  uVar3 = uVar14 * uVar22;
  bVar23 = uVar3 * 0x10000 <= uVar12;
  uVar2 = uVar3 >> 0x10;
  bVar1 = uVar5 - uVar2 < (uint)bVar23;
  bVar24 = uVar2 < uVar5 || bVar1;
  uVar7 = (uVar5 - uVar2) - (uint)!bVar23;
  uVar15 = uVar14 * uVar16;
  if (uVar2 < uVar5 || bVar1) {
    bVar24 = uVar15 * 0x10000 <= uVar7;
  }
  uVar7 = uVar7 + uVar15 * -0x10000;
  uVar10 = uVar10 >> 0x14;
  uVar5 = uVar9 + uVar10;
  uVar4 = ((((uVar18 - uVar8 * uVar14) - (uint)(uVar4 < uVar14 * uVar17)) - (uVar15 >> 0x10)) -
          (uint)!bVar24) * 0x4000 | uVar7 >> 0x12;
  uVar2 = uVar7 * 0x4000 | uVar12 + uVar3 * -0x10000 >> 0x12;
  uVar18 = uVar3 * -0x40000000;
  uVar14 = uVar14 << 0x1c;
  uVar16 = uVar16 | uVar8 << 0x10;
  uVar22 = uVar22 | uVar17 << 0x10;
  bVar1 = uVar4 - uVar16 < (uint)(uVar22 <= uVar2);
  uVar8 = uVar4;
  if (uVar16 < uVar4 || bVar1) {
    uVar8 = (uVar4 - uVar16) - (uint)(uVar22 > uVar2);
    uVar2 = uVar2 - uVar22;
  }
  uVar3 = uVar3 * -0x80000000;
  bVar23 = CARRY4(uVar2,uVar2) || CARRY4(uVar2 * 2,(uint)CARRY4(uVar18,uVar18));
  uVar2 = uVar2 * 2 + (uint)CARRY4(uVar18,uVar18);
  bVar24 = CARRY4(uVar8 * 2,(uint)bVar23);
  bVar25 = CARRY4(uVar8,uVar8) || bVar24;
  uVar17 = uVar8 * 2 + (uint)bVar23;
  bVar23 = bVar25 < (uVar16 < uVar17 || uVar17 - uVar16 < (uint)(uVar22 <= uVar2));
  if ((CARRY4(uVar8,uVar8) || bVar24) || bVar23) {
    uVar17 = (uVar17 - uVar16) - (uint)(uVar22 > uVar2);
    uVar2 = uVar2 - uVar22;
  }
  bVar26 = CARRY4(uVar2,uVar2) || CARRY4(uVar2 * 2,(uint)CARRY4(uVar3,uVar3));
  uVar2 = uVar2 * 2 + (uint)CARRY4(uVar3,uVar3);
  bVar24 = CARRY4(uVar17 * 2,(uint)bVar26);
  bVar27 = CARRY4(uVar17,uVar17) || bVar24;
  uVar3 = uVar17 * 2 + (uint)bVar26;
  bVar26 = bVar27 < (uVar16 < uVar3 || uVar3 - uVar16 < (uint)(uVar22 <= uVar2));
  if ((CARRY4(uVar17,uVar17) || bVar24) || bVar26) {
    uVar3 = (uVar3 - uVar16) - (uint)(uVar22 > uVar2);
    uVar2 = uVar2 - uVar22;
  }
  if (uVar3 != 0 || uVar2 != 0) {
    uVar14 = uVar14 | 1;
  }
  uVar2 = (((uint)(uVar16 < uVar4 || bVar1) * 2 + (uint)(bVar25 || bVar23)) * 2 +
          (uint)(bVar27 || bVar26)) * 0x10000000;
  uVar3 = uVar5 + CARRY4(uVar14,uVar2);
  iVar11 = uVar19 * 0x10000 + uVar20 * 8 + (uVar13 >> 0x1a) +
           (uint)CARRY4(uVar21 * 0x400000,uVar6 * 0x200) + (uint)CARRY4(uVar9,uVar10) +
           (uint)CARRY4(uVar5,(uint)CARRY4(uVar14,uVar2));
  if (-1 < iVar11) {
    return CONCAT44(iVar11 * 2 +
                    (uint)(CARRY4(uVar3,uVar3) ||
                          CARRY4(uVar3 * 2,(uint)CARRY4(uVar14 + uVar2,uVar14 + uVar2))),param_1) &
           0xffffffff80000000;
  }
  return CONCAT44(iVar11,param_1) & 0xffffffff80000000;
}



/* ===== FUN_00021628 @ 00021628, 220 bytes ===== */

undefined8 FUN_00021628(uint param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  uint unaff_r6;
  int unaff_r11;
  bool bVar3;
  
  if ((int)param_4 < 0) {
    unaff_r6 = (unaff_r6 | unaff_r6 << 0x10) >> 0x10;
    if ((int)param_4 < -0x3f) {
      unaff_r6 = (unaff_r6 | param_3 | (unaff_r6 | param_3) << 0x10) >> 0x10 | param_2;
      if ((int)param_4 < -0x40) {
        unaff_r6 = (unaff_r6 | unaff_r6 << 0x10) >> 0x10;
      }
      param_4 = 0;
      param_3 = 0;
      param_2 = 0;
    }
    else {
      uVar2 = param_4;
      if ((int)param_4 < -0x1f) {
        unaff_r6 = unaff_r6 | param_3;
        uVar2 = param_4 + 0x20;
        param_2 = 0;
        param_3 = param_2;
      }
      uVar1 = -uVar2;
      param_4 = 0;
      if (uVar1 != 0) {
        unaff_r6 = (unaff_r6 | unaff_r6 << 0x10) >> 0x10 | param_3 << (uVar2 + 0x20 & 0xff);
        uVar2 = param_2 << (uVar2 + 0x20 & 0xff);
        param_2 = param_2 >> (uVar1 & 0xff);
        param_4 = 0;
        param_3 = param_3 >> (uVar1 & 0xff) | uVar2;
      }
    }
  }
  if (((unaff_r6 & 0x7fffffff) != 0) || ((unaff_r6 & 0x80000000) != 0)) {
    bVar3 = unaff_r11 != -1 && 0xfffffffe < param_3;
    if (unaff_r11 != -1 && 0xfffffffe < param_3) {
      bVar3 = 0xfffffffe < param_2;
      param_2 = param_2 + 1;
    }
    if (bVar3 != false) {
      param_2 = 0x80000000;
    }
    param_4 = param_4 + bVar3;
  }
  return CONCAT44(param_2,param_4 | param_1 & 0x80000000);
}



/* ===== FUN_00021704 @ 00021704, 42 bytes ===== */

void FUN_00021704(int *param_1,int *param_2)

{
  if ((param_1[1] & ~(*param_1 << 1)) < 0 && (param_2[1] & ~(*param_2 << 1)) < 0) {
    FUN_000212f0(*param_1,param_1[1],param_1[2]);
    FUN_00021628();
  }
  return;
}



/* ===== FUN_0002172e @ 0002172e, 42 bytes ===== */

void FUN_0002172e(uint *param_1,uint *param_2)

{
  if ((*param_1 & 0x40000000) == 0 && (*param_2 & 0x40000000) == 0) {
    FUN_00021758(*param_1,param_1[1],param_1[2]);
    FUN_00021628();
  }
  return;
}



/* ===== FUN_00021758 @ 00021758, 580 bytes ===== */

ulonglong FUN_00021758(uint param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  uint unaff_r4;
  uint unaff_r5;
  uint uVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  uint uVar14;
  uint uVar15;
  uint uVar16;
  uint uVar17;
  bool bVar18;
  bool bVar19;
  
  param_1 = param_1 ^ param_4;
  if (param_3 == 0) {
    if (unaff_r5 == 0) {
      uVar5 = unaff_r4 >> 0x10;
      uVar6 = param_2 >> 0x10;
      uVar14 = unaff_r4 & ~(uVar5 << 0x10);
      param_2 = param_2 & ~(uVar6 << 0x10);
      uVar7 = uVar14 * uVar6;
      uVar14 = param_2 * uVar14;
      param_2 = uVar5 * param_2;
      uVar3 = uVar7 * 0x10000;
      uVar1 = uVar14 + uVar3;
      uVar2 = param_2 * 0x10000;
      uVar4 = uVar1 + uVar2;
      iVar8 = uVar5 * uVar6 + (uVar7 >> 0x10) + (uint)CARRY4(uVar14,uVar3) +
              (param_2 >> 0x10) + (uint)CARRY4(uVar1,uVar2);
      if (-1 < iVar8) {
        return CONCAT44(iVar8 * 2 + (uint)CARRY4(uVar4,uVar4),param_1) & 0xffffffff80000000;
      }
      return CONCAT44(iVar8,param_1) & 0xffffffff80000000;
    }
    uVar4 = param_2 >> 0x10;
    uVar12 = unaff_r4 >> 0x10;
    param_2 = param_2 & ~(uVar4 << 0x10);
    uVar15 = unaff_r4 & ~(uVar12 << 0x10);
    uVar5 = uVar15 * param_2;
    uVar15 = uVar4 * uVar15;
    uVar3 = param_2 * uVar12 * 0x10000;
    uVar6 = uVar5 + uVar3;
    uVar13 = unaff_r5 >> 0x10;
    uVar2 = uVar15 * 0x10000;
    uVar7 = uVar6 + uVar2;
    uVar16 = unaff_r5 & ~(uVar13 << 0x10);
    uVar9 = uVar16 * param_2;
    uVar16 = uVar4 * uVar16;
    uVar14 = param_2 * uVar13 * 0x10000;
    uVar10 = uVar9 + uVar14;
    uVar1 = uVar16 * 0x10000;
    uVar11 = uVar10 + uVar1;
    uVar14 = uVar4 * uVar13 + (param_2 * uVar13 >> 0x10) + (uint)CARRY4(uVar9,uVar14) +
             (uVar16 >> 0x10) + (uint)CARRY4(uVar10,uVar1);
    uVar1 = uVar14 + uVar7;
    iVar8 = uVar4 * uVar12 + (param_2 * uVar12 >> 0x10) + (uint)CARRY4(uVar5,uVar3) +
            (uVar15 >> 0x10) + (uint)CARRY4(uVar6,uVar2) + (uint)CARRY4(uVar14,uVar7);
    if (-1 < iVar8) {
      return CONCAT44(iVar8 * 2 +
                      (uint)(CARRY4(uVar1,uVar1) || CARRY4(uVar1 * 2,(uint)CARRY4(uVar11,uVar11))),
                      param_1) & 0xffffffff80000000;
    }
    return CONCAT44(iVar8,param_1) & 0xffffffff80000000;
  }
  if (unaff_r5 == 0) {
    uVar7 = unaff_r4 >> 0x10;
    uVar13 = param_2 >> 0x10;
    uVar9 = unaff_r4 & ~(uVar7 << 0x10);
    param_2 = param_2 & ~(uVar13 << 0x10);
    uVar4 = param_2 * uVar9;
    param_2 = uVar7 * param_2;
    uVar3 = uVar9 * uVar13 * 0x10000;
    uVar5 = uVar4 + uVar3;
    uVar15 = param_3 >> 0x10;
    uVar2 = param_2 * 0x10000;
    uVar6 = uVar5 + uVar2;
    param_3 = param_3 & ~(uVar15 << 0x10);
    uVar10 = param_3 * uVar9;
    param_3 = uVar7 * param_3;
    uVar14 = uVar9 * uVar15 * 0x10000;
    uVar11 = uVar10 + uVar14;
    uVar1 = param_3 * 0x10000;
    uVar12 = uVar11 + uVar1;
    uVar1 = uVar7 * uVar15 + (uVar9 * uVar15 >> 0x10) + (uint)CARRY4(uVar10,uVar14) +
            (param_3 >> 0x10) + (uint)CARRY4(uVar11,uVar1);
    uVar14 = uVar1 + uVar6;
    iVar8 = uVar7 * uVar13 + (uVar9 * uVar13 >> 0x10) + (uint)CARRY4(uVar4,uVar3) +
            (param_2 >> 0x10) + (uint)CARRY4(uVar5,uVar2) + (uint)CARRY4(uVar1,uVar6);
    if (-1 < iVar8) {
      return CONCAT44(iVar8 * 2 +
                      (uint)(CARRY4(uVar14,uVar14) || CARRY4(uVar14 * 2,(uint)CARRY4(uVar12,uVar12))
                            ),param_1) & 0xffffffff80000000;
    }
    return CONCAT44(iVar8,param_1) & 0xffffffff80000000;
  }
  uVar1 = param_2 >> 0x10;
  uVar4 = unaff_r4 >> 0x10;
  uVar11 = param_2 & ~(uVar1 << 0x10);
  uVar9 = unaff_r4 & ~(uVar4 << 0x10);
  uVar5 = uVar11 * uVar4;
  uVar11 = uVar9 * uVar11;
  uVar9 = uVar1 * uVar9;
  uVar3 = uVar5 * 0x10000;
  uVar12 = uVar11 + uVar3;
  uVar15 = param_3 >> 0x10;
  uVar2 = uVar9 * 0x10000;
  uVar13 = uVar12 + uVar2;
  uVar6 = unaff_r5 >> 0x10;
  uVar16 = param_3 & ~(uVar15 << 0x10);
  uVar10 = unaff_r5 & ~(uVar6 << 0x10);
  uVar7 = uVar16 * uVar6;
  uVar16 = uVar10 * uVar16;
  uVar10 = uVar15 * uVar10;
  uVar14 = uVar7 * 0x10000;
  uVar17 = uVar16 + uVar14;
  uVar7 = uVar15 * uVar6 + (uVar7 >> 0x10) + (uint)CARRY4(uVar16,uVar14);
  iVar8 = 0;
  uVar14 = uVar10 * 0x10000;
  uVar15 = uVar17 + uVar14;
  uVar6 = uVar7 + (uVar10 >> 0x10) + (uint)CARRY4(uVar17,uVar14);
  uVar14 = uVar13 + uVar6;
  uVar2 = uVar1 * uVar4 + (uVar5 >> 0x10) + (uint)CARRY4(uVar11,uVar3) +
          (uVar9 >> 0x10) + (uint)CARRY4(uVar12,uVar2) + (uint)CARRY4(uVar13,uVar6);
  uVar4 = uVar14 + uVar15;
  uVar1 = uVar14 + uVar2 + CARRY4(uVar14,uVar15);
  bVar19 = param_3 <= param_2;
  param_2 = param_2 - param_3;
  bVar18 = param_2 == 0;
  uVar3 = 0;
  if (!bVar19) {
    uVar3 = 0xffffffff;
    iVar8 = unaff_r4 - unaff_r5;
  }
  if (!bVar18) {
    bVar19 = unaff_r4 <= unaff_r5;
    uVar7 = unaff_r5 - unaff_r4;
    bVar18 = uVar7 == 0;
  }
  if (bVar18) {
    uVar3 = 0;
  }
  if (!bVar19) {
    uVar3 = ~uVar3;
    iVar8 = iVar8 - param_2;
  }
  uVar9 = param_2 >> 0x10;
  param_2 = param_2 & ~(uVar9 << 0x10);
  uVar12 = uVar7 >> 0x10;
  uVar7 = uVar7 & ~(uVar12 << 0x10);
  uVar13 = param_2 * uVar12;
  param_2 = uVar7 * param_2;
  uVar7 = uVar9 * uVar7;
  uVar5 = uVar13 * 0x10000;
  uVar10 = param_2 + uVar5;
  uVar6 = uVar7 * 0x10000;
  uVar11 = uVar10 + uVar6;
  uVar5 = uVar9 * uVar12 + iVar8 + (uVar13 >> 0x10) + (uint)CARRY4(param_2,uVar5) +
          (uVar7 >> 0x10) + (uint)CARRY4(uVar10,uVar6);
  uVar6 = uVar1 + uVar5 + CARRY4(uVar4,uVar11);
  iVar8 = uVar3 + uVar2 + (CARRY4(uVar14,uVar2) ||
                          CARRY4(uVar14 + uVar2,(uint)CARRY4(uVar14,uVar15))) +
          (uint)(CARRY4(uVar1,uVar5) || CARRY4(uVar1 + uVar5,(uint)CARRY4(uVar4,uVar11)));
  uVar3 = uVar4 + uVar11 | (uVar15 | uVar15 * 4) >> 2;
  if (-1 < iVar8) {
    return CONCAT44(iVar8 * 2 +
                    (uint)(CARRY4(uVar6,uVar6) || CARRY4(uVar6 * 2,(uint)CARRY4(uVar3,uVar3))),
                    param_1) & 0xffffffff80000000;
  }
  return CONCAT44(iVar8,param_1) & 0xffffffff80000000;
}



/* ===== thunk_EXT_FUN_1fffa30c @ 0002199c, 10 bytes ===== */

void thunk_EXT_FUN_1fffa30c(void)

{
  fast_sincos_lut();
  return;
}



/* ===== thunk_EXT_FUN_1fff9950 @ 000219a6, 10 bytes ===== */

void thunk_EXT_FUN_1fff9950(void)

{
  flash_program_words();
  return;
}



/* ===== thunk_EXT_FUN_1fff9c40 @ 000219b0, 10 bytes ===== */

void thunk_EXT_FUN_1fff9c40(void)

{
  svpwm_write_compare();
  return;
}



/* ===== thunk_EXT_FUN_1fffa3aa @ 000219ba, 10 bytes ===== */

void thunk_EXT_FUN_1fffa3aa(void)

{
  wrap_periodic_value();
  return;
}



/* ===== thunk_EXT_FUN_1fff987c @ 000219c4, 10 bytes ===== */

void thunk_EXT_FUN_1fff987c(void)

{
  output_sensor_lookup_and_unwrap();
  return;
}



/* ===== thunk_EXT_FUN_1fffa160 @ 000219ce, 10 bytes ===== */

void thunk_EXT_FUN_1fffa160(void)

{
  identification_rls2_step();
  return;
}



/* ===== thunk_EXT_FUN_1fffa38a @ 000219d8, 10 bytes ===== */

void thunk_EXT_FUN_1fffa38a(void)

{
  clampf();
  return;
}



/* ===== thunk_EXT_FUN_1fffa234 @ 000219e2, 10 bytes ===== */

void thunk_EXT_FUN_1fffa234(void)

{
  identification_flux_observer_step();
  return;
}



/* ===== thunk_EXT_FUN_1fff9e44 @ 000219ec, 10 bytes ===== */

void thunk_EXT_FUN_1fff9e44(void)

{
  identification_filter_step();
  return;
}



/* ===== adc_sampling_init @ 00021a00, 226 bytes ===== */

void adc_sampling_init(void)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  int iVar3;
  undefined1 *puVar4;
  
  puVar1 = DAT_00021ae4;
  *DAT_00021ae4 = 0xa501;
  puVar1[2] = 0x8000;
  puVar1[4] = 0x8000;
  puVar1[6] = 0x8000;
  puVar2 = DAT_00021ae4;
  DAT_00021ae4[10] = 0x8000;
  puVar2[0xc] = 0x8000;
  puVar2[0xe] = 0x8000;
  puVar2[0x10] = 0x8000;
  DAT_00021ae4[0x24] = 0x8000;
  *puVar1 = 0xa500;
  iVar3 = DAT_00021ae8;
  *(ushort *)(DAT_00021ae8 + 0x3fe) = *(ushort *)(DAT_00021ae8 + 0x3fe) | 0xa502;
  *(undefined2 *)(iVar3 + 0x10) = 0;
  *DAT_00021aec = 0x80;
  *DAT_00021af0 = 1;
  *(ushort *)(iVar3 + 0x3fe) = *(ushort *)(iVar3 + 0x3fe) & 0xfffd | 0xa500;
  *(uint *)(DAT_00021af4 + 0xc) = *(uint *)(DAT_00021af4 + 0xc) & 0xfffffff8;
  puVar4 = DAT_00021af8;
  *(undefined2 *)(DAT_00021af8 + 0x4c) = 0;
  *puVar4 = 0;
  *(undefined2 *)(puVar4 + 2) = 0;
  *(undefined2 *)(puVar4 + 0x38) = 0x3210;
  *(undefined4 *)(puVar4 + 0xc) = 7;
  puVar4[0x20] = 0x14;
  puVar4[0x21] = 0x14;
  puVar4[0x22] = 0x14;
  puVar4 = DAT_00021afc;
  *DAT_00021afc = 0;
  *(undefined2 *)(puVar4 + 2) = 0;
  *(undefined2 *)(DAT_00021afc + 0x38) = 0x3510;
  *(undefined4 *)(DAT_00021afc + 0xc) = 7;
  puVar4 = DAT_00021afc;
  DAT_00021afc[0x20] = 0x14;
  puVar4[0x21] = 0x14;
  puVar4[0x22] = 0x14;
  puVar4 = DAT_00021af0;
  DAT_00021af0[-0x18] = 0;
  *(undefined2 *)(puVar4 + -0x16) = 0;
  *(undefined2 *)(DAT_00021af0 + 0x20) = 0x3b76;
  *(undefined4 *)(DAT_00021af0 + -0xc) = 7;
  puVar4 = DAT_00021af0;
  DAT_00021af0[8] = 0x14;
  puVar4[9] = 0x14;
  puVar4[10] = 0x14;
  return;
}



/* ===== calibrate_adc_offsets @ 00021b00, 262 bytes ===== */

void calibrate_adc_offsets(float *param_1)

{
  undefined1 *puVar1;
  uint in_fpscr;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  ushort local_24;
  
  puVar1 = DAT_00021c0c;
  fVar2 = DAT_00021c08;
  fVar3 = DAT_00021c08;
  fVar5 = DAT_00021c08;
  fVar4 = DAT_00021c08;
  for (local_24 = 0; fVar6 = DAT_00021c18, local_24 < 1000; local_24 = local_24 + 1) {
    *puVar1 = 1;
    *DAT_00021c10 = 1;
    *DAT_00021c14 = 1;
    do {
    } while ((puVar1[0x44] & 1) == 0);
    do {
    } while ((DAT_00021c10[0x44] & 1) == 0);
    do {
    } while ((DAT_00021c14[0x44] & 1) == 0);
    fVar6 = (float)VectorUnsignedToFloat
                             ((uint)*(ushort *)(puVar1 + 0x50),(byte)(in_fpscr >> 0x16) & 3);
    fVar4 = fVar4 + fVar6;
    fVar6 = (float)VectorUnsignedToFloat
                             ((uint)*(ushort *)(DAT_00021c10 + 0x50),(byte)(in_fpscr >> 0x16) & 3);
    fVar5 = fVar5 + fVar6;
    fVar6 = (float)VectorUnsignedToFloat
                             ((uint)*(ushort *)(DAT_00021c14 + 0x50),(byte)(in_fpscr >> 0x16) & 3);
    fVar3 = fVar3 + fVar6;
    puVar1[0x46] = 1;
    fVar6 = (float)VectorUnsignedToFloat
                             ((uint)*(ushort *)(puVar1 + 0x52),(byte)(in_fpscr >> 0x16) & 3);
    fVar2 = fVar2 + fVar6;
    DAT_00021c10[0x46] = 1;
  }
  fVar5 = fVar5 / DAT_00021c18;
  *param_1 = fVar4 / DAT_00021c18;
  param_1[1] = fVar5;
  param_1[2] = fVar3 / fVar6;
  param_1[3] = fVar2 / fVar6;
  return;
}



/* ===== FUN_00021c1c @ 00021c1c, 108 bytes ===== */

void FUN_00021c1c(undefined4 param_1,int param_2,int param_3,uint param_4)

{
  byte *pbVar1;
  int iVar2;
  uint uVar3;
  undefined4 local_3c;
  undefined4 uStack_38;
  undefined4 uStack_34;
  undefined4 uStack_30;
  undefined4 local_2c;
  undefined4 uStack_28;
  undefined4 uStack_24;
  undefined4 uStack_20;
  
  uVar3 = 0;
  local_3c = DAT_00021c88;
  uStack_38 = DAT_00021c8c;
  uStack_34 = DAT_00021c90;
  uStack_30 = DAT_00021c94;
  iVar2 = 0x10;
  do {
    if (param_4 <= uVar3) {
      return;
    }
    if (iVar2 == 0x10) {
      local_2c = local_3c;
      uStack_28 = uStack_38;
      uStack_24 = uStack_34;
      uStack_20 = uStack_30;
      FUN_00021ed0(&local_2c,param_1);
      iVar2 = 0xf;
      do {
        if (*(char *)((int)&local_3c + iVar2) != -1) {
          *(char *)((int)&local_3c + iVar2) = *(char *)((int)&local_3c + iVar2) + '\x01';
          break;
        }
        *(undefined1 *)((int)&local_3c + iVar2) = 0;
        iVar2 = iVar2 + -1;
      } while (-1 < iVar2);
      iVar2 = 0;
    }
    pbVar1 = (byte *)((int)&local_2c + iVar2);
    iVar2 = iVar2 + 1;
    *(byte *)(param_2 + uVar3) = *(byte *)(param_3 + uVar3) ^ *pbVar1;
    uVar3 = uVar3 + 1;
  } while( true );
}



/* ===== FUN_00021c98 @ 00021c98, 48 bytes ===== */

void FUN_00021c98(int param_1,int param_2,int param_3)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  
  uVar2 = 0;
  do {
    uVar1 = 0;
    iVar3 = param_2 + uVar2 * 4;
    do {
      *(byte *)(iVar3 + uVar1) =
           *(byte *)(iVar3 + uVar1) ^ *(byte *)(param_3 + uVar1 + (uVar2 + param_1 * 4) * 4);
      uVar1 = uVar1 + 1 & 0xff;
    } while (uVar1 < 4);
    uVar2 = uVar2 + 1 & 0xff;
  } while (uVar2 < 4);
  return;
}



/* ===== mcan1_configure_classic @ 00021cc8, 396 bytes ===== */

void mcan1_configure_classic(uint param_1,uint param_2)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  int iVar3;
  uint *puVar4;
  int iVar5;
  uint *puVar6;
  ushort uVar7;
  
  *(uint *)(DAT_00021e58 + 4) = *(uint *)(DAT_00021e58 + 4) & 0xfffffffe;
  puVar1 = DAT_00021e5c;
  *DAT_00021e5c = 0xa501;
  puVar2 = DAT_00021e5c;
  DAT_00021e5c[0x2e] = 0x20;
  puVar2[0x30] = 0x20;
  puVar2[0x2f] = 0x33;
  DAT_00021e5c[0x31] = 0x32;
  *puVar1 = 0xa500;
  iVar3 = DAT_00021e60;
  *(ushort *)(DAT_00021e60 + 0x3fe) = *(ushort *)(DAT_00021e60 + 0x3fe) | 0xa501;
  *(ushort *)(iVar3 + 0x18) = *(ushort *)(iVar3 + 0x18) & 0xf0 | 8;
  *(ushort *)(iVar3 + 0x3fe) = *(ushort *)(iVar3 + 0x3fe) & 0xfffd | 0xa500;
  iVar3 = DAT_00021e64;
  *(uint *)(DAT_00021e64 + 0x18) = *(uint *)(DAT_00021e64 + 0x18) & 0xffffffef;
  do {
  } while (*(int *)(iVar3 + 0x18) << 0x1c < 0);
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) | 1;
  do {
  } while ((*(uint *)(iVar3 + 0x18) & 1) == 0);
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) | 2;
  if (4 < param_1) {
    param_1 = 4;
  }
  switch(param_1) {
  case 0:
    iVar5 = 8;
    break;
  case 1:
    iVar5 = 5;
    break;
  case 2:
    iVar5 = 4;
    break;
  case 3:
    iVar5 = 2;
    break;
  default:
    iVar5 = 1;
  }
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xffffffbf;
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xffffbfff;
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) | 0x1000;
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xfffffcff;
  *(uint *)(iVar3 + 0x1c) = iVar5 * 0x10000 - 0x10000U | 0x26003a13;
  *(undefined4 *)(iVar3 + 0xbc) = 0x777;
  *(undefined4 *)(iVar3 + 200) = 7;
  *(undefined4 *)(iVar3 + 0x84) = 0x20000;
  *(undefined4 *)(iVar3 + 0x88) = 0x40;
  *(int *)(iVar3 + 0xa0) = DAT_00021e68;
  *(int *)(iVar3 + 0xb0) = DAT_00021e68 + 0xd8;
  *(undefined4 *)(iVar3 + 0xac) = 0x230;
  *(undefined4 *)(iVar3 + 0xf0) = DAT_00021e6c;
  *(undefined4 *)(iVar3 + 0xc0) = DAT_00021e70;
  puVar4 = DAT_00021e74;
  uVar7 = 0;
  puVar6 = DAT_00021e74;
  do {
    uVar7 = uVar7 + 1;
    *puVar6 = 0;
    puVar6 = puVar6 + 1;
  } while (uVar7 < 0x100);
  *(uint *)(iVar3 + 0x80) = *(uint *)(iVar3 + 0x80) & 0xffffffc0 | 0x2b;
  *puVar4 = DAT_00021e78 | (param_2 & 0x7ff) << 0x10;
  puVar4[1] = DAT_00021e7c;
  *(int *)(iVar3 + 0x54) = DAT_00021e80;
  *(int *)(iVar3 + 0x58) = DAT_00021e80 + -0x10;
  *(undefined4 *)(iVar3 + 0x5c) = 3;
  short_busy_wait(0xf);
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xfffffffe;
  do {
  } while ((*(uint *)(iVar3 + 0x18) & 1) != 0);
  *(undefined4 *)(iVar3 + 0x50) = 0xffffffff;
  *(undefined4 *)(DAT_00021e84 + 0x68) = 0x199;
  FUN_0002386c(3);
  FUN_00023816(3);
  DAT_e000e100 = 8;
  return;
}



/* ===== FUN_00021e88 @ 00021e88, 36 bytes ===== */

void FUN_00021e88(undefined4 param_1,uint param_2)

{
  uint uVar1;
  
  for (uVar1 = 0; uVar1 < param_2; uVar1 = uVar1 + 1) {
  }
  return;
}



/* ===== crc_clock_enable @ 00021eb0, 22 bytes ===== */

void crc_clock_enable(void)

{
  uint *puVar1;
  
  puVar1 = DAT_00021ecc;
  DAT_00021ecc[4] = DAT_00021ec8;
  *puVar1 = *puVar1 & 0xff7fffff;
  puVar1[4] = DAT_00021ec8 - 1;
  return;
}



/* ===== FUN_00021ed0 @ 00021ed0, 80 bytes ===== */

void FUN_00021ed0(undefined4 param_1,undefined4 param_2)

{
  byte bVar1;
  
  FUN_00021c98(0,param_1,param_2);
  bVar1 = 1;
  do {
    FUN_00023494(param_1);
    FUN_00023460(param_1);
    FUN_00022f40(param_1);
    FUN_00021c98(bVar1,param_1,param_2);
    bVar1 = bVar1 + 1;
  } while (bVar1 < 0xe);
  FUN_00023494(param_1);
  FUN_00023460(param_1);
  FUN_00021c98(0xe,param_1,param_2);
  return;
}



/* ===== short_busy_wait @ 00021f20, 6 bytes ===== */

void short_busy_wait(int param_1)

{
  bool bVar1;
  
  do {
    bVar1 = param_1 != 0;
    param_1 = param_1 + -1;
  } while (bVar1);
  return;
}



/* ===== delay_ms @ 00021f28, 52 bytes ===== */

void delay_ms(int param_1)

{
  undefined4 *puVar1;
  
  puVar1 = DAT_00021f5c;
  for (param_1 = param_1 << 1; param_1 != 0; param_1 = param_1 + -1) {
    *puVar1 = 50000;
    *(byte *)(puVar1 + 0x20) = *(byte *)(puVar1 + 0x20) | 1;
    do {
    } while (-1 < (int)((uint)*(byte *)((int)puVar1 + 0x81) << 0x18));
    *(byte *)((int)puVar1 + 0x81) = *(byte *)((int)puVar1 + 0x81) & 0x7f;
  }
  return;
}



/* ===== delay_us @ 00021f60, 94 bytes ===== */

void delay_us(uint param_1)

{
  int *piVar1;
  
  piVar1 = DAT_00021fc0;
  for (; 500 < param_1; param_1 = param_1 - 500) {
    *piVar1 = 50000;
    *(byte *)(piVar1 + 0x20) = *(byte *)(piVar1 + 0x20) | 1;
    do {
    } while (-1 < (int)((uint)*(byte *)((int)piVar1 + 0x81) << 0x18));
    *(byte *)((int)piVar1 + 0x81) = *(byte *)((int)piVar1 + 0x81) & 0x7f;
  }
  *piVar1 = param_1 * 100;
  *(byte *)(piVar1 + 0x20) = *(byte *)(piVar1 + 0x20) | 1;
  do {
  } while (-1 < (int)((uint)*(byte *)((int)piVar1 + 0x81) << 0x18));
  *(byte *)((int)piVar1 + 0x81) = *(byte *)((int)piVar1 + 0x81) & 0x7f;
  return;
}



/* ===== mcan1_configure_fd @ 00021fc4, 526 bytes ===== */

void mcan1_configure_fd(uint param_1,uint param_2)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  int iVar3;
  uint *puVar4;
  uint *puVar5;
  ushort uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  
  *(uint *)(DAT_000221c0 + 4) = *(uint *)(DAT_000221c0 + 4) & 0xfffffffe;
  puVar1 = DAT_000221c4;
  *DAT_000221c4 = 0xa501;
  puVar2 = DAT_000221c4;
  DAT_000221c4[0x2e] = 0x20;
  puVar2[0x30] = 0x20;
  puVar2[0x2f] = 0x33;
  puVar2[0x31] = 0x32;
  *puVar1 = 0xa500;
  iVar3 = DAT_000221c8;
  *(ushort *)(DAT_000221c8 + 0x3fe) = *(ushort *)(DAT_000221c8 + 0x3fe) | 0xa501;
  *(ushort *)(iVar3 + 0x18) = *(ushort *)(iVar3 + 0x18) & 0xf0 | 8;
  *(ushort *)(iVar3 + 0x3fe) = *(ushort *)(iVar3 + 0x3fe) & 0xfffd | 0xa500;
  iVar3 = DAT_000221cc;
  *(uint *)(DAT_000221cc + 0x18) = *(uint *)(DAT_000221cc + 0x18) & 0xffffffef;
  do {
  } while (*(int *)(iVar3 + 0x18) << 0x1c < 0);
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) | 1;
  do {
  } while ((*(uint *)(iVar3 + 0x18) & 1) == 0);
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) | 2;
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xffffffbf;
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xffffbfff;
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xffffefff;
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) | 0x300;
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xffffff5b;
  *(uint *)(iVar3 + 0x10) = *(uint *)(iVar3 + 0x10) & 0xffffffef;
  *(undefined4 *)(iVar3 + 0x1c) = DAT_000221d0;
  if (param_1 < 4) {
    param_1 = 4;
  }
  switch(param_1) {
  default:
    iVar7 = 2;
    goto LAB_00022092;
  case 5:
    iVar7 = 1;
LAB_00022092:
    iVar8 = 0x1f;
    iVar10 = 8;
    iVar9 = 8;
    break;
  case 6:
    iVar7 = 1;
    iVar8 = 0x19;
    iVar10 = 6;
    iVar9 = 6;
    break;
  case 7:
    iVar7 = 1;
    iVar8 = 0x13;
    iVar10 = 5;
    iVar9 = 5;
    break;
  case 8:
    iVar7 = 1;
    iVar8 = 0xf;
    iVar10 = 4;
    iVar9 = 4;
    break;
  case 9:
    iVar7 = 1;
    iVar8 = 0xc;
    iVar10 = 3;
    iVar9 = 3;
    break;
  case 10:
    iVar8 = 7;
    goto LAB_000220cc;
  case 0xb:
    iVar8 = 5;
LAB_000220cc:
    iVar7 = 1;
    iVar10 = 2;
    iVar9 = 2;
  }
  *(uint *)(iVar3 + 0xc) =
       iVar8 * 0x100 - 0x100U | (iVar7 + -1) * 0x10000 | iVar10 * 0x10 - 0x10U | iVar9 - 1U;
  *(int *)(iVar3 + 0x48) = (int)(short)iVar7 * (int)(short)iVar8 * 0x100;
  if (5 < param_1) {
    *(uint *)(iVar3 + 0xc) = *(uint *)(iVar3 + 0xc) | 0x800000;
  }
  *(undefined4 *)(iVar3 + 0xbc) = 0x777;
  *(undefined4 *)(iVar3 + 200) = 7;
  *(undefined4 *)(iVar3 + 0x84) = 0x20000;
  *(undefined4 *)(iVar3 + 0x88) = 0x40;
  *(int *)(iVar3 + 0xa0) = DAT_000221d4;
  *(int *)(iVar3 + 0xb0) = DAT_000221d4 + 0xd8;
  *(undefined4 *)(iVar3 + 0xac) = 0x230;
  *(undefined4 *)(iVar3 + 0xf0) = DAT_000221d8;
  *(undefined4 *)(iVar3 + 0xc0) = DAT_000221dc;
  puVar4 = DAT_000221e0;
  uVar6 = 0;
  puVar5 = DAT_000221e0;
  do {
    uVar6 = uVar6 + 1;
    *puVar5 = 0;
    puVar5 = puVar5 + 1;
  } while (uVar6 < 0x100);
  *(uint *)(iVar3 + 0x80) = *(uint *)(iVar3 + 0x80) & 0xffffffc0 | 0x2b;
  *puVar4 = DAT_000221e4 | (param_2 & 0x7ff) << 0x10;
  puVar4[1] = DAT_000221e8;
  *(int *)(iVar3 + 0x54) = DAT_000221ec;
  *(int *)(iVar3 + 0x58) = DAT_000221ec + -0x10;
  *(undefined4 *)(iVar3 + 0x5c) = 3;
  short_busy_wait(0xf);
  *(uint *)(iVar3 + 0x18) = *(uint *)(iVar3 + 0x18) & 0xfffffffe;
  do {
  } while ((*(uint *)(iVar3 + 0x18) & 1) != 0);
  *(undefined4 *)(iVar3 + 0x50) = 0xffffffff;
  *(undefined4 *)(DAT_000221f0 + 0x68) = 0x199;
  FUN_0002386c(3);
  FUN_00023816(3);
  DAT_e000e100 = 8;
  return;
}



/* ===== read_hardware_variant @ 000221f4, 68 bytes ===== */

ushort read_hardware_variant(void)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  
  puVar1 = DAT_00022238;
  *DAT_00022238 = 0xa501;
  *DAT_0002223c = 2;
  puVar2 = DAT_0002223c;
  DAT_0002223c[-0x4a] = 2;
  puVar2[-0x48] = 0x40;
  puVar2[-0x46] = 0x40;
  *puVar1 = 0xa500;
  *DAT_00022240 = *DAT_00022240 | 0x2000;
  DAT_00022240[0x19] = DAT_00022240[0x19] | 4;
  return DAT_00022240[-4] >> 0xe;
}



/* ===== debug_uart_receive_irq @ 00022244, 716 bytes ===== */

void debug_uart_receive_irq(void)

{
  char cVar1;
  char cVar2;
  char cVar3;
  ushort uVar4;
  int iVar5;
  char *pcVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  int iVar10;
  undefined1 *puVar11;
  undefined4 *puVar12;
  undefined4 uVar13;
  uint uVar14;
  int iVar15;
  
  iVar5 = DAT_00022518;
  if ((*DAT_00022514 & 0x100) == 0) goto LAB_000224f0;
  *(uint *)(DAT_00022518 + 0x34) = *(uint *)(DAT_00022518 + 0x34) | 1;
  uVar4 = 200 - (short)((uint)*(undefined4 *)(iVar5 + 0x68) >> 0x10);
  iVar15 = (int)(short)uVar4;
  DAT_0002251c[1] = uVar4;
  *(uint *)(iVar5 + 0x48) = *(uint *)(iVar5 + 0x48) & 0xffff | 0xc80000;
  pcVar6 = DAT_00022520;
  *(char **)(iVar5 + 0x44) = DAT_00022520;
  puVar12 = DAT_00022554;
  puVar11 = DAT_00022540;
  iVar10 = DAT_0002253c;
  iVar9 = DAT_00022538;
  iVar8 = DAT_00022534;
  iVar7 = DAT_00022528;
  iVar5 = DAT_00022524;
  cVar3 = *pcVar6;
  if (cVar3 == '\x1b') {
    *(undefined4 *)(DAT_00022528 + 0x38) = 0;
    if (*(int *)(iVar5 + 0x80) == 1) {
      *(undefined4 *)(iVar5 + 0x80) = 0;
    }
    *(undefined4 *)(iVar7 + 0x3c) = 1;
LAB_000222c0:
    if (cVar3 == 'm') {
      if (*(uint *)(iVar5 + 0x80) < 2) {
        *(undefined4 *)(iVar7 + 0x38) = 2;
        *(undefined4 *)(iVar5 + 0x80) = 1;
        *(undefined4 *)(iVar7 + 0x3c) = 1;
      }
    }
    else if (cVar3 == 's') {
      *(undefined4 *)(iVar7 + 0x38) = 4;
    }
  }
  else {
    if (cVar3 == 'X' && iVar15 == 1) {
      disableIRQinterrupts();
      select_configuration_bank_b();
      delay_us(100);
      DataSynchronizationBarrier(0xf);
      *DAT_0002252c = *DAT_0002252c & 0x700 | DAT_00022530;
      DataSynchronizationBarrier(0xf);
      do {
                    /* WARNING: Do nothing block with infinite loop */
      } while( true );
    }
    if (*(int *)(DAT_00022528 + 0x38) == 0) goto LAB_000222c0;
    if (*(int *)(DAT_00022528 + 0x38) == 4) {
      cVar1 = DAT_00022520[1];
      cVar2 = DAT_00022520[2];
      if (cVar3 == 'U') {
        if (cVar1 == 'c') {
          *(undefined4 *)(DAT_00022534 + 0x10) = 1;
        }
        else if (cVar1 == 'l') {
          *(undefined4 *)(DAT_00022534 + 0x14) = 1;
        }
        else if (cVar1 == 'd') {
          if (iVar15 == *DAT_0002251c + 1) {
            runtime_memcpy(DAT_0002253c,pcVar6 + 1);
            runtime_memcpy(DAT_00022544 + (uint)*(byte *)(iVar10 + 0x3e) * 0x3c,iVar10 + 1,
                           (uint)*(byte *)(iVar10 + 0x3d) << 2);
            *puVar11 = 100;
            puVar11[1] = *(undefined1 *)(iVar10 + 0x3e);
            debug_uart_write(DAT_00022540,2);
            if (*(char *)(iVar10 + 0x3e) == '\x11') {
              *(undefined4 *)(iVar8 + 0x1c) = 1;
              *(undefined4 *)(DAT_00022548 + 8) = *(undefined4 *)(iVar7 + 0x34);
            }
          }
        }
        else if (cVar1 == 'M') {
          if (iVar15 == *DAT_0002251c + 1) {
            runtime_memcpy(DAT_0002253c,pcVar6 + 1);
            runtime_memcpy(DAT_0002254c + (uint)*(byte *)(iVar10 + 0x3e) * 0x3c,iVar10 + 1,
                           (uint)*(byte *)(iVar10 + 0x3d) << 1);
            *puVar11 = 0x4d;
            puVar11[1] = *(undefined1 *)(iVar10 + 0x3e);
            debug_uart_write(DAT_00022540,2);
            if (*(char *)(iVar10 + 0x3e) == -0x78) {
              *(undefined4 *)(iVar8 + 0x1c) = 2;
            }
          }
        }
        else if (cVar1 == 'e') {
          if (cVar2 == -0x56) {
            if (iVar15 != 3) goto LAB_000224ec;
            *DAT_00022540 = 0x65;
            *(undefined4 *)(puVar11 + 1) = *(undefined4 *)(iVar9 + 0x44);
            *(undefined4 *)(puVar11 + 5) = *(undefined4 *)(iVar9 + 0x48);
            *(undefined4 *)(puVar11 + 9) = *(undefined4 *)(iVar9 + 0x4c);
            debug_uart_write(DAT_00022540,0xd);
          }
          if ((pcVar6[2] == 'U') && (DAT_0002251c[1] == 3)) {
            *(undefined4 *)(iVar8 + 0x24) = 1;
          }
        }
        else if (cVar1 == 'f') {
          if (cVar2 == -0x56 && iVar15 == 3) {
            *DAT_00022540 = 0x66;
            iVar5 = DAT_00022550;
            *(undefined4 *)(puVar11 + 1) = *(undefined4 *)(DAT_00022550 + 8);
            *(undefined4 *)(puVar11 + 5) = *(undefined4 *)(iVar5 + 0xc);
            debug_uart_write(DAT_00022540,9);
            *(undefined4 *)(iVar7 + 0x38) = 0;
          }
        }
        else if (cVar1 == 'g') {
          if (cVar2 == -0x56) {
            if (iVar15 == 3) {
              *(undefined4 *)(DAT_00022534 + 0x28) = 1;
            }
          }
          else if (cVar2 == 'U') {
            if (iVar15 == 0x83) {
              runtime_memcpy(DAT_00022538,pcVar6 + 3,0x80);
              *(undefined4 *)(iVar8 + 0x28) = 2;
            }
          }
          else if (cVar2 == 'Z') {
            if (iVar15 == 0x83) {
              runtime_memcpy(DAT_00022538,pcVar6 + 3,0x80);
              uVar13 = 4;
              goto LAB_000224c2;
            }
          }
          else if (cVar2 == 'Q' && iVar15 == 0x83) {
            runtime_memcpy(DAT_00022538,pcVar6 + 3,iVar15 + -3);
            uVar13 = 6;
LAB_000224c2:
            *(undefined4 *)(iVar8 + 0x28) = uVar13;
          }
        }
      }
      else if (((cVar3 == 'F' && cVar1 == 'C') && cVar2 == 'B') &&
              (uVar14 = (byte)pcVar6[3] - 0x30 & 0xff, uVar14 < 0xc)) {
        *(uint *)(DAT_00022538 + 0x8c) = uVar14;
        uVar13 = DAT_00022558;
        if (uVar14 < 5) {
          uVar13 = DAT_0002255c;
        }
        *puVar12 = uVar13;
        uVar13 = DAT_00022560;
        if (uVar14 < 5) {
          uVar13 = DAT_00022564;
        }
        puVar12[1] = uVar13;
        *(undefined4 *)(iVar8 + 0xc) = 1;
      }
    }
  }
LAB_000224ec:
  *(undefined4 *)(DAT_00022518 + 0x1c) = 1;
LAB_000224f0:
  *(uint *)(DAT_00022568 + 0x10) = *(uint *)(DAT_00022568 + 0x10) & 0xfffffffe;
  *DAT_0002256c = *DAT_0002256c | 0x1b0000;
  DAT_e000e280 = 0x10;
  return;
}



/* ===== FUN_00022570 @ 00022570, 216 bytes ===== */

void FUN_00022570(int param_1,int param_2)

{
  byte bVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  byte local_20 [4];
  undefined4 uStack_1c;
  
  uStack_1c = DAT_0002264c;
  uVar3 = 0;
  do {
    iVar5 = param_1 + uVar3 * 4;
    *(undefined1 *)(param_1 + uVar3 * 4) = *(undefined1 *)(param_2 + uVar3 * 4);
    iVar4 = param_2 + uVar3 * 4;
    uVar3 = uVar3 + 1;
    *(undefined1 *)(iVar5 + 1) = *(undefined1 *)(iVar4 + 1);
    *(undefined1 *)(iVar5 + 2) = *(undefined1 *)(iVar4 + 2);
    *(undefined1 *)(iVar5 + 3) = *(undefined1 *)(iVar4 + 3);
    iVar4 = DAT_00022654;
  } while (uVar3 < 8);
  uVar3 = 8;
  do {
    iVar5 = uVar3 * 4;
    bVar1 = *(byte *)(param_1 + iVar5 + -4);
    iVar6 = iVar5 + -4 + param_1;
    uVar2 = CONCAT13(*(byte *)(iVar6 + 3),
                     CONCAT12(*(byte *)(iVar6 + 2),CONCAT11(*(byte *)(iVar6 + 1),bVar1)));
    if ((uVar3 & 7) == 0) {
      uVar2 = (uint)*(byte *)(iVar4 + (uint)*(byte *)(iVar6 + 2)) << 8 |
              (uint)*(byte *)(iVar4 + (uint)*(byte *)(iVar6 + 3)) << 0x10 |
              (uint)*(byte *)(iVar4 + (uint)bVar1) << 0x18 |
              (uint)(*(byte *)(iVar4 + (uint)*(byte *)(iVar6 + 1)) ^ local_20[uVar3 >> 3]);
    }
    if ((uVar3 & 7) == 4) {
      uVar2 = CONCAT13(*(undefined1 *)(iVar4 + (uVar2 >> 0x18)),
                       CONCAT12(*(undefined1 *)(iVar4 + ((uVar2 & 0xff0000) >> 0x10)),
                                CONCAT11(*(undefined1 *)(iVar4 + ((uVar2 & 0xff00) >> 8)),
                                         *(undefined1 *)(iVar4 + (uVar2 & 0xff)))));
    }
    uVar3 = uVar3 + 1;
    *(byte *)(param_1 + iVar5) = *(byte *)(param_1 + iVar5 + -0x20) ^ (byte)uVar2;
    iVar6 = param_1 + iVar5 + -0x20;
    iVar5 = iVar5 + param_1;
    *(byte *)(iVar5 + 1) = *(byte *)(iVar6 + 1) ^ (byte)(uVar2 >> 8);
    *(byte *)(iVar5 + 2) = *(byte *)(iVar6 + 2) ^ (byte)(uVar2 >> 0x10);
    *(byte *)(iVar5 + 3) = *(byte *)(iVar6 + 3) ^ (byte)(uVar2 >> 0x18);
  } while (uVar3 < 0x3c);
  return;
}



/* ===== load_and_validate_calibration @ 00022658, 470 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void load_and_validate_calibration(void)

{
  byte bVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int iVar5;
  undefined4 *puVar6;
  uint uVar7;
  float fVar8;
  undefined4 uVar9;
  uint uVar10;
  float *pfVar11;
  uint uVar12;
  undefined4 uVar13;
  undefined4 uVar14;
  uint in_fpscr;
  float fVar15;
  float fVar16;
  float fVar17;
  undefined8 uVar18;
  
  iVar4 = DAT_00022838;
  iVar2 = DAT_00022834;
  fVar17 = DAT_00022830;
  uVar7 = 0;
  do {
    pfVar11 = (float *)(iVar4 + uVar7 * 4);
    fVar8 = *(float *)(iVar2 + uVar7 * 4);
    *pfVar11 = fVar8;
    if (0x7f800000 - (int)ABS(fVar8) < 0) {
      *pfVar11 = fVar17;
    }
    iVar5 = DAT_0002283c;
    iVar3 = DAT_00022834;
    uVar7 = uVar7 + 1 & 0xffff;
  } while (uVar7 < 0x100);
  uVar7 = *(uint *)(DAT_00022834 + 0x400);
  *(uint *)(DAT_0002283c + 0x14) = uVar7;
  if ((int)(0x7f800000 - (uVar7 & 0x7fffffff)) < 0) {
    *(float *)(iVar5 + 0x14) = fVar17;
  }
  uVar7 = *(uint *)(iVar3 + 0x408);
  *(uint *)(iVar5 + 0x34) = uVar7;
  if ((int)(0x7f800000 - (uVar7 & 0x7fffffff)) < 0) {
    *(undefined4 *)(iVar5 + 0x34) = 0x3f800000;
  }
  iVar2 = DAT_00022840;
  uVar9 = DAT_00022844[1];
  *(undefined4 *)(DAT_00022840 + 4) = *DAT_00022844;
  *(undefined4 *)(iVar2 + 8) = uVar9;
  pfVar11 = (float *)(iVar2 + 4);
  if (0x7f800000 - (int)ABS(*pfVar11) < 0) {
    *pfVar11 = fVar17;
  }
  if ((int)(0x7f800000 - (*(uint *)(iVar2 + 8) & 0x7fffffff)) < 0) {
    *(float *)(iVar2 + 8) = fVar17;
    fVar8 = fVar17;
  }
  else {
    fVar8 = *(float *)(iVar2 + 8);
  }
  *(float *)(iVar5 + 0x24) = fVar8;
  puVar6 = DAT_0002284c;
  iVar2 = DAT_00022848;
  *(float *)(DAT_00022848 + 0x38) = *pfVar11;
  uVar9 = puVar6[1];
  uVar13 = puVar6[2];
  uVar14 = puVar6[3];
  *(undefined4 *)(iVar2 + -0x130) = *puVar6;
  *(undefined4 *)(iVar2 + -300) = uVar9;
  *(undefined4 *)(iVar2 + -0x128) = uVar13;
  *(undefined4 *)(iVar2 + -0x124) = uVar14;
  *(undefined4 *)(iVar2 + 0x14) = *puVar6;
  *(undefined4 *)(iVar2 + 0x1c) = puVar6[1];
  *(undefined4 *)(iVar2 + 0x28) = puVar6[2];
  thunk_EXT_FUN_1fffa30c(puVar6[3],iVar2 + 0x2c);
  runtime_memcpy_aligned(DAT_00022854,DAT_00022850,0x2000);
  iVar2 = DAT_00022870;
  uVar7 = 1;
  do {
    uVar12 = (uint)*(ushort *)(DAT_00022854 + uVar7 * 2);
    uVar10 = (uint)*(ushort *)(DAT_00022854 + uVar7 * 2 + -2);
    fVar15 = (float)VectorUnsignedToFloat(uVar10,(byte)(in_fpscr >> 0x16) & 3);
    fVar16 = (float)VectorUnsignedToFloat(uVar12,(byte)(in_fpscr >> 0x16) & 3);
    fVar8 = DAT_00022868;
    if (uVar10 == 0xffff || uVar12 == 0xffff) break;
    fVar15 = ABS(fVar16 * DAT_00022858 - fVar15 * DAT_00022858);
    if (DAT_0002285c < (int)fVar15) {
      fVar15 = fVar15 - DAT_00022864;
    }
    else if (DAT_00022860 < (int)fVar15) {
      fVar15 = DAT_00022864 - fVar15;
    }
    uVar10 = in_fpscr & 0xfffffff | (uint)(fVar15 < fVar17) << 0x1f |
             (uint)(fVar15 == fVar17) << 0x1e;
    in_fpscr = uVar10 | (uint)(NAN(fVar15) || NAN(fVar17)) << 0x1c;
    bVar1 = (byte)(uVar10 >> 0x18);
    fVar8 = fVar17;
    if (!(bool)(bVar1 >> 6 & 1) && bVar1 >> 7 == ((byte)(in_fpscr >> 0x1c) & 1)) {
      fVar8 = fVar15;
    }
    uVar7 = uVar7 + 1 & 0xffff;
    fVar17 = fVar8;
  } while (uVar7 < 0x1000);
  if (DAT_0002286c < (int)fVar8) {
    debug_printf("Error,O-sensor need calibration!\r\n");
    uVar9 = 2;
  }
  else {
    if ((int)fVar8 <= _DAT_00022898) {
      *(undefined4 *)(DAT_00022870 + 0x80) = 0;
      *DAT_000228b8 = *DAT_000228b8 | 0x2000;
      *DAT_000228bc = *DAT_000228bc | 4;
      return;
    }
    uVar18 = FUN_00027ad8(fVar8);
    uVar9 = (undefined4)((ulonglong)uVar18 >> 0x20);
    debug_printf("O-sensor fail!Max=%.4f\r\n",uVar9,(int)uVar18,uVar9);
    uVar9 = 3;
  }
  *(undefined4 *)(iVar2 + 0x80) = uVar9;
  return;
}



/* ===== load_motor_configuration @ 000228c0, 706 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void load_motor_configuration(void)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  float fVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  bool bVar7;
  uint in_fpscr;
  float fVar8;
  float fVar9;
  undefined4 uVar10;
  float fVar11;
  
  uVar2 = DAT_00022b88;
  fVar9 = DAT_00022b84;
  iVar6 = *(int *)(DAT_00022b8c + 0x3c);
  bVar7 = iVar6 != -1;
  if (bVar7) {
    iVar6 = *(int *)(DAT_00022b8c + 0x38);
  }
  iVar5 = DAT_00022b8c;
  if (bVar7 && iVar6 != -1) {
    iVar5 = *(int *)(DAT_00022b8c + 0x20);
  }
  if ((!bVar7 || iVar6 == -1) || iVar5 == -1) {
    disableIRQinterrupts();
    thunk_EXT_FUN_1fff9950(DAT_00022b8c,DAT_00022b90,0x25);
    enableIRQinterrupts();
  }
  runtime_memcpy_aligned(DAT_00022b90,DAT_00022b8c,0x94);
  iVar6 = DAT_00022b90;
  *(uint *)(DAT_00022b90 + 0x38) = DAT_00022b94 & 0xffffff | 0x37000000;
  *(undefined4 *)(iVar6 + 0x90) = _DAT_00022b9c;
  if (0xb < *(uint *)(iVar6 + 0x8c)) {
    *(undefined4 *)(iVar6 + 0x8c) = 4;
  }
  if ((int)(0x7f800000 - (*(uint *)(iVar6 + 0x80) & 0x7fffffff)) < 0) {
    *(undefined4 *)(iVar6 + 0x80) = DAT_00022ba0;
  }
  iVar5 = DAT_00022ba4;
  if (DAT_00022ba4 <= *(int *)(iVar6 + 0x80)) {
    *(undefined4 *)(iVar6 + 0x80) = DAT_00022ba8;
  }
  if ((int)(0x7f800000 - (*(uint *)(iVar6 + 0x84) & 0x7fffffff)) < 0) {
    *(undefined4 *)(iVar6 + 0x84) = DAT_00022bac;
  }
  if ((int)(0x7f800000 - (*(uint *)(iVar6 + 0x88) & 0x7fffffff)) < 0) {
    *(undefined4 *)(iVar6 + 0x88) = DAT_00022bb0;
  }
  puVar1 = DAT_00022bb4;
  if (*(uint *)(iVar6 + 0x8c) < 5) {
    *DAT_00022bb4 = DAT_00022bb8;
    uVar10 = DAT_00022bbc;
  }
  else {
    if (0xb < *(uint *)(iVar6 + 0x8c)) goto LAB_000229a2;
    *DAT_00022bb4 = DAT_00022bc0;
    uVar10 = DAT_00022bc4;
  }
  puVar1[1] = uVar10;
LAB_000229a2:
  puVar1 = DAT_00022bcc;
  *DAT_00022bcc = DAT_00022bc8;
  puVar1[1] = DAT_00022bd0;
  fVar11 = DAT_00022bd4;
  puVar1[2] = DAT_00022bd4;
  puVar1[3] = DAT_00022bd8;
  uVar10 = DAT_00022bdc;
  puVar1[4] = DAT_00022bdc;
  puVar1[5] = 0x40a00000;
  puVar1[9] = 0x3f800000;
  puVar1[10] = 0x40000000;
  puVar1[0xb] = uVar10;
  puVar1[0xc] = 0x3f800000;
  puVar1[6] = 0x40000000;
  puVar1[7] = DAT_00022be0;
  puVar1[8] = DAT_00022be4;
  fVar8 = *(float *)(iVar6 + 0x80);
  puVar1[0xd] = fVar8;
  puVar1[0xe] = *(undefined4 *)(iVar6 + 0x60);
  puVar1[0xf] = *(undefined4 *)(iVar6 + 0x84);
  uVar10 = VectorUnsignedToFloat(*(undefined4 *)(iVar6 + 0x40),(byte)(in_fpscr >> 0x16) & 3);
  puVar1[0x10] = uVar10;
  puVar1[0x11] = *(undefined4 *)(iVar6 + 0x44);
  puVar1[0x12] = *(undefined4 *)(iVar6 + 0x48);
  puVar1[0x13] = *(undefined4 *)(iVar6 + 0x4c);
  puVar1[0x14] = *(undefined4 *)(iVar6 + 0x30);
  puVar1[0x15] = *(undefined4 *)(iVar6 + 0x7c);
  puVar1[0x16] = fVar9;
  puVar1[0x17] = DAT_00022be8;
  iVar4 = DAT_00022bf0;
  fVar3 = DAT_00022bec;
  puVar1 = (undefined4 *)(DAT_00022bf0 + 0x6c);
  if ((int)fVar8 < iVar5) {
    fVar9 = fVar9 / (fVar9 + fVar8 * DAT_00022bec);
    *(float *)(DAT_00022bf0 + 0x68) = fVar9;
    *(float *)(iVar4 + 0x6c) = 1.0 - fVar9;
  }
  else {
    *(undefined4 *)(DAT_00022bf0 + 0x68) = uVar2;
    *puVar1 = 0x3f800000;
  }
  derive_control_parameters();
  puVar1 = DAT_00022bf4;
  uVar10 = *(undefined4 *)(iVar6 + 0x40);
  fVar9 = (float)VectorUnsignedToFloat(uVar10,(byte)(in_fpscr >> 0x16) & 3);
  fVar8 = *(float *)(iVar6 + 0x4c);
  *DAT_00022bf4 = *(undefined4 *)(iVar6 + 100);
  puVar1[1] = *(undefined4 *)(iVar6 + 0x68);
  puVar1 = DAT_00022bf8;
  *DAT_00022bf8 = *(undefined4 *)(iVar6 + 0x6c);
  puVar1[1] = *(undefined4 *)(iVar6 + 0x70);
  in_fpscr = in_fpscr & 0xfffffff;
  if (*(float *)(iVar6 + 4) == 0.0) {
    fVar11 = *(float *)(iVar6 + 0x50) * fVar9 * 1.5 * fVar8 * fVar11 * *(float *)(iVar6 + 0x78);
  }
  else {
    fVar11 = *(float *)(iVar6 + 4) * fVar11;
  }
  *(float *)(iVar4 + 0x44) = fVar11;
  fVar8 = (float)VectorUnsignedToFloat(uVar10,(byte)(in_fpscr >> 0x16) & 3);
  *(float *)(iVar4 + 0x48) = 1.0 / fVar11;
  fVar11 = (float)VectorUnsignedToFloat(uVar10,(byte)(in_fpscr >> 0x16) & 3);
  *(undefined4 *)(iVar4 + 0x54) = uVar10;
  fVar9 = (float)VectorUnsignedToFloat(uVar10,(byte)(in_fpscr >> 0x16) & 3);
  *(float *)(iVar4 + 0x58) = fVar8 / fVar3;
  *(float *)(iVar4 + 100) = 1.0 / fVar11;
  fVar11 = *(float *)(iVar6 + 0x50);
  *(float *)(iVar4 + 0x5c) = 1.0 / fVar11;
  *(float *)(iVar4 + 0x60) = fVar9 * fVar11;
  *(float *)(iVar4 + 0x4c) = fVar11;
  if (3 < *(int *)(iVar6 + 0x28) - 1U) {
    *(undefined4 *)(iVar6 + 0x28) = 1;
  }
  uVar10 = DAT_00022c00;
  iVar5 = DAT_00022bfc;
  *(undefined4 *)(DAT_00022bfc + 0x3c) = *(undefined4 *)(iVar6 + 0x28);
  *(undefined4 *)(iVar4 + 0x74) = uVar10;
  *(undefined4 *)(iVar4 + 0x78) = DAT_00022c04;
  *(undefined4 *)(iVar5 + 0xac) = DAT_00022c08;
  *(undefined4 *)(iVar5 + 0xb0) = DAT_00022c0c;
  *(undefined4 *)(iVar5 + 0x11c) = 0;
  *(undefined4 *)(iVar5 + 0x38) = 0;
  *(undefined4 *)(iVar4 + 0x20) = uVar2;
  return;
}



/* ===== position_sensor_configure @ 00022c10, 132 bytes ===== */

void position_sensor_configure(void)

{
  int iVar1;
  int iVar2;
  undefined1 extraout_var;
  undefined1 extraout_var_00;
  uint uVar3;
  undefined4 local_28;
  undefined4 uStack_24;
  undefined4 uStack_20;
  
  local_28 = DAT_00022c94;
  uStack_24 = DAT_00022c98;
  uStack_20 = DAT_00022c9c;
  position_sensor_spi_gpio_configure();
  iVar1 = DAT_00022ca0;
  uVar3 = 0;
  do {
    position_sensor_spi_transfer16((uint)*(byte *)((int)&local_28 + uVar3) << 8 | 0x4000);
    delay_ms(1);
    position_sensor_spi_transfer16(0);
    *(undefined1 *)(iVar1 + uVar3) = extraout_var;
    delay_ms(0x19);
    iVar2 = DAT_00022ca4;
    uVar3 = uVar3 + 1 & 0xff;
  } while (uVar3 < 0xb);
  uVar3 = 0;
  do {
    if ((uint)*(byte *)(iVar1 + uVar3) != (uint)*(byte *)(iVar2 + uVar3)) {
      position_sensor_spi_transfer16
                ((uint)*(byte *)(iVar2 + uVar3) |
                 (uint)*(byte *)((int)&local_28 + uVar3) << 8 | 0x8000);
      delay_ms(0x19);
      position_sensor_spi_transfer16(0);
      *(undefined1 *)(iVar1 + uVar3) = extraout_var_00;
      delay_ms(5);
    }
    uVar3 = uVar3 + 1 & 0xff;
  } while (uVar3 < 10);
  position_sensor_dma_configure();
  return;
}



/* ===== power_stage_switch_response_test @ 00022ca8, 642 bytes ===== */

byte power_stage_switch_response_test(void)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  undefined1 *puVar3;
  byte bVar4;
  byte *pbVar5;
  ushort *puVar6;
  byte *pbVar7;
  ushort *puVar8;
  undefined1 *puVar9;
  undefined1 *puVar10;
  
  puVar1 = DAT_00022f2c;
  *DAT_00022f2c = 0xa501;
  puVar2 = DAT_00022f2c;
  DAT_00022f2c[0x3c] = 0x10;
  puVar2[0x3e] = 0x10;
  puVar2[0x40] = 0x10;
  puVar2 = DAT_00022f2c;
  DAT_00022f2c[0x12] = 0x10;
  puVar2[0x14] = 0x10;
  puVar2[0x16] = 0x10;
  *DAT_00022f30 = *DAT_00022f30 | 0xe000;
  puVar6 = DAT_00022f30;
  puVar8 = DAT_00022f30 + -8;
  *puVar8 = *puVar8 | 0x700;
  *puVar1 = 0xa500;
  delay_us(0x14);
  puVar6 = puVar6 + -9;
  *puVar6 = *puVar6 | 0x100;
  delay_us(10);
  puVar3 = DAT_00022f34;
  *DAT_00022f34 = 1;
  *DAT_00022f38 = 1;
  *DAT_00022f3c = 1;
  do {
  } while ((puVar3[0x44] & 1) == 0);
  puVar3[0x46] = 1;
  DAT_00022f38[0x46] = 1;
  DAT_00022f3c[0x46] = 1;
  bVar4 = 2000 < *(ushort *)(puVar3 + 0x50) - 1000;
  *puVar8 = *puVar8 | 0x100;
  delay_us(0x14);
  *puVar6 = *puVar6 | 0x200;
  delay_us(10);
  *puVar3 = 1;
  *DAT_00022f38 = 1;
  *DAT_00022f3c = 1;
  pbVar5 = DAT_00022f38 + 0x44;
  do {
  } while ((*pbVar5 & 1) == 0);
  puVar3[0x46] = 1;
  DAT_00022f38[0x46] = 1;
  DAT_00022f3c[0x46] = 1;
  if (2000 < *(ushort *)(DAT_00022f38 + 0x50) - 1000) {
    bVar4 = bVar4 | 4;
  }
  *puVar8 = *puVar8 | 0x200;
  delay_us(0x14);
  *puVar6 = *puVar6 | 0x400;
  delay_us(10);
  *puVar3 = 1;
  *DAT_00022f38 = 1;
  *DAT_00022f3c = 1;
  pbVar7 = DAT_00022f3c + 0x44;
  do {
  } while ((*pbVar7 & 1) == 0);
  puVar3[0x46] = 1;
  DAT_00022f38[0x46] = 1;
  DAT_00022f3c[0x46] = 1;
  if (2000 < *(ushort *)(DAT_00022f3c + 0x50) - 1000) {
    bVar4 = bVar4 | 0x10;
  }
  *puVar8 = *puVar8 | 0x400;
  delay_us(0x14);
  DAT_00022f30[-1] = DAT_00022f30[-1] | 0x2000;
  delay_us(10);
  *puVar3 = 1;
  *DAT_00022f38 = 1;
  *DAT_00022f3c = 1;
  do {
  } while ((puVar3[0x44] & 1) == 0);
  puVar3[0x46] = 1;
  DAT_00022f38[0x46] = 1;
  puVar9 = DAT_00022f3c + 0x46;
  *puVar9 = 1;
  if (2000 < *(ushort *)(puVar3 + 0x50) - 1000) {
    bVar4 = bVar4 | 2;
  }
  *DAT_00022f30 = *DAT_00022f30 | 0x2000;
  delay_us(0x14);
  puVar6 = DAT_00022f30 + -1;
  *puVar6 = 0x4000;
  delay_us(10);
  *puVar3 = 1;
  *DAT_00022f38 = 1;
  *DAT_00022f3c = 1;
  do {
  } while ((*pbVar5 & 1) == 0);
  puVar3[0x46] = 1;
  puVar10 = DAT_00022f38 + 0x46;
  *puVar10 = 1;
  *puVar9 = 1;
  puVar8 = DAT_00022f30;
  if (2000 < *(ushort *)(DAT_00022f38 + 0x50) - 1000) {
    bVar4 = bVar4 | 8;
  }
  *DAT_00022f30 = *DAT_00022f30 | 0x4000;
  delay_us(0x14);
  *puVar6 = *puVar6 | 0x8000;
  delay_us(10);
  *puVar3 = 1;
  *DAT_00022f38 = 1;
  *DAT_00022f3c = 1;
  do {
  } while ((*pbVar7 & 1) == 0);
  puVar3[0x46] = 1;
  *puVar10 = 1;
  *puVar9 = 1;
  if (2000 < *(ushort *)(DAT_00022f3c + 0x50) - 1000) {
    bVar4 = bVar4 | 0x20;
  }
  *puVar8 = *puVar8 | 0x8000;
  delay_us(0x14);
  return bVar4;
}



/* ===== FUN_00022f40 @ 00022f40, 106 bytes ===== */

void FUN_00022f40(int param_1)

{
  byte bVar1;
  byte bVar2;
  byte bVar3;
  int iVar4;
  int extraout_r2;
  int extraout_r2_00;
  int extraout_r2_01;
  int extraout_r2_02;
  uint uVar5;
  int extraout_r3;
  int extraout_r3_00;
  
  uVar5 = 0;
  do {
    iVar4 = param_1 + uVar5 * 4;
    bVar3 = *(byte *)(param_1 + uVar5 * 4);
    bVar1 = *(byte *)(iVar4 + 1) ^ bVar3 ^ *(byte *)(iVar4 + 2) ^ *(byte *)(iVar4 + 3);
    bVar2 = FUN_00027034();
    *(byte *)(param_1 + extraout_r3 * 4) = *(byte *)(param_1 + extraout_r3 * 4) ^ bVar2 ^ bVar1;
    bVar2 = FUN_00027034(*(byte *)(extraout_r2 + 1) ^ *(byte *)(extraout_r2 + 2));
    *(byte *)(extraout_r2_00 + 1) = *(byte *)(extraout_r2_00 + 1) ^ bVar2 ^ bVar1;
    bVar2 = FUN_00027034(*(byte *)(extraout_r2_00 + 2) ^ *(byte *)(extraout_r2_00 + 3));
    *(byte *)(extraout_r2_01 + 2) = *(byte *)(extraout_r2_01 + 2) ^ bVar2 ^ bVar1;
    bVar3 = FUN_00027034(*(byte *)(extraout_r2_01 + 3) ^ bVar3);
    uVar5 = extraout_r3_00 + 1U & 0xff;
    *(byte *)(extraout_r2_02 + 3) = *(byte *)(extraout_r2_02 + 3) ^ bVar3 ^ bVar1;
  } while (uVar5 < 4);
  return;
}



/* ===== pwm_timer_init @ 00022fac, 258 bytes ===== */

void pwm_timer_init(void)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  int iVar3;
  undefined4 uVar4;
  
  *(uint *)(DAT_000230b0 + 8) = *(uint *)(DAT_000230b0 + 8) & 0xfffffdff;
  puVar1 = DAT_000230b4;
  *DAT_000230b4 = 0xa501;
  puVar2 = DAT_000230b4;
  DAT_000230b4[0x13] = 2;
  puVar2[0x15] = 2;
  puVar2[0x17] = 2;
  puVar2 = DAT_000230b4;
  DAT_000230b4[0x3d] = 2;
  puVar2[0x3f] = 2;
  puVar2[0x41] = 2;
  *puVar1 = 0xa500;
  iVar3 = DAT_000230b8;
  *(undefined2 *)(DAT_000230b8 + 0x58) = 0x70;
  *(undefined2 *)(iVar3 + 0x5a) = 0;
  *(undefined2 *)(iVar3 + 0x50) = 5000;
  *(undefined2 *)(iVar3 + 0x20) = 0xff00;
  *(undefined2 *)(iVar3 + 0x24) = 0xff00;
  *(undefined2 *)(iVar3 + 0x28) = 0xff00;
  *(undefined2 *)(iVar3 + 0x22) = 8;
  *(undefined2 *)(iVar3 + 0x26) = 8;
  *(undefined2 *)(iVar3 + 0x2a) = 8;
  *(undefined2 *)(iVar3 + 4) = 0x9c4;
  *(undefined2 *)(iVar3 + 0xc) = 0x9c4;
  *(undefined2 *)(iVar3 + 0x14) = 0x9c4;
  uVar4 = DAT_000230bc;
  *(undefined4 *)(iVar3 + 0x34) = DAT_000230bc;
  *(undefined4 *)(iVar3 + 0x3c) = uVar4;
  *(undefined4 *)(iVar3 + 0x44) = uVar4;
  *(ushort *)(iVar3 + 0x20) = *(ushort *)(iVar3 + 0x20) | 0xff02;
  *(ushort *)(iVar3 + 0x24) = *(ushort *)(iVar3 + 0x24) | 0xff02;
  *(ushort *)(iVar3 + 0x28) = *(ushort *)(iVar3 + 0x28) | 0xff02;
  *(undefined2 *)(iVar3 + 0xa0) = 0x10;
  *(undefined2 *)(iVar3 + 0xa4) = 0x10;
  *(undefined2 *)(iVar3 + 0xa8) = 0x10;
  *(undefined4 *)(iVar3 + 0xf4) = 0;
  *(undefined2 *)(iVar3 + 0x84) = 0x50;
  *(undefined2 *)(iVar3 + 0x8c) = 0x50;
  *(undefined2 *)(iVar3 + 0x94) = 0x50;
  *(undefined2 *)(iVar3 + 0x86) = 0x50;
  *(undefined2 *)(iVar3 + 0x8e) = 0x50;
  *(undefined2 *)(iVar3 + 0x96) = 0x50;
  *(uint *)(iVar3 + 0x5c) = *(uint *)(iVar3 + 0x5c) | 0xff;
  *(undefined2 *)(iVar3 + 0xb4) = 0x78;
  *(undefined2 *)(iVar3 + 0xd6) = 0xff00;
  *(undefined2 *)(iVar3 + 0xd4) = 0x4000;
  *(ushort *)(iVar3 + 0x58) = *(ushort *)(iVar3 + 0x58) | 0x400;
  *(undefined4 *)(DAT_000230c0 + 0x5c) = 0xc9;
  DAT_e000e400 = 0;
  DAT_e000e280 = 1;
  DAT_e000e100 = 1;
  *(uint *)(iVar3 + 0x5c) = *(uint *)(iVar3 + 0x5c) | 0x100;
  *(ushort *)(iVar3 + 0x58) = *(ushort *)(iVar3 + 0x58) & 0xffbf;
  return;
}



/* ===== position_sensor_dma_configure @ 000230c4, 140 bytes ===== */

void position_sensor_dma_configure(void)

{
  uint uVar1;
  uint *puVar2;
  undefined4 *puVar3;
  int iVar4;
  uint uVar5;
  
  puVar2 = DAT_00023154;
  uVar1 = DAT_00023150;
  DAT_00023154[4] = DAT_00023150;
  *puVar2 = *puVar2 & 0xffff7fff;
  uVar5 = uVar1 - 1;
  puVar2[4] = uVar5;
  *DAT_00023158 = 1;
  puVar3 = DAT_00023158;
  DAT_00023158[0x10] = DAT_0002315c;
  puVar3[0x11] = DAT_00023160;
  puVar3[0x12] = 0x10001;
  DAT_00023158[0x17] = 0x1100;
  DAT_00023158[7] = 1;
  puVar3 = DAT_00023158;
  DAT_00023158[5] = 0xf000f;
  puVar3[6] = 0xf000f;
  DAT_00023158[3] = 0x10001;
  DAT_00023158[4] = 0x10000;
  puVar2[4] = uVar1;
  *puVar2 = *puVar2 & 0xfffdffff;
  *DAT_00023164 = 0x173;
  puVar2[4] = uVar5;
  *(undefined4 *)(DAT_00023168 + 0x60) = 0x41;
  iVar4 = DAT_0002316c;
  *(undefined1 *)(DAT_0002316c + 0x400) = 0x10;
  *(undefined4 *)(iVar4 + 0x27f) = 2;
  *(undefined4 *)(iVar4 + 0xff) = 2;
  return;
}



/* ===== position_sensor_spi_gpio_configure @ 00023170, 110 bytes ===== */

void position_sensor_spi_gpio_configure(void)

{
  undefined2 *puVar1;
  undefined2 *puVar2;
  int iVar3;
  
  *(uint *)(DAT_000231e0 + 4) = *(uint *)(DAT_000231e0 + 4) & 0xfffbffff;
  puVar1 = DAT_000231e4;
  *DAT_000231e4 = 0xa501;
  puVar2 = DAT_000231e4;
  DAT_000231e4[0x34] = 0x20;
  puVar2[0x32] = 0x20;
  DAT_000231e4[0x2c] = 0x20;
  DAT_000231e4[-4] = DAT_000231e4[-4] & 3;
  puVar2 = DAT_000231e4;
  DAT_000231e4[0x35] = 0x2b;
  puVar2[0x33] = 0x28;
  puVar2 = DAT_000231e4;
  DAT_000231e4[0x2d] = 0x2a;
  puVar2[0x2b] = 0x29;
  *puVar1 = 0xa500;
  iVar3 = DAT_000231e8;
  *(undefined4 *)(DAT_000231e8 + 0x18) = 0xec08;
  *(undefined4 *)(iVar3 + 0xc) = 0x50000000;
  *(undefined4 *)(iVar3 + 4) = 8;
  *(uint *)(iVar3 + 4) = *(uint *)(iVar3 + 4) | 0x40;
  return;
}



/* ===== position_sensor_spi_transfer16 @ 000231ec, 22 bytes ===== */

uint position_sensor_spi_transfer16(uint param_1)

{
  uint *puVar1;
  
  puVar1 = DAT_00023204;
  do {
  } while (-1 < (int)(DAT_00023204[5] << 0x1a));
  *DAT_00023204 = param_1;
  do {
  } while (-1 < (int)(puVar1[5] << 0x18));
  return *puVar1 & 0xffff;
}



/* ===== system_clock_and_systick_init @ 00023208, 554 bytes ===== */

void system_clock_and_systick_init(void)

{
  int *piVar1;
  int iVar2;
  undefined4 *puVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  uint unaff_r7;
  undefined4 *puVar10;
  int iVar11;
  int iVar12;
  
  piVar1 = DAT_00023434;
  iVar12 = *DAT_00023434;
  iVar11 = DAT_00023434[1];
  iVar9 = DAT_00023434[2];
  iVar8 = DAT_00023434[3];
  FUN_00023830(1);
  FUN_00023830(2);
  FUN_00023830(3);
  iVar7 = DAT_00023438;
  piVar1[4] = DAT_00023438;
  iVar5 = DAT_00023440;
  iVar2 = DAT_0002343c;
  uVar4 = DAT_0002343c >> 0x15;
  if (*(char *)(DAT_0002343c + 0x26) == '\x05') {
    *piVar1 = DAT_00023440;
    iVar5 = iVar5 >> 0xb;
    piVar1[1] = iVar5;
    piVar1[2] = iVar5;
    piVar1[3] = iVar5;
    do {
      unaff_r7 = unaff_r7 + 1 & 0xffff;
    } while (unaff_r7 < uVar4);
  }
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) | 0xa501;
  *(undefined4 *)(iVar2 + 0x20) = DAT_00023444;
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) & 0xfffe | 0xa500;
  uVar6 = 0;
  do {
    uVar6 = uVar6 + 1 & 0xffff;
  } while (uVar6 < uVar4);
  *piVar1 = iVar12;
  piVar1[1] = iVar11;
  piVar1[2] = iVar9;
  piVar1[3] = iVar8;
  piVar1[4] = DAT_00023438 + -1;
  uVar6 = 0;
  do {
    uVar6 = uVar6 + 1 & 0xffff;
  } while (uVar6 < uVar4);
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) | 0xa501;
  *DAT_00023448 = 0xa0;
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) & 0xfffe | 0xa500;
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) | 0xa501;
  *(undefined1 *)(iVar2 + 0x32) = 0;
  do {
  } while (-1 < (int)((uint)*(byte *)(iVar2 + 0x3c) << 0x1c));
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) & 0xfffe | 0xa500;
  puVar3 = DAT_0002344c;
  *DAT_0002344c = 0x77;
  puVar10 = DAT_0002344c + 2;
  *puVar10 = 0x77;
  puVar3[-1] = 0x11000000;
  *puVar3 = 0x76;
  *puVar10 = 0x76;
  *(undefined4 *)(DAT_00023448 + 0x7c) = 0x8043;
  puVar3 = DAT_00023450;
  *DAT_00023450 = 0x123;
  *puVar3 = 0x3210;
  DAT_00023450[6] = DAT_00023454;
  *puVar3 = 0x3210;
  *puVar3 = 0x3210;
  *(undefined2 *)(iVar2 + 0x3fe) = 0xa501;
  *(uint *)(iVar2 + 0x100) = *(uint *)(iVar2 + 0x100) & 0xffffff7f;
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) & 0xfffe | 0xa500;
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) | 0xa501;
  *(undefined4 *)(iVar2 + 0x100) = DAT_00023458;
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) & 0xfffe | 0xa500;
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) | 0xa501;
  *(undefined1 *)(iVar2 + 0x2a) = 0;
  do {
  } while (-1 < (int)((uint)*(byte *)(iVar2 + 0x3c) << 0x1a));
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) & 0xfffe | 0xa500;
  piVar1[4] = iVar7;
  iVar7 = DAT_00023440;
  *piVar1 = DAT_00023440;
  iVar7 = iVar7 >> 0xb;
  piVar1[1] = iVar7;
  piVar1[2] = iVar7;
  piVar1[3] = iVar7;
  uVar6 = 0;
  do {
    uVar6 = uVar6 + 1 & 0xffff;
  } while (uVar6 < uVar4);
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) | 0xa501;
  *(undefined1 *)(iVar2 + 0x26) = 5;
  *(ushort *)(iVar2 + 0x3fe) = *(ushort *)(iVar2 + 0x3fe) & 0xfffe | 0xa500;
  uVar6 = 0;
  do {
    uVar6 = uVar6 + 1 & 0xffff;
  } while (uVar6 < uVar4);
  *piVar1 = iVar12;
  piVar1[1] = iVar11;
  piVar1[2] = iVar9;
  piVar1[3] = iVar8;
  piVar1[4] = DAT_00023438 + -1;
  uVar6 = 0;
  do {
    uVar6 = uVar6 + 1 & 0xffff;
  } while (uVar6 < uVar4);
  DAT_e000e014 = 0xffffff;
  DAT_e000e018 = 0;
  DAT_e000e010 = 4;
  piVar1[2] = piVar1[2] & 0xffefffff;
  puVar3 = DAT_0002345c;
  *DAT_0002345c = 0;
  *(undefined1 *)(puVar3 + 0x20) = 0x10;
  *(undefined1 *)((int)puVar3 + 0x81) = 1;
  return;
}



/* ===== FUN_00023460 @ 00023460, 50 bytes ===== */

void FUN_00023460(int param_1)

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



/* ===== FUN_00023494 @ 00023494, 36 bytes ===== */

void FUN_00023494(int param_1)

{
  int iVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  
  iVar1 = DAT_000234b8;
  uVar3 = 0;
  do {
    uVar2 = 0;
    do {
      iVar4 = param_1 + uVar2 * 4;
      uVar2 = uVar2 + 1 & 0xff;
      *(undefined1 *)(iVar4 + uVar3) = *(undefined1 *)(iVar1 + (uint)*(byte *)(iVar4 + uVar3));
    } while (uVar2 < 4);
    uVar3 = uVar3 + 1 & 0xff;
  } while (uVar3 < 4);
  return;
}



/* ===== system_clock_source_configure @ 000234bc, 120 bytes ===== */

void system_clock_source_configure(void)

{
  int iVar1;
  int iVar2;
  undefined4 uVar3;
  uint uVar4;
  
  iVar2 = DAT_0002354c;
  iVar1 = DAT_00023540;
  uVar3 = DAT_00023544;
  if ((*DAT_0002353c & 1) == 0) {
    uVar3 = DAT_00023548;
  }
  *(undefined4 *)(DAT_00023540 + 4) = uVar3;
  uVar4 = *(byte *)(iVar2 + 0x26) & 7;
  if (5 < uVar4) {
    return;
  }
                    /* WARNING: Could not recover jumptable at 0x000234e8. Too many branches */
                    /* WARNING: Treating indirect jump as call */
  (*(code *)(&switchD_000234e8::switchdataD_000234ec +
            (uint)(&switchD_000234e8::switchdataD_000234ec)[uVar4] * 2))(iVar1,DAT_00023550);
  return;
}



/* ===== SystemInit @ 00023554, 24 bytes ===== */

void SystemInit(void)

{
  *DAT_0002356c = *DAT_0002356c | 0xf00000;
  system_clock_source_configure();
  *DAT_00023570 = 0;
  return;
}



/* ===== debug_uart_gpio_configure @ 00023574, 30 bytes ===== */

void debug_uart_gpio_configure(void)

{
  undefined4 *puVar1;
  
  *(uint *)(DAT_00023594 + 8) = *(uint *)(DAT_00023594 + 8) & 0xffffefff;
  puVar1 = DAT_00023598;
  *DAT_00023598 = 0;
  puVar1[4] = 0x5530;
  puVar1[2] = 100;
  puVar1[5] = 0;
  return;
}



/* ===== debug_usart1_dma_init @ 0002359c, 454 bytes ===== */

void debug_usart1_dma_init(void)

{
  ulonglong uVar1;
  longlong lVar2;
  int iVar3;
  uint *puVar4;
  undefined2 *puVar5;
  undefined2 *puVar6;
  uint *puVar7;
  float fVar8;
  undefined4 *puVar9;
  uint *puVar10;
  uint uVar11;
  uint uVar12;
  uint extraout_r3;
  uint uVar13;
  uint in_fpscr;
  float fVar14;
  float fVar15;
  uint uVar16;
  
  debug_uart_gpio_configure();
  puVar4 = DAT_00023764;
  DAT_00023764[3] = DAT_00023764[3] & 0xffefffff;
  puVar5 = DAT_00023768;
  *DAT_00023768 = 0xa501;
  puVar6 = DAT_00023768;
  DAT_00023768[0x19] = 0x20;
  puVar6[0x1b] = 0x21;
  *puVar5 = 0xa500;
  puVar7 = DAT_00023770;
  *DAT_00023770 = DAT_0002376c;
  puVar7[1] = 0;
  puVar10 = puVar7 + 2;
  *puVar10 = 0;
  puVar7[-1] = 0xffff;
  puVar7[3] = 0;
  *puVar10 = *puVar10 & 0xffffffdf;
  puVar7[3] = 0;
  *puVar7 = (int)puVar7 << 0x1d;
  puVar7[1] = (int)puVar10 >> 0x13;
  *puVar10 = 0;
  fVar8 = DAT_00023774;
  iVar3 = (int)(*puVar7 << 0x10) >> 0x1f;
  uVar12 = -iVar3;
  fVar14 = (float)VectorUnsignedToFloat(extraout_r3,(byte)(in_fpscr >> 0x16) & 3);
  fVar15 = (float)VectorUnsignedToFloat(uVar12,(byte)(in_fpscr >> 0x16) & 3);
  fVar14 = DAT_00023774 / (fVar14 * 8.0 * (2.0 - fVar15)) - 1.0;
  uVar16 = VectorFloatToUnsigned(fVar14,3);
  uVar11 = in_fpscr & 0xfffffff | (uint)(fVar14 < 0.0) << 0x1f;
  if ((SUB41(uVar11 >> 0x1f,0) == NAN(fVar14)) && (uVar16 < 0x100)) {
    fVar15 = (float)VectorUnsignedToFloat(uVar16,(byte)(uVar11 >> 0x16) & 3);
    if (DAT_00023778 < (int)(fVar14 - fVar15)) {
      uVar13 = iVar3 + 2;
      uVar1 = (ulonglong)uVar13 * (ulonglong)(uVar16 + 1);
      lVar2 = (uVar1 & 0xffffffff) * (ulonglong)extraout_r3;
      uVar11 = (uint)lVar2;
      fVar14 = (float)FUN_00027b3c(uVar11 << 0xb,
                                   ((uVar13 * (0xfffffffe < uVar16) +
                                    -(uint)(2 < uVar12) * (uVar16 + 1) + (int)(uVar1 >> 0x20)) *
                                    extraout_r3 + (int)((ulonglong)lVar2 >> 0x20)) * 0x800 |
                                   uVar11 >> 0x15);
      uVar11 = VectorFloatToUnsigned(fVar14 / fVar8 - DAT_0002377c,3);
      if (uVar11 < 0x80) {
        uVar12 = 0x20000000;
      }
      else {
        uVar12 = 0;
      }
      *puVar7 = *puVar7 | uVar12;
      puVar7[-1] = uVar11 | uVar16 << 8;
    }
  }
  uVar11 = DAT_00023780;
  puVar4[4] = DAT_00023780;
  *puVar4 = *puVar4 & 0xffffbfff;
  uVar12 = uVar11 - 1;
  puVar4[4] = uVar12;
  puVar9 = DAT_00023784;
  *DAT_00023784 = 1;
  puVar9[0x17] = puVar9[0x17] & 0xffffefff;
  puVar9[0x10] = (int)DAT_00023770 + -6;
  puVar9[0x11] = DAT_00023788;
  puVar9[0x12] = DAT_0002378c;
  puVar9[0x17] = 4;
  puVar9[7] = 1;
  puVar9[5] = 0xf000f;
  puVar9[6] = 0xf000f;
  puVar4[4] = uVar11;
  *puVar4 = *puVar4 & 0xfffdffff;
  *DAT_00023790 = 0x142;
  puVar4[4] = uVar12;
  *puVar7 = *puVar7 | 3;
  *(undefined4 *)(DAT_00023794 + 0x6c) = 0x144;
  iVar3 = DAT_00023798;
  *(undefined1 *)(DAT_00023798 + 0x400) = 0x40;
  *(undefined4 *)(iVar3 + 0x27c) = 0x10;
  *(undefined4 *)(iVar3 + 0xfc) = 0x10;
  *puVar7 = *puVar7 | 0xc;
  return;
}



/* ===== debug_uart_write @ 0002379c, 32 bytes ===== */

void debug_uart_write(byte *param_1,uint param_2)

{
  int *piVar1;
  uint uVar2;
  ushort *puVar3;
  
  piVar1 = DAT_000237bc;
  puVar3 = (ushort *)(DAT_000237bc + 1);
  for (uVar2 = 0; uVar2 < param_2; uVar2 = uVar2 + 1 & 0xffff) {
    do {
    } while (-1 < *piVar1 << 0x18);
    *puVar3 = (ushort)*param_1;
    param_1 = param_1 + 1;
  }
  return;
}



/* ===== FUN_000237c0 @ 000237c0, 48 bytes ===== */

uint FUN_000237c0(int param_1,uint param_2)

{
  uint uVar1;
  
  uVar1 = 0;
  if (param_1 != 0 || (param_2 & 0xfffff) != 0) {
    uVar1 = 4;
  }
  if ((param_2 & 0x7fffffff) >> 0x14 != 0) {
    uVar1 = uVar1 | 1;
  }
  if ((param_2 & 0x7fffffff) >> 0x14 == 0x7ff) {
    uVar1 = uVar1 | 2;
  }
  if (uVar1 == 1) {
    uVar1 = 5;
  }
  return uVar1;
}



/* ===== FUN_000237f0 @ 000237f0, 38 bytes ===== */

uint FUN_000237f0(uint param_1)

{
  uint uVar1;
  
  uVar1 = 0;
  if ((param_1 & 0x7fffff) != 0) {
    uVar1 = 4;
  }
  if ((param_1 & 0x7fffffff) >> 0x17 != 0) {
    uVar1 = uVar1 | 1;
  }
  if ((~(param_1 << 1) & 0xff000000) == 0) {
    uVar1 = uVar1 | 2;
  }
  if (uVar1 == 1) {
    uVar1 = 5;
  }
  return uVar1;
}



/* ===== FUN_00023816 @ 00023816, 26 bytes ===== */

void FUN_00023816(uint param_1)

{
  if (-1 < (int)param_1) {
    (&DAT_e000e280)[param_1 >> 5] = 1 << (param_1 & 0x1f);
  }
  return;
}



/* ===== FUN_00023830 @ 00023830, 34 bytes ===== */

void FUN_00023830(uint param_1)

{
  if (-1 < (int)param_1) {
    (&DAT_e000e180)[param_1 >> 5] = 1 << (param_1 & 0x1f);
    DataSynchronizationBarrier(0xf);
    InstructionSynchronizationBarrier(0xf);
  }
  return;
}



/* ===== FUN_0002386c @ 0002386c, 32 bytes ===== */

void FUN_0002386c(uint param_1,char param_2)

{
  if (-1 < (int)param_1) {
    (&DAT_e000e400)[param_1] = param_2 << 4;
    return;
  }
  (&DAT_e000ed14)[param_1 & 0xf] = param_2 << 4;
  return;
}



/* ===== FUN_00023890 @ 00023890, 782 bytes ===== */

undefined4 FUN_00023890(void)

{
  uint uVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar6;
  undefined4 uVar7;
  longlong in_d0;
  undefined4 extraout_s1;
  undefined4 extraout_s1_00;
  undefined4 extraout_s1_01;
  undefined4 extraout_s1_02;
  undefined4 extraout_s1_03;
  undefined4 extraout_s1_04;
  undefined4 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined8 uVar11;
  undefined8 uVar12;
  undefined8 uVar13;
  undefined8 uVar14;
  undefined4 uVar15;
  int local_10;
  uint uStack_c;
  undefined4 uVar5;
  
  uVar13 = DAT_00023bb8;
  uVar11 = DAT_00023b80;
  uStack_c = (uint)((ulonglong)in_d0 >> 0x20);
  uVar1 = uStack_c & 0x7fffffff;
  local_10 = (int)in_d0;
  if ((int)uVar1 < DAT_00023b78) {
    uVar8 = (undefined4)DAT_00023ba0;
    uVar3 = (undefined4)((ulonglong)DAT_00023ba0 >> 0x20);
    uVar9 = (undefined4)DAT_00023ba8;
    uVar2 = (undefined4)((ulonglong)DAT_00023ba8 >> 0x20);
    if (DAT_00023b98 <= (int)uVar1) {
      uVar6 = (undefined4)DAT_00023bc8;
      uVar4 = (undefined4)((ulonglong)DAT_00023bc8 >> 0x20);
      uVar10 = (undefined4)DAT_00023bd0;
      uVar5 = (undefined4)((ulonglong)DAT_00023bd0 >> 0x20);
      if (in_d0 < 0) {
        uVar13 = FUN_000270d8();
        uVar13 = FUN_00027558((int)uVar13,(int)((ulonglong)uVar13 >> 0x20),uVar6,uVar4);
        uVar6 = (undefined4)((ulonglong)uVar13 >> 0x20);
        uVar4 = (undefined4)uVar13;
        uVar7 = FUN_00024188(uVar4,DAT_00023bd8 + 0x23a0a,6);
        uVar13 = FUN_00027558(uVar7,extraout_s1_01,uVar4,uVar6);
        uVar7 = FUN_00024188(uVar4,DAT_00023bdc + 0x23a2c,4);
        uVar12 = FUN_00027558(uVar7,extraout_s1_02,uVar4,uVar6);
        uVar12 = FUN_000270d8((int)uVar12,(int)((ulonglong)uVar12 >> 0x20),uVar8,uVar3);
        uVar14 = FUN_00026e20(uVar4,uVar6);
        uVar3 = (undefined4)((ulonglong)uVar14 >> 0x20);
        uVar13 = FUN_00027228((int)uVar13,(int)((ulonglong)uVar13 >> 0x20),(int)uVar12,
                              (int)((ulonglong)uVar12 >> 0x20));
        uVar13 = FUN_00027558((int)uVar13,(int)((ulonglong)uVar13 >> 0x20),(int)uVar14,uVar3);
        uVar13 = FUN_00027904((int)uVar13,(int)((ulonglong)uVar13 >> 0x20),uVar9,uVar2);
        uVar13 = FUN_000270d8((int)uVar14,uVar3,(int)uVar13,(int)((ulonglong)uVar13 >> 0x20));
        uVar13 = FUN_00027558((int)uVar13,(int)((ulonglong)uVar13 >> 0x20),uVar10,uVar5);
        uVar2 = FUN_00027754((int)uVar13,(int)((ulonglong)uVar13 >> 0x20),(int)uVar11,
                             (int)((ulonglong)uVar11 >> 0x20));
        return uVar2;
      }
      uVar11 = FUN_00027754(local_10,uStack_c,uVar8,uVar3);
      uVar11 = FUN_00027558((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),uVar6,uVar4);
      uVar4 = (undefined4)((ulonglong)uVar11 >> 0x20);
      uVar9 = (undefined4)uVar11;
      uVar11 = FUN_00026e20();
      uVar6 = (undefined4)((ulonglong)uVar11 >> 0x20);
      uVar15 = 0;
      uVar2 = uVar6;
      uVar13 = FUN_000270d8((int)uVar11,uVar6,0,uVar6,0,uVar6);
      uVar12 = FUN_00027558(uVar15,uVar2,uVar15,uVar2);
      uVar12 = FUN_00027754((int)uVar12,(int)((ulonglong)uVar12 >> 0x20),uVar9,uVar4);
      uVar13 = FUN_00027228((int)uVar12,(int)((ulonglong)uVar12 >> 0x20),(int)uVar13,
                            (int)((ulonglong)uVar13 >> 0x20));
      uVar7 = FUN_00024188(uVar9,DAT_00023be0 + 0x23af8,6);
      uVar12 = FUN_00027558(uVar7,extraout_s1_03,uVar9,uVar4);
      uVar7 = FUN_00024188(uVar9,DAT_00023be4 + 0x23b1a,4);
      uVar14 = FUN_00027558(uVar7,extraout_s1_04,uVar9,uVar4);
      uVar14 = FUN_000270d8((int)uVar14,(int)((ulonglong)uVar14 >> 0x20),uVar8,uVar3);
      uVar12 = FUN_00027228((int)uVar12,(int)((ulonglong)uVar12 >> 0x20),(int)uVar14,
                            (int)((ulonglong)uVar14 >> 0x20));
      uVar11 = FUN_00027558((int)uVar12,(int)((ulonglong)uVar12 >> 0x20),(int)uVar11,uVar6);
      uVar11 = FUN_000270d8((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),(int)uVar13,
                            (int)((ulonglong)uVar13 >> 0x20));
      uVar11 = FUN_000270d8(uVar15,uVar2,(int)uVar11,(int)((ulonglong)uVar11 >> 0x20));
      uVar2 = FUN_00027558((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),uVar10,uVar5);
      return uVar2;
    }
    uVar11 = DAT_00023bb8;
    if (DAT_00023bb0 < (int)uVar1) {
      uVar11 = FUN_00027558(local_10,uStack_c,local_10,uStack_c);
      uVar4 = (undefined4)((ulonglong)uVar11 >> 0x20);
      uVar5 = (undefined4)uVar11;
      uVar6 = FUN_00024188(uVar5,DAT_00023bc0 + 0x23954,6);
      uVar11 = FUN_00027558(uVar6,extraout_s1,uVar5,uVar4);
      uVar6 = FUN_00024188(uVar5,DAT_00023bc4 + 0x23976,4);
      uVar12 = FUN_00027558(uVar6,extraout_s1_00,uVar5,uVar4);
      uVar12 = FUN_000270d8((int)uVar12,(int)((ulonglong)uVar12 >> 0x20),uVar8,uVar3);
      uVar11 = FUN_00027228((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),(int)uVar12,
                            (int)((ulonglong)uVar12 >> 0x20));
      uVar11 = FUN_00027558(local_10,uStack_c,(int)uVar11,(int)((ulonglong)uVar11 >> 0x20));
      uVar11 = FUN_00027754((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),uVar9,uVar2);
      uVar11 = FUN_00027754((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),local_10,uStack_c);
      uVar2 = FUN_00027754((int)uVar11,(int)((ulonglong)uVar11 >> 0x20),(int)uVar13,
                           (int)((ulonglong)uVar13 >> 0x20));
      return uVar2;
    }
  }
  else {
    if (local_10 != 0 || uVar1 != 0x3ff00000) {
      if (((int)uVar1 <= (int)DAT_00023b88) && ((uVar1 != DAT_00023b88 || (local_10 == 0)))) {
        FUN_000208d6(1);
        uVar2 = (undefined4)((ulonglong)DAT_000242b0 >> 0x20);
        uVar2 = FUN_00027228((int)DAT_000242b0,uVar2,(int)DAT_000242b0,uVar2);
        return uVar2;
      }
      uVar2 = FUN_000270d8(local_10,uStack_c,local_10,uStack_c);
      return uVar2;
    }
    if (0 < (int)uStack_c) {
      return (int)DAT_00023b90;
    }
  }
  return (int)uVar11;
}



/* ===== FUN_00023be8 @ 00023be8, 600 bytes ===== */

float FUN_00023be8(float param_1,float param_2)

{
  int iVar1;
  int iVar2;
  float fVar3;
  uint uVar4;
  int iVar5;
  bool bVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  
  uVar4 = 0xe8000000;
  bVar6 = 0xe4ffffff < (int)param_1 * 2 + 0xe8000000U;
  if (!bVar6) {
    uVar4 = (int)param_2 * 2 + 0xe8000000;
  }
  fVar10 = param_1;
  fVar7 = param_2;
  if (bVar6 || 0xe4ffffff < uVar4) {
    if (0xff000000 < (uint)((int)param_1 << 1) || 0xff000000 < (uint)((int)param_2 << 1)) {
      return param_1 + param_2;
    }
    if ((((uint)param_1 | (uint)param_2) & 0x7fffffff) == 0) {
      param_2 = (float)((uint)param_2 | 0x7f800000);
      fVar7 = param_2;
    }
    else if (ABS(param_1) == INFINITY && ABS(param_2) == INFINITY) {
      fVar10 = (float)((uint)param_1 & 0xbfffffff);
      param_2 = (float)((uint)param_2 & 0xbfffffff);
      fVar7 = param_2;
      param_1 = fVar10;
    }
    else if (ABS(param_1) == INFINITY || ABS(param_2) == 0.0) {
      fVar10 = (float)((uint)param_1 | 0x7f800000);
      param_2 = (float)((uint)param_2 & 0x80000000);
    }
    else if (ABS(param_2) == INFINITY || ABS(param_1) == 0.0) {
      fVar10 = (float)((uint)param_1 & 0x80000000);
      param_2 = (float)((uint)param_2 | 0x7f800000);
    }
    else if (-1 < ((int)param_2 * 2 ^ (int)param_1 << 1)) {
      fVar9 = DAT_00023e8c;
      if ((int)param_2 * 2 < 0) {
        fVar9 = DAT_00023e90;
      }
      param_2 = param_2 * fVar9;
      fVar10 = param_1 * fVar9;
      fVar7 = param_2;
      param_1 = param_1 * fVar9;
    }
  }
  iVar5 = ((uint)ABS(fVar10) >> 0x17) - ((uint)ABS(param_2) >> 0x17);
  if (0x1b < iVar5) {
    fVar7 = DAT_00023e3c;
    if (((uint)fVar10 & 0x80000000) == 0) {
      fVar7 = DAT_00023e40;
    }
    return fVar7;
  }
  if (-0x1b < iVar5) {
    if ((uint)((int)fVar10 * 2) < (uint)((int)param_2 * 2) ||
        (int)fVar10 * 2 + (int)param_2 * -2 == 0) {
      fVar8 = DAT_00023e64;
      fVar9 = DAT_00023e64;
      fVar3 = fVar7;
      if ((((uint)param_2 & 0x80000000) != 0) &&
         (fVar8 = DAT_00023e6c, fVar9 = DAT_00023e68, ((uint)fVar10 & 0x80000000) == 0)) {
        fVar8 = DAT_00023e74;
        fVar9 = DAT_00023e70;
      }
    }
    else {
      fVar8 = DAT_00023e50;
      fVar9 = DAT_00023e4c;
      if (((uint)fVar10 & 0x80000000) == 0) {
        fVar8 = DAT_00023e58;
        fVar9 = DAT_00023e54;
      }
      fVar3 = -fVar10;
      fVar10 = param_2;
      param_2 = fVar3;
      fVar3 = -param_1;
      param_1 = fVar7;
    }
    if ((uint)(((int)param_2 - (int)fVar10) * 2) < 0x1000000) {
      if ((((uint)fVar10 ^ (uint)param_2) & 0x80000000) == 0) {
        fVar10 = 0.5;
        fVar9 = fVar9 + DAT_00023e5c;
        fVar8 = fVar8 + DAT_00023e60;
      }
      else {
        fVar10 = -0.5;
        fVar9 = fVar9 - DAT_00023e5c;
        fVar8 = fVar8 - DAT_00023e60;
      }
      param_1 = (param_1 - fVar10 * fVar3) / (fVar3 + param_1 * fVar10);
    }
    else {
      param_1 = param_1 / fVar3;
    }
    fVar10 = param_1 * param_1;
    return fVar8 + param_1 * fVar10 *
                   (DAT_00023e88 +
                   fVar10 * (DAT_00023e84 +
                            fVar10 * (DAT_00023e80 + fVar10 * (DAT_00023e7c + fVar10 * DAT_00023e78)
                                     ))) + param_1 + fVar9;
  }
  if (((uint)param_2 & 0x80000000) == 0) {
    fVar10 = param_1 / fVar7;
    iVar5 = FUN_000237f0(fVar10);
    if (iVar5 == 4) {
      FUN_000242d4();
    }
    iVar5 = FUN_000237f0(param_1);
    iVar1 = FUN_000237f0(fVar7);
    iVar2 = FUN_000237f0(fVar10);
    if (((iVar5 == 4 || iVar5 == 5) && (iVar1 == 4 || iVar1 == 5)) && (iVar2 == 0)) {
      FUN_000208d6(2);
    }
    return fVar10;
  }
  fVar7 = DAT_00023e44;
  if (((uint)fVar10 & 0x80000000) != 0) {
    fVar7 = DAT_00023e48;
  }
  return fVar7;
}



/* ===== FUN_00023e94 @ 00023e94, 208 bytes ===== */

int FUN_00023e94(float param_1)

{
  float fVar1;
  float fVar2;
  
  fVar1 = (float)FUN_00027c3c();
  fVar2 = param_1 - fVar1;
  if (((uint)param_1 & 0x80000000) == 0) {
    if ((0xbf000000 < (uint)fVar2) && ((uint)((int)fVar2 << 1) < 0xff000001)) goto LAB_00023efa;
    if ((int)fVar2 < 0x3f000000) goto LAB_00023f18;
  }
  else {
    if (0xbeffffff < (uint)fVar2 && (uint)((int)fVar2 << 1) < 0xff000001) {
LAB_00023efa:
      fVar1 = fVar1 - 1.0;
      goto LAB_00023f18;
    }
    if ((int)fVar2 < 0x3f000001) goto LAB_00023f18;
  }
  if ((uint)((int)fVar2 << 1) < 0xff000001) {
    fVar1 = fVar1 + 1.0;
  }
LAB_00023f18:
  if (((0x4f000000 < (int)fVar1) && ((uint)((int)fVar1 << 1) < 0xff000001)) ||
     ((0xcf000000 < (uint)fVar1 && ((uint)((int)fVar1 << 1) < 0xff000001)))) {
    FUN_000208d6(1);
  }
  return (int)fVar1;
}



/* ===== FUN_00023f56 @ 00023f56, 122 bytes ===== */

undefined8 FUN_00023f56(int param_1,uint param_2)

{
  undefined8 uVar1;
  
  uVar1 = FUN_0002776c(param_1,param_2);
  if (((int)(0x7ff00000 - ((uint)((int)uVar1 != 0) | (uint)((ulonglong)uVar1 >> 0x20) & 0x7fffffff))
       < 0) && (-1 < (int)(0x7ff00000 - (param_2 & 0x7fffffff | (uint)(param_1 != 0))))) {
    FUN_000208d6(1);
  }
  return uVar1;
}



/* ===== FUN_00023fd0 @ 00023fd0, 58 bytes ===== */

float FUN_00023fd0(float param_1)

{
  if ((0x7f800000 - (int)ABS(SQRT(param_1)) < 0) && (-1 < 0x7f800000 - (int)ABS(param_1))) {
    FUN_000208d6(1);
  }
  return SQRT(param_1);
}



/* ===== FUN_0002400c @ 0002400c, 338 bytes ===== */

/* WARNING: Restarted to delay deadcode elimination for space: stack */

float FUN_0002400c(float param_1)

{
  int iVar1;
  code *UNRECOVERED_JUMPTABLE;
  float fVar2;
  uint local_10;
  
  if ((uint)((int)param_1 * 2) < DAT_00024150) {
    if ((uint)((int)param_1 * 2) < 0x73000000) {
      local_10 = 0xffffffff;
    }
    else {
      local_10 = 0;
    }
  }
  else if ((uint)ABS(param_1) < (uint)DAT_0002416c) {
    if (((uint)param_1 & 0x80000000) == 0) {
      fVar2 = (param_1 * DAT_00024170 + DAT_00024174) - DAT_00024174;
    }
    else {
      fVar2 = (param_1 * DAT_00024170 - DAT_00024174) + DAT_00024174;
    }
    local_10 = (int)fVar2 & 3;
    param_1 = (((param_1 - fVar2 * DAT_00024178) - fVar2 * DAT_0002417c) - fVar2 * DAT_00024180) -
              fVar2 * DAT_00024184;
  }
  else {
    param_1 = (float)FUN_000242e4(&local_10);
  }
  if (-1 < (int)local_10) {
    fVar2 = param_1 * param_1;
    if ((local_10 & 1) == 0) {
                    /* WARNING: Could not recover jumptable at 0x00024086. Too many branches */
                    /* WARNING: Treating indirect jump as call */
      fVar2 = (float)(*UNRECOVERED_JUMPTABLE)();
      return fVar2;
    }
    return -1.0 / (param_1 +
                  param_1 * (DAT_00024168 +
                            fVar2 * (DAT_00024164 +
                                    fVar2 * (DAT_00024160 +
                                            fVar2 * (DAT_0002415c +
                                                    fVar2 * (DAT_00024158 + fVar2 * DAT_00024154))))
                            ) * fVar2);
  }
  if ((uint)((int)param_1 * 2) < 0xff000000) {
    iVar1 = FUN_000237f0(param_1);
    if (iVar1 == 4) {
      FUN_000242d4();
    }
    return param_1;
  }
  if ((int)param_1 * 2 == 0xff000000) {
    FUN_000208d6(1);
    return DAT_000242d0 / DAT_000242d0;
  }
  return param_1 + param_1;
}



/* ===== FUN_00024188 @ 00024188, 248 bytes ===== */

undefined4 FUN_00024188(undefined8 *param_1,int param_2)

{
  undefined4 uVar1;
  undefined4 uVar2;
  uint uVar3;
  undefined4 uVar4;
  undefined8 in_d0;
  undefined8 uVar5;
  undefined4 uVar6;
  
  uVar3 = param_2 - 1;
  uVar5 = param_1[uVar3];
  while( true ) {
    uVar4 = (undefined4)uVar5;
    uVar1 = (undefined4)((ulonglong)uVar5 >> 0x20);
    if ((uVar3 & 0xfffffff9) == 0) break;
    uVar5 = FUN_00027558(uVar4,uVar1);
    uVar3 = uVar3 - 1;
    uVar5 = FUN_000270d8((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),(int)param_1[uVar3],
                         (int)((ulonglong)param_1[uVar3] >> 0x20));
  }
  uVar6 = (undefined4)in_d0;
  uVar2 = (undefined4)((ulonglong)in_d0 >> 0x20);
  if (uVar3 != 2) {
    if (uVar3 != 4) {
      if (uVar3 != 6) {
        return uVar4;
      }
      uVar5 = FUN_00027558(uVar4,uVar1,uVar6,uVar2);
      uVar5 = FUN_000270d8((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),(int)param_1[5],
                           (int)((ulonglong)param_1[5] >> 0x20));
      uVar5 = FUN_00027558((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),uVar6,uVar2);
      uVar5 = FUN_000270d8((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),(int)param_1[4],
                           (int)((ulonglong)param_1[4] >> 0x20));
    }
    uVar5 = FUN_00027558((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),uVar6,uVar2);
    uVar5 = FUN_000270d8((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),(int)param_1[3],
                         (int)((ulonglong)param_1[3] >> 0x20));
    uVar5 = FUN_00027558((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),uVar6,uVar2);
    uVar5 = FUN_000270d8((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),(int)param_1[2],
                         (int)((ulonglong)param_1[2] >> 0x20));
  }
  uVar5 = FUN_00027558((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),uVar6,uVar2);
  uVar5 = FUN_000270d8((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),(int)param_1[1],
                       (int)((ulonglong)param_1[1] >> 0x20));
  uVar5 = FUN_00027558((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),uVar6,uVar2);
  uVar1 = FUN_000270d8((int)uVar5,(int)((ulonglong)uVar5 >> 0x20),(int)*param_1,
                       (int)((ulonglong)*param_1 >> 0x20));
  return uVar1;
}



/* ===== FUN_000242d4 @ 000242d4, 10 bytes ===== */

float FUN_000242d4(void)

{
  return DAT_000242e0 * DAT_000242e0;
}



/* ===== FUN_000242e4 @ 000242e4, 316 bytes ===== */

float FUN_000242e4(float param_1,uint *param_2,uint param_3)

{
  longlong lVar1;
  uint uVar2;
  int iVar3;
  float fVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint in_fpscr;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  
  fVar12 = DAT_00024424;
  if (0xfeffffff < param_3 * 2) {
    *param_2 = 0xffffffff;
    return param_1;
  }
  uVar5 = param_3 << 8 | 0x80000000;
  uVar2 = ((param_3 & 0x7fffffff) >> 0x17) - 0x78;
  uVar9 = uVar2 & 0x1f;
  uVar8 = 0x20 - uVar9;
  iVar3 = DAT_00024420 + 0x2431c + ((int)(((param_3 & 0x7fffffff) >> 0x17) - 0x78) >> 5) * 4;
  uVar2 = *(uint *)(DAT_00024420 + 0x2431c + ((int)uVar2 >> 5) * 4);
  if (uVar9 == 0) {
    uVar6 = *(uint *)(iVar3 + 4);
    uVar8 = *(uint *)(iVar3 + 8);
  }
  else {
    uVar2 = uVar2 << uVar9 | *(uint *)(iVar3 + 4) >> (uVar8 & 0xff);
    uVar6 = *(uint *)(iVar3 + 4) << uVar9 | *(uint *)(iVar3 + 8) >> (uVar8 & 0xff);
    uVar8 = *(uint *)(iVar3 + 8) << uVar9 | *(uint *)(iVar3 + 0xc) >> (uVar8 & 0xff);
  }
  uVar9 = (uint)((ulonglong)uVar6 * (ulonglong)uVar5 >> 0x20);
  uVar8 = (uint)((ulonglong)uVar8 * (ulonglong)uVar5 >> 0x20);
  lVar1 = (ulonglong)uVar6 * (ulonglong)uVar5 +
          CONCAT44((int)((ulonglong)uVar2 * (ulonglong)uVar5),uVar8);
  uVar7 = (uint)lVar1;
  uVar6 = (uint)((ulonglong)lVar1 >> 0x20);
  if (uVar7 < uVar8) {
    if (uVar6 <= uVar9) {
LAB_00024382:
      iVar3 = 1;
      goto LAB_00024388;
    }
  }
  else if (uVar6 < uVar9) goto LAB_00024382;
  iVar3 = 0;
LAB_00024388:
  iVar3 = iVar3 + (int)((ulonglong)uVar2 * (ulonglong)uVar5 >> 0x20);
  fVar13 = (float)VectorSignedToFloat(iVar3 * 0x4000000 | (uVar6 >> 0x13) << 0xd,
                                      (byte)(in_fpscr >> 0x16) & 3);
  uVar2 = iVar3 + 0x20U >> 6;
  fVar10 = (float)VectorUnsignedToFloat(uVar6 * 0x2000,(byte)(in_fpscr >> 0x16) & 3);
  *param_2 = uVar2;
  fVar11 = (float)VectorUnsignedToFloat(uVar7,(byte)(in_fpscr >> 0x16) & 3);
  fVar4 = (float)((int)(fVar13 + fVar10 * fVar12 + fVar11 * DAT_00024428) + 0x800U & 0xfffff000);
  fVar12 = fVar4 * DAT_0002442c +
           (fVar11 * DAT_00024428 - ((fVar4 - fVar13) - fVar10 * fVar12)) * DAT_00024430 +
           fVar4 * DAT_00024434;
  if ((param_3 & 0x80000000) != 0) {
    *param_2 = 0x10000000 - uVar2;
    return -fVar12;
  }
  return fVar12;
}



/* ===== FUN_00024438 @ 00024438, 14 bytes ===== */

undefined4 FUN_00024438(int param_1)

{
  if (param_1 - 0x30U < 10) {
    return 1;
  }
  return 0;
}



/* ===== measure_position_sensor_offset @ 00024448, 616 bytes ===== */

void measure_position_sensor_offset(void)

{
  ushort uVar1;
  int iVar2;
  float fVar3;
  float fVar4;
  int iVar5;
  ushort *puVar6;
  float *pfVar7;
  undefined8 uVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  undefined4 uVar11;
  undefined4 uVar12;
  uint uVar13;
  uint in_fpscr;
  float fVar14;
  uint uVar15;
  undefined4 uVar16;
  uint uVar18;
  float fVar19;
  float fVar20;
  ulonglong uVar17;
  float fVar21;
  float fVar22;
  float fVar23;
  float fVar24;
  float fVar25;
  float fVar26;
  undefined8 uVar27;
  undefined8 uVar28;
  undefined1 local_78;
  float local_77;
  float local_73;
  float local_6f;
  undefined2 local_6b;
  undefined2 local_69;
  undefined4 local_67;
  undefined4 local_60;
  float local_5c;
  float local_58;
  
  fVar4 = DAT_000246cc;
  uVar28 = DAT_000246c0;
  fVar3 = DAT_000246b8;
  iVar2 = DAT_000246b0;
  fVar14 = *(float *)(DAT_000246b0 + 0x4c);
  uVar18 = VectorFloatToUnsigned(fVar14 * DAT_000246b4,3);
  fVar19 = (float)VectorUnsignedToFloat
                            (*(undefined4 *)(DAT_000246b0 + 0x54),(byte)(in_fpscr >> 0x16) & 3);
  fVar19 = fVar19 * DAT_000246b8;
  fVar20 = (float)VectorUnsignedToFloat(uVar18,(byte)(in_fpscr >> 0x16) & 3);
  fVar26 = *(float *)(DAT_000246c8 + 0x10);
  uVar13 = 0;
  do {
    thunk_EXT_FUN_1fff9c40(fVar26);
    delay_us(100);
    uVar9 = DAT_000246ec;
    uVar8 = DAT_000246e0;
    pfVar7 = DAT_000246d8;
    puVar6 = DAT_000246d4;
    iVar5 = DAT_000246d0;
    uVar13 = uVar13 + 1;
  } while (uVar13 < 20000);
  uVar13 = 0;
  if (uVar18 != 0) {
    do {
      do {
      } while ((*(byte *)(iVar5 + 0x44) & 1) == 0);
      uVar1 = *puVar6;
      fVar25 = (float)VectorUnsignedToFloat
                                ((uint)*(ushort *)(iVar5 + 0x54),(byte)(in_fpscr >> 0x16) & 3);
      fVar25 = *pfVar7 * pfVar7[2] + fVar25 * pfVar7[3];
      *pfVar7 = fVar25;
      uVar15 = VectorFloatToUnsigned(fVar25,3);
      fVar25 = (float)VectorUnsignedToFloat((uint)uVar1,(byte)(in_fpscr >> 0x16) & 3);
      fVar25 = pfVar7[1] * pfVar7[2] + fVar25 * pfVar7[3];
      pfVar7[1] = fVar25;
      uVar16 = VectorFloatToUnsigned(fVar25,3);
      fVar23 = pfVar7[7];
      fVar25 = (float)VectorUnsignedToFloat(uVar15 & 0xffff,(byte)(in_fpscr >> 0x16) & 3);
      uVar17 = CONCAT44(uVar16,fVar25 - pfVar7[5]) & 0xffffffffffff;
      fVar21 = (float)VectorUnsignedToFloat((int)(uVar17 >> 0x20),(byte)(in_fpscr >> 0x16) & 3);
      fVar24 = pfVar7[10];
      fVar22 = pfVar7[0xc];
      uVar27 = FUN_00027ad8((fVar19 * fVar14) / fVar20);
      uVar28 = FUN_000270d8((int)uVar27,(int)((ulonglong)uVar27 >> 0x20),(int)uVar28,
                            (int)((ulonglong)uVar28 >> 0x20));
      uVar12 = (undefined4)((ulonglong)uVar28 >> 0x20);
      uVar10 = (undefined4)uVar28;
      FUN_00027558(uVar10,uVar12,(int)uVar8,(int)((ulonglong)uVar8 >> 0x20));
      uVar11 = FUN_000274d8();
      fVar25 = (float)VectorUnsignedToFloat(uVar11,(byte)(in_fpscr >> 0x16) & 3);
      uVar27 = FUN_00027ad8(fVar25 * fVar3);
      FUN_00027754((int)uVar27,(int)((ulonglong)uVar27 >> 0x20),uVar10,uVar12);
      local_60 = FUN_00027074();
      thunk_EXT_FUN_1fffa3aa(uVar9,&local_60);
      thunk_EXT_FUN_1fffa30c(local_60,&local_5c,&local_58);
      thunk_EXT_FUN_1fff9c40(local_58 * fVar26 - local_5c * fVar4);
      *(undefined1 *)(iVar5 + 0x46) = 1;
      local_78 = 0x48;
      fVar25 = (float)VectorUnsignedToFloat
                                (*(undefined4 *)(iVar2 + 0x54),(byte)(in_fpscr >> 0x16) & 3);
      uVar27 = FUN_00027ad8(fVar25 * *(float *)(iVar2 + 0x4c));
      FUN_00027228(uVar10,uVar12,(int)uVar27,(int)((ulonglong)uVar27 >> 0x20));
      local_77 = (float)FUN_00027074();
      local_73 = (float)FUN_00023be8((fVar21 - fVar23) * fVar24 - (float)uVar17 * fVar22);
      local_6f = local_77 - local_73;
      if (DAT_000246f0 < (int)local_6f) {
        local_6f = local_6f - fVar3;
      }
      else if ((uint)DAT_000246f4 < (uint)local_6f) {
        local_6f = local_6f + fVar3;
      }
      local_6b = (undefined2)uVar15;
      local_69 = (undefined2)uVar16;
      local_67 = FUN_00021e88(&local_78,0x11);
      debug_uart_write(&local_78,0x15);
      uVar13 = uVar13 + 1;
    } while (uVar13 < uVar18);
  }
  thunk_EXT_FUN_1fff9c40(fVar4);
  return;
}



/* ===== calibrate_position_sensor @ 000246f8, 960 bytes ===== */

/* WARNING: Heritage AFTER dead removal. Example location: s1 : 0x00024876 */
/* WARNING: Restarted to delay deadcode elimination for space: register */

void calibrate_position_sensor(undefined4 param_1,undefined4 param_2)

{
  float *pfVar1;
  float *pfVar2;
  ushort uVar3;
  float *pfVar4;
  int iVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  int iVar13;
  uint in_fpscr;
  uint uVar14;
  uint uVar15;
  undefined4 uVar16;
  float fVar17;
  undefined4 uVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  float fVar23;
  float fVar24;
  float fVar25;
  float fVar26;
  float fVar27;
  undefined8 uVar28;
  undefined1 local_78;
  undefined4 local_77;
  undefined4 local_73;
  undefined4 local_6f;
  undefined2 local_6b;
  undefined2 local_69;
  undefined4 local_67;
  float local_60;
  float local_5c;
  float local_58;
  
  iVar5 = DAT_00024ac8;
  pfVar4 = DAT_00024ac4;
  fVar17 = DAT_00024ab8;
  iVar13 = 0;
  local_60 = DAT_00024ab8;
  uVar11 = 0x1000;
  uVar12 = 0;
  uVar10 = 0;
  uVar9 = 0x1000;
  DAT_e000e180 = 4;
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  do {
  } while ((*(byte *)(DAT_00024abc + 0x44) & 1) == 0);
  uVar3 = *DAT_00024ac0;
  pfVar1 = DAT_00024ac4 + 2;
  fVar19 = (float)VectorUnsignedToFloat
                            ((uint)*(ushort *)(DAT_00024abc + 0x54),(byte)(in_fpscr >> 0x16) & 3);
  pfVar2 = DAT_00024ac4 + 3;
  *DAT_00024ac4 = *DAT_00024ac4 * *pfVar1 + fVar19 * *pfVar2;
  fVar19 = (float)VectorUnsignedToFloat((uint)uVar3,(byte)(in_fpscr >> 0x16) & 3);
  pfVar4[1] = pfVar4[1] * *pfVar1 + fVar19 * *pfVar2;
  fVar26 = DAT_00024ad8;
  uVar18 = DAT_00024ad4;
  fVar19 = DAT_00024acc;
  fVar27 = *(float *)(iVar5 + 0x18) + DAT_00024acc;
  fVar24 = fVar17;
  fVar25 = fVar17;
  fVar22 = fVar17;
  fVar23 = fVar17;
  do {
    do {
      pfVar4 = DAT_00024ac4;
    } while ((*(byte *)(DAT_00024abc + 0x44) & 1) == 0);
    uVar3 = *DAT_00024ac0;
    pfVar1 = DAT_00024ac4 + 2;
    fVar21 = (float)VectorUnsignedToFloat
                              ((uint)*(ushort *)(DAT_00024abc + 0x54),(byte)(in_fpscr >> 0x16) & 3);
    pfVar2 = DAT_00024ac4 + 3;
    fVar20 = *DAT_00024ac4 * *pfVar1 + fVar21 * *pfVar2;
    *DAT_00024ac4 = fVar20;
    fVar21 = (float)VectorUnsignedToFloat((uint)uVar3,(byte)(in_fpscr >> 0x16) & 3);
    fVar21 = pfVar4[1] * *pfVar1 + fVar21 * *pfVar2;
    uVar14 = VectorFloatToUnsigned(fVar20,3);
    pfVar4[1] = fVar21;
    iVar6 = DAT_00024adc;
    uVar15 = VectorFloatToUnsigned(fVar21,3);
    uVar7 = uVar14 & 0xffff;
    uVar8 = uVar15 & 0xffff;
    if (uVar12 < uVar7) {
      fVar25 = *(float *)(iVar5 + 0x18);
      uVar12 = uVar7;
    }
    if (uVar7 < uVar11) {
      fVar23 = *(float *)(iVar5 + 0x18);
      uVar11 = uVar7;
    }
    if (uVar10 < uVar8) {
      fVar24 = *(float *)(iVar5 + 0x18);
      uVar10 = uVar8;
    }
    if (uVar8 < uVar9) {
      fVar22 = *(float *)(iVar5 + 0x18);
      uVar9 = uVar8;
    }
    if (*(int *)(DAT_00024adc + 4) != 0) {
      *(undefined4 *)(DAT_00024adc + 4) = 0;
      fVar20 = (float)VectorUnsignedToFloat
                                (*(undefined4 *)(iVar5 + 0x54),(byte)(in_fpscr >> 0x16) & 3);
      uVar16 = VectorFloatToUnsigned(*(float *)(iVar6 + 8) * *(float *)(iVar5 + 0x58),3);
      fVar21 = (float)VectorUnsignedToFloat(uVar16,(byte)(in_fpscr >> 0x16) & 3);
      local_60 = (*(float *)(iVar6 + 8) * fVar20 - fVar21 * fVar19) + *(float *)(iVar5 + 0x14);
      thunk_EXT_FUN_1fffa3aa(uVar18,param_2,*(float *)(iVar5 + 0x58),&local_60);
      thunk_EXT_FUN_1fffa30c(local_60,&local_5c,&local_58);
    }
    iVar13 = iVar13 + 1;
    if (iVar13 == 0x14) {
      local_78 = 0x68;
      iVar13 = 0;
      local_77 = 0;
      local_73 = 0;
      local_6f = 0;
      local_6b = (undefined2)uVar14;
      local_69 = (undefined2)uVar15;
      local_67 = FUN_00021e88(&local_78,0x11);
      debug_uart_write(&local_78,0x15);
    }
    fVar20 = *(float *)(iVar5 + 0x18);
    *(float *)(DAT_00024ae0 + 0x18) = fVar17;
    *(float *)(DAT_00024ae4 + 0x18) = fVar26;
    thunk_EXT_FUN_1fff9c40(fVar17 * local_58 - local_5c * fVar26);
    *(undefined1 *)(DAT_00024abc + 0x46) = 1;
    *DAT_00024ae8 = 1;
    *DAT_00024aec = 1;
    fVar21 = DAT_00024af8;
    iVar6 = DAT_00024af4;
    pfVar4 = DAT_00024ac4;
  } while (DAT_00024af0 < (int)ABS(fVar20 - fVar27));
  fVar17 = (float)VectorUnsignedToFloat(uVar12 + uVar11,(byte)(in_fpscr >> 0x16) & 3);
  fVar24 = fVar24 - fVar25;
  DAT_00024ac4[5] = fVar17 * 0.5;
  fVar17 = (float)VectorUnsignedToFloat(uVar10 + uVar9,(byte)(in_fpscr >> 0x16) & 3);
  pfVar4[7] = fVar17 * 0.5;
  fVar26 = (float)VectorSignedToFloat(uVar12 - uVar11,(byte)(in_fpscr >> 0x16) & 3);
  fVar17 = (float)VectorSignedToFloat(uVar10 - uVar9,(byte)(in_fpscr >> 0x16) & 3);
  if (iVar6 < (int)fVar24) {
    fVar24 = fVar24 - fVar19;
  }
  else if ((uint)fVar21 < (uint)fVar24) {
    fVar24 = fVar24 + fVar19;
  }
  fVar22 = fVar22 - fVar23;
  if (iVar6 < (int)fVar22) {
    fVar22 = fVar22 - fVar19;
  }
  else if ((uint)fVar21 < (uint)fVar22) {
    fVar22 = fVar22 + fVar19;
  }
  fVar19 = (fVar24 + fVar22) * 0.5;
  thunk_EXT_FUN_1fffa30c(fVar19,pfVar4 + 0xb);
  pfVar1 = DAT_00024afc;
  pfVar4[10] = fVar26 / fVar17;
  *pfVar1 = pfVar4[5];
  pfVar1[1] = pfVar4[7];
  pfVar1[2] = fVar26 / fVar17;
  pfVar1[3] = fVar19;
  local_78 = 0x4a;
  local_77 = 0;
  local_73 = 0;
  local_6f = 0;
  local_6b = *(undefined2 *)(iVar5 + 0x54);
  uVar18 = VectorFloatToUnsigned(*(undefined4 *)(iVar5 + 0x4c),3);
  local_69 = (undefined2)uVar18;
  local_67 = FUN_00021e88(&local_78,0x11);
  debug_uart_write(&local_78,0x15);
  delay_ms(1);
  measure_position_sensor_offset();
  local_78 = 0x5a;
  local_77 = 0;
  local_73 = 0;
  local_6f = 0;
  local_6b = 0xffff;
  local_69 = 0xffff;
  local_67 = FUN_00021e88(&local_78,0x11);
  debug_uart_write(&local_78,0x15);
  delay_ms(1);
  FUN_00027ad8(pfVar4[10]);
  FUN_00027ad8(pfVar1[3]);
  FUN_00027ad8(pfVar4[7]);
  uVar28 = FUN_00027ad8(pfVar4[5]);
  uVar18 = (undefined4)((ulonglong)uVar28 >> 0x20);
  debug_printf("u=%.4f v=%.4f  w=%.4f c=%.4f\r\n",uVar18,(int)uVar28,uVar18);
  *(undefined1 *)(DAT_00024abc + 0x46) = 1;
  *DAT_00024ae8 = 1;
  *DAT_00024aec = 1;
  DAT_e000e280 = 4;
  DAT_e000e100 = 4;
  return;
}



/* ===== measure_encoder_alignment @ 00024b20, 852 bytes ===== */

void measure_encoder_alignment(void)

{
  int iVar1;
  float fVar2;
  undefined2 *puVar3;
  undefined4 uVar4;
  int iVar5;
  undefined4 uVar6;
  undefined4 uVar7;
  int iVar8;
  undefined4 uVar9;
  undefined4 uVar10;
  uint uVar11;
  uint uVar12;
  uint in_fpscr;
  float fVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  undefined4 uVar18;
  float fVar19;
  undefined4 uVar20;
  undefined8 uVar21;
  undefined8 uVar22;
  undefined8 uVar23;
  undefined8 uVar24;
  undefined8 uVar25;
  undefined8 uVar26;
  undefined1 local_90;
  undefined4 local_8f;
  float local_8b;
  undefined4 local_87;
  undefined2 local_83;
  short local_81;
  undefined4 local_7f;
  undefined4 local_78;
  float local_74;
  float local_70;
  undefined4 local_6c;
  
  fVar2 = DAT_00024e78;
  iVar1 = DAT_00024e74;
  iVar5 = *(int *)(DAT_00024e74 + 0x54);
  fVar13 = (float)VectorUnsignedToFloat(iVar5,(byte)(in_fpscr >> 0x16) & 3);
  fVar17 = (float)VectorUnsignedToFloat(iVar5 * 0x2800,(byte)(in_fpscr >> 0x16) & 3);
  uVar21 = FUN_00027ad8((fVar13 * DAT_00024e78) / fVar17);
  uVar23 = DAT_00024e7c;
  uVar10 = (undefined4)((ulonglong)uVar21 >> 0x20);
  enforce_device_key_binding_or_halt();
  fVar13 = DAT_00024e88;
  fVar17 = *(float *)(DAT_00024e84 + 0x10);
  uVar11 = 0;
  do {
    thunk_EXT_FUN_1fff9c40(fVar17);
    delay_us(100);
    uVar4 = DAT_00024e9c;
    uVar24 = DAT_00024e90;
    puVar3 = DAT_00024e8c;
    uVar11 = uVar11 + 1;
  } while (uVar11 < 20000);
  uVar11 = 0;
  fVar19 = *(float *)(DAT_00024e8c + 4);
  while( true ) {
    uVar20 = (undefined4)uVar24;
    uVar6 = (undefined4)((ulonglong)uVar24 >> 0x20);
    if ((uint)(iVar5 * 0x100) <= uVar11) break;
    uVar12 = 0;
    while( true ) {
      uVar18 = (undefined4)uVar23;
      uVar7 = (undefined4)((ulonglong)uVar23 >> 0x20);
      if (0x27 < uVar12) break;
      uVar23 = FUN_000270d8(uVar18,uVar7,(int)uVar21,uVar10);
      uVar18 = (undefined4)((ulonglong)uVar23 >> 0x20);
      FUN_00027558((int)uVar23,uVar18,uVar20,uVar6);
      uVar7 = FUN_000274d8();
      fVar15 = (float)VectorUnsignedToFloat(uVar7,(byte)(in_fpscr >> 0x16) & 3);
      uVar25 = FUN_00027ad8(fVar15 * fVar2);
      FUN_00027754((int)uVar25,(int)((ulonglong)uVar25 >> 0x20),(int)uVar23,uVar18);
      local_78 = FUN_00027074();
      thunk_EXT_FUN_1fffa3aa(uVar4,&local_78);
      thunk_EXT_FUN_1fffa30c(local_78,&local_74,&local_70);
      thunk_EXT_FUN_1fff9c40(local_70 * fVar17 - local_74 * fVar13);
      delay_us(100);
      uVar12 = uVar12 + 1;
    }
    fVar14 = *(float *)(puVar3 + 4);
    fVar15 = fVar14;
    if (0x40800000 < (int)(fVar14 - fVar19)) {
      fVar15 = fVar14 - fVar2;
    }
    fVar16 = fVar15 - fVar19;
    fVar19 = fVar15;
    if (0xc0800000 < (uint)fVar16) {
      fVar19 = fVar15 + fVar2;
    }
    uVar25 = FUN_00027ad8(fVar19);
    uVar26 = FUN_00027532(*(undefined4 *)(iVar1 + 0x54));
    local_6c = (undefined4)((ulonglong)uVar26 >> 0x20);
    uVar22 = FUN_00027228(uVar18,uVar7,(int)uVar26,local_6c);
    FUN_00027904((int)uVar22,(int)((ulonglong)uVar22 >> 0x20),(int)uVar25,
                 (int)((ulonglong)uVar25 >> 0x20));
    uVar6 = FUN_00027074();
    local_90 = 0x89;
    FUN_00027228(uVar18,uVar7,(int)uVar26,local_6c);
    local_8f = FUN_00027074();
    local_83 = *puVar3;
    local_81 = (short)uVar11;
    local_8b = fVar14;
    local_87 = uVar6;
    local_7f = FUN_00021e88(&local_90,0x11);
    debug_uart_write(&local_90,0x15);
    uVar11 = uVar11 + 1;
  }
  for (uVar11 = 0; uVar11 < (uint)(iVar5 * 0x100); uVar11 = uVar11 + 1) {
    uVar12 = 0;
    while( true ) {
      uVar18 = (undefined4)uVar23;
      uVar7 = (undefined4)((ulonglong)uVar23 >> 0x20);
      if (0x27 < uVar12) break;
      uVar23 = FUN_00027904(uVar18,uVar7,(int)uVar21,uVar10);
      uVar18 = (undefined4)((ulonglong)uVar23 >> 0x20);
      FUN_00027558((int)uVar23,uVar18,uVar20,uVar6);
      uVar7 = FUN_000274d8();
      fVar15 = (float)VectorUnsignedToFloat(uVar7,(byte)(in_fpscr >> 0x16) & 3);
      uVar24 = FUN_00027ad8(fVar15 * fVar2);
      FUN_00027754((int)uVar24,(int)((ulonglong)uVar24 >> 0x20),(int)uVar23,uVar18);
      local_78 = FUN_00027074();
      thunk_EXT_FUN_1fffa3aa(uVar4,&local_78);
      thunk_EXT_FUN_1fffa30c(local_78,&local_74,&local_70);
      thunk_EXT_FUN_1fff9c40(local_70 * fVar17 - local_74 * fVar13);
      delay_us(100);
      uVar12 = uVar12 + 1;
    }
    fVar14 = *(float *)(puVar3 + 4);
    fVar15 = fVar14;
    if (0x40800000 < (int)(fVar14 - fVar19)) {
      fVar15 = fVar14 - fVar2;
    }
    fVar16 = fVar15 - fVar19;
    fVar19 = fVar15;
    if (0xc0800000 < (uint)fVar16) {
      fVar19 = fVar15 + fVar2;
    }
    uVar24 = FUN_00027ad8(fVar19);
    iVar8 = *(int *)(iVar1 + 0x54);
    uVar25 = FUN_00027532();
    local_6c = (undefined4)((ulonglong)uVar25 >> 0x20);
    uVar26 = FUN_00027228(uVar18,uVar7,(int)uVar25,local_6c);
    FUN_00027904((int)uVar26,(int)((ulonglong)uVar26 >> 0x20),(int)uVar24,
                 (int)((ulonglong)uVar24 >> 0x20));
    uVar9 = FUN_00027074();
    local_90 = 0x89;
    FUN_00027228(uVar18,uVar7,(int)uVar25,local_6c);
    local_8f = FUN_00027074();
    local_83 = *puVar3;
    local_81 = (short)uVar11 + (short)(iVar8 << 8);
    local_8b = fVar14;
    local_87 = uVar9;
    local_7f = FUN_00021e88(&local_90,0x11);
    debug_uart_write(&local_90,0x15);
  }
  thunk_EXT_FUN_1fff9c40(fVar13);
  *(undefined1 *)(DAT_00024ea0 + 0x46) = 1;
  *DAT_00024ea4 = 1;
  DAT_e000e280 = 4;
  DAT_e000e100 = 4;
  return;
}



/* ===== validate_current_sensors @ 00024ec0, 484 bytes ===== */

void validate_current_sensors(void)

{
  uint uVar1;
  float *pfVar2;
  ushort uVar3;
  byte bVar4;
  float *pfVar5;
  undefined1 *puVar6;
  undefined4 *puVar7;
  byte *pbVar8;
  undefined1 *puVar9;
  int iVar10;
  int iVar11;
  uint *puVar12;
  ushort uVar13;
  undefined4 uVar14;
  uint in_fpscr;
  float fVar15;
  float fVar16;
  float fVar17;
  undefined8 uVar18;
  
  fVar16 = DAT_000250a4;
  delay_ms(5);
  puVar9 = DAT_000250b8;
  pbVar8 = DAT_000250b4;
  puVar7 = DAT_000250b0;
  puVar6 = DAT_000250ac;
  pfVar2 = DAT_000250a8;
  uVar13 = 0;
  DAT_000250a8[0x11] = 0.0;
  fVar17 = fVar16;
  do {
    *puVar9 = 1;
    *puVar6 = 1;
    *DAT_000250bc = 1;
    *puVar7 = 0;
    delay_us(100);
    do {
    } while ((puVar9[0x44] & 1) == 0);
    fVar15 = (float)VectorUnsignedToFloat
                              ((uint)*(ushort *)(puVar9 + 0x54),(byte)(in_fpscr >> 0x16) & 3);
    fVar17 = fVar15 + fVar17;
    do {
    } while ((*pbVar8 & 1) == 0);
    uVar3 = *DAT_000250c0;
    puVar9[0x46] = 1;
    fVar15 = (float)VectorUnsignedToFloat((uint)uVar3,(byte)(in_fpscr >> 0x16) & 3);
    fVar16 = fVar15 + fVar16;
    *DAT_000250c4 = 1;
    *DAT_000250c8 = 1;
    iVar11 = DAT_000250d8;
    uVar1 = DAT_000250d4;
    iVar10 = DAT_000250d0;
    uVar13 = uVar13 + 1;
  } while (uVar13 < 1000);
  fVar17 = fVar17 * DAT_000250cc;
  fVar16 = fVar16 * DAT_000250cc;
  if (DAT_000250d4 <= (uint)((int)fVar17 + DAT_000250d0)) {
    *(undefined4 *)(DAT_000250d8 + 0x80) = 4;
    uVar18 = FUN_00027ad8(fVar17);
    uVar14 = (undefined4)((ulonglong)uVar18 >> 0x20);
    debug_printf("Sensor U broken!U=%.4f\r\n",uVar14,(int)uVar18,uVar14);
  }
  if (uVar1 <= (uint)((int)fVar16 + iVar10)) {
    *(undefined4 *)(iVar11 + 0x80) = 4;
    uVar18 = FUN_00027ad8(fVar16);
    uVar14 = (undefined4)((ulonglong)uVar18 >> 0x20);
    debug_printf("Sensor V broken!V=%.4f\r\n",uVar14,(int)uVar18,uVar14);
  }
  *pfVar2 = fVar17;
  pfVar2[1] = fVar16;
  pfVar5 = DAT_000250a8;
  pfVar2[4] = fVar17 - pfVar2[5];
  pfVar2[6] = (fVar16 - pfVar2[7]) * pfVar2[10];
  thunk_EXT_FUN_1fff987c(pfVar5);
  fVar16 = DAT_00025118;
  fVar17 = pfVar2[0xf];
  if (DAT_00025114 < (int)fVar17) {
    fVar17 = fVar17 - DAT_00025118;
    pfVar2[0xf] = fVar17;
    pfVar2[0x11] = (float)((int)pfVar2[0x11] + -1);
  }
  if ((uint)DAT_0002511c < (uint)fVar17) {
    fVar17 = fVar17 + fVar16;
    pfVar2[0xf] = fVar17;
    pfVar2[0x11] = (float)((int)pfVar2[0x11] + 1);
  }
  puVar12 = DAT_0002512c;
  iVar10 = DAT_00025120;
  pfVar2 = (float *)(DAT_00025120 + 0x24);
  fVar15 = *(float *)(DAT_00025124 + 8);
  fVar17 = (*(float *)(DAT_00025120 + 0x4c) * (fVar17 + *pfVar2) - fVar15) / fVar16;
  uVar1 = in_fpscr & 0xfffffff | (uint)(fVar17 < 0.0) << 0x1f | (uint)(fVar17 == 0.0) << 0x1e;
  bVar4 = (byte)(uVar1 >> 0x18);
  if ((bool)(bVar4 >> 6 & 1) || (bool)(bVar4 >> 7) != NAN(fVar17)) {
    fVar17 = fVar17 - 0.5;
  }
  else {
    fVar17 = fVar17 + 0.5;
  }
  *(int *)(DAT_00025120 + 8) = (int)fVar17;
  fVar17 = (float)VectorSignedToFloat((int)fVar17,(byte)(uVar1 >> 0x16) & 3);
  *(float *)(iVar10 + 0x18) = (fVar15 + fVar17 * fVar16) * *(float *)(iVar10 + 0x5c) - *pfVar2;
  puVar12[4] = DAT_00025128;
  *puVar12 = *puVar12 & 0xfffdffff;
  puVar12[4] = DAT_00025130;
  *DAT_00025134 = 0xce;
  *(undefined2 *)(puVar9 + 10) = 0x81;
  *(undefined2 *)(puVar9 + 0x4c) = 0x30;
  *(ushort *)(puVar9 + 0x4c) = *(ushort *)(puVar9 + 0x4c) | 1;
  puVar9[0x45] = 1;
  return;
}



/* ===== derive_control_parameters @ 00025138, 368 bytes ===== */

void derive_control_parameters(void)

{
  float *pfVar1;
  float *pfVar2;
  int iVar3;
  float fVar4;
  undefined4 *puVar5;
  int iVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  undefined4 uVar12;
  float fVar13;
  
  enforce_device_key_binding_or_halt();
  fVar4 = DAT_000252bc;
  iVar3 = DAT_000252b8;
  pfVar2 = DAT_000252b4;
  pfVar1 = DAT_000252b0;
  fVar8 = *(float *)(DAT_000252a8 + 0x68);
  if (fVar8 <= *DAT_000252ac) {
    *DAT_000252b0 = DAT_000252bc;
    *pfVar2 = fVar4;
    pfVar1[1] = fVar4;
    pfVar2[1] = fVar4;
  }
  else {
    fVar7 = *(float *)(DAT_000252b8 + 0x48);
    fVar9 = ((*(float *)(DAT_000252b8 + 0x38) * fVar7 * *(float *)(DAT_000252b8 + 8)) / fVar8) *
            DAT_000252c0;
    *DAT_000252b0 = fVar9;
    *pfVar2 = fVar9;
    fVar7 = (*(float *)(iVar3 + 0x44) / fVar7) * *(float *)(iVar3 + 0xc);
    pfVar1[1] = fVar7;
    pfVar2[1] = fVar7;
  }
  puVar5 = DAT_000252c4;
  fVar7 = *(float *)(iVar3 + 0xc);
  DAT_000252c4[4] = fVar7;
  fVar10 = *(float *)(iVar3 + 8);
  fVar9 = (fVar8 * DAT_000252c8) / (fVar10 * *(float *)(iVar3 + 0x48));
  puVar5[8] = fVar9;
  puVar5[9] = 1.0 / fVar9;
  puVar5[10] = fVar9 * fVar7;
  uVar12 = *(undefined4 *)(iVar3 + 0x38);
  *puVar5 = uVar12;
  fVar8 = *(float *)(iVar3 + 0x3c);
  puVar5[7] = fVar8;
  fVar11 = fVar8 * 2.0 * fVar7;
  fVar13 = fVar8 * fVar8 * fVar7;
  puVar5[0xb] = fVar11;
  puVar5[0xc] = fVar13;
  puVar5[0x11] = 0xbf800000;
  puVar5[0x12] = 0x3f800000;
  puVar5 = DAT_000252cc;
  DAT_000252cc[4] = fVar7;
  puVar5[8] = fVar9;
  puVar5[9] = 1.0 / fVar9;
  puVar5[10] = fVar9 * fVar7;
  *puVar5 = uVar12;
  puVar5[7] = fVar8;
  puVar5[0xb] = fVar11;
  puVar5[0xc] = fVar13;
  puVar5[0x11] = 0xbf800000;
  puVar5[0x12] = 0x3f800000;
  iVar6 = DAT_000252d0;
  fVar8 = (*(float *)(iVar3 + 0x40) * 1.5 * *(float *)(iVar3 + 0x4c) * fVar10) /
          *(float *)(iVar3 + 0x50);
  *(float *)(DAT_000252d0 + 0x28) = fVar8;
  uVar12 = DAT_000252d4;
  *(float *)(iVar6 + 0x2c) = 1.0 / fVar8;
  *(undefined4 *)(iVar6 + 0x1c) = uVar12;
  *(float *)(iVar6 + 8) = fVar4;
  *(float *)(iVar6 + 0x14) = fVar4;
  *(float *)(iVar6 + 0x18) = fVar4;
  *(undefined4 *)(iVar6 + 0x20) = *(undefined4 *)(iVar3 + 0x58);
  *(undefined4 *)(iVar6 + 0x24) = *(undefined4 *)(iVar3 + 0x5c);
  *(undefined4 *)(iVar6 + 0x34) = 0xbf800000;
  *(undefined4 *)(iVar6 + 0x38) = 0x3f800000;
  return;
}



/* ===== copy_persistent_configuration @ 000252d8, 18 bytes ===== */

void copy_persistent_configuration(void)

{
  undefined4 *puVar1;
  undefined4 uVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 uVar5;
  
  puVar1 = DAT_000252ec;
  uVar2 = DAT_000252f0[1];
  uVar3 = DAT_000252f0[2];
  uVar4 = DAT_000252f0[3];
  uVar5 = DAT_000252f0[4];
  *DAT_000252ec = *DAT_000252f0;
  puVar1[1] = uVar2;
  puVar1[2] = uVar3;
  puVar1[3] = uVar4;
  puVar1[4] = uVar5;
  return;
}



/* ===== main @ 000252f4, 720 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void main(void)

{
  undefined4 *puVar1;
  int iVar2;
  ushort *puVar3;
  ushort *puVar4;
  int iVar5;
  int iVar6;
  undefined1 uVar7;
  uint uVar8;
  undefined4 uVar9;
  undefined8 uVar10;
  
  *DAT_000255c4 = 0x20000;
  system_clock_and_systick_init();
  if (*(int *)(DAT_000255c8 + 4) != 1) {
    select_configuration_bank_a();
  }
  store_hardware_variant(DAT_000255cc & 0xffffff | 0x37000000);
  enableIRQinterrupts();
  delay_ms(500);
  uVar7 = read_hardware_variant();
  *DAT_000255d4 = uVar7;
  debug_usart1_dma_init(0xe1000);
  adc_sampling_init();
  power_stage_self_test_or_halt();
  crc_clock_enable();
  position_sensor_configure();
  load_and_validate_calibration();
  puVar1 = DAT_000255d8;
  calibrate_adc_offsets(DAT_000255d8 + 0x10);
  puVar1[0x1a] = (float)puVar1[0x13] * DAT_000255dc;
  load_motor_configuration();
  validate_current_sensors();
  uVar9 = DAT_000255e0;
  puVar1[0x24] = 0;
  puVar1[0x27] = uVar9;
  iVar2 = DAT_000255e4;
  puVar1[-0x11] = 0;
  puVar1[-0x10] = 1;
  (*(code *)puVar1[0x66])(*(undefined2 *)(iVar2 + 0x8c),*(undefined2 *)(iVar2 + 0x20));
  debug_print_device_info();
  puVar4 = DAT_000255ec;
  puVar3 = DAT_000255e8;
  if ((int)puVar1[0x1a] < 0x42000001) {
    pwm_timer_init();
    *(undefined4 *)(DAT_000255f4 + 100) = 0x1e0;
    iVar2 = DAT_000255f8;
    *(undefined1 *)(DAT_000255f8 + 0x400) = 0x20;
    *(undefined4 *)(iVar2 + 0x27e) = 4;
    *(undefined4 *)(iVar2 + 0xfe) = 4;
    puVar3 = DAT_000255fc;
    do {
      if (puVar1[-0x10] == 1) {
        if (puVar1[-0x11] == 0) {
          puVar1[-0x10] = 0;
          *puVar1 = uVar9;
          puVar1[1] = uVar9;
          puVar1[2] = uVar9;
          puVar1[3] = uVar9;
          puVar1[4] = uVar9;
          debug_printf(&DAT_00025600);
          debug_printf(" Commands:\n\r");
          delay_us(10);
          debug_printf(" m - Motor Mode\n\r");
          delay_us(10);
          debug_printf(" s - Setup Mode\n\r");
          delay_us(10);
          debug_printf(" esc - Exit to Menu\n\r");
          delay_us(10);
          *puVar3 = *puVar3 | 0x2000;
          *_DAT_00025654 = *_DAT_00025654 | 4;
        }
        else if (puVar1[-0x11] == 2) {
          puVar1[-0x10] = 0;
          debug_printf("\n\r Entering Motor Mode \n\r");
          *DAT_000255e8 = *DAT_000255e8 | 0x2000;
          *DAT_000255ec = *DAT_000255ec | 4;
        }
      }
      iVar5 = DAT_00025674;
      if (*(int *)(DAT_00025674 + 0x2c) != 0) {
        if ((1 < (uint)puVar1[0x20]) &&
           (uVar8 = *(int *)(DAT_00025674 + 0x48) + 1, *(uint *)(DAT_00025674 + 0x48) = uVar8,
           0xfa < uVar8)) {
          *(undefined4 *)(iVar5 + 0x48) = 0;
          *puVar3 = *puVar3 | 0x2000;
          *_DAT_00025678 = *_DAT_00025678 | 4;
        }
        *(undefined4 *)(iVar5 + 0x2c) = 0;
      }
      if (*(int *)(iVar5 + 0x30) == 1) {
        debug_printf("CAN Error 1\n\r");
        *(undefined4 *)(iVar5 + 0x30) = 0;
      }
      else if (*(int *)(iVar5 + 0x30) == 2) {
        debug_printf("CAN Error 2\n\r");
        *(undefined4 *)(iVar5 + 0x30) = 0;
      }
      if (*(int *)(iVar5 + 0xc) != 0) {
        disableIRQinterrupts();
        thunk_EXT_FUN_1fff9950(DAT_0002569c,DAT_000255e4,0x25);
        iVar6 = DAT_000256a0;
        *(undefined4 *)(iVar5 + 0xc) = 0;
        *(undefined1 *)(iVar6 + 0x46) = 1;
        *DAT_000256a4 = 1;
        *(undefined4 *)(iVar2 + 0x27e) = 4;
        *(ushort *)(DAT_000256a8 + 0x58) = *(ushort *)(DAT_000256a8 + 0x58) & 0xf5ff;
        *(undefined4 *)(iVar2 + 0x27e) = 1;
        enableIRQinterrupts();
      }
      if (*(int *)(iVar5 + 8) != 0) {
        *(undefined4 *)(iVar5 + 8) = 0;
      }
      if (*(int *)(iVar5 + 0x10) != 0) {
        detect_motor_direction_and_pole_pairs();
        measure_encoder_alignment();
        *(undefined4 *)(iVar5 + 0x10) = 0;
        puVar1[-0x11] = 0;
      }
      if (*(int *)(iVar5 + 0x1c) != 0) {
        disableIRQinterrupts();
        if (*(int *)(iVar5 + 0x1c) == 1) {
          thunk_EXT_FUN_1fff9950(DAT_000256b0,DAT_000256ac,0x103);
        }
        if (*(int *)(iVar5 + 0x1c) == 2) {
          thunk_EXT_FUN_1fff9950(DAT_000256b8,DAT_000256b4,0x800);
          thunk_EXT_FUN_1fff9950(DAT_000256c0,DAT_000256bc,4);
        }
        *(undefined4 *)(iVar5 + 0x1c) = 0;
        load_and_validate_calibration();
        *(undefined4 *)(DAT_000256c4 + 0x10) = uVar9;
        *(undefined1 *)(DAT_000256a0 + 0x46) = 1;
        *DAT_000256a4 = 1;
        *(undefined4 *)(iVar2 + 0x27e) = 4;
        *(ushort *)(DAT_000256a8 + 0x58) = *(ushort *)(DAT_000256a8 + 0x58) & 0xf5ff;
        *(undefined4 *)(iVar2 + 0x27e) = 1;
        enableIRQinterrupts();
        puVar1[-0x11] = 0;
      }
      if (*(int *)(iVar5 + 0x24) != 0) {
        run_motor_parameter_identification();
        *(undefined4 *)(iVar5 + 0x24) = 0;
        puVar1[-0x11] = 0;
      }
      if (*(uint *)(iVar5 + 0x28) != 0) {
        handle_firmware_control_request(*(uint *)(iVar5 + 0x28) & 0xff);
        *(undefined4 *)(iVar5 + 0x28) = 0;
        puVar1[-0x11] = 0;
      }
      if (*(int *)(iVar5 + 0x14) != 0) {
        calibrate_position_sensor();
        *(undefined4 *)(iVar5 + 0x14) = 0;
      }
    } while( true );
  }
  do {
    *puVar3 = *puVar3 | 0x2000;
    *puVar4 = *puVar4 | 4;
    delay_ms(1000);
    uVar10 = FUN_00027ad8(puVar1[0x1a]);
    uVar9 = (undefined4)((ulonglong)uVar10 >> 0x20);
    debug_printf(DAT_000255f0,uVar9,(int)uVar10,uVar9);
  } while( true );
}



/* ===== power_stage_self_test_or_halt @ 000256c8, 148 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void power_stage_self_test_or_halt(void)

{
  ushort *puVar1;
  int iVar2;
  ushort *puVar3;
  char local_30 [5];
  char local_2b;
  char local_26;
  char local_21;
  char local_1c;
  char local_17;
  
  runtime_memcpy_aligned(local_30,"W  | W  | V  | V  | U  | U ",0x1c);
  iVar2 = power_stage_switch_response_test();
  if (iVar2 != 0) {
    debug_usart1_dma_init(0xe1000);
    puVar1 = _DAT_00025778;
    local_30[0] = '0' - (char)((iVar2 << 0x1a) >> 0x1f);
    local_2b = '0' - (char)((iVar2 << 0x1b) >> 0x1f);
    local_26 = '0' - (char)((iVar2 << 0x1c) >> 0x1f);
    local_21 = '0' - (char)((iVar2 << 0x1d) >> 0x1f);
    local_1c = '0' - (char)((iVar2 << 0x1e) >> 0x1f);
    local_17 = ((byte)iVar2 & 1) + 0x30;
    puVar3 = _DAT_00025778 + 0x16;
    do {
      debug_printf("MOSFET ERROR,  WH | WL | VH | VL | UH | UL\r\n");
      debug_printf("               %s\r\n",local_30);
      debug_printf(&DAT_000257c0);
      *puVar1 = *puVar1 | 0x2000;
      *puVar3 = *puVar3 | 4;
      delay_ms(500);
    } while( true );
  }
  return;
}



/* ===== run_motor_parameter_identification @ 000257c4, 3084 bytes ===== */

/* WARNING: Heritage AFTER dead removal. Example location: s1 : 0x00025a12 */
/* WARNING: Restarted to delay deadcode elimination for space: register */

void run_motor_parameter_identification(undefined4 param_1,undefined4 param_2)

{
  byte bVar1;
  float fVar2;
  float *pfVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;
  int extraout_r1;
  undefined4 uVar8;
  int iVar9;
  uint uVar10;
  int iVar11;
  int iVar12;
  uint uVar13;
  uint in_fpscr;
  float fVar14;
  float fVar15;
  float fVar16;
  undefined4 uVar17;
  undefined4 uVar18;
  float fVar19;
  float fVar20;
  float fVar21;
  float fVar22;
  undefined4 extraout_s1;
  undefined4 extraout_s1_00;
  undefined4 extraout_s1_01;
  float extraout_s7;
  undefined8 uVar23;
  undefined8 uVar24;
  float fVar25;
  float fVar26;
  float fVar27;
  float fVar28;
  undefined8 uVar29;
  undefined8 uVar30;
  float local_180;
  float local_17c;
  float local_178;
  ushort local_174;
  ushort local_170;
  ushort local_16c;
  float local_168;
  float local_164;
  undefined4 local_160;
  undefined4 local_15c;
  float local_158;
  float local_154;
  float local_150;
  float local_14c;
  float local_148;
  float local_144;
  float local_140;
  float local_134;
  float local_130;
  float local_12c;
  float fStack_128;
  float fStack_124;
  float local_120;
  float fStack_11c;
  float fStack_118;
  float local_114;
  float local_10c;
  float local_108;
  float local_104;
  float local_100;
  float local_f0;
  float local_e0;
  float local_dc;
  float local_d8;
  float local_c8;
  float local_b8;
  float local_b4;
  float local_b0;
  float local_ac;
  float local_a8;
  float local_a4;
  float local_a0;
  float local_9c;
  float local_98;
  undefined1 local_94;
  undefined4 local_93;
  undefined4 local_8f;
  undefined4 local_8b;
  undefined4 local_87;
  undefined4 local_83;
  float local_7c;
  float local_78;
  undefined4 local_74;
  float local_70;
  float local_6c;
  undefined1 auStack_68 [4];
  
  uVar23 = DAT_00025bd8;
  fVar2 = DAT_00025bd4;
  fVar25 = DAT_00025bd0;
  uVar13 = 0;
  uVar10 = 0;
  iVar9 = 0;
  local_17c = DAT_00025bd0;
  local_178 = 1.0;
  local_6c = DAT_00025bd4;
  runtime_memcpy_aligned(&local_108,DAT_00025be0,0x28);
  runtime_memcpy_aligned(&local_e0,DAT_00025be0 + 0x28,0x28);
  DAT_e000e180 = 4;
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  enforce_device_key_binding_or_halt();
  do {
    thunk_EXT_FUN_1fff9c40(DAT_00025be4);
    delay_us(100);
    iVar4 = DAT_00025bec;
    pfVar3 = DAT_00025be8;
    uVar13 = uVar13 + 1;
  } while (uVar13 < 0x188c);
  local_78 = DAT_00025be8[5] / (DAT_00025be8[2] * fVar2);
  local_b4 = DAT_00025bd0;
  local_b0 = DAT_00025bd0;
  local_ac = DAT_00025bd0;
  local_a8 = DAT_00025bd0;
  local_a4 = fVar2;
  local_a0 = DAT_00025bd0;
  local_9c = DAT_00025bd0;
  local_98 = fVar2;
  *(undefined1 *)(DAT_00025bec + 0x46) = 1;
  *DAT_00025bf0 = 1;
  *DAT_00025bf4 = 1;
  iVar5 = DAT_00025c00;
  fVar2 = DAT_00025bfc;
  iVar12 = DAT_00025bf8;
  uVar13 = 0;
  fVar26 = fVar25;
  fVar14 = fVar25;
  fVar16 = fVar25;
  do {
    do {
    } while ((*(byte *)(iVar4 + 0x44) & 1) == 0);
    uVar10 = uVar10 + 1;
    if (uVar10 < 0x2711) {
      if (uVar10 % 100 == 0) {
        fVar26 = fVar26 + local_78;
      }
    }
    if (uVar10 % 10 == 0) {
      fVar14 = (float)VectorUnsignedToFloat(uVar10,(byte)(in_fpscr >> 0x16) & 3);
      local_168 = local_6c * fVar14 * DAT_00025c04;
      uVar18 = VectorFloatToUnsigned(local_168 * DAT_00025c08,3);
      fVar14 = (float)VectorUnsignedToFloat(uVar18,(byte)(in_fpscr >> 0x16) & 3);
      local_168 = local_168 - fVar14 * fVar2;
      thunk_EXT_FUN_1fffa3aa(DAT_00025c10,&local_168);
      thunk_EXT_FUN_1fffa30c(local_168,&local_7c,auStack_68);
      fVar14 = fVar26 * local_7c;
    }
    local_174 = *(ushort *)(iVar4 + 0x50);
    local_170 = *(ushort *)(DAT_00025bf0 + 10);
    local_16c = *(ushort *)(iVar4 + 0x52);
    fVar19 = *pfVar3;
    fVar15 = (float)VectorUnsignedToFloat((uint)local_16c,(byte)(in_fpscr >> 0x16) & 3);
    *(float *)(iVar12 + 0x68) = fVar19 * fVar15;
    fVar28 = DAT_00025c14;
    *(float *)(iVar12 + 0x6c) = fVar19 * fVar15 * DAT_00025c14;
    iVar6 = DAT_00025c1c;
    fVar15 = (float)VectorUnsignedToFloat((uint)local_174,(byte)(in_fpscr >> 0x16) & 3);
    fVar15 = (*(float *)(iVar12 + 0x40) - fVar15) * DAT_00025c18;
    fVar19 = (float)VectorUnsignedToFloat((uint)local_170,(byte)(in_fpscr >> 0x16) & 3);
    fVar28 = (fVar15 + (*(float *)(iVar12 + 0x44) - fVar19) * DAT_00025c18 * 2.0) * fVar28;
    if (*(int *)(DAT_00025c1c + 4) != 0) {
      *(undefined4 *)(DAT_00025c1c + 4) = 0;
      fVar20 = (float)VectorUnsignedToFloat
                                (*(undefined4 *)(iVar5 + 0x54),(byte)(in_fpscr >> 0x16) & 3);
      uVar18 = VectorFloatToUnsigned(*(float *)(iVar6 + 8) * *(float *)(iVar5 + 0x58),3);
      fVar19 = (float)VectorUnsignedToFloat(uVar18,(byte)(in_fpscr >> 0x16) & 3);
      local_180 = (*(float *)(iVar6 + 8) * fVar20 - fVar19 * fVar2) + *(float *)(iVar5 + 0x14);
      thunk_EXT_FUN_1fffa3aa(DAT_00025c10,param_2,*(float *)(iVar5 + 0x58),&local_180);
      thunk_EXT_FUN_1fffa30c(local_180,&local_17c,&local_178);
    }
    fVar19 = local_178 * fVar15 + local_17c * fVar28;
    fVar28 = local_178 * fVar28;
    fVar15 = local_17c * fVar15;
    if (20000 < uVar10) {
      local_b8 = pfVar3[2] * fVar19;
      local_b0 = *(float *)(iVar12 + 0x6c) * fVar16;
      thunk_EXT_FUN_1fffa160(&local_b8);
    }
    fVar20 = pfVar3[6];
    fVar16 = (float)thunk_EXT_FUN_1fffa38a(fVar20 * (fVar14 - fVar19));
    thunk_EXT_FUN_1fff9c40(local_178 * fVar16 - local_17c * -(fVar20 * (fVar28 - fVar15)));
    *(undefined1 *)(iVar4 + 0x46) = 1;
    *DAT_00025bf0 = 1;
    *DAT_00025bf4 = 1;
    uVar13 = uVar13 + 1;
  } while (uVar13 < 60000);
  delay_ms(5);
  fVar26 = DAT_00025bd0;
  thunk_EXT_FUN_1fff9c40(DAT_00025bd0);
  iVar6 = DAT_00025c20;
  fVar14 = pfVar3[3];
  *(float *)(DAT_00025c20 + 0x48) = fVar14 / local_a8;
  fVar15 = (1.0 - local_ac) / local_a8;
  *(float *)(iVar6 + 0x44) = fVar15;
  if (0.0 <= fVar15) {
    uVar10 = in_fpscr & 0xfffffff | (uint)(0.0 <= fVar14 / local_a8) << 0x1d;
    if ((byte)(uVar10 >> 0x1d) != 0) {
      delay_ms(5);
      iVar6 = DAT_00025c1c;
      local_164 = *(float *)(DAT_00025c20 + 0x48);
      local_160 = DAT_00025c30;
      local_15c = DAT_00025be4;
      local_144 = fVar26;
      local_140 = fVar26;
      local_114 = DAT_00025be8[3];
      local_12c = *(float *)(DAT_00025c20 + 0x44) / local_164;
      fStack_128 = DAT_00025c34 / local_164;
      fStack_124 = 1.0 / local_164;
      local_134 = fVar26;
      local_130 = fVar26;
      *(float *)(DAT_00025c1c + 0x10) = fVar26;
      *(undefined1 *)(iVar4 + 0x46) = 1;
      *DAT_00025bf0 = 1;
      *DAT_00025bf4 = 1;
      iVar11 = 0;
      uVar13 = 0;
      *(undefined4 *)(iVar6 + 4) = 0;
      fVar14 = fVar26;
      fVar15 = fVar25;
      fVar28 = fVar25;
      local_120 = local_12c;
      fStack_11c = fStack_128;
      fStack_118 = fStack_124;
      do {
        do {
          fVar19 = DAT_00025c14;
          pfVar3 = DAT_00025be8;
        } while ((*(byte *)(iVar4 + 0x44) & 1) == 0);
        local_174 = *(ushort *)(iVar4 + 0x50);
        local_170 = *(ushort *)(DAT_00025bf0 + 10);
        local_16c = *(ushort *)(iVar4 + 0x52);
        fVar21 = *DAT_00025be8;
        fVar20 = (float)VectorUnsignedToFloat((uint)local_16c,(byte)(uVar10 >> 0x16) & 3);
        *(float *)(iVar12 + 0x68) = fVar21 * fVar20;
        *(float *)(iVar12 + 0x6c) = fVar21 * fVar20 * fVar19;
        fVar20 = DAT_00025c18;
        fVar21 = (float)VectorUnsignedToFloat((uint)local_174,(byte)(uVar10 >> 0x16) & 3);
        fVar27 = (*(float *)(iVar12 + 0x40) - fVar21) * DAT_00025c18;
        iVar11 = iVar11 + 1;
        fVar21 = (float)VectorUnsignedToFloat((uint)local_170,(byte)(uVar10 >> 0x16) & 3);
        fVar19 = (fVar27 + (*(float *)(iVar12 + 0x44) - fVar21) * DAT_00025c18 * 2.0) * fVar19;
        if (*(int *)(iVar6 + 4) != 0) {
          *(undefined4 *)(iVar6 + 4) = 0;
          fVar22 = (float)VectorUnsignedToFloat
                                    (*(undefined4 *)(iVar5 + 0x54),(byte)(uVar10 >> 0x16) & 3);
          uVar18 = VectorFloatToUnsigned(*(float *)(iVar6 + 8) * *(float *)(iVar5 + 0x58),3);
          fVar21 = (float)VectorUnsignedToFloat(uVar18,(byte)(uVar10 >> 0x16) & 3);
          local_180 = (*(float *)(iVar6 + 8) * fVar22 - fVar21 * fVar2) + *(float *)(iVar5 + 0x14);
          thunk_EXT_FUN_1fffa3aa(DAT_00025c10,param_2,*(float *)(iVar5 + 0x58),&local_180);
          thunk_EXT_FUN_1fffa30c(local_180,&local_17c,&local_178);
        }
        local_154 = pfVar3[2];
        local_158 = local_154 * (local_178 * fVar27 + local_17c * fVar19);
        local_154 = local_154 * (local_178 * fVar19 - local_17c * fVar27);
        local_150 = *(float *)(iVar12 + 0x6c) * fVar16;
        local_14c = *(float *)(iVar12 + 0x6c) * fVar14;
        local_148 = fVar15;
        thunk_EXT_FUN_1fffa234(&local_164,pfVar3);
        if (iVar11 == 0x14) {
          fVar15 = (float)VectorUnsignedToFloat
                                    (*(undefined4 *)(iVar5 + 0x54),(byte)(uVar10 >> 0x16) & 3);
          fVar15 = *(float *)(iVar6 + 0x10) * fVar15 * DAT_00026150;
          fVar19 = *(float *)(iVar5 + 0x68);
          fVar21 = *(float *)(iVar5 + 0x6c);
          *(float *)(iVar6 + 0x10) = fVar26;
          fVar15 = fVar19 * fVar28 + fVar15 * fVar21;
          fVar14 = fVar14 + DAT_00026154;
          fVar28 = *(float *)(extraout_r1 + 0x20);
          uVar10 = uVar10 & 0xfffffff | (uint)(fVar28 == fVar14) << 0x1e |
                   (uint)(fVar14 <= fVar28) << 0x1d;
          bVar1 = (byte)(uVar10 >> 0x18);
          if (!(bool)(bVar1 >> 5 & 1) || (bool)(bVar1 >> 6)) {
            fVar14 = fVar28;
          }
          iVar11 = 0;
          fVar28 = fVar15;
        }
        fVar16 = (float)thunk_EXT_FUN_1fffa38a
                                  (fVar16 - *(float *)(extraout_r1 + 0xc) * 10.0 * extraout_s7,
                                   param_2,DAT_00026158);
        thunk_EXT_FUN_1fff9c40(local_178 * fVar16 - local_17c * fVar14);
        *(undefined1 *)(iVar4 + 0x46) = 1;
        *DAT_00026160 = 1;
        *DAT_00026164 = 1;
        uVar13 = uVar13 + 1;
      } while (uVar13 < 40000);
      delay_ms(1);
      fVar26 = DAT_00025bd0;
      thunk_EXT_FUN_1fff9c40(DAT_00025bd0);
      pfVar3 = DAT_0002616c;
      iVar12 = DAT_00026168;
      *(float *)(DAT_00026168 + 0x4c) = local_10c;
      *(float *)(iVar6 + 0x10) = fVar26;
      iVar11 = DAT_00026178;
      local_104 = (*(float *)(iVar12 + 0x44) / *(float *)(iVar12 + 0x48)) * pfVar3[3];
      local_6c = pfVar3[2];
      local_108 = ((*(float *)(iVar12 + 0x60) * local_6c * *(float *)(iVar12 + 0x48)) /
                  *(float *)(DAT_00026170 + 0x68)) * DAT_00026174;
      uVar18 = *(undefined4 *)(iVar5 + 0x54);
      fVar16 = (float)VectorUnsignedToFloat(uVar18,(byte)(uVar10 >> 0x16) & 3);
      fVar15 = (float)VectorUnsignedToFloat(uVar18,(byte)(uVar10 >> 0x16) & 3);
      fVar14 = (float)VectorUnsignedToFloat(uVar18,(byte)(uVar10 >> 0x16) & 3);
      fVar16 = fVar16 * 1.5 * local_10c * *(float *)(iVar12 + 0x50) * local_6c;
      *(float *)(iVar5 + 0x44) = fVar16;
      local_6c = fVar15 * 1.5 * local_10c * local_6c;
      *(float *)(iVar5 + 0x48) = 1.0 / fVar16;
      uVar18 = VectorFloatToUnsigned(*(float *)(iVar11 + 8) * *(float *)(iVar5 + 0x58),3);
      fVar16 = (float)VectorUnsignedToFloat(uVar18,(byte)(uVar10 >> 0x16) & 3);
      local_180 = (*(float *)(iVar11 + 8) * fVar14 - fVar16 * fVar2) + *(float *)(iVar5 + 0x14);
      local_e0 = local_108;
      local_dc = local_104;
      thunk_EXT_FUN_1fffa3aa(DAT_00025c10,&local_180);
      thunk_EXT_FUN_1fffa30c(local_180,&local_17c,&local_178);
      iVar12 = DAT_00026178;
      *(float *)(DAT_00026178 + 0x10) = fVar26;
      *(undefined4 *)(iVar12 + 4) = 0;
      iVar12 = 0;
      local_78 = pfVar3[9] / pfVar3[2];
      fVar14 = pfVar3[10];
      *(undefined1 *)(iVar4 + 0x46) = 1;
      *DAT_00026160 = 1;
      *DAT_00026164 = 1;
      uVar13 = 0;
      uVar24 = uVar23;
      uVar30 = uVar23;
      do {
        do {
          uVar18 = (undefined4)((ulonglong)uVar30 >> 0x20);
        } while ((*(byte *)(iVar4 + 0x44) & 1) == 0);
        iVar9 = iVar9 + 1;
        fVar16 = (float)VectorUnsignedToFloat(iVar9,(byte)(uVar10 >> 0x16) & 3);
        fVar15 = fVar14 * fVar16 * DAT_00025c04;
        uVar17 = VectorFloatToUnsigned(fVar15 * DAT_00025c08,3);
        fVar16 = (float)VectorUnsignedToFloat(uVar17,(byte)(uVar10 >> 0x16) & 3);
        local_168 = fVar15 - fVar16 * fVar2;
        iVar12 = iVar12 + 1;
        thunk_EXT_FUN_1fffa3aa(DAT_00025c10,&local_168);
        thunk_EXT_FUN_1fffa30c(local_168,&local_7c,auStack_68);
        fVar16 = local_78 * local_7c;
        if (DAT_0002617c < (int)fVar15) {
          uVar8 = (undefined4)((ulonglong)uVar23 >> 0x20);
          uVar17 = FUN_00027558((int)uVar30,uVar18,(int)uVar23,uVar8);
          uVar17 = FUN_00023f56(uVar17);
          uVar17 = FUN_00027228((int)uVar24,(int)((ulonglong)uVar24 >> 0x20),uVar17,extraout_s1);
          uVar17 = FUN_00023890(uVar17);
          local_74 = FUN_00027074(uVar17,extraout_s1_00);
          uVar18 = FUN_00027228((int)uVar30,uVar18,(int)uVar23,uVar8);
          uVar18 = FUN_00023f56(uVar18);
          local_70 = (float)FUN_00027074(uVar18,extraout_s1_01);
          local_70 = local_70 * local_6c;
          iVar9 = 0;
          uVar23 = DAT_00026180;
          uVar24 = DAT_00026180;
          param_2 = extraout_s1_01;
          uVar30 = DAT_00026180;
        }
        iVar11 = DAT_00026170;
        local_174 = *(ushort *)(iVar4 + 0x50);
        local_170 = *(ushort *)(DAT_00026160 + 10);
        local_16c = *(ushort *)(iVar4 + 0x52);
        fVar15 = (float)VectorUnsignedToFloat((uint)local_16c,(byte)(uVar10 >> 0x16) & 3);
        *(float *)(DAT_00026170 + 0x68) = *DAT_0002616c * fVar15;
        fVar15 = (float)VectorUnsignedToFloat((uint)local_174,(byte)(uVar10 >> 0x16) & 3);
        fVar28 = (*(float *)(iVar11 + 0x40) - fVar15) * fVar20;
        fVar15 = (float)VectorUnsignedToFloat((uint)local_170,(byte)(uVar10 >> 0x16) & 3);
        fVar15 = (fVar28 + (*(float *)(iVar11 + 0x44) - fVar15) * fVar20 * 2.0) * DAT_00026188;
        if (*(int *)(iVar6 + 4) != 0) {
          *(undefined4 *)(iVar6 + 4) = 0;
          fVar21 = (float)VectorUnsignedToFloat
                                    (*(undefined4 *)(iVar5 + 0x54),(byte)(uVar10 >> 0x16) & 3);
          uVar18 = VectorFloatToUnsigned(*(float *)(iVar6 + 8) * *(float *)(iVar5 + 0x58),3);
          fVar19 = (float)VectorUnsignedToFloat(uVar18,(byte)(uVar10 >> 0x16) & 3);
          local_180 = (*(float *)(iVar6 + 8) * fVar21 - fVar19 * fVar2) + *(float *)(iVar5 + 0x14);
          thunk_EXT_FUN_1fffa3aa(DAT_00026190,param_2,*(float *)(iVar5 + 0x58),&local_180);
          thunk_EXT_FUN_1fffa30c(local_180,&local_17c,&local_178);
        }
        fVar21 = local_178 * fVar28;
        fVar19 = local_17c * fVar15;
        fVar15 = local_178 * fVar15 - local_17c * fVar28;
        if (iVar12 == 0x14) {
          iVar12 = 0;
          fVar28 = (float)*(undefined8 *)(iVar5 + 0x68);
          fVar27 = (float)((ulonglong)*(undefined8 *)(iVar5 + 0x68) >> 0x20);
          fVar25 = fVar28 * fVar25 + fVar15 * fVar27;
          fVar26 = fVar28 * fVar26 + *(float *)(iVar6 + 0x10) * DAT_00026150 * fVar27;
          *(float *)(iVar6 + 0x10) = DAT_00026194;
          uVar29 = FUN_00027ad8(fVar25 * fVar25);
          uVar30 = FUN_000270d8((int)uVar29,(int)((ulonglong)uVar29 >> 0x20),(int)uVar30,
                                (int)((ulonglong)uVar30 >> 0x20));
          uVar29 = FUN_00027ad8(fVar25 * fVar26);
          uVar24 = FUN_000270d8((int)uVar29,(int)((ulonglong)uVar29 >> 0x20),(int)uVar24,
                                (int)((ulonglong)uVar24 >> 0x20));
          uVar29 = FUN_00027ad8(fVar26 * fVar26);
          uVar23 = FUN_000270d8((int)uVar29,(int)((ulonglong)uVar29 >> 0x20),(int)uVar23,
                                (int)((ulonglong)uVar23 >> 0x20));
        }
        local_100 = -(fVar21 + fVar19);
        local_d8 = fVar16 - fVar15;
        thunk_EXT_FUN_1fff9e44(&local_108);
        thunk_EXT_FUN_1fff9e44(&local_e0);
        thunk_EXT_FUN_1fff9c40(local_178 * local_f0 - local_17c * local_c8);
        *(undefined1 *)(iVar4 + 0x46) = 1;
        *DAT_00026484 = 1;
        *DAT_00026488 = 1;
        uVar13 = uVar13 + 1;
      } while (uVar13 < DAT_0002648c);
      uVar13 = 0;
      do {
        do {
          fVar25 = DAT_00026194;
        } while ((*(byte *)(iVar4 + 0x44) & 1) == 0);
        local_174 = *(ushort *)(iVar4 + 0x50);
        local_170 = *(ushort *)(DAT_00026484 + 10);
        local_16c = *(ushort *)(iVar4 + 0x52);
        fVar26 = (float)VectorUnsignedToFloat((uint)local_16c,(byte)(uVar10 >> 0x16) & 3);
        *(float *)(iVar11 + 0x68) = *DAT_00026490 * fVar26;
        fVar26 = (float)VectorUnsignedToFloat((uint)local_174,(byte)(uVar10 >> 0x16) & 3);
        fVar16 = (*(float *)(iVar11 + 0x40) - fVar26) * fVar20;
        fVar26 = (float)VectorUnsignedToFloat((uint)local_170,(byte)(uVar10 >> 0x16) & 3);
        fVar26 = (fVar16 + (*(float *)(iVar11 + 0x44) - fVar26) * fVar20 * 2.0) * DAT_00026188;
        if (*(int *)(iVar6 + 4) != 0) {
          *(undefined4 *)(iVar6 + 4) = 0;
          fVar28 = (float)VectorUnsignedToFloat
                                    (*(undefined4 *)(iVar5 + 0x54),(byte)(uVar10 >> 0x16) & 3);
          uVar18 = VectorFloatToUnsigned(*(float *)(iVar6 + 8) * *(float *)(iVar5 + 0x58),3);
          fVar15 = (float)VectorUnsignedToFloat(uVar18,(byte)(uVar10 >> 0x16) & 3);
          local_180 = (*(float *)(iVar6 + 8) * fVar28 - fVar15 * fVar2) + *(float *)(iVar5 + 0x14);
          thunk_EXT_FUN_1fffa3aa(DAT_00026190,param_2,*(float *)(iVar5 + 0x58),&local_180);
          thunk_EXT_FUN_1fffa30c(local_180,&local_17c,&local_178);
        }
        local_100 = -(local_178 * fVar16 + local_17c * fVar26);
        local_d8 = fVar25 - (local_178 * fVar26 - local_17c * fVar16);
        thunk_EXT_FUN_1fff9e44(&local_108);
        thunk_EXT_FUN_1fff9e44(&local_e0);
        thunk_EXT_FUN_1fff9c40(local_178 * local_f0 - local_17c * local_c8);
        *(undefined1 *)(iVar4 + 0x46) = 1;
        *DAT_00026484 = 1;
        *DAT_00026488 = 1;
        uVar13 = uVar13 + 1;
      } while (uVar13 < 20000);
      delay_ms(1);
      thunk_EXT_FUN_1fff9c40(DAT_00026194);
      fVar25 = (float)FUN_0002400c(local_74);
      fVar26 = (float)FUN_00023fd0(fVar25 * fVar25 + 1.0);
      iVar12 = DAT_00026498;
      iVar9 = DAT_00026494;
      *(float *)(DAT_00026494 + 0x2c) = local_70 / fVar26;
      *(float *)(iVar9 + 0x30) = ((local_70 / fVar26) * fVar25) / (fVar14 * fVar2);
      *(undefined4 *)(iVar12 + 0xc) = 1;
      local_94 = 0x65;
      local_93 = *(undefined4 *)(iVar9 + 0x44);
      local_8f = *(undefined4 *)(iVar9 + 0x48);
      local_8b = *(undefined4 *)(iVar9 + 0x4c);
      local_87 = *(undefined4 *)(iVar9 + 0x2c);
      local_83 = *(undefined4 *)(iVar9 + 0x30);
      debug_uart_write(&local_94,0x15);
      pfVar3 = DAT_00026490;
      fVar25 = (float)VectorUnsignedToFloat
                                (*(undefined4 *)(iVar9 + 0x40),(byte)(uVar10 >> 0x16) & 3);
      DAT_00026490[0x10] = fVar25;
      pfVar3[0x11] = *(float *)(iVar9 + 0x44);
      pfVar3[0x12] = *(float *)(iVar9 + 0x48);
      pfVar3[0x13] = *(float *)(iVar9 + 0x4c);
      pfVar3[0x14] = *(float *)(iVar9 + 0x30);
      pfVar3[0x15] = *(float *)(iVar9 + 0x7c);
      derive_control_parameters();
      puVar7 = DAT_0002649c;
      DAT_0002649c[1] = *(undefined4 *)(iVar9 + 0x68);
      *puVar7 = *(undefined4 *)(iVar9 + 100);
      puVar7 = DAT_000264a0;
      *DAT_000264a0 = *(undefined4 *)(iVar9 + 0x6c);
      puVar7[1] = *(undefined4 *)(iVar9 + 0x70);
      *(undefined1 *)(iVar4 + 0x46) = 1;
      *DAT_00026484 = 1;
      *DAT_00026488 = 1;
      DAT_e000e280 = 4;
      DAT_e000e100 = 4;
      return;
    }
  }
  debug_printf("error!/r/n");
  return;
}



/* ===== detect_motor_direction_and_pole_pairs @ 000264a4, 516 bytes ===== */

void detect_motor_direction_and_pole_pairs(void)

{
  int iVar1;
  byte bVar2;
  int iVar3;
  int iVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  undefined4 uVar8;
  undefined4 uVar9;
  int iVar10;
  uint uVar11;
  uint in_fpscr;
  uint uVar12;
  undefined4 uVar13;
  float fVar14;
  float fVar15;
  float fVar16;
  float fVar17;
  float fVar18;
  undefined4 uVar19;
  float local_58;
  undefined1 local_54;
  undefined4 local_53;
  undefined1 local_4f;
  float local_4c;
  float local_48 [2];
  
  fVar14 = DAT_000266a8;
  DAT_e000e180 = 4;
  DataSynchronizationBarrier(0xf);
  InstructionSynchronizationBarrier(0xf);
  enforce_device_key_binding_or_halt();
  iVar3 = DAT_000266ac;
  uVar11 = 0;
  do {
    iVar1 = uVar11 * 4;
    uVar11 = uVar11 + 1;
    *(float *)(iVar3 + iVar1) = fVar14;
    iVar4 = DAT_000266b4;
    iVar1 = DAT_000266b0;
  } while (uVar11 < 0x100);
  uVar19 = 0x3f800000;
  *(undefined4 *)(DAT_000266b0 + 0x34) = 0x3f800000;
  fVar17 = *(float *)(iVar4 + 0x10);
  uVar11 = 0;
  do {
    thunk_EXT_FUN_1fff9c40(fVar17,fVar14);
    delay_us(0x32);
    iVar4 = DAT_000266d0;
    uVar9 = DAT_000266cc;
    uVar8 = DAT_000266c8;
    fVar7 = DAT_000266c4;
    fVar6 = DAT_000266c0;
    fVar5 = DAT_000266bc;
    iVar3 = DAT_000266b8;
    uVar11 = uVar11 + 1;
  } while (uVar11 < 10000);
  fVar18 = *(float *)(DAT_000266b8 + 8);
  fVar15 = fVar14;
  fVar16 = fVar14;
  while ((iVar10 = DAT_000266e0, (int)fVar15 < iVar4 && ((uint)fVar15 < (uint)DAT_000266dc))) {
    fVar16 = fVar16 + fVar5;
    uVar13 = VectorFloatToUnsigned(fVar16 * fVar6,3);
    fVar15 = (float)VectorUnsignedToFloat(uVar13,(byte)(in_fpscr >> 0x16) & 3);
    local_58 = fVar16 - fVar15 * fVar7;
    thunk_EXT_FUN_1fffa3aa(uVar9,uVar8,&local_58);
    thunk_EXT_FUN_1fffa30c(local_58,&local_4c,local_48);
    thunk_EXT_FUN_1fff9c40
              (local_48[0] * fVar17 - local_4c * fVar14,local_4c * fVar17 + local_48[0] * fVar14);
    delay_us(0x32);
    fVar15 = *(float *)(iVar3 + 8) - fVar18;
    if (DAT_000266d4 < (int)fVar15) {
      fVar15 = fVar15 - fVar7;
    }
    if ((uint)DAT_000266d8 < (uint)fVar15) {
      fVar15 = fVar15 + fVar7;
    }
  }
  uVar11 = in_fpscr & 0xfffffff | (uint)(fVar15 < 0.0) << 0x1f | (uint)(fVar15 == 0.0) << 0x1e;
  uVar12 = uVar11 | (uint)NAN(fVar15) << 0x1c;
  bVar2 = (byte)(uVar11 >> 0x18);
  if ((bool)(bVar2 >> 6 & 1) || bVar2 >> 7 != ((byte)(uVar12 >> 0x1c) & 1)) {
    uVar19 = 0x40000000;
  }
  do {
    fVar16 = fVar16 + fVar5;
    uVar13 = VectorFloatToUnsigned(fVar16 * fVar6,3);
    fVar15 = (float)VectorUnsignedToFloat(uVar13,(byte)(uVar12 >> 0x16) & 3);
    local_58 = fVar16 - fVar15 * fVar7;
    thunk_EXT_FUN_1fffa3aa(uVar9,uVar8,&local_58);
    thunk_EXT_FUN_1fffa30c(local_58,&local_4c,local_48);
    thunk_EXT_FUN_1fff9c40
              (local_48[0] * fVar17 - local_4c * fVar14,local_4c * fVar17 + local_48[0] * fVar14);
    delay_us(0x32);
  } while (iVar10 < (int)ABS(*(float *)(iVar3 + 8) - fVar18));
  thunk_EXT_FUN_1fff9c40(fVar14,fVar14);
  *(undefined4 *)(iVar1 + 0x34) = uVar19;
  local_53 = FUN_00023e94(fVar16 / fVar7);
  iVar3 = DAT_000266e4;
  *(undefined4 *)(iVar1 + 0x54) = local_53;
  fVar14 = (float)VectorUnsignedToFloat(local_53,(byte)(uVar12 >> 0x16) & 3);
  *(undefined4 *)(iVar3 + 0x40) = local_53;
  fVar17 = (float)VectorUnsignedToFloat(local_53,(byte)(uVar12 >> 0x16) & 3);
  *(float *)(iVar1 + 0x58) = fVar14 / fVar7;
  *(float *)(iVar1 + 0x60) = fVar17 * *(float *)(iVar3 + 0x50);
  local_54 = 99;
  local_4f = 0x55;
  debug_uart_write(&local_54,6);
  return;
}



/* ===== halt_on_bus_overvoltage @ 000266e8, 64 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void halt_on_bus_overvoltage(void)

{
  int iVar1;
  ushort *puVar2;
  ushort *puVar3;
  undefined4 uVar4;
  undefined8 uVar5;
  
  puVar3 = _DAT_00026730;
  puVar2 = DAT_0002672c;
  iVar1 = DAT_00026728;
  if (0x42000000 < *(int *)(DAT_00026728 + 0x68)) {
    do {
      *puVar2 = *puVar2 | 0x2000;
      *puVar3 = *puVar3 | 4;
      delay_ms(1000);
      uVar5 = FUN_00027ad8(*(undefined4 *)(iVar1 + 0x68));
      uVar4 = (undefined4)((ulonglong)uVar5 >> 0x20);
      debug_printf("V_BUS= %.4f Over Voltage!!!\r\n",uVar4,(int)uVar5,uVar4);
    } while( true );
  }
  return;
}



/* ===== debug_print_device_info @ 00026758, 676 bytes ===== */

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void debug_print_device_info(void)

{
  char cVar1;
  int iVar2;
  int iVar3;
  char *pcVar4;
  undefined4 extraout_r1;
  uint in_fpscr;
  float fVar5;
  float fVar6;
  undefined4 uVar7;
  float unaff_s16;
  undefined8 uVar8;
  
  if ((*(uint *)(DAT_00026a00 + 0x18) & 0x300) == 0) {
    fVar5 = (float)VectorUnsignedToFloat
                             (((*(uint *)(DAT_00026a00 + 0x1c) & 0xffffff) >> 0x10) + 1,
                              (byte)(in_fpscr >> 0x16) & 3);
    fVar6 = (float)VectorUnsignedToFloat
                             (((*(uint *)(DAT_00026a00 + 0x1c) & 0xffff) >> 8) +
                              (*(uint *)(DAT_00026a00 + 0x1c) & 0xff) + 3,
                              (byte)(in_fpscr >> 0x16) & 3);
    unaff_s16 = (_DAT_00026a04 / fVar5) / fVar6;
  }
  else if ((~*(uint *)(DAT_00026a00 + 0x18) & 0x300) == 0) {
    fVar5 = (float)VectorUnsignedToFloat
                             (((*(uint *)(DAT_00026a00 + 0xc) & 0x1fffff) >> 0x10) + 1,
                              (byte)(in_fpscr >> 0x16) & 3);
    fVar6 = (float)VectorUnsignedToFloat
                             (((*(uint *)(DAT_00026a00 + 0xc) & 0x1fff) >> 8) +
                              ((*(uint *)(DAT_00026a00 + 0xc) & 0xff) >> 4) + 3,
                              (byte)(in_fpscr >> 0x16) & 3);
    unaff_s16 = (_DAT_00026a04 / fVar5) / fVar6;
  }
  debug_printf("DMBOT Motor Driver");
  cVar1 = *DAT_00026a1c;
  if (cVar1 == '\0') {
    pcVar4 = "--V2.0";
  }
  else if (cVar1 == '\x01') {
    pcVar4 = "--V3.0";
  }
  else if (cVar1 == '\x02') {
    pcVar4 = "--V4.0";
  }
  else {
    if (cVar1 != '\x03') goto LAB_00026810;
    pcVar4 = "--V1.0";
  }
  debug_printf(pcVar4);
LAB_00026810:
  debug_printf("\n\r Debug Info:\n\r");
  debug_printf("Firmware Version: %d\r\n",0x1399);
  debug_printf("Sub Version: %03d\r\n",4);
  debug_printf("Imax: %f\r\n",extraout_r1,(int)DAT_00026a80,(int)((ulonglong)DAT_00026a80 >> 0x20));
  iVar2 = DAT_00026a94;
  uVar8 = FUN_00027ad8(*(undefined4 *)(DAT_00026a94 + 0x40));
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(" I_U Offset:     %.4f\r\n",uVar7,(int)uVar8,uVar7);
  uVar8 = FUN_00027ad8(*(undefined4 *)(iVar2 + 0x44));
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(" I_V Offset:     %.4f\r\n",uVar7,(int)uVar8,uVar7);
  uVar8 = FUN_00027ad8(*(undefined4 *)(iVar2 + 0x48));
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(" I_W Offset:     %.4f\r\n",uVar7,(int)uVar8,uVar7);
  uVar8 = FUN_00027ad8(*(undefined4 *)(iVar2 + -0x68));
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(" Position Sensor Electrical Offset:   %.4f\n\r",uVar7,(int)uVar8,uVar7);
  uVar8 = FUN_00027ad8(*(undefined4 *)(iVar2 + -0x58));
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(" Mechanical Offset:   %.4f\n\r",uVar7,(int)uVar8,uVar7);
  uVar8 = FUN_00027ad8(*(undefined4 *)(iVar2 + -100));
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(" Output Position:  %.4f\n\r",uVar7,(int)uVar8,uVar7);
  iVar3 = DAT_00026b4c;
  debug_printf(" CAN ID:     0x%03x\n\r",*(undefined4 *)(DAT_00026b4c + 0x20));
  debug_printf(" MASTER ID:  0x%03x\n\r",*(undefined4 *)(iVar3 + 0x1c));
  if ((int)unaff_s16 < _DAT_00026b80) {
    uVar7 = VectorFloatToUnsigned(unaff_s16,3);
    debug_printf(" CAN Baud: %dKbps\n\r",uVar7);
  }
  else {
    uVar8 = FUN_00027ad8(unaff_s16 * _DAT_00026b98);
    uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
    debug_printf(" CAN Baud: %.2fMbps\n\r",uVar7,(int)uVar8,uVar7);
  }
  debug_printf("\n\r Motor Info:\n\r");
  uVar8 = FUN_00027ad8(*(float *)(iVar3 + 0x44) * DAT_00026bc8);
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(&DAT_00026bcc,uVar7,(int)uVar8,uVar7);
  uVar8 = FUN_00027ad8(*(float *)(iVar3 + 0x48) * DAT_00026be0);
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(&DAT_00026be4,uVar7,(int)uVar8,uVar7);
  uVar8 = FUN_00027ad8(*(undefined4 *)(iVar3 + 0x4c));
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf(&DAT_00026bf8,uVar7,(int)uVar8,uVar7);
  uVar8 = FUN_00027ad8(*(undefined4 *)(iVar2 + 0x68));
  uVar7 = (undefined4)((ulonglong)uVar8 >> 0x20);
  debug_printf("V_BUS=%.4f\r\n",uVar7,(int)uVar8,uVar7);
  debug_printf("\n\r Control Mode : \r\n");
  if (*(int *)(iVar2 + 0x3c) == 1) {
    debug_printf("1:MIT Mode <----\n\r");
    debug_printf("2:position-speed cascade Mode\n\r");
    debug_printf("3:speed Mode\n\r");
    debug_printf("4:Hybrid control Mode\n\r");
  }
  if (*(int *)(iVar2 + 0x3c) == 2) {
    debug_printf("1:MIT Mode\n\r");
    debug_printf("2:position-speed cascade Mode <----\n\r");
    debug_printf("3:speed Mode\n\r");
    debug_printf("4:Hybrid control Mode\n\r");
  }
  if (*(int *)(iVar2 + 0x3c) == 3) {
    debug_printf("1:MIT Mode\n\r");
    debug_printf("2:position-speed cascade Mode\n\r");
    debug_printf("3:speed Mode <----\n\r");
    debug_printf("4:Hybrid control Mode\n\r");
  }
  if (*(int *)(iVar2 + 0x3c) != 4) {
    return;
  }
  debug_printf("1:MIT Mode\n\r");
  debug_printf("2:position-speed cascade Mode\n\r");
  debug_printf("3:speed Mode\n\r");
  debug_printf("4:Hybrid control Mode <----\n\r");
  return;
}



/* ===== handle_firmware_control_request @ 00026d00, 202 bytes ===== */

void handle_firmware_control_request(int param_1)

{
  int iVar1;
  undefined4 *puVar2;
  int *piVar3;
  undefined1 *puVar4;
  int *piVar5;
  int *piVar6;
  
  puVar4 = (undefined1 *)FUN_000203c8(0x82);
  if (param_1 == 1) {
    *puVar4 = 0x67;
    runtime_memcpy(puVar4 + 1,DAT_00026dcc,0x80);
    puVar4[0x81] = 0xaa;
    debug_uart_write(puVar4,0x82);
  }
  else {
    if (param_1 == 2) {
      disableIRQinterrupts();
      thunk_EXT_FUN_1fff9950(DAT_00026dd0,DAT_00026dcc,0x25);
      enableIRQinterrupts();
      *puVar4 = 0x55;
      puVar4[1] = 0x67;
      puVar4[2] = 0x55;
      debug_uart_write(puVar4,3);
      disableIRQinterrupts();
      delay_us(100);
      DataSynchronizationBarrier(0xf);
      *DAT_00026dd4 = *DAT_00026dd4 & 0x700 | DAT_00026dd8;
      DataSynchronizationBarrier(0xf);
      do {
                    /* WARNING: Do nothing block with infinite loop */
      } while( true );
    }
    if (param_1 == 6) {
      *puVar4 = 0x55;
      puVar4[1] = 0x67;
      iVar1 = DAT_00026dcc;
      puVar4[2] = 0x51;
      *(undefined4 *)(DAT_00026ddc + 0x38) = *(undefined4 *)(iVar1 + 0x60);
      derive_control_parameters();
      puVar2 = DAT_00026de0;
      *DAT_00026de0 = *(undefined4 *)(iVar1 + 100);
      puVar2[1] = *(undefined4 *)(iVar1 + 0x68);
      puVar2 = DAT_00026de4;
      *DAT_00026de4 = *(undefined4 *)(iVar1 + 0x6c);
      puVar2[1] = *(undefined4 *)(iVar1 + 0x70);
      debug_uart_write(puVar4,3);
    }
  }
  piVar6 = (int *)(puVar4 + -4);
  puVar2 = (undefined4 *)FUN_00020828();
  if (puVar4 == (undefined1 *)0x0) {
    return;
  }
  piVar3 = (int *)*puVar2;
  for (piVar5 = (int *)((int *)*puVar2)[1]; piVar5 != (int *)0x0 && piVar5 < piVar6;
      piVar5 = (int *)piVar5[1]) {
    piVar3 = piVar5;
  }
  if ((int *)(*piVar3 + (int)piVar3) == piVar6) {
    *piVar3 = *piVar3 + *piVar6;
  }
  else {
    piVar3[1] = (int)piVar6;
    piVar3 = piVar6;
  }
  if ((int *)(*piVar3 + (int)piVar3) == piVar5) {
    piVar3[1] = piVar5[1];
    *piVar3 = *piVar3 + *piVar5;
    return;
  }
  piVar3[1] = (int)piVar5;
  return;
}



/* ===== select_configuration_bank_a @ 00026de8, 34 bytes ===== */

void select_configuration_bank_a(void)

{
  undefined4 *puVar1;
  
  copy_persistent_configuration();
  puVar1 = DAT_00026e00;
  DAT_00026e00[1] = 1;
  *puVar1 = 0;
  (*(code *)&LAB_1fff8640)();
  return;
}



/* ===== select_configuration_bank_b @ 00026e04, 24 bytes ===== */

void select_configuration_bank_b(void)

{
  undefined4 *puVar1;
  
  copy_persistent_configuration();
  puVar1 = DAT_00026e1c;
  *DAT_00026e1c = 1;
  puVar1[1] = 0;
  (*(code *)&LAB_1fff8640)();
  return;
}



/* ===== FUN_00026e20 @ 00026e20, 110 bytes ===== */

undefined8 FUN_00026e20(int param_1,uint param_2)

{
  undefined8 uVar1;
  
  uVar1 = FUN_0002776c();
  if (((int)(0x7ff00000 - ((uint)((int)uVar1 != 0) | (uint)((ulonglong)uVar1 >> 0x20) & 0x7fffffff))
       < 0) && (-1 < (int)(0x7ff00000 - (param_2 & 0x7fffffff | (uint)(param_1 != 0))))) {
    FUN_000208d6(1);
  }
  return uVar1;
}



/* ===== store_hardware_variant @ 00026e90, 32 bytes ===== */

void store_hardware_variant(int param_1)

{
  if (*(int *)(DAT_00026eb0 + 0xc) == param_1 + -0x30303030) {
    return;
  }
  copy_persistent_configuration();
  *(int *)(DAT_00026eb4 + 0xc) = param_1 + -0x30303030;
  (*(code *)&LAB_1fff8640)();
  return;
}



/* ===== enforce_device_key_binding_or_halt @ 00026eb8, 288 bytes ===== */

void enforce_device_key_binding_or_halt(void)

{
  uint uVar1;
  undefined1 *puVar2;
  uint uVar3;
  uint uVar4;
  uint uVar5;
  undefined1 auStack_148 [240];
  byte local_58;
  byte local_57;
  byte local_56;
  byte local_55;
  byte local_54;
  uint local_53;
  byte local_4f;
  byte local_4e;
  byte local_4d;
  byte local_4c;
  undefined4 local_4b;
  undefined1 local_3c;
  undefined1 local_3b;
  undefined1 local_3a;
  undefined1 local_39;
  undefined1 auStack_38 [36];
  
  runtime_memcpy_aligned(auStack_38,&DAT_00026fd8,0x20);
  uVar1 = 0;
  puVar2 = DAT_00026ff8;
  do {
    *(undefined1 *)((int)&local_4b + uVar1 + 3) = *puVar2;
    uVar1 = uVar1 + 1 & 0xff;
    puVar2 = puVar2 + 1;
  } while (uVar1 < 0xc);
  local_3c = 1;
  local_3b = 2;
  local_3a = 3;
  local_39 = 4;
  FUN_00022570(auStack_148,auStack_38);
  FUN_00021c1c(auStack_148,&local_58,(int)&local_4b + 3,0x10);
  uVar4 = (uint)local_54 | local_53 << 8;
  uVar5 = (uint)local_55 << 0x18 | (uint)local_56 << 0x10 | (uint)local_57 << 8 | (uint)local_58;
  uVar3 = (uint)local_4c | local_4b << 8;
  uVar1 = (uint)local_4d << 0x18 | (uint)local_4e << 0x10 | (uint)local_4f << 8 | local_53 >> 0x18;
  if ((((*DAT_00026ffc != uVar5 || DAT_00026ffc[1] != uVar4) ||
       (DAT_00026ffc[2] != uVar1 || DAT_00026ffc[3] != uVar3)) &&
      ((DAT_00026ffc[0x10] != uVar5 || DAT_00026ffc[0x11] != uVar4 ||
       (DAT_00026ffc[0x12] != uVar1 || DAT_00026ffc[0x13] != uVar3)))) &&
     ((uVar5 != DAT_00026ffc[0x20] || uVar4 != DAT_00026ffc[0x21] ||
      (uVar1 != DAT_00026ffc[0x22] || uVar3 != DAT_00026ffc[0x23])))) {
    do {
      delay_ms(500);
      debug_printf("The key verification failed. This is a duplicate!\n");
    } while( true );
  }
  return;
}



/* ===== FUN_00027034 @ 00027034, 18 bytes ===== */

uint FUN_00027034(uint param_1)

{
  return ((param_1 >> 7) * 0x1b ^ param_1 << 1) & 0xff;
}



/* ===== FUN_00027048 @ 00027048, 34 bytes ===== */

undefined * FUN_00027048(undefined4 param_1,char *param_2)

{
  int iVar1;
  
  if (((param_2 != (char *)0x0) && (*param_2 != '\0')) &&
     (iVar1 = FUN_00021140(DAT_0002706c + 0x2705a), iVar1 != 0)) {
    return (undefined *)0x0;
  }
  return &UNK_0002706a + DAT_00027070;
}



/* ===== FUN_00027074 @ 00027074, 98 bytes ===== */

uint FUN_00027074(uint param_1,uint param_2)

{
  uint extraout_r1;
  uint uVar1;
  bool bVar2;
  
  uVar1 = (param_2 & 0x7fffffff) + 0xc8000000;
  bVar2 = uVar1 == 0x100000;
  if (0xfffff < uVar1) {
    bVar2 = uVar1 == 0xff00000;
  }
  if ((0xfffff >= uVar1 || 0xff00000 < uVar1) || bVar2) {
    if (0xfffff < (int)uVar1) {
      if (0xffdfffff < param_2 << 1) {
        FUN_000276ac(param_1);
        param_2 = extraout_r1;
      }
      return (param_2 >> 0x17 | 0xff) << 0x17;
    }
    return param_2 & 0x80000000;
  }
  bVar2 = (param_1 & 0x10000000) != 0;
  uVar1 = (param_2 & 0x80000000 | uVar1 * 8) + (param_1 >> 0x1d) + (uint)bVar2;
  if ((param_1 & 0xfffffff) == 0) {
    if (bVar2) {
      uVar1 = uVar1 & 0xfffffffe;
    }
    return uVar1;
  }
  return uVar1;
}



/* ===== FUN_000270d8 @ 000270d8, 316 bytes ===== */

/* WARNING: Control flow encountered bad instruction data */

ulonglong FUN_000270d8(uint param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  uint uVar10;
  bool bVar11;
  bool bVar12;
  byte bVar13;
  
  if ((int)(param_2 ^ param_4) < 0) {
    param_4 = param_4 ^ 0x80000000;
    bVar11 = param_3 <= param_1;
    uVar6 = param_1 - param_3;
    if (param_2 <= param_4 && (uint)bVar11 <= param_2 - param_4) {
      bVar12 = CARRY4(param_3,uVar6);
      param_3 = param_3 + uVar6;
      uVar7 = (param_2 - param_4) - (uint)!bVar11 ^ 0x80000000;
      param_4 = param_4 + uVar7 + bVar12;
      bVar11 = param_1 < uVar6;
      param_1 = param_1 - uVar6;
      param_2 = (param_2 - uVar7) - (uint)bVar11;
    }
    uVar7 = param_2 >> 0x14;
    uVar6 = uVar7 - (param_4 >> 0x14);
    if ((DAT_00027ad4 & param_4 << 1) == 0 || DAT_00027ad4 == uVar7 << 0x15) {
      if (DAT_00027ad4 != uVar7 << 0x15) {
        if ((param_2 & DAT_00027ad4 >> 1) == 0) {
          param_2 = 0;
          param_1 = 0;
        }
        return CONCAT44(param_2,param_1);
      }
      FUN_000276ac();
      do {
        software_interrupt(0x40);
      } while( true );
    }
    uVar5 = -param_3;
    uVar1 = param_2 & ~(uVar7 << 0x14);
    iVar8 = ((int)DAT_00027ad4 >> 1) - (param_4 & ~((int)DAT_00027ad4 >> 1));
    if (param_3 != 0) {
      iVar8 = iVar8 + -1;
    }
    if (uVar6 < 0x21) {
      uVar2 = uVar5 >> (uVar6 & 0xff);
      uVar9 = param_1 + uVar2;
      uVar1 = uVar1 + (iVar8 >> (uVar6 & 0xff)) + (uint)CARRY4(param_1,uVar2);
      uVar10 = iVar8 << (0x20 - uVar6 & 0xff);
      uVar2 = uVar9 + uVar10;
      bVar11 = CARRY4(uVar1,(uint)CARRY4(uVar9,uVar10));
      iVar8 = uVar1 + CARRY4(uVar9,uVar10);
      uVar6 = 0x20 - uVar6;
    }
    else {
      uVar5 = (uint)(param_3 * -2 != 0) | (iVar8 * 2 + (uint)CARRY4(uVar5,uVar5)) * 2;
      uVar6 = uVar6 - 0x20;
      if (0x1d < uVar6) {
        return CONCAT44(uVar1 + uVar7 * 0x100000,param_1);
      }
      uVar9 = iVar8 >> (uVar6 & 0xff);
      uVar2 = param_1 + uVar9;
      bVar11 = uVar1 != 0 || CARRY4(uVar1 - 1,(uint)CARRY4(param_1,uVar9));
      iVar8 = (uVar1 - 1) + (uint)CARRY4(param_1,uVar9);
      uVar6 = 0x1e - uVar6;
    }
    if (-1 < iVar8) {
      iVar4 = uVar5 << (uVar6 & 0xff);
      iVar8 = iVar8 + uVar7 * 0x100000;
      if (-1 < iVar4) {
        return CONCAT44(iVar8,uVar2);
      }
      uVar2 = uVar2 + 1;
      if (uVar2 != 0 && iVar4 != -0x80000000) {
        return CONCAT44(iVar8,uVar2);
      }
      if (uVar2 == 0) {
        iVar8 = iVar8 + 1;
        uVar2 = 0;
      }
      else {
        uVar2 = uVar2 & 0xfffffffe;
      }
      return CONCAT44(iVar8,uVar2);
    }
    uVar1 = uVar6 + 1 & 0xff;
    bVar11 = uVar1 == 0 && bVar11 || uVar1 != 0 && (uVar5 << uVar1 - 1 & 0x80000000) != 0;
    uVar9 = uVar2 * 2 + (uint)bVar11;
    uVar6 = iVar8 * 2 + (uint)(CARRY4(uVar2,uVar2) || CARRY4(uVar2 * 2,(uint)bVar11));
    uVar2 = uVar6 + uVar7 * 0x200000;
    bVar11 = (uVar2 >> 0x14 & 1) != 0;
    if (bVar11 && uVar2 >> 0x15 != 0) {
      uVar2 = -((int)(uVar5 << uVar1) >> 0x1f);
      uVar10 = uVar9 + uVar2;
      bVar11 = uVar10 == 0;
      iVar8 = uVar6 + uVar7 * 0x100000;
      if (!CARRY4(uVar9,uVar2)) {
        bVar11 = uVar5 << uVar1 == -0x80000000;
      }
      if (!bVar11) {
        return CONCAT44(iVar8,uVar10);
      }
      if (uVar10 == 0) {
        iVar8 = iVar8 + 1;
        uVar10 = 0;
      }
      else {
        uVar10 = uVar10 & 0xfffffffe;
      }
      return CONCAT44(iVar8,uVar10);
    }
    if (!bVar11) {
      iVar8 = uVar6 + 0x200000;
      if (iVar8 == 0) {
        uVar6 = uVar9 << LZCOUNT(uVar9);
        if (uVar6 == 0) {
          return (ulonglong)uVar9;
        }
        iVar4 = ((uVar7 & 0xfffff7ff) - LZCOUNT(uVar9)) + -0x17;
        iVar8 = uVar6 << 0x15;
        uVar6 = uVar6 >> 0xb;
      }
      else {
        uVar5 = LZCOUNT(iVar8) - 0xb;
        iVar4 = ((uVar7 & 0xfffff7ff) - uVar5) + -2;
        uVar6 = iVar8 << (uVar5 & 0xff) | uVar9 >> (0x20 - uVar5 & 0xff);
        iVar8 = uVar9 << (uVar5 & 0xff);
      }
      iVar3 = uVar6 + (param_2 & 0x80000000) + iVar4 * 0x100000;
      if (-1 < iVar4) {
        return CONCAT44(iVar3,iVar8);
      }
      return (ulonglong)(iVar3 + 0x60000000U & 0x80000000) << 0x20;
    }
    uVar7 = ((int)uVar6 >> 1) + uVar7 * 0x100000;
    uVar6 = (uint)((uVar6 & 1) != 0) << 0x1f | uVar9 >> 1;
    if (uVar7 * 2 == 0 && uVar6 == 0) {
      return (ulonglong)uVar6;
    }
    if (0x1fffff < uVar7 * 2) {
      return CONCAT44(uVar7,uVar6);
    }
    return (ulonglong)(uVar7 & 0x80000000) << 0x20;
  }
  uVar6 = param_1 - param_3;
  iVar8 = (param_2 - param_4) - (uint)(param_3 > param_1);
  if (param_2 <= param_4 && (uint)(param_3 <= param_1) <= param_2 - param_4) {
    bVar11 = CARRY4(param_3,uVar6);
    param_3 = param_3 + uVar6;
    param_4 = param_4 + iVar8 + (uint)bVar11;
    bVar11 = param_1 < uVar6;
    param_1 = param_1 - uVar6;
    param_2 = (param_2 - iVar8) - (uint)bVar11;
  }
  uVar7 = param_2 >> 0x14;
  uVar6 = uVar7 - (param_4 >> 0x14);
  if ((DAT_00027224 & param_4 << 1) == 0 || DAT_00027224 == uVar7 << 0x15) {
    if (DAT_00027224 != uVar7 << 0x15) {
      if ((param_2 & DAT_00027224 >> 1) == 0) {
        param_2 = param_2 & 0x80000000;
        param_1 = 0;
      }
      return CONCAT44(param_2,param_1);
    }
    FUN_000276ac();
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  param_2 = param_2 & ~(uVar7 << 0x14);
  uVar5 = param_4 & ~DAT_00027224 | 0x100000;
  if (uVar6 < 0x21) {
    uVar9 = param_3 >> (uVar6 & 0xff);
    uVar1 = param_1 + uVar9;
    uVar10 = uVar5 << (0x20 - uVar6 & 0xff);
    uVar2 = uVar1 + uVar10;
    param_2 = param_2 + (uVar5 >> (uVar6 & 0xff)) + (uint)CARRY4(param_1,uVar9) +
              (uint)CARRY4(uVar1,uVar10);
    uVar6 = 0x20 - uVar6;
    if (param_2 < 0x100000) {
      param_2 = param_2 + uVar7 * 0x100000;
      goto LAB_00027154;
    }
LAB_000271b6:
    bVar13 = (byte)uVar2 & 1;
    uVar5 = (uint)((param_2 + 0x100000 & 1) != 0) << 0x1f | uVar2 >> 1;
    param_2 = (param_2 + 0x100000 >> 1) + uVar7 * 0x100000;
    if ((uVar2 & 1) == 0) {
LAB_000271e4:
      if (param_2 * 2 < 0xffe00000) {
        return CONCAT44(param_2,uVar5);
      }
      goto LAB_00027748;
    }
    bVar11 = CARRY4(uVar5,(uint)bVar13);
    uVar5 = uVar5 + bVar13;
    uVar7 = uVar5;
    if (!bVar11) {
      uVar7 = param_3 << (uVar6 & 0xff);
    }
    if (uVar7 != 0) goto LAB_000271e4;
  }
  else {
    param_3 = uVar5 * 2 + (uint)(param_3 != 0);
    uVar1 = uVar6 - 0x20;
    uVar6 = 0x1f - uVar1;
    if (uVar1 < 0x20) {
      uVar5 = uVar5 >> (uVar1 & 0xff);
      uVar2 = param_1 + uVar5;
    }
    else {
      uVar6 = 0;
      uVar2 = param_1;
    }
    param_2 = param_2 + uVar7 * 0x100000 + (uint)(uVar1 < 0x20 && CARRY4(param_1,uVar5));
    if (uVar7 != param_2 >> 0x14) {
      param_2 = param_2 + uVar7 * -0x100000;
      goto LAB_000271b6;
    }
LAB_00027154:
    param_3 = param_3 << (uVar6 & 0xff);
    if (-1 < (int)param_3) {
      return CONCAT44(param_2,uVar2);
    }
    uVar5 = uVar2 + 1;
    uVar6 = uVar5;
    if (uVar2 != 0xffffffff) {
      uVar6 = param_3 & 0x7fffffff;
    }
    if (uVar6 != 0) {
      return CONCAT44(param_2,uVar5);
    }
  }
  if (uVar5 == 0) {
    param_2 = param_2 + 1;
    uVar5 = 0;
  }
  else {
    uVar5 = uVar5 & 0xfffffffe;
  }
  if (param_2 << 1 < 0xffe00000) {
    return CONCAT44(param_2,uVar5);
  }
LAB_00027748:
  return (ulonglong)((uint)((int)(param_2 + 0xa0000000) < 0) << 0x1f | 0x7ff00000) << 0x20;
}



/* ===== FUN_00027228 @ 00027228, 538 bytes ===== */

longlong FUN_00027228(uint param_1,uint param_2,uint param_3,uint param_4)

{
  longlong lVar1;
  ulonglong uVar2;
  uint uVar3;
  uint extraout_r3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  uint uVar7;
  int iVar8;
  uint uVar9;
  int iVar10;
  int iVar11;
  uint unaff_lr;
  uint uVar12;
  bool bVar13;
  
  if ((DAT_000274d0 & ~(param_2 >> 4)) == 0 || (DAT_000274d0 & ~(param_4 >> 4)) == 0) {
    FUN_000276ac();
    param_2 = extraout_r3;
  }
  else {
    uVar7 = param_2 ^ param_4;
    uVar4 = DAT_000274d0 & param_2 >> 4;
    bVar13 = uVar4 == 0;
    uVar6 = uVar7 >> 0x1f;
    if (!bVar13) {
      uVar7 = DAT_000274d0 & param_4 >> 4;
      bVar13 = uVar7 == 0;
    }
    if (bVar13) {
      if ((param_2 & DAT_000274d0 << 4) == 0) {
        if ((param_4 & DAT_000274d0 << 4) == 0) {
          return (ulonglong)DAT_000274d4 << 0x20;
        }
        return (ulonglong)((param_2 ^ param_4) & 0x80000000) << 0x20;
      }
      param_2 = param_2 ^ param_4;
    }
    else {
      iVar5 = (uVar4 | uVar6) - uVar7;
      uVar7 = param_4 << 0xb | 0x80000000;
      uVar4 = param_2 << 0xb | 0x80000000 | param_1 >> 0x15;
      uVar3 = uVar7 | param_3 >> 0x15;
      uVar6 = iVar5 + 0x3fe0000;
      if (uVar4 == uVar3 && param_1 * 0x800 == param_3 * 0x800) {
        uVar3 = 0x100000;
        uVar4 = 0;
      }
      else {
        uVar9 = (uint)*(byte *)((uVar7 >> 0x18) + 0x273d0);
        uVar9 = uVar9 * (0x1000000 - uVar9 * (uVar7 >> 0x10)) >> 0xf;
        iVar10 = (int)((ulonglong)uVar9 * (ulonglong)uVar3);
        iVar8 = -(int)((ulonglong)uVar9 * (ulonglong)uVar3 >> 0x20);
        iVar11 = iVar8 + 0x10000;
        if (iVar10 != 0) {
          iVar11 = iVar8 + 0xffff;
        }
        uVar9 = uVar9 * iVar11 + (int)((ulonglong)uVar9 * (ulonglong)(uint)-iVar10 >> 0x20);
        lVar1 = (ulonglong)uVar9 * (ulonglong)uVar3 +
                ((ulonglong)uVar9 * (ulonglong)(param_3 * 0x800) >> 0x20);
        iVar10 = (int)lVar1;
        iVar8 = -(int)((ulonglong)lVar1 >> 0x20);
        uVar3 = iVar8 + 0x80000000;
        if (iVar10 != 0) {
          uVar3 = iVar8 + 0x7fffffff;
        }
        uVar2 = (ulonglong)uVar9 * (ulonglong)uVar3 +
                ((ulonglong)uVar9 * (ulonglong)(uint)-iVar10 >> 0x20);
        uVar9 = (uint)(uVar2 >> 0x20);
        uVar3 = (uint)((ulonglong)uVar4 * (uVar2 & 0xffffffff) >> 0x20);
        uVar12 = (uint)((ulonglong)(param_1 * 0x800) * (ulonglong)uVar9 >> 0x20);
        uVar2 = (ulonglong)uVar4 * (ulonglong)uVar9 +
                (ulonglong)CONCAT14(CARRY4(uVar3,uVar12),uVar3 + uVar12);
        iVar8 = (int)(uVar2 >> 0x20);
        unaff_lr = iVar8 + 0x70000000;
        if (!SCARRY4(iVar8,0x70000000)) {
          uVar6 = iVar5 + 0x3fd0000;
          uVar2 = CONCAT44(iVar8 * 2 + (uint)((uVar2 & 0x80000000) != 0),(int)uVar2 << 1);
        }
        uVar3 = (uint)(uVar2 + 0x80 >> 0x20);
        uVar4 = (uint)(uVar2 + 0x80) >> 8 | uVar3 * 0x1000000;
        uVar3 = uVar3 >> 8;
        if ((int)uVar2 * 0x1000000 + 0x91000000U < 0x10000001) {
          uVar9 = param_3 & 0x1fffff | (param_3 >> 0x15) << 0x15;
          iVar5 = uVar3 * uVar9 +
                  uVar4 * (uVar7 >> 0xb) + (int)((ulonglong)uVar4 * (ulonglong)uVar9 >> 0x20);
          if (-1 < (int)unaff_lr) {
            iVar5 = iVar5 + param_1 * -0x100000;
          }
          unaff_lr = iVar5 + param_1 * -0x100000 +
                     (uVar7 >> 0xc) +
                     (uint)CARRY4((uint)((ulonglong)uVar4 * (ulonglong)uVar9),
                                  uVar9 >> 1 | (uVar7 >> 0xb) << 0x1f);
          if ((int)unaff_lr < 0) {
            bVar13 = 0xfffffffe < uVar4;
            uVar4 = uVar4 + 1;
            uVar3 = uVar3 + bVar13;
          }
        }
      }
      iVar5 = uVar3 + uVar6 * -0x80000000 + (uVar6 & 0xfffffffe) * 0x10;
      if ((uVar6 & 0xfffffffe) < 0x7f00001) {
        return CONCAT44(iVar5,uVar4);
      }
      if (-1 < (int)uVar6) {
        unaff_lr = iVar5 + 0x100000;
      }
      if (-1 < (int)uVar6 && -1 < (int)(unaff_lr ^ uVar6 << 0x1f)) {
        return CONCAT44(iVar5,uVar4);
      }
      if ((int)uVar6 < 0) {
        return (ulonglong)(iVar5 + 0x60000000U & 0x80000000) << 0x20;
      }
      param_2 = iVar5 + 0xa0000000;
    }
  }
  return (ulonglong)((uint)((int)param_2 < 0) << 0x1f | 0x7ff00000) << 0x20;
}



/* ===== FUN_000274d8 @ 000274d8, 88 bytes ===== */

uint FUN_000274d8(uint param_1,uint param_2)

{
  int iVar1;
  uint uVar2;
  undefined2 *unaff_r4;
  uint uVar3;
  bool bVar4;
  
  iVar1 = (param_2 >> 0x14) - 0x400;
  bVar4 = SBORROW4(0x1e,iVar1);
  uVar2 = -iVar1 + 0x1e;
  uVar3 = uVar2;
  if (iVar1 < 0x1f) {
    bVar4 = SBORROW4(0x20,uVar2);
    uVar3 = 0x20 - uVar2;
  }
  if ((int)uVar3 < 0 == bVar4) {
    return (param_2 << 0xb | 0x80000000 | param_1 >> 0x15) >> (uVar2 & 0xff);
  }
  if ((int)param_2 < 0) {
    return 0;
  }
  if (0x10 < (int)uVar2) {
    return 0;
  }
  if (-iVar1 != -0x3ff) {
    return 0xffffffff;
  }
  FUN_000276ac(param_1);
  do {
    *unaff_r4 = (short)unaff_r4;
  } while( true );
}



/* ===== FUN_00027532 @ 00027532, 38 bytes ===== */

undefined8 FUN_00027532(int param_1)

{
  uint uVar1;
  
  uVar1 = param_1 << LZCOUNT(param_1);
  if (uVar1 != 0) {
    return CONCAT44((0x41d - LZCOUNT(param_1)) * 0x100000 + (uVar1 >> 0xb),uVar1 << 0x15);
  }
  return 0;
}



/* ===== FUN_00027558 @ 00027558, 312 bytes ===== */

longlong FUN_00027558(uint param_1,uint param_2,uint param_3,uint param_4)

{
  longlong lVar1;
  longlong lVar2;
  longlong lVar3;
  longlong lVar4;
  longlong lVar5;
  uint uVar6;
  int extraout_r1;
  int iVar7;
  uint uVar8;
  uint extraout_r3;
  uint unaff_r5;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  uint uVar12;
  uint uVar13;
  int iVar14;
  int iVar15;
  bool bVar16;
  
  uVar13 = DAT_000276a4 & param_2 >> 4;
  bVar16 = uVar13 == 0;
  if (!bVar16) {
    unaff_r5 = DAT_000276a4 & param_4 >> 4;
    bVar16 = unaff_r5 == 0;
  }
  if (!bVar16) {
    bVar16 = uVar13 == DAT_000276a4;
  }
  if (!bVar16) {
    bVar16 = unaff_r5 == DAT_000276a4;
  }
  if (bVar16) {
    if (uVar13 != DAT_000276a4 && (DAT_000276a4 & param_4 >> 4) != DAT_000276a4) {
      return (ulonglong)((param_2 ^ param_4) & 0x80000000) << 0x20;
    }
    FUN_000276ac();
    uVar13 = extraout_r1 << 2 ^ extraout_r3;
  }
  else {
    uVar6 = param_2 & ~(DAT_000276a4 << 5) | 0x100000;
    uVar8 = param_4 & ~(DAT_000276a4 << 5) | 0x100000;
    iVar14 = (uVar13 | (param_2 ^ param_4) >> 0x1f) + unaff_r5;
    lVar1 = (ulonglong)uVar6 * (ulonglong)param_3;
    uVar11 = (uint)((ulonglong)lVar1 >> 0x20);
    lVar2 = (ulonglong)param_1 * (ulonglong)uVar8;
    uVar9 = (uint)((ulonglong)lVar2 >> 0x20);
    iVar15 = iVar14 + -0x3fc0000;
    lVar4 = lVar1 + lVar2;
    uVar10 = (uint)((ulonglong)lVar4 >> 0x20);
    lVar3 = (ulonglong)uVar6 * (ulonglong)uVar8;
    uVar13 = (uint)lVar3;
    uVar12 = (uint)((ulonglong)param_1 * (ulonglong)param_3 >> 0x20);
    lVar5 = lVar4 + CONCAT44(uVar13,uVar12);
    uVar8 = (uint)lVar5;
    uVar6 = (uint)((ulonglong)lVar5 >> 0x20);
    uVar13 = (int)((ulonglong)lVar3 >> 0x20) +
             (uint)(CARRY4(uVar9,uVar11) ||
                   CARRY4(uVar9 + uVar11,(uint)CARRY4((uint)lVar2,(uint)lVar1))) +
             (uint)(CARRY4(uVar10,uVar13) ||
                   CARRY4(uVar10 + uVar13,(uint)CARRY4((uint)lVar4,uVar12)));
    if ((int)((ulonglong)param_1 * (ulonglong)param_3) != 0) {
      uVar8 = uVar8 | 1;
    }
    if ((uVar13 & 0x200) == 0) {
      uVar9 = uVar8 << 0xc;
      uVar13 = uVar13 * 0x1000 | uVar6 >> 0x14;
      uVar6 = uVar6 << 0xc | uVar8 >> 0x14;
      iVar7 = -4;
    }
    else {
      uVar9 = uVar8 << 0xb;
      uVar13 = uVar13 * 0x800 | uVar6 >> 0x15;
      uVar6 = uVar6 << 0xb | uVar8 >> 0x15;
      iVar7 = -3;
    }
    uVar8 = iVar7 + (iVar15 >> 0x10);
    uVar13 = uVar13 + uVar8 * 0x100000 ^ iVar14 * -0x80000000;
    if (uVar9 != 0) {
      bVar16 = (uVar9 & 0x80000000) != 0;
      if ((uVar9 & 0x7fffffff) != 0) {
        uVar9 = 0;
      }
      uVar13 = uVar13 + CARRY4(uVar6,(uint)bVar16);
      uVar6 = uVar6 + bVar16 & ~(uVar9 >> 0x1f);
    }
    if (uVar8 < 0x7fe) {
      return CONCAT44(uVar13,uVar6);
    }
    if (iVar15 < 0x4000000) {
      return (ulonglong)(uVar13 + 0x60000000 & 0x80000000) << 0x20;
    }
    uVar13 = uVar13 + 0xa0000000;
  }
  return (ulonglong)((uint)((int)uVar13 < 0) << 0x1f | 0x7ff00000) << 0x20;
}



/* ===== FUN_000276ac @ 000276ac, 152 bytes ===== */

int FUN_000276ac(int param_1,uint param_2,int param_3,uint param_4)

{
  int iVar1;
  uint unaff_r5;
  uint uVar2;
  uint uVar3;
  int unaff_lr;
  uint *puVar4;
  bool bVar5;
  
  puVar4 = (uint *)(unaff_lr + 2U & 0xfffffffc);
  uVar2 = *puVar4;
  if ((((int)uVar2 < 0) || (unaff_r5 = param_4 * 2 + (uint)(param_3 != 0), unaff_r5 < 0xffe00001))
     && (uVar3 = param_2 * 2 + (uint)(param_1 != 0), uVar3 < 0xffe00001)) {
    if (uVar3 == 0xffe00000) {
      uVar3 = ((int)param_2 >> 0x1f) * -3 + 2;
      if (unaff_r5 == 0xffe00000) {
        uVar3 = uVar3 + (1 - ((int)param_4 >> 0x1f));
      }
    }
    else {
      uVar3 = param_4 >> 0x1f;
    }
  }
  else {
    uVar3 = 8;
  }
  uVar2 = uVar2 >> (uVar3 * 3 & 0xff) & 7;
  switch(uVar2) {
  case 4:
    param_1 = param_3;
    param_2 = param_4;
switchD_00027718_caseD_5:
    bVar5 = CARRY4(param_2,param_2) || CARRY4(param_2 * 2,(uint)(param_1 != 0));
    uVar2 = param_2 * 2 + (uint)(param_1 != 0);
    if (uVar2 != 0) {
      bVar5 = uVar2 < 0x200001;
    }
    if (bVar5 && (uVar2 != 0 && uVar2 != 0x200000)) {
      param_1 = 0;
    }
    return param_1;
  case 5:
    goto switchD_00027718_caseD_5;
  case 6:
  case 7:
    return 0;
  default:
                    /* WARNING: Could not recover jumptable at 0x00027716. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    iVar1 = (*(code *)(puVar4 + uVar2 + 1))();
    return iVar1;
  }
}



/* ===== FUN_00027754 @ 00027754, 22 bytes ===== */

/* WARNING: Control flow encountered bad instruction data */

ulonglong FUN_00027754(uint param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  uint uVar11;
  bool bVar12;
  bool bVar13;
  byte bVar14;
  
  uVar2 = param_2 ^ 0x80000000;
  if (-1 < (int)(param_2 ^ param_4)) {
    param_4 = param_4 ^ 0x80000000;
    bVar12 = param_3 <= param_1;
    uVar8 = param_1 - param_3;
    if (uVar2 <= param_4 && (uint)bVar12 <= uVar2 - param_4) {
      bVar13 = CARRY4(param_3,uVar8);
      param_3 = param_3 + uVar8;
      uVar9 = (uVar2 - param_4) - (uint)!bVar12 ^ 0x80000000;
      param_4 = param_4 + uVar9 + bVar13;
      bVar12 = param_1 < uVar8;
      param_1 = param_1 - uVar8;
      uVar2 = (uVar2 - uVar9) - (uint)bVar12;
    }
    uVar9 = uVar2 >> 0x14;
    uVar8 = uVar9 - (param_4 >> 0x14);
    if ((DAT_00027ad4 & param_4 << 1) == 0 || DAT_00027ad4 == uVar9 << 0x15) {
      if (DAT_00027ad4 != uVar9 << 0x15) {
        if ((uVar2 & DAT_00027ad4 >> 1) == 0) {
          uVar2 = 0;
          param_1 = 0;
        }
        return CONCAT44(uVar2,param_1);
      }
      FUN_000276ac();
      do {
        software_interrupt(0x40);
      } while( true );
    }
    uVar6 = -param_3;
    uVar3 = uVar2 & ~(uVar9 << 0x14);
    iVar7 = ((int)DAT_00027ad4 >> 1) - (param_4 & ~((int)DAT_00027ad4 >> 1));
    if (param_3 != 0) {
      iVar7 = iVar7 + -1;
    }
    if (uVar8 < 0x21) {
      uVar10 = uVar6 >> (uVar8 & 0xff);
      uVar1 = param_1 + uVar10;
      uVar3 = uVar3 + (iVar7 >> (uVar8 & 0xff)) + (uint)CARRY4(param_1,uVar10);
      uVar11 = iVar7 << (0x20 - uVar8 & 0xff);
      uVar10 = uVar1 + uVar11;
      bVar12 = CARRY4(uVar3,(uint)CARRY4(uVar1,uVar11));
      iVar7 = uVar3 + CARRY4(uVar1,uVar11);
      uVar8 = 0x20 - uVar8;
    }
    else {
      uVar6 = (uint)(param_3 * -2 != 0) | (iVar7 * 2 + (uint)CARRY4(uVar6,uVar6)) * 2;
      uVar8 = uVar8 - 0x20;
      if (0x1d < uVar8) {
        return CONCAT44(uVar3 + uVar9 * 0x100000,param_1);
      }
      uVar1 = iVar7 >> (uVar8 & 0xff);
      uVar10 = param_1 + uVar1;
      bVar12 = uVar3 != 0 || CARRY4(uVar3 - 1,(uint)CARRY4(param_1,uVar1));
      iVar7 = (uVar3 - 1) + (uint)CARRY4(param_1,uVar1);
      uVar8 = 0x1e - uVar8;
    }
    if (-1 < iVar7) {
      iVar5 = uVar6 << (uVar8 & 0xff);
      iVar7 = iVar7 + uVar9 * 0x100000;
      if (-1 < iVar5) {
        return CONCAT44(iVar7,uVar10);
      }
      uVar10 = uVar10 + 1;
      if (uVar10 != 0 && iVar5 != -0x80000000) {
        return CONCAT44(iVar7,uVar10);
      }
      if (uVar10 == 0) {
        iVar7 = iVar7 + 1;
        uVar10 = 0;
      }
      else {
        uVar10 = uVar10 & 0xfffffffe;
      }
      return CONCAT44(iVar7,uVar10);
    }
    uVar3 = uVar8 + 1 & 0xff;
    bVar12 = uVar3 == 0 && bVar12 || uVar3 != 0 && (uVar6 << uVar3 - 1 & 0x80000000) != 0;
    uVar1 = uVar10 * 2 + (uint)bVar12;
    uVar8 = iVar7 * 2 + (uint)(CARRY4(uVar10,uVar10) || CARRY4(uVar10 * 2,(uint)bVar12));
    uVar10 = uVar8 + uVar9 * 0x200000;
    bVar12 = (uVar10 >> 0x14 & 1) != 0;
    if (bVar12 && uVar10 >> 0x15 != 0) {
      uVar2 = -((int)(uVar6 << uVar3) >> 0x1f);
      uVar10 = uVar1 + uVar2;
      bVar12 = uVar10 == 0;
      iVar7 = uVar8 + uVar9 * 0x100000;
      if (!CARRY4(uVar1,uVar2)) {
        bVar12 = uVar6 << uVar3 == -0x80000000;
      }
      if (!bVar12) {
        return CONCAT44(iVar7,uVar10);
      }
      if (uVar10 == 0) {
        iVar7 = iVar7 + 1;
        uVar10 = 0;
      }
      else {
        uVar10 = uVar10 & 0xfffffffe;
      }
      return CONCAT44(iVar7,uVar10);
    }
    if (!bVar12) {
      iVar7 = uVar8 + 0x200000;
      if (iVar7 == 0) {
        uVar8 = uVar1 << LZCOUNT(uVar1);
        if (uVar8 == 0) {
          return (ulonglong)uVar1;
        }
        iVar5 = ((uVar9 & 0xfffff7ff) - LZCOUNT(uVar1)) + -0x17;
        iVar7 = uVar8 << 0x15;
        uVar8 = uVar8 >> 0xb;
      }
      else {
        uVar6 = LZCOUNT(iVar7) - 0xb;
        iVar5 = ((uVar9 & 0xfffff7ff) - uVar6) + -2;
        uVar8 = iVar7 << (uVar6 & 0xff) | uVar1 >> (0x20 - uVar6 & 0xff);
        iVar7 = uVar1 << (uVar6 & 0xff);
      }
      iVar4 = uVar8 + (uVar2 & 0x80000000) + iVar5 * 0x100000;
      if (-1 < iVar5) {
        return CONCAT44(iVar4,iVar7);
      }
      return (ulonglong)(iVar4 + 0x60000000U & 0x80000000) << 0x20;
    }
    uVar9 = ((int)uVar8 >> 1) + uVar9 * 0x100000;
    uVar2 = (uint)((uVar8 & 1) != 0) << 0x1f | uVar1 >> 1;
    if (uVar9 * 2 == 0 && uVar2 == 0) {
      return (ulonglong)uVar2;
    }
    if (0x1fffff < uVar9 * 2) {
      return CONCAT44(uVar9,uVar2);
    }
    return (ulonglong)(uVar9 & 0x80000000) << 0x20;
  }
  uVar8 = param_1 - param_3;
  iVar7 = (uVar2 - param_4) - (uint)(param_3 > param_1);
  if (uVar2 <= param_4 && (uint)(param_3 <= param_1) <= uVar2 - param_4) {
    bVar12 = CARRY4(param_3,uVar8);
    param_3 = param_3 + uVar8;
    param_4 = param_4 + iVar7 + (uint)bVar12;
    bVar12 = param_1 < uVar8;
    param_1 = param_1 - uVar8;
    uVar2 = (uVar2 - iVar7) - (uint)bVar12;
  }
  uVar9 = uVar2 >> 0x14;
  uVar8 = uVar9 - (param_4 >> 0x14);
  if ((DAT_00027224 & param_4 << 1) == 0 || DAT_00027224 == uVar9 << 0x15) {
    if (DAT_00027224 != uVar9 << 0x15) {
      if ((uVar2 & DAT_00027224 >> 1) == 0) {
        uVar2 = uVar2 & 0x80000000;
        param_1 = 0;
      }
      return CONCAT44(uVar2,param_1);
    }
    FUN_000276ac();
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  uVar2 = uVar2 & ~(uVar9 << 0x14);
  uVar6 = param_4 & ~DAT_00027224 | 0x100000;
  if (uVar8 < 0x21) {
    uVar1 = param_3 >> (uVar8 & 0xff);
    uVar3 = param_1 + uVar1;
    uVar11 = uVar6 << (0x20 - uVar8 & 0xff);
    uVar10 = uVar3 + uVar11;
    uVar2 = uVar2 + (uVar6 >> (uVar8 & 0xff)) + (uint)CARRY4(param_1,uVar1) +
            (uint)CARRY4(uVar3,uVar11);
    uVar8 = 0x20 - uVar8;
    if (uVar2 < 0x100000) {
      uVar2 = uVar2 + uVar9 * 0x100000;
      goto LAB_00027154;
    }
LAB_000271b6:
    bVar14 = (byte)uVar10 & 1;
    uVar6 = (uint)((uVar2 + 0x100000 & 1) != 0) << 0x1f | uVar10 >> 1;
    uVar2 = (uVar2 + 0x100000 >> 1) + uVar9 * 0x100000;
    if ((uVar10 & 1) == 0) {
LAB_000271e4:
      if (uVar2 * 2 < 0xffe00000) {
        return CONCAT44(uVar2,uVar6);
      }
      goto LAB_00027748;
    }
    bVar12 = CARRY4(uVar6,(uint)bVar14);
    uVar6 = uVar6 + bVar14;
    uVar9 = uVar6;
    if (!bVar12) {
      uVar9 = param_3 << (uVar8 & 0xff);
    }
    if (uVar9 != 0) goto LAB_000271e4;
  }
  else {
    param_3 = uVar6 * 2 + (uint)(param_3 != 0);
    uVar3 = uVar8 - 0x20;
    uVar8 = 0x1f - uVar3;
    if (uVar3 < 0x20) {
      uVar6 = uVar6 >> (uVar3 & 0xff);
      uVar10 = param_1 + uVar6;
    }
    else {
      uVar8 = 0;
      uVar10 = param_1;
    }
    uVar2 = uVar2 + uVar9 * 0x100000 + (uint)(uVar3 < 0x20 && CARRY4(param_1,uVar6));
    if (uVar9 != uVar2 >> 0x14) {
      uVar2 = uVar2 + uVar9 * -0x100000;
      goto LAB_000271b6;
    }
LAB_00027154:
    param_3 = param_3 << (uVar8 & 0xff);
    if (-1 < (int)param_3) {
      return CONCAT44(uVar2,uVar10);
    }
    uVar6 = uVar10 + 1;
    uVar8 = uVar6;
    if (uVar10 != 0xffffffff) {
      uVar8 = param_3 & 0x7fffffff;
    }
    if (uVar8 != 0) {
      return CONCAT44(uVar2,uVar6);
    }
  }
  if (uVar6 == 0) {
    uVar2 = uVar2 + 1;
    uVar6 = 0;
  }
  else {
    uVar6 = uVar6 & 0xfffffffe;
  }
  if (uVar2 << 1 < 0xffe00000) {
    return CONCAT44(uVar2,uVar6);
  }
LAB_00027748:
  return (ulonglong)((uint)((int)(uVar2 + 0xa0000000) < 0) << 0x1f | 0x7ff00000) << 0x20;
}



/* ===== FUN_0002776c @ 0002776c, 354 bytes ===== */

ulonglong FUN_0002776c(uint param_1,uint param_2)

{
  longlong lVar1;
  ulonglong uVar2;
  ulonglong uVar3;
  longlong lVar4;
  uint uVar5;
  int iVar6;
  int iVar7;
  int extraout_r2;
  uint unaff_r4;
  uint uVar8;
  uint uVar9;
  int iVar10;
  uint uVar11;
  uint uVar12;
  int extraout_r12;
  byte bVar13;
  char cVar14;
  bool bVar15;
  char cVar16;
  undefined8 uVar17;
  
  uVar11 = param_2 + 0x100000;
  cVar16 = SBORROW4(uVar11,0x200000);
  if (0x1fffff < (int)uVar11) {
    uVar12 = (param_2 >> 0x14) + 0xfd;
    uVar11 = param_2 & ~((param_2 >> 0x14) << 0x14) | 0x100000;
    if ((uVar12 & 1) != 0) {
      uVar5 = param_1 & 0x80000000;
      param_1 = param_1 << 1;
      uVar11 = uVar11 * 2 + (uint)(uVar5 != 0);
    }
    uVar5 = uVar11 << 10 | param_1 >> 0x16;
    uVar9 = param_1 << 10;
    uVar8 = (uint)*(byte *)(((uVar11 & 0x3fffff) >> 0x10) + 0x2785c);
    lVar1 = (ulonglong)uVar8 * (ulonglong)(0xc0000000 - ((uVar11 & 0x3fffff) >> 6) * uVar8 * uVar8);
    uVar8 = (uint)lVar1 >> 0x17 | (int)((ulonglong)lVar1 >> 0x20) << 9;
    lVar1 = (ulonglong)(uVar8 * uVar8) * (ulonglong)uVar5;
    iVar7 = (int)lVar1;
    iVar10 = -(int)((ulonglong)lVar1 >> 0x20);
    uVar11 = iVar10 + 0xc0000000;
    if (iVar7 != 0) {
      uVar11 = iVar10 + 0xbfffffff;
    }
    lVar1 = (ulonglong)uVar8 * (ulonglong)uVar11 +
            ((ulonglong)uVar8 * (ulonglong)(uint)-iVar7 >> 0x20);
    uVar8 = (uint)lVar1 >> 0xf | (int)((ulonglong)lVar1 >> 0x20) << 0x11;
    uVar11 = (uint)((ulonglong)uVar8 * (ulonglong)uVar8 >> 0x20);
    lVar1 = (ulonglong)uVar11 * (ulonglong)uVar5 + ((ulonglong)uVar11 * (ulonglong)uVar9 >> 0x20) +
            ((ulonglong)uVar5 * ((ulonglong)uVar8 * (ulonglong)uVar8 & 0xffffffff) >> 0x20);
    iVar7 = (int)lVar1;
    iVar10 = -(int)((ulonglong)lVar1 >> 0x20);
    uVar11 = iVar10 + 0xc0000000;
    if (iVar7 != 0) {
      uVar11 = iVar10 + 0xbfffffff;
    }
    uVar3 = (ulonglong)uVar8 * (ulonglong)uVar11 +
            ((ulonglong)uVar8 * (ulonglong)(uint)-iVar7 >> 0x20);
    uVar11 = (uint)(uVar3 >> 0x20);
    uVar8 = (uint)((ulonglong)uVar11 * (uVar3 & 0xffffffff) >> 0x20);
    uVar2 = (ulonglong)uVar11 * (ulonglong)uVar11 + (ulonglong)uVar8 + (ulonglong)uVar8;
    uVar8 = (uint)(uVar2 >> 0x20);
    lVar1 = (ulonglong)uVar8 * (ulonglong)uVar5 + ((ulonglong)uVar5 * (uVar2 & 0xffffffff) >> 0x20)
            + ((ulonglong)uVar8 * (ulonglong)uVar9 >> 0x20);
    iVar7 = (int)lVar1;
    iVar10 = -(int)((ulonglong)lVar1 >> 0x20);
    uVar8 = iVar10 + 0x30000000;
    if (iVar7 != 0) {
      uVar8 = iVar10 + 0x2fffffff;
    }
    uVar2 = (ulonglong)uVar11 * (ulonglong)uVar8 + ((uVar3 & 0xffffffff) * (ulonglong)uVar8 >> 0x20)
            + ((ulonglong)uVar11 * (ulonglong)(uint)-iVar7 >> 0x20);
    uVar11 = (uint)(uVar2 >> 0x20);
    lVar1 = (ulonglong)uVar5 * (ulonglong)uVar11 + ((ulonglong)uVar9 * (ulonglong)uVar11 >> 0x20) +
            ((ulonglong)uVar5 * (uVar2 & 0xffffffff) >> 0x20);
    lVar4 = lVar1 + 0x20;
    uVar11 = (uint)((ulonglong)lVar4 >> 0x20);
    uVar5 = uVar11 >> 6;
    uVar11 = (uint)lVar4 >> 6 | uVar11 * 0x4000000;
    if (((int)lVar1 - 0x1bU & 0x3f) < 0xb) {
      uVar8 = (uint)((ulonglong)uVar11 * (ulonglong)uVar11);
      iVar10 = (int)((ulonglong)uVar11 * (ulonglong)uVar11 >> 0x20) + uVar11 * uVar5 * 2 +
               param_1 * -0x100000;
      if (iVar10 < 0) {
        if ((int)(iVar10 + uVar5 + CARRY4(uVar8,uVar11)) < 0) {
          bVar15 = 0xfffffffe < uVar11;
          uVar11 = uVar11 + 1;
          uVar5 = uVar5 + bVar15;
        }
      }
      else if (-1 < (int)((iVar10 - uVar5) - (uint)(uVar8 < uVar11))) {
        bVar15 = uVar11 == 0;
        uVar11 = uVar11 - 1;
        uVar5 = uVar5 - bVar15;
      }
    }
    return CONCAT44(uVar5 + ((uVar12 >> 1) + 0x180) * 0x100000,uVar11);
  }
  bVar13 = ((uVar11 & 0x7fffffff) >> 0x14 & 1) != 0;
  cVar14 = '\0';
  if ((uVar11 & 0x7fffffff) >> 0x15 == 0) {
    if ((bool)bVar13) {
      return (ulonglong)(param_2 & 0x80000000) << 0x20;
    }
    uVar17 = FUN_000276ac();
    iVar10 = (int)((ulonglong)uVar17 >> 0x20);
    uVar11 = (uint)uVar17;
    if (cVar14 != cVar16) {
      iVar7 = iVar10 + -1 + (uint)bVar13;
      if (-1 < iVar7) {
        iVar10 = extraout_r2 << (-extraout_r12 + 0x1eU & 0xff);
        iVar7 = iVar7 + unaff_r4 * 0x100000;
        if (-1 < iVar10) {
          return CONCAT44(iVar7,uVar11);
        }
        uVar11 = uVar11 + 1;
        if (uVar11 == 0 || iVar10 == -0x80000000) {
          if (uVar11 == 0) {
            iVar7 = iVar7 + 1;
            uVar11 = 0;
          }
          else {
            uVar11 = uVar11 & 0xfffffffe;
          }
          return CONCAT44(iVar7,uVar11);
        }
        return CONCAT44(iVar7,uVar11);
      }
      uVar12 = -extraout_r12 + 0x1fU & 0xff;
      bVar15 = uVar12 == 0 && (iVar10 != 0 || CARRY4(iVar10 - 1,(uint)bVar13)) ||
               uVar12 != 0 && (extraout_r2 << uVar12 - 1 & 0x80000000U) != 0;
      uVar5 = uVar11 * 2 + (uint)bVar15;
      uVar11 = iVar7 * 2 + (uint)(CARRY4(uVar11,uVar11) || CARRY4(uVar11 * 2,(uint)bVar15));
      uVar8 = uVar11 + unaff_r4 * 0x200000;
      bVar15 = (uVar8 >> 0x14 & 1) != 0;
      if (bVar15 && uVar8 >> 0x15 != 0) {
        uVar8 = -((extraout_r2 << uVar12) >> 0x1f);
        uVar9 = uVar5 + uVar8;
        bVar15 = uVar9 == 0;
        iVar10 = uVar11 + unaff_r4 * 0x100000;
        if (!CARRY4(uVar5,uVar8)) {
          bVar15 = extraout_r2 << uVar12 == -0x80000000;
        }
        if (bVar15) {
          if (uVar9 == 0) {
            iVar10 = iVar10 + 1;
            uVar9 = 0;
          }
          else {
            uVar9 = uVar9 & 0xfffffffe;
          }
          return CONCAT44(iVar10,uVar9);
        }
        return CONCAT44(iVar10,uVar9);
      }
      if (!bVar15) {
        iVar10 = uVar11 + 0x200000;
        if (iVar10 == 0) {
          uVar11 = uVar5 << LZCOUNT(uVar5);
          if (uVar11 == 0) {
            return (ulonglong)uVar5;
          }
          iVar7 = ((unaff_r4 & 0xfffff7ff) - LZCOUNT(uVar5)) + -0x17;
          iVar10 = uVar11 << 0x15;
          uVar11 = uVar11 >> 0xb;
        }
        else {
          uVar12 = LZCOUNT(iVar10) - 0xb;
          iVar7 = ((unaff_r4 & 0xfffff7ff) - uVar12) + -2;
          uVar11 = iVar10 << (uVar12 & 0xff) | uVar5 >> (0x20 - uVar12 & 0xff);
          iVar10 = uVar5 << (uVar12 & 0xff);
        }
        iVar6 = uVar11 + (unaff_r4 >> 0xb) * -0x80000000 + iVar7 * 0x100000;
        if (iVar7 < 0) {
          return (ulonglong)(iVar6 + 0x60000000U & 0x80000000) << 0x20;
        }
        return CONCAT44(iVar6,iVar10);
      }
      uVar12 = ((int)uVar11 >> 1) + unaff_r4 * 0x100000;
      uVar11 = (uint)((uVar11 & 1) != 0) << 0x1f | uVar5 >> 1;
      if (uVar12 * 2 == 0 && uVar11 == 0) {
        return (ulonglong)uVar11;
      }
      if (uVar12 * 2 < 0x200000) {
        return (ulonglong)(uVar12 & 0x80000000) << 0x20;
      }
      return CONCAT44(uVar12,uVar11);
    }
    software_bkpt(0xff);
  }
  return (ulonglong)DAT_00027900 << 0x20;
}



/* ===== FUN_00027904 @ 00027904, 464 bytes ===== */

/* WARNING: Control flow encountered bad instruction data */

ulonglong FUN_00027904(uint param_1,uint param_2,uint param_3,uint param_4)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  int iVar4;
  uint uVar5;
  int iVar6;
  uint uVar7;
  uint uVar8;
  uint uVar9;
  uint uVar10;
  bool bVar11;
  bool bVar12;
  byte bVar13;
  
  if (-1 < (int)(param_2 ^ param_4)) {
    bVar11 = param_3 <= param_1;
    uVar7 = param_1 - param_3;
    if (param_2 <= param_4 && (uint)bVar11 <= param_2 - param_4) {
      bVar12 = CARRY4(param_3,uVar7);
      param_3 = param_3 + uVar7;
      uVar8 = (param_2 - param_4) - (uint)!bVar11 ^ 0x80000000;
      param_4 = param_4 + uVar8 + bVar12;
      bVar11 = param_1 < uVar7;
      param_1 = param_1 - uVar7;
      param_2 = (param_2 - uVar8) - (uint)bVar11;
    }
    uVar8 = param_2 >> 0x14;
    uVar7 = uVar8 - (param_4 >> 0x14);
    if ((DAT_00027ad4 & param_4 << 1) == 0 || DAT_00027ad4 == uVar8 << 0x15) {
      if (DAT_00027ad4 != uVar8 << 0x15) {
        if ((param_2 & DAT_00027ad4 >> 1) == 0) {
          param_2 = 0;
          param_1 = 0;
        }
        return CONCAT44(param_2,param_1);
      }
      FUN_000276ac();
      do {
        software_interrupt(0x40);
      } while( true );
    }
    uVar5 = -param_3;
    uVar2 = param_2 & ~(uVar8 << 0x14);
    iVar6 = ((int)DAT_00027ad4 >> 1) - (param_4 & ~((int)DAT_00027ad4 >> 1));
    if (param_3 != 0) {
      iVar6 = iVar6 + -1;
    }
    if (uVar7 < 0x21) {
      uVar9 = uVar5 >> (uVar7 & 0xff);
      uVar1 = param_1 + uVar9;
      uVar2 = uVar2 + (iVar6 >> (uVar7 & 0xff)) + (uint)CARRY4(param_1,uVar9);
      uVar10 = iVar6 << (0x20 - uVar7 & 0xff);
      uVar9 = uVar1 + uVar10;
      bVar11 = CARRY4(uVar2,(uint)CARRY4(uVar1,uVar10));
      iVar6 = uVar2 + CARRY4(uVar1,uVar10);
      uVar7 = 0x20 - uVar7;
    }
    else {
      uVar5 = (uint)(param_3 * -2 != 0) | (iVar6 * 2 + (uint)CARRY4(uVar5,uVar5)) * 2;
      uVar7 = uVar7 - 0x20;
      if (0x1d < uVar7) {
        return CONCAT44(uVar2 + uVar8 * 0x100000,param_1);
      }
      uVar1 = iVar6 >> (uVar7 & 0xff);
      uVar9 = param_1 + uVar1;
      bVar11 = uVar2 != 0 || CARRY4(uVar2 - 1,(uint)CARRY4(param_1,uVar1));
      iVar6 = (uVar2 - 1) + (uint)CARRY4(param_1,uVar1);
      uVar7 = 0x1e - uVar7;
    }
    if (-1 < iVar6) {
      iVar4 = uVar5 << (uVar7 & 0xff);
      iVar6 = iVar6 + uVar8 * 0x100000;
      if (-1 < iVar4) {
        return CONCAT44(iVar6,uVar9);
      }
      uVar9 = uVar9 + 1;
      if (uVar9 != 0 && iVar4 != -0x80000000) {
        return CONCAT44(iVar6,uVar9);
      }
      if (uVar9 == 0) {
        iVar6 = iVar6 + 1;
        uVar9 = 0;
      }
      else {
        uVar9 = uVar9 & 0xfffffffe;
      }
      return CONCAT44(iVar6,uVar9);
    }
    uVar2 = uVar7 + 1 & 0xff;
    bVar11 = uVar2 == 0 && bVar11 || uVar2 != 0 && (uVar5 << uVar2 - 1 & 0x80000000) != 0;
    uVar1 = uVar9 * 2 + (uint)bVar11;
    uVar7 = iVar6 * 2 + (uint)(CARRY4(uVar9,uVar9) || CARRY4(uVar9 * 2,(uint)bVar11));
    uVar9 = uVar7 + uVar8 * 0x200000;
    bVar11 = (uVar9 >> 0x14 & 1) != 0;
    if (bVar11 && uVar9 >> 0x15 != 0) {
      uVar9 = -((int)(uVar5 << uVar2) >> 0x1f);
      uVar10 = uVar1 + uVar9;
      bVar11 = uVar10 == 0;
      iVar6 = uVar7 + uVar8 * 0x100000;
      if (!CARRY4(uVar1,uVar9)) {
        bVar11 = uVar5 << uVar2 == -0x80000000;
      }
      if (!bVar11) {
        return CONCAT44(iVar6,uVar10);
      }
      if (uVar10 == 0) {
        iVar6 = iVar6 + 1;
        uVar10 = 0;
      }
      else {
        uVar10 = uVar10 & 0xfffffffe;
      }
      return CONCAT44(iVar6,uVar10);
    }
    if (!bVar11) {
      iVar6 = uVar7 + 0x200000;
      if (iVar6 == 0) {
        uVar7 = uVar1 << LZCOUNT(uVar1);
        if (uVar7 == 0) {
          return (ulonglong)uVar1;
        }
        iVar4 = ((uVar8 & 0xfffff7ff) - LZCOUNT(uVar1)) + -0x17;
        iVar6 = uVar7 << 0x15;
        uVar7 = uVar7 >> 0xb;
      }
      else {
        uVar5 = LZCOUNT(iVar6) - 0xb;
        iVar4 = ((uVar8 & 0xfffff7ff) - uVar5) + -2;
        uVar7 = iVar6 << (uVar5 & 0xff) | uVar1 >> (0x20 - uVar5 & 0xff);
        iVar6 = uVar1 << (uVar5 & 0xff);
      }
      iVar3 = uVar7 + (param_2 & 0x80000000) + iVar4 * 0x100000;
      if (-1 < iVar4) {
        return CONCAT44(iVar3,iVar6);
      }
      return (ulonglong)(iVar3 + 0x60000000U & 0x80000000) << 0x20;
    }
    uVar8 = ((int)uVar7 >> 1) + uVar8 * 0x100000;
    uVar7 = (uint)((uVar7 & 1) != 0) << 0x1f | uVar1 >> 1;
    if (uVar8 * 2 == 0 && uVar7 == 0) {
      return (ulonglong)uVar7;
    }
    if (0x1fffff < uVar8 * 2) {
      return CONCAT44(uVar8,uVar7);
    }
    return (ulonglong)(uVar8 & 0x80000000) << 0x20;
  }
  param_4 = param_4 ^ 0x80000000;
  uVar7 = param_1 - param_3;
  iVar6 = (param_2 - param_4) - (uint)(param_3 > param_1);
  if (param_2 <= param_4 && (uint)(param_3 <= param_1) <= param_2 - param_4) {
    bVar11 = CARRY4(param_3,uVar7);
    param_3 = param_3 + uVar7;
    param_4 = param_4 + iVar6 + (uint)bVar11;
    bVar11 = param_1 < uVar7;
    param_1 = param_1 - uVar7;
    param_2 = (param_2 - iVar6) - (uint)bVar11;
  }
  uVar8 = param_2 >> 0x14;
  uVar7 = uVar8 - (param_4 >> 0x14);
  if ((DAT_00027224 & param_4 << 1) == 0 || DAT_00027224 == uVar8 << 0x15) {
    if (DAT_00027224 != uVar8 << 0x15) {
      if ((param_2 & DAT_00027224 >> 1) == 0) {
        param_2 = param_2 & 0x80000000;
        param_1 = 0;
      }
      return CONCAT44(param_2,param_1);
    }
    FUN_000276ac();
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  param_2 = param_2 & ~(uVar8 << 0x14);
  uVar5 = param_4 & ~DAT_00027224 | 0x100000;
  if (uVar7 < 0x21) {
    uVar1 = param_3 >> (uVar7 & 0xff);
    uVar2 = param_1 + uVar1;
    uVar10 = uVar5 << (0x20 - uVar7 & 0xff);
    uVar9 = uVar2 + uVar10;
    param_2 = param_2 + (uVar5 >> (uVar7 & 0xff)) + (uint)CARRY4(param_1,uVar1) +
              (uint)CARRY4(uVar2,uVar10);
    uVar7 = 0x20 - uVar7;
    if (param_2 < 0x100000) {
      param_2 = param_2 + uVar8 * 0x100000;
      goto LAB_00027154;
    }
LAB_000271b6:
    bVar13 = (byte)uVar9 & 1;
    uVar5 = (uint)((param_2 + 0x100000 & 1) != 0) << 0x1f | uVar9 >> 1;
    param_2 = (param_2 + 0x100000 >> 1) + uVar8 * 0x100000;
    if ((uVar9 & 1) == 0) {
LAB_000271e4:
      if (param_2 * 2 < 0xffe00000) {
        return CONCAT44(param_2,uVar5);
      }
      goto LAB_00027748;
    }
    bVar11 = CARRY4(uVar5,(uint)bVar13);
    uVar5 = uVar5 + bVar13;
    uVar8 = uVar5;
    if (!bVar11) {
      uVar8 = param_3 << (uVar7 & 0xff);
    }
    if (uVar8 != 0) goto LAB_000271e4;
  }
  else {
    param_3 = uVar5 * 2 + (uint)(param_3 != 0);
    uVar2 = uVar7 - 0x20;
    uVar7 = 0x1f - uVar2;
    if (uVar2 < 0x20) {
      uVar5 = uVar5 >> (uVar2 & 0xff);
      uVar9 = param_1 + uVar5;
    }
    else {
      uVar7 = 0;
      uVar9 = param_1;
    }
    param_2 = param_2 + uVar8 * 0x100000 + (uint)(uVar2 < 0x20 && CARRY4(param_1,uVar5));
    if (uVar8 != param_2 >> 0x14) {
      param_2 = param_2 + uVar8 * -0x100000;
      goto LAB_000271b6;
    }
LAB_00027154:
    param_3 = param_3 << (uVar7 & 0xff);
    if (-1 < (int)param_3) {
      return CONCAT44(param_2,uVar9);
    }
    uVar5 = uVar9 + 1;
    uVar7 = uVar5;
    if (uVar9 != 0xffffffff) {
      uVar7 = param_3 & 0x7fffffff;
    }
    if (uVar7 != 0) {
      return CONCAT44(param_2,uVar5);
    }
  }
  if (uVar5 == 0) {
    param_2 = param_2 + 1;
    uVar5 = 0;
  }
  else {
    uVar5 = uVar5 & 0xfffffffe;
  }
  if (param_2 << 1 < 0xffe00000) {
    return CONCAT44(param_2,uVar5);
  }
LAB_00027748:
  return (ulonglong)((uint)((int)(param_2 + 0xa0000000) < 0) << 0x1f | 0x7ff00000) << 0x20;
}



/* ===== FUN_00027ad8 @ 00027ad8, 72 bytes ===== */

longlong FUN_00027ad8(uint param_1,int param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  
  uVar1 = (uint)((param_1 & 0x80000000) != 0) << 0x1f;
  uVar2 = (param_1 & 0x7fffffff) >> 3;
  uVar4 = param_1 << 1 ^ param_1;
  if (uVar4 != 0) {
    param_1 = param_1 << 0x1d;
    param_2 = (uVar1 | uVar2) + 0x38000000;
  }
  if (uVar4 == 0 || (uVar4 & 0x7f000000) == 0) {
    if ((uVar2 & 0x8000000) != 0) {
      iVar3 = FUN_00027b9c(uVar1 | param_1 >> 0x1d | uVar2 << 3,param_2);
      return (ulonglong)((uint)(iVar3 < 0) << 0x1f | 0x7ff00000) << 0x20;
    }
    return (ulonglong)uVar1 << 0x20;
  }
  return CONCAT44(param_2,param_1);
}



/* ===== FUN_00027b3c @ 00027b3c, 76 bytes ===== */

uint FUN_00027b3c(uint param_1,uint param_2)

{
  uint uVar1;
  uint uVar2;
  int iVar3;
  uint uVar4;
  
  if (param_2 != 0) {
    iVar3 = 0x20;
    uVar1 = param_2;
  }
  else {
    iVar3 = 0;
    uVar1 = param_1;
  }
  if (param_2 != 0 || param_1 != 0) {
    uVar4 = iVar3 - LZCOUNT(uVar1);
    uVar2 = uVar1 << LZCOUNT(uVar1) | param_1 >> (uVar4 & 0xff);
    uVar1 = uVar2 >> 7;
    uVar2 = uVar4 * 0x800000 + 0x4e800000 + (uVar2 >> 8) + (uint)((byte)uVar1 & 1);
    if ((uVar1 & 1) != 0) {
      uVar2 = uVar2 & 0xfffffffe;
    }
    return uVar2;
  }
  return param_1;
}



/* ===== FUN_00027b9c @ 00027b9c, 136 bytes ===== */

uint FUN_00027b9c(uint param_1,uint param_2,uint param_3)

{
  uint uVar1;
  uint uVar2;
  int unaff_lr;
  uint *puVar3;
  bool bVar4;
  
  puVar3 = (uint *)(unaff_lr + 2U & 0xfffffffc);
  uVar1 = *puVar3;
  if ((((int)uVar1 < 0) || (param_3 = param_2 * 2, param_3 < 0xff000001)) &&
     (param_1 * 2 < 0xff000001)) {
    if (param_1 * 2 == 0xff000000) {
      uVar2 = ((int)param_1 >> 0x1f) * -3 + 2;
      if (param_3 == 0xff000000) {
        uVar2 = uVar2 + (1 - ((int)param_2 >> 0x1f));
      }
    }
    else {
      uVar2 = param_2 >> 0x1f;
    }
  }
  else {
    uVar2 = 8;
  }
  uVar1 = uVar1 >> (uVar2 * 3 & 0xff) & 7;
  switch(uVar1) {
  case 4:
    param_1 = param_2;
switchD_00027c04_caseD_5:
    bVar4 = (param_1 & 0x80000000) != 0;
    uVar1 = param_1 * 2;
    if (uVar1 != 0) {
      bVar4 = uVar1 < 0x1000001;
    }
    if (bVar4 && (uVar1 != 0 && uVar1 != 0x1000000)) {
      param_1 = param_1 & 0x80000000;
    }
    return param_1;
  case 5:
    goto switchD_00027c04_caseD_5;
  case 6:
  case 7:
    return 0x7fc00000;
  default:
                    /* WARNING: Could not recover jumptable at 0x00027c02. Too many branches */
                    /* WARNING: Treating indirect jump as call */
    uVar1 = (*(code *)(puVar3 + uVar1 + 1))();
    return uVar1;
  }
}



/* ===== FUN_00027c28 @ 00027c28, 10 bytes ===== */

undefined4 FUN_00027c28(void)

{
  return 0x3000000;
}



/* ===== FUN_00027c3c @ 00027c3c, 100 bytes ===== */

/* WARNING: Control flow encountered bad instruction data */

uint FUN_00027c3c(uint param_1)

{
  undefined1 uVar1;
  int iVar2;
  uint *puVar3;
  undefined4 *puVar4;
  int extraout_r2;
  int iVar5;
  uint unaff_r5;
  int unaff_r6;
  char *pcVar6;
  int unaff_r7;
  char *pcVar7;
  int unaff_r10;
  uint unaff_r11;
  uint uVar8;
  uint uVar9;
  char cVar10;
  bool bVar11;
  char cVar12;
  undefined8 uVar13;
  undefined1 auStack_6c [32];
  int iStack_4c;
  undefined4 uStack_48;
  int iStack_44;
  int iStack_40;
  uint uStack_3c;
  int iStack_38;
  char acStack_2f [3];
  
  uVar8 = (param_1 & 0x7fffffff) >> 0x17;
  iVar5 = uVar8 - 0x7f;
  if (iVar5 < 0) {
    bVar11 = iVar5 == -1;
    if (iVar5 == -1) {
      bVar11 = (param_1 & 0xffffff) == 0;
    }
    uVar8 = param_1 & 0x80000000;
    if ((iVar5 == -1 && (param_1 & 0x1000000) != 0) && !bVar11) {
      uVar8 = uVar8 | 0x3f800000;
    }
    return uVar8;
  }
  if (0x17U - iVar5 != 0 && iVar5 < 0x18) {
    uVar9 = 1 << (0x17U - iVar5 & 0xff);
    uVar8 = uVar9 - 1 & param_1;
    param_1 = param_1 & ~uVar8;
    if (uVar9 >> 1 <= uVar8 && (uVar8 != uVar9 >> 1 || (param_1 & uVar9) != 0)) {
      param_1 = param_1 + uVar9;
    }
    return param_1;
  }
  cVar12 = SBORROW4(uVar8,0xff);
  cVar10 = (int)(uVar8 - 0xff) < 0;
  if (uVar8 < 0xff) {
    return param_1;
  }
  uVar13 = FUN_00027b9c();
  iVar5 = (int)((ulonglong)uVar13 >> 0x20);
  puVar3 = (uint *)uVar13;
  if (cVar10 != cVar12) {
                    /* WARNING: Bad instruction - Truncating control flow here */
    halt_baddata();
  }
  software_bkpt(0xb6);
  puVar4 = (undefined4 *)(extraout_r2 + 7U & 0xfffffff8);
  uStack_48 = *puVar4;
  iStack_44 = puVar4[1];
  iVar2 = FUN_000237c0();
  if (iStack_44 < 0) {
    iStack_4c = 0x2d;
  }
  else if ((int)(*puVar3 << 0x1e) < 0) {
    iStack_4c = 0x2b;
  }
  else {
    iStack_4c = (*puVar3 & 4) << 3;
  }
  if ((iVar2 == 3) || (6 < iVar2)) {
    FUN_00020f0c(puVar3,iVar5,iVar2,iStack_4c);
    return 3;
  }
  if ((int)((uint)(byte)*puVar3 << 0x1a) < 0) {
    uVar8 = puVar3[7];
  }
  else {
    uVar8 = 6;
  }
  if (iVar5 == 0x65) {
    if ((int)uVar8 < 0x11) {
      iVar5 = uVar8 + 1;
    }
    else {
      iVar5 = 0x11;
    }
    FUN_00020996(&iStack_40,auStack_6c,&uStack_48,iVar5,0);
    unaff_r5 = uVar8 + 1;
    unaff_r7 = iStack_40;
  }
  else {
    if (iVar5 == 0x66) {
      unaff_r7 = -0x80000000;
      FUN_00020996(&iStack_40,auStack_6c,&uStack_48,uVar8,1);
      unaff_r6 = 0;
      unaff_r5 = uStack_3c;
      if (iStack_38 == 0) {
        unaff_r5 = iStack_40 + uVar8 + 1;
      }
      if (-1 < (int)(uVar8 - unaff_r5)) {
        unaff_r6 = -1 - (uVar8 - unaff_r5);
        unaff_r5 = uVar8 + 1;
      }
      unaff_r10 = unaff_r5 - uVar8;
      unaff_r11 = uStack_3c;
      goto LAB_00020c96;
    }
    if (iVar5 != 0x67) goto LAB_00020c96;
    if ((int)uVar8 < 1) {
      uVar8 = 1;
    }
    uVar9 = uVar8;
    if (0x11 < (int)uVar8) {
      uVar9 = 0x11;
    }
    FUN_00020996(&iStack_40,auStack_6c,&uStack_48,uVar9,0);
    unaff_r6 = 0;
    unaff_r5 = uVar8;
    if (-1 < (int)((uint)(byte)*puVar3 << 0x1c)) {
      if ((int)uStack_3c < (int)uVar8) {
        unaff_r5 = uStack_3c;
      }
      for (; (1 < (int)unaff_r5 && (auStack_6c[unaff_r5 - 1] == '0')); unaff_r5 = unaff_r5 - 1) {
      }
    }
    unaff_r7 = iStack_40;
    if ((iStack_40 < (int)uVar8) && (-5 < iStack_40)) {
      if (iStack_40 < 1) {
        unaff_r5 = unaff_r5 - iStack_40;
        unaff_r6 = iStack_40;
      }
      else if ((int)unaff_r5 < iStack_40 + 1) {
        unaff_r5 = iStack_40 + 1;
      }
      unaff_r10 = (iStack_40 - unaff_r6) + 1;
      unaff_r7 = -0x80000000;
      unaff_r11 = uStack_3c;
      goto LAB_00020c96;
    }
  }
  unaff_r6 = 0;
  unaff_r10 = 1;
  unaff_r11 = uStack_3c;
LAB_00020c96:
  if ((-1 < (int)((uint)(byte)*puVar3 << 0x1c)) && ((int)unaff_r5 <= unaff_r10)) {
    unaff_r10 = -1;
  }
  pcVar7 = acStack_2f + 2;
  acStack_2f[2] = 0;
  if (unaff_r7 != -0x80000000) {
    iVar5 = 2;
    cVar10 = '+';
    pcVar6 = pcVar7;
    if (unaff_r7 < 0) {
      unaff_r7 = -unaff_r7;
      cVar10 = '-';
    }
    for (; (0 < iVar5 || (unaff_r7 != 0)); unaff_r7 = unaff_r7 / 10) {
      pcVar6[-1] = (char)unaff_r7 + (char)(unaff_r7 / 10) * -10 + '0';
      iVar5 = iVar5 + -1;
      pcVar6 = pcVar6 + -1;
    }
    pcVar6[-1] = cVar10;
    if ((int)((uint)(ushort)*puVar3 << 0x14) < 0) {
      cVar10 = 'E';
    }
    else {
      cVar10 = 'e';
    }
    pcVar7 = pcVar6 + -2;
    pcVar6[-2] = cVar10;
  }
  pcVar6 = acStack_2f + (2 - (int)pcVar7);
  puVar3[6] = (puVar3[6] - (int)(pcVar6 + (iStack_4c != 0) + unaff_r5 + (unaff_r10 >> 0x1f))) - 1;
  if (-1 < (int)((uint)(byte)*puVar3 << 0x1b)) {
    FUN_0002048c(puVar3);
  }
  if (iStack_4c != 0) {
    (*(code *)puVar3[1])(iStack_4c,puVar3[2]);
    puVar3[8] = puVar3[8] + 1;
  }
  if ((int)((uint)(byte)*puVar3 << 0x1b) < 0) {
    FUN_0002048c(puVar3);
  }
  while (uVar8 = unaff_r5 - 1, 0 < (int)unaff_r5) {
    if ((unaff_r6 < 0) || ((int)unaff_r11 <= unaff_r6)) {
      uVar1 = 0x30;
    }
    else {
      uVar1 = auStack_6c[unaff_r6];
    }
    (*(code *)puVar3[1])(uVar1,puVar3[2]);
    puVar3[8] = puVar3[8] + 1;
    unaff_r6 = unaff_r6 + 1;
    unaff_r10 = unaff_r10 + -1;
    unaff_r5 = uVar8;
    if (unaff_r10 == 0) {
      iVar5 = FUN_00020e0c();
      (*(code *)puVar3[1])
                (*(undefined1 *)((int)*(int **)(iVar5 + 0xc) + **(int **)(iVar5 + 0xc)),puVar3[2]);
      puVar3[8] = puVar3[8] + 1;
    }
  }
  while (0 < (int)pcVar6) {
    (*(code *)puVar3[1])(*pcVar7,puVar3[2]);
    puVar3[8] = puVar3[8] + 1;
    pcVar6 = pcVar6 + -1;
    pcVar7 = pcVar7 + 1;
  }
  FUN_000204b8(puVar3);
  return 3;
}



/* ===== FUN_00028680 @ 00028680, 310 bytes ===== */

void FUN_00028680(void)

{
  float *pfVar1;
  float *pfVar2;
  int iVar3;
  int iVar4;
  float fVar5;
  float fVar6;
  
  iVar3 = DAT_00028a80;
  pfVar2 = DAT_00028a7c;
  pfVar1 = DAT_00028a78;
  fVar5 = DAT_00028a7c[9];
  if (((uint)fVar5 < (uint)*DAT_00028a78) && (fVar5 != 0.0)) {
    *DAT_00028a78 = fVar5;
    pfVar1[1] = 1.4013e-45;
    *(undefined4 *)(iVar3 + 0x80) = 0xd;
  }
  iVar4 = DAT_00028a84;
  if (*(float *)(DAT_00028a84 + 0x40) <= pfVar2[2]) {
    if (pfVar1[1] == 0.0) {
      pfVar1[0x11] = 0.0;
    }
  }
  else {
    fVar5 = pfVar1[0x11];
    pfVar1[0x11] = (float)((int)fVar5 + 1U);
    if (8000 < (int)fVar5 + 1U) {
      *(undefined4 *)(iVar3 + 0x80) = 0xc;
      pfVar1[0x11] = 1.12104e-41;
      pfVar1[1] = 1.4013e-45;
    }
  }
  if (DAT_00028a88 < *(int *)(iVar3 + 0x84)) {
    fVar5 = pfVar1[0x10];
    pfVar1[0x10] = (float)((int)fVar5 + 1U);
    if (8000 < (int)fVar5 + 1U) {
      *(undefined4 *)(iVar3 + 0x80) = 0xb;
      pfVar1[0x10] = 1.12104e-41;
      pfVar1[1] = 1.4013e-45;
    }
  }
  else if (pfVar1[1] == 0.0) {
    pfVar1[0x10] = 0.0;
  }
  if (ABS(*(float *)(iVar3 + 100)) <= pfVar2[3]) {
    if (pfVar1[1] == 0.0) {
      pfVar1[0xf] = 0.0;
    }
  }
  else {
    fVar5 = pfVar1[0xf];
    pfVar1[0xf] = (float)((int)fVar5 + 1U);
    if (5000 < (int)fVar5 + 1U) {
      *(undefined4 *)(iVar3 + 0x80) = 10;
      pfVar1[0xf] = 7.00649e-42;
      pfVar1[1] = 1.4013e-45;
    }
  }
  fVar5 = *(float *)(iVar3 + 0x68);
  if (*pfVar2 < fVar5) {
    if (pfVar1[1] == 0.0) {
      pfVar1[0xd] = 0.0;
    }
  }
  else {
    fVar6 = pfVar1[0xd];
    pfVar1[0xd] = (float)((int)fVar6 + 1U);
    if (5000 < (int)fVar6 + 1U) {
      *(undefined4 *)(iVar3 + 0x80) = 9;
      pfVar1[0xd] = 7.00649e-42;
      pfVar1[1] = 1.4013e-45;
    }
  }
  if (fVar5 <= pfVar2[0x1d]) {
    if (pfVar1[1] == 0.0) {
      pfVar1[0xe] = 0.0;
    }
  }
  else {
    fVar5 = pfVar1[0xe];
    pfVar1[0xe] = (float)((int)fVar5 + 1U);
    if (20000 < (int)fVar5 + 1U) {
      pfVar1[0xe] = 2.8026e-41;
      pfVar1[1] = 1.4013e-45;
      *(undefined4 *)(iVar3 + 0x80) = 8;
      goto LAB_000287a8;
    }
  }
  if (*(uint *)(iVar3 + 0x80) < 8) {
    return;
  }
LAB_000287a8:
  if (*(int *)(iVar4 + 0x38) == 2) {
    *(undefined4 *)(iVar4 + 0x38) = 0;
    *(undefined4 *)(iVar4 + 0x3c) = 1;
  }
  return;
}



/* ===== FUN_000287b6 @ 000287b6, 1170 bytes ===== */

void FUN_000287b6(void)

{
  float *pfVar1;
  float *pfVar2;
  ushort uVar3;
  int *piVar4;
  int iVar5;
  float *pfVar6;
  int iVar7;
  ushort *puVar8;
  undefined1 *puVar9;
  int iVar10;
  float *pfVar11;
  undefined4 *puVar12;
  uint in_fpscr;
  float fVar13;
  float fVar14;
  undefined4 uVar15;
  float fVar16;
  float fVar17;
  
  pfVar6 = DAT_00028a80;
  DAT_00028a80[0xe] = (float)((int)DAT_00028a80[0xe] + 1);
  puVar8 = DAT_00028a90;
  iVar7 = DAT_00028a8c;
  *DAT_00028a90 = *(ushort *)(DAT_00028a8c + 0x50);
  puVar8[1] = *DAT_00028a94;
  puVar8[2] = *DAT_00028a98;
  puVar8[3] = *(ushort *)(iVar7 + 0x52);
  puVar8[6] = *DAT_00028a9c;
  puVar8[7] = *DAT_00028aa0;
  puVar8[4] = *(ushort *)(iVar7 + 0x54);
  puVar8[5] = *DAT_00028aa4;
  puVar8[8] = *DAT_00028aa8;
  iVar10 = DAT_00028ab0;
  puVar9 = DAT_00028aac;
  iVar7 = DAT_00028a84;
  pfVar6[0x21] = *(float *)(DAT_00028ab0 + ((puVar8[6] & 0xfff) >> 4) * 4);
  uVar3 = puVar8[7];
  *puVar9 = (char)(((uint)uVar3 << 0x14) >> 0x18);
  *(float *)(iVar7 + 0x40) =
       *(float *)(iVar7 + 0x40) * *(float *)(iVar7 + 0x74) +
       *(float *)(iVar7 + 0x78) * *(float *)(iVar10 + ((uVar3 & 0xfff) >> 4) * 4);
  fVar13 = (float)VectorUnsignedToFloat((uint)puVar8[3],(byte)(in_fpscr >> 0x16) & 3);
  fVar13 = fVar13 * DAT_00028ab4;
  pfVar6[0x1a] = fVar13;
  fVar16 = DAT_00028ab8;
  pfVar6[0x1b] = fVar13 * DAT_00028ab8;
  fVar13 = DAT_00028abc;
  fVar14 = (float)VectorUnsignedToFloat((uint)*puVar8,(byte)(in_fpscr >> 0x16) & 3);
  fVar14 = (pfVar6[0x10] - fVar14) * DAT_00028abc;
  pfVar6[0x13] = fVar14;
  fVar17 = (float)VectorUnsignedToFloat((uint)puVar8[1],(byte)(in_fpscr >> 0x16) & 3);
  pfVar6[0x14] = (pfVar6[0x11] - fVar17) * fVar13;
  fVar13 = (float)FUN_0002aa0a(fVar14,0xbf800000,0x3f800000);
  pfVar6[0x13] = fVar13;
  fVar13 = (float)FUN_0002aa0a(pfVar6[0x14],0xbf800000,0x3f800000);
  pfVar6[0x14] = fVar13;
  pfVar11 = DAT_00028ac0;
  pfVar1 = DAT_00028ac0 + 2;
  fVar13 = (float)VectorUnsignedToFloat((uint)puVar8[4],(byte)(in_fpscr >> 0x16) & 3);
  pfVar2 = DAT_00028ac0 + 3;
  fVar13 = *DAT_00028ac0 * *pfVar1 + fVar13 * *pfVar2;
  *DAT_00028ac0 = fVar13;
  fVar14 = (float)VectorUnsignedToFloat((uint)puVar8[5],(byte)(in_fpscr >> 0x16) & 3);
  fVar14 = pfVar11[1] * *pfVar1 + fVar14 * *pfVar2;
  pfVar11[1] = fVar14;
  pfVar11[4] = fVar13 - pfVar11[5];
  pfVar11[6] = (fVar14 - pfVar11[7]) * pfVar11[10];
  FUN_00029efc();
  iVar10 = DAT_00028ac4;
  if (*(int *)(DAT_00028ac4 + 4) != 0) {
    *(undefined4 *)(DAT_00028ac4 + 4) = 0;
    fVar14 = (float)VectorUnsignedToFloat
                              (*(undefined4 *)(iVar7 + 0x54),(byte)(in_fpscr >> 0x16) & 3);
    uVar15 = VectorFloatToUnsigned(*(float *)(iVar10 + 8) * *(float *)(iVar7 + 0x58),3);
    fVar13 = (float)VectorUnsignedToFloat(uVar15,(byte)(in_fpscr >> 0x16) & 3);
    *(float *)(iVar7 + 0xc) =
         (*(float *)(iVar10 + 8) * fVar14 - fVar13 * DAT_00028ac8) + *(float *)(iVar7 + 0x14);
    FUN_0002aa2a(DAT_00028ad0,DAT_00028acc,iVar7 + 0xc);
    FUN_0002a98c(*(undefined4 *)(iVar7 + 0xc),pfVar6 + 0x1e);
  }
  fVar13 = pfVar6[0x13];
  pfVar6[0x16] = fVar13;
  pfVar1 = DAT_00028ad4;
  iVar5 = DAT_00028a7c;
  piVar4 = DAT_00028a78;
  fVar16 = (fVar13 + pfVar6[0x14] * 2.0) * fVar16;
  pfVar6[0x17] = fVar16;
  pfVar6[0x18] = pfVar6[0x1f] * fVar13 + pfVar6[0x1e] * fVar16;
  pfVar6[0x19] = pfVar6[0x1f] * fVar16 - pfVar6[0x1e] * fVar13;
  fVar16 = DAT_00028ad8;
  if (pfVar6[0xe] != 2.8026e-44) goto LAB_00028b6e;
  fVar13 = *(float *)(iVar10 + 0x10) * DAT_00028adc;
  *(float *)(iVar7 + 0x20) = fVar13;
  fVar13 = *(float *)(iVar7 + 4) * *(float *)(iVar7 + 0x68) + fVar13 * *(float *)(iVar7 + 0x6c);
  *(float *)(iVar7 + 4) = fVar13;
  *(float *)(iVar7 + 0x1c) = fVar13 * *(float *)(iVar7 + 0x5c);
  pfVar6[0xe] = 0.0;
  *(float *)(iVar10 + 0x10) = fVar16;
  piVar4[0xb] = 1;
  fVar14 = *pfVar6 - *(float *)(iVar7 + 0x18);
  pfVar1[0xc] = fVar14;
  fVar14 = pfVar1[10] * fVar14;
  pfVar1[0x10] = fVar14;
  fVar13 = pfVar6[0xf];
  if (fVar13 == 2.8026e-45 || fVar13 == 5.60519e-45) {
    fVar13 = (float)FUN_0002aa0a(fVar14,-pfVar6[1]);
    pfVar6[7] = fVar13;
  }
  else if (fVar13 == 4.2039e-45) {
    fVar13 = (float)FUN_0002aa0a(pfVar6[1],-*(float *)(iVar5 + 0x18));
    pfVar6[7] = fVar13;
  }
  fVar13 = pfVar6[7] - pfVar6[8];
  pfVar6[9] = fVar13;
  fVar14 = *(float *)(iVar5 + 0x10);
  if (fVar14 < fVar13) {
LAB_00028b0a:
    pfVar6[8] = pfVar6[8] + fVar14;
  }
  else {
    fVar14 = *(float *)(iVar5 + 0x14);
    if (fVar13 < fVar14) goto LAB_00028b0a;
    pfVar6[8] = pfVar6[7];
  }
  puVar12 = DAT_00028cb0;
  fVar14 = pfVar6[0x19];
  fVar13 = *(float *)(iVar7 + 0x68) * *(float *)(iVar7 + 0x70) + *(float *)(iVar7 + 0x6c) * fVar14;
  *(float *)(iVar7 + 0x70) = fVar13;
  *(float *)(iVar7 + 0x30) = *(float *)(iVar7 + 0x44) * fVar13;
  *puVar12 = *(undefined4 *)(iVar7 + 4);
  puVar12[1] = fVar14;
  FUN_0002a65e(puVar12);
  fVar13 = pfVar6[8];
  fVar14 = (float)puVar12[5];
  pfVar1[2] = fVar13 - fVar14;
  pfVar1[6] = *pfVar1 * (fVar13 - fVar14) - (float)puVar12[0xc];
LAB_00028b6e:
  fVar13 = pfVar6[0xf];
  if (fVar13 == 1.4013e-45) {
    fVar13 = (*pfVar6 - *(float *)(iVar7 + 0x18)) * pfVar6[3] +
             pfVar6[4] * (pfVar6[1] - *(float *)(iVar7 + 0x1c)) + pfVar6[2];
    *(float *)(puVar9 + 4) = fVar13;
    fVar13 = *(float *)(iVar7 + 0x48) * fVar13;
    pfVar6[0xb] = fVar13;
    fVar13 = (float)FUN_0002aa0a(fVar13,-*(float *)(iVar5 + 0xc));
    pfVar6[0xb] = fVar13;
  }
  else if (fVar13 == 2.8026e-45 || fVar13 == 4.2039e-45) {
    fVar13 = pfVar1[6];
    pfVar6[0xb] = fVar13;
    fVar13 = (float)FUN_0002aa0a(fVar13,-*(float *)(iVar5 + 0xc));
    pfVar6[0xb] = fVar13;
  }
  else if (fVar13 == 5.60519e-45) {
    fVar13 = pfVar1[6];
    pfVar6[0xb] = fVar13;
    fVar13 = (float)FUN_0002aa0a(fVar13,-pfVar6[0xc]);
    pfVar6[0xb] = fVar13;
  }
  else {
    pfVar6[0xb] = fVar16;
  }
  iVar10 = DAT_00028cb4;
  *(float *)(DAT_00028cb4 + 8) = pfVar6[0x18];
  *(float *)(iVar10 + 4) = fVar16;
  *(float *)(iVar10 + 0x54) = pfVar6[0x19];
  *(float *)(iVar10 + 0x50) = pfVar6[0xb];
  FUN_0002a6fa(iVar10);
  FUN_0002a6fa(iVar10 + 0x4c);
  FUN_00028680();
  if (*(int *)(iVar7 + 0x38) == 2) {
    *piVar4 = *piVar4 + 1;
  }
  else {
    *piVar4 = 0;
    FUN_0002a448();
  }
  FUN_0002aa94(DAT_00028cb8,iVar10 + 0x40,iVar10 + 0x8c);
  fVar13 = *(float *)(iVar10 + 0x40);
  fVar16 = *(float *)(iVar10 + 0x8c);
  pfVar6[0x1c] = pfVar6[0x1f] * fVar13 - pfVar6[0x1e] * fVar16;
  pfVar6[0x1d] = pfVar6[0x1e] * fVar13 + pfVar6[0x1f] * fVar16;
  FUN_0002a2c0();
  *(undefined1 *)(DAT_00028cbc + 0x46) = 1;
  DAT_e000e280 = 4;
  return;
}



/* ===== FUN_00028cc0 @ 00028cc0, 228 bytes ===== */

void FUN_00028cc0(void)

{
  undefined4 *puVar1;
  uint *puVar2;
  undefined4 *puVar3;
  uint *puVar4;
  undefined4 *puVar5;
  uint uVar6;
  uint uVar7;
  undefined4 *puVar8;
  undefined4 *puVar9;
  uint *puVar10;
  
  puVar1 = DAT_00028da4;
  disableIRQinterrupts();
  *DAT_00028da4 = 0x123;
  *puVar1 = 0x3210;
  uVar6 = DAT_00028da8;
  puVar1[1] = DAT_00028da8;
  puVar1[1] = ~uVar6;
  puVar2 = DAT_00028dac;
  uVar6 = *DAT_00028dac;
  *DAT_00028dac = *DAT_00028dac & 0xfff0ffff;
  puVar3 = DAT_00028db0;
  *DAT_00028db0 = 0x3f;
  puVar4 = DAT_00028db4;
  puVar10 = puVar3 + -1;
  do {
  } while ((*puVar10 & 0x100) == 0);
  *DAT_00028db4 = *DAT_00028db4 & 0xfffffff8 | 4;
  puVar5 = DAT_00028db8;
  *DAT_00028db8 = 0x8000;
  *DAT_00028dbc = 0;
  do {
  } while ((*puVar10 & 0x100) == 0);
  *puVar3 = 0x10;
  *puVar4 = *puVar4 & 0xfffffff8 | 3;
  uVar7 = 0;
  puVar8 = DAT_00028dc0;
  puVar9 = DAT_00028dbc;
  do {
    *puVar9 = *puVar8;
    do {
    } while ((*puVar10 & 0x10) == 0);
    *puVar3 = 0x10;
    uVar7 = uVar7 + 1;
    puVar8 = puVar8 + 1;
    puVar9 = puVar9 + 1;
  } while (uVar7 < 5);
  *puVar4 = *puVar4 & 0xfffffff8;
  uVar7 = *puVar10;
  while ((uVar7 & 0x100) == 0) {
    *puVar3 = 0x10;
    uVar7 = *puVar10;
  }
  *puVar5 = 0;
  *puVar2 = *puVar2 | uVar6 & 0x70000;
  *puVar4 = *puVar4 | 0x10000;
  *puVar1 = 0x3210;
  enableIRQinterrupts();
  return;
}



/* ===== FUN_00029efc @ 00029efc, 182 bytes ===== */

void FUN_00029efc(int param_1)

{
  int iVar1;
  uint in_fpscr;
  float fVar2;
  int iVar3;
  float fVar4;
  
  fVar2 = (float)thunk_FUN_00023be8(*(float *)(param_1 + 0x18) -
                                    *(float *)(param_1 + 0x10) * *(float *)(param_1 + 0x30),
                                    *(float *)(param_1 + 0x10) * *(float *)(param_1 + 0x2c));
  iVar1 = DAT_00029fc4;
  iVar3 = (int)(DAT_00029fb8 + fVar2 * DAT_00029fb4);
  if (iVar3 < 0) {
    iVar3 = iVar3 + 0x1000;
  }
  if (0xfff < iVar3) {
    iVar3 = iVar3 + -0x1000;
  }
  fVar2 = (float)VectorUnsignedToFloat
                           ((uint)*(ushort *)(DAT_00029fbc + iVar3 * 2),(byte)(in_fpscr >> 0x16) & 3
                           );
  fVar2 = (fVar2 - DAT_00029fb8) * DAT_00029fc0;
  *(float *)(param_1 + 0x34) = fVar2;
  fVar4 = fVar2 - *(float *)(param_1 + 0x40);
  if (iVar1 < (int)fVar4) {
    *(int *)(param_1 + 0x44) = *(int *)(param_1 + 0x44) + -1;
  }
  if ((uint)DAT_00029fc8 < (uint)fVar4) {
    *(int *)(param_1 + 0x44) = *(int *)(param_1 + 0x44) + 1;
  }
  *(float *)(param_1 + 0x40) = fVar2;
  fVar4 = (float)VectorSignedToFloat(*(undefined4 *)(param_1 + 0x44),(byte)(in_fpscr >> 0x16) & 3);
  *(float *)(param_1 + 0x3c) = (fVar2 + fVar4 * DAT_00029fcc) - *(float *)(param_1 + 0x38);
  return;
}



/* ===== FUN_00029fd0 @ 00029fd0, 232 bytes ===== */

void FUN_00029fd0(undefined4 *param_1,undefined4 *param_2,uint param_3)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  int *piVar3;
  uint uVar4;
  undefined4 uVar5;
  uint uVar6;
  int *piVar7;
  undefined4 *puVar8;
  uint *puVar9;
  uint *puVar10;
  
  puVar1 = DAT_0002a140;
  *DAT_0002a140 = 0x123;
  *puVar1 = 0x3210;
  uVar6 = DAT_0002a144;
  puVar1[1] = DAT_0002a144;
  puVar1[1] = ~uVar6;
  puVar10 = DAT_0002a140 + 6;
  uVar6 = *puVar10;
  *puVar10 = *puVar10 & 0xfff0ffff;
  puVar2 = DAT_0002a140;
  puVar8 = DAT_0002a140 + 9;
  *puVar8 = 0x3f;
  piVar7 = puVar2 + 8;
  do {
  } while (-1 < *piVar7 << 0x17);
  puVar9 = DAT_0002a140 + 7;
  *puVar9 = (*puVar9 & 0xfffffff8) + 4;
  piVar3 = DAT_0002a148;
  *DAT_0002a148 = 1 << ((uint)param_1 >> 0xd & 0xff);
  *param_1 = 0;
  do {
  } while (-1 < *piVar7 << 0x17);
  *puVar8 = 0x10;
  *puVar9 = (*puVar9 & 0xfffffff8) + 3;
  for (uVar4 = 0; uVar4 < param_3; uVar4 = uVar4 + 1) {
    uVar5 = *param_2;
    param_2 = param_2 + 1;
    *param_1 = uVar5;
    param_1 = param_1 + 1;
    do {
    } while (-1 < *piVar7 << 0x1b);
    *puVar8 = 0x10;
  }
  *puVar9 = *puVar9 & 0xfffffff8;
  while (-1 < *piVar7 << 0x17) {
    *puVar8 = 0x10;
  }
  *piVar3 = 0;
  *puVar10 = *puVar10 | uVar6 & 0x70000;
  *puVar9 = *puVar9 | 0x10000;
  *puVar1 = 0x3210;
  return;
}



/* ===== FUN_0002a0b8 @ 0002a0b8, 136 bytes ===== */

void FUN_0002a0b8(undefined4 *param_1)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  uint uVar3;
  uint *puVar4;
  uint *puVar5;
  undefined4 *puVar6;
  
  puVar1 = DAT_0002a140;
  *DAT_0002a140 = 0x123;
  *puVar1 = 0x3210;
  uVar3 = DAT_0002a144;
  puVar1[1] = DAT_0002a144;
  puVar1[1] = ~uVar3;
  puVar4 = DAT_0002a140 + 6;
  uVar3 = *puVar4;
  *puVar4 = *puVar4 & 0xfff0ffff;
  puVar2 = DAT_0002a140;
  puVar6 = DAT_0002a140 + 9;
  *puVar6 = 0x3f;
  do {
  } while (-1 < (int)(puVar2[8] << 0x17));
  puVar5 = DAT_0002a140 + 7;
  *puVar5 = (*puVar5 & 0xfffffff8) + 4;
  *DAT_0002a148 = 1 << ((uint)param_1 >> 0xd & 0xff);
  *param_1 = 0;
  do {
  } while (-1 < (int)(puVar2[8] << 0x17));
  *puVar6 = 0x10;
  *puVar4 = *puVar4 | uVar3 & 0x70000;
  *puVar5 = *puVar5 | 0x10000;
  *puVar1 = 0x3210;
  return;
}



/* ===== FUN_0002a14c @ 0002a14c, 66 bytes ===== */

void FUN_0002a14c(uint *param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  iVar1 = DAT_0002a2b4;
  uVar3 = (*(uint *)(DAT_0002a2b4 + 0xc4) & 0x1fffff) >> 0x10;
  iVar2 = DAT_0002a2b8 + uVar3 * 0x48;
  *(int *)(iVar2 + 0x328) = param_2 << 0x12;
  *(int *)(iVar2 + 0x32c) = param_3 << 0x10;
  *(uint *)(iVar2 + 0x330) = *param_1 & 0xffffff | (uint)*(byte *)((int)param_1 + 3) << 0x18;
  *(uint *)(iVar2 + 0x334) = param_1[1] & 0xffffff | (uint)*(byte *)((int)param_1 + 7) << 0x18;
  *(int *)(iVar1 + 0xd0) = 1 << uVar3;
  return;
}



/* ===== FUN_0002a18e @ 0002a18e, 146 bytes ===== */

void FUN_0002a18e(int param_1,int param_2,uint param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint *puVar4;
  uint uVar5;
  
  iVar1 = DAT_0002a2b4;
  uVar5 = (*(uint *)(DAT_0002a2b4 + 0xc4) & 0x1fffff) >> 0x10;
  uVar3 = param_3;
  if (8 < param_3) {
    if (param_3 < 0xd) {
      uVar3 = 9;
    }
    else if (param_3 < 0x11) {
      uVar3 = 10;
    }
    else if (param_3 < 0x15) {
      uVar3 = 0xb;
    }
    else if (param_3 < 0x19) {
      uVar3 = 0xc;
    }
    else if (param_3 < 0x21) {
      uVar3 = 0xd;
    }
    else if (param_3 < 0x31) {
      uVar3 = 0xe;
    }
    else {
      uVar3 = param_4;
      if (param_3 < 0x41) {
        uVar3 = 0xf;
      }
    }
  }
  iVar2 = DAT_0002a2b8 + uVar5 * 0x48;
  *(int *)(iVar2 + 0x328) = param_2 << 0x12;
  *(uint *)(iVar2 + 0x32c) = uVar3 << 0x10 | 0x200000;
  puVar4 = (uint *)(DAT_0002a2bc + uVar5 * 0x48);
  for (uVar3 = 0; uVar3 < param_3; uVar3 = uVar3 + 4 & 0xff) {
    *puVar4 = *(uint *)(param_1 + uVar3) & 0xffffff |
              (uint)*(byte *)((int)(param_1 + uVar3) + 3) << 0x18;
    puVar4 = puVar4 + 1;
  }
  *(int *)(iVar1 + 0xd0) = 1 << uVar5;
  return;
}



/* ===== FUN_0002a220 @ 0002a220, 146 bytes ===== */

void FUN_0002a220(int param_1,int param_2,uint param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint *puVar4;
  uint uVar5;
  
  iVar1 = DAT_0002a2b4;
  uVar5 = (*(uint *)(DAT_0002a2b4 + 0xc4) & 0x1fffff) >> 0x10;
  uVar3 = param_3;
  if (8 < param_3) {
    if (param_3 < 0xd) {
      uVar3 = 9;
    }
    else if (param_3 < 0x11) {
      uVar3 = 10;
    }
    else if (param_3 < 0x15) {
      uVar3 = 0xb;
    }
    else if (param_3 < 0x19) {
      uVar3 = 0xc;
    }
    else if (param_3 < 0x21) {
      uVar3 = 0xd;
    }
    else if (param_3 < 0x31) {
      uVar3 = 0xe;
    }
    else {
      uVar3 = param_4;
      if (param_3 < 0x41) {
        uVar3 = 0xf;
      }
    }
  }
  iVar2 = DAT_0002a2b8 + uVar5 * 0x48;
  *(int *)(iVar2 + 0x328) = param_2 << 0x12;
  *(uint *)(iVar2 + 0x32c) = uVar3 << 0x10 | 0x300000;
  puVar4 = (uint *)(DAT_0002a2bc + uVar5 * 0x48);
  for (uVar3 = 0; uVar3 < param_3; uVar3 = uVar3 + 4 & 0xff) {
    *puVar4 = *(uint *)(param_1 + uVar3) & 0xffffff |
              (uint)*(byte *)((int)(param_1 + uVar3) + 3) << 0x18;
    puVar4 = puVar4 + 1;
  }
  *(int *)(iVar1 + 0xd0) = 1 << uVar5;
  return;
}



/* ===== FUN_0002a2c0 @ 0002a2c0, 260 bytes ===== */

void FUN_0002a2c0(float param_1,float param_2)

{
  float fVar1;
  int iVar2;
  short sVar3;
  float fVar4;
  undefined4 uVar5;
  float local_10;
  float local_c;
  
  iVar2 = DAT_0002a3d4;
  fVar1 = DAT_0002a3d0;
  fVar4 = param_1 * DAT_0002a3cc + param_2 * 0.5;
  local_10 = fVar4 - param_2;
  if (fVar4 <= 0.0) {
    sVar3 = 3;
  }
  else {
    sVar3 = 2;
  }
  if (0.0 < local_10) {
    sVar3 = sVar3 + -1;
  }
  if (param_2 < 0.0) {
    sVar3 = 7 - sVar3;
  }
  switch(sVar3) {
  default:
    local_10 = DAT_0002a3d8;
    fVar4 = DAT_0002a3d8;
    local_c = local_10;
    goto LAB_0002a33e;
  case 1:
  case 4:
    local_c = param_2 - local_10;
    local_10 = fVar4;
    break;
  case 2:
  case 5:
    local_10 = local_10 + fVar4;
    fVar4 = -param_2;
    local_c = param_2;
    goto LAB_0002a33e;
  case 3:
  case 6:
    fVar4 = param_2 + fVar4;
    local_c = -local_10;
  }
  fVar4 = -fVar4;
LAB_0002a33e:
  uVar5 = VectorFloatToUnsigned((1.0 - fVar4) * DAT_0002a3d0,3);
  *(short *)(DAT_0002a3d4 + 0x14) = (short)uVar5;
  uVar5 = VectorFloatToUnsigned((1.0 - local_c) * fVar1,3);
  *(short *)(iVar2 + 0xc) = (short)uVar5;
  uVar5 = VectorFloatToUnsigned((1.0 - local_10) * fVar1,3);
  *(short *)(iVar2 + 4) = (short)uVar5;
  return;
}



/* ===== FUN_0002a448 @ 0002a448, 124 bytes ===== */

void FUN_0002a448(void)

{
  undefined4 uVar1;
  undefined *puVar2;
  int iVar3;
  int iVar4;
  
  iVar4 = DAT_0002a7b4;
  puVar2 = PTR_DAT_0002a79c;
  uVar1 = DAT_0002a798;
  *(undefined4 *)(PTR_DAT_0002a79c + 0x1c) = DAT_0002a798;
  *(undefined4 *)(puVar2 + 0x20) = uVar1;
  *(undefined4 *)(puVar2 + 0x2c) = uVar1;
  iVar3 = DAT_0002a7b0;
  *(undefined4 *)(DAT_0002a7b0 + 0xc) = uVar1;
  *(undefined4 *)(iVar4 + 0xc) = uVar1;
  *(undefined4 *)(iVar3 + 0x14) = uVar1;
  *(undefined4 *)(iVar4 + 0x14) = uVar1;
  *(undefined4 *)(iVar3 + 0x18) = uVar1;
  *(undefined4 *)(iVar4 + 0x18) = uVar1;
  *(undefined4 *)(iVar3 + 0x34) = uVar1;
  *(undefined4 *)(iVar4 + 0x34) = uVar1;
  *(undefined4 *)(iVar3 + 0x38) = uVar1;
  *(undefined4 *)(iVar4 + 0x38) = uVar1;
  *(undefined4 *)(iVar3 + 0x40) = uVar1;
  *(undefined4 *)(iVar4 + 0x40) = uVar1;
  *(undefined4 *)(iVar3 + 0x3c) = uVar1;
  iVar3 = DAT_0002a7a8;
  *(undefined4 *)(iVar4 + 0x3c) = uVar1;
  *(undefined4 *)(iVar3 + 8) = uVar1;
  *(undefined4 *)(iVar3 + 0x10) = uVar1;
  *(undefined4 *)(iVar3 + 0x1c) = uVar1;
  *(undefined4 *)(iVar3 + 0x14) = uVar1;
  *(undefined4 *)(iVar3 + 0x18) = uVar1;
  iVar3 = DAT_0002a7ac;
  *(undefined4 *)(DAT_0002a7ac + 8) = uVar1;
  *(undefined4 *)(iVar3 + 0x10) = uVar1;
  *(undefined4 *)(iVar3 + 0x1c) = uVar1;
  *(undefined4 *)(iVar3 + 0x14) = uVar1;
  *(undefined4 *)(iVar3 + 0x18) = uVar1;
  return;
}



/* ===== FUN_0002a65e @ 0002a65e, 156 bytes ===== */

void FUN_0002a65e(float *param_1)

{
  float fVar1;
  float fVar2;
  
  fVar1 = *param_1 - param_1[5];
  param_1[2] = fVar1;
  fVar2 = (fVar1 - param_1[3]) * DAT_0002a7dc;
  param_1[4] = fVar2;
  param_1[3] = fVar1;
  param_1[5] = param_1[5] +
               param_1[7] * (param_1[6] + param_1[8] * fVar1 + param_1[10] * param_1[1]);
  fVar1 = param_1[6] + param_1[7] * param_1[9] * (fVar2 + param_1[8] * fVar1);
  param_1[6] = fVar1;
  fVar1 = fVar1 * param_1[0xb];
  param_1[0xc] = fVar1;
  fVar2 = param_1[0xe];
  if (fVar2 < fVar1) {
    param_1[0xc] = fVar2;
    fVar1 = fVar2;
  }
  if (fVar1 < param_1[0xd]) {
    param_1[0xc] = param_1[0xd];
  }
  return;
}



/* ===== FUN_0002a6fa @ 0002a6fa, 158 bytes ===== */

void FUN_0002a6fa(float *param_1)

{
  float fVar1;
  float fVar2;
  
  fVar1 = param_1[2] - param_1[5];
  param_1[3] = fVar1;
  fVar2 = param_1[5] + param_1[4] * param_1[6] + param_1[0xb] * fVar1 + param_1[10] * param_1[0xf];
  param_1[5] = fVar2;
  fVar1 = param_1[6] + param_1[0xc] * fVar1;
  param_1[6] = fVar1;
  fVar2 = param_1[1] - fVar2;
  param_1[0xd] = fVar2;
  fVar2 = *param_1 * fVar2;
  param_1[0xe] = fVar2;
  fVar1 = (fVar2 - fVar1) * param_1[9];
  param_1[0xf] = fVar1;
  fVar2 = param_1[0x11];
  if (fVar1 < fVar2) {
    param_1[0xf] = fVar2;
    fVar1 = fVar2;
  }
  fVar2 = param_1[0x12];
  if (fVar2 < fVar1) {
    param_1[0xf] = fVar2;
    fVar1 = fVar2;
  }
  param_1[0x10] = fVar1;
  return;
}



/* ===== FUN_0002a98c @ 0002a98c, 126 bytes ===== */

void FUN_0002a98c(float param_1,float *param_2,float *param_3)

{
  int iVar1;
  float *pfVar2;
  uint in_fpscr;
  uint uVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  
  iVar1 = DAT_0002ab48;
  fVar5 = DAT_0002ab40 + param_1 * DAT_0002ab3c;
  if ((fVar5 != 2048.0) && (fVar5 != 0.0)) {
    uVar3 = VectorFloatToUnsigned(fVar5,3);
    uVar3 = uVar3 & 0xffff;
    pfVar2 = (float *)(DAT_0002ab48 + uVar3 * 4);
    fVar4 = (float)VectorUnsignedToFloat(uVar3,(byte)((in_fpscr & 0xfffffff) >> 0x16) & 3);
    fVar6 = *pfVar2;
    *param_2 = fVar6 + (fVar5 - fVar4) * (pfVar2[1] - fVar6);
    pfVar2 = (float *)(iVar1 + (uVar3 + 0x200 & 0x7ff) * 4);
    fVar6 = *pfVar2;
    *param_3 = fVar6 + (fVar5 - fVar4) * (pfVar2[1] - fVar6);
    return;
  }
  *param_2 = DAT_0002ab44;
  *param_3 = -1.0;
  return;
}



/* ===== FUN_0002aa0a @ 0002aa0a, 32 bytes ===== */

float FUN_0002aa0a(float param_1,float param_2,float param_3)

{
  if ((param_1 <= param_3) && (param_3 = param_1, param_1 < param_2)) {
    return param_2;
  }
  return param_3;
}



/* ===== FUN_0002aa2a @ 0002aa2a, 52 bytes ===== */

void FUN_0002aa2a(float param_1,float param_2,float *param_3)

{
  float fVar1;
  
  fVar1 = *param_3;
  if (param_2 < fVar1) {
    fVar1 = fVar1 - (param_2 - param_1);
    *param_3 = fVar1;
  }
  if (fVar1 < param_1) {
    *param_3 = fVar1 + (param_2 - param_1);
  }
  return;
}



/* ===== FUN_0002aa94 @ 0002aa94, 86 bytes ===== */

void FUN_0002aa94(float param_1,float *param_2,float *param_3)

{
  float fVar1;
  
  fVar1 = (float)thunk_FUN_00023fd0(*param_2 * *param_2 + *param_3 * *param_3);
  if (param_1 < fVar1) {
    *param_2 = (*param_2 * param_1) / fVar1;
    *param_3 = (*param_3 * param_1) / fVar1;
  }
  return;
}



/* ===== thunk_FUN_00023be8 @ 0002ab7a, 10 bytes ===== */

void thunk_FUN_00023be8(void)

{
  FUN_00023be8();
  return;
}



/* ===== thunk_FUN_00023fd0 @ 0002ab84, 10 bytes ===== */

void thunk_FUN_00023fd0(void)

{
  FUN_00023fd0();
  return;
}



/* ===== motor_fault_monitor @ 1fff8000, 310 bytes ===== */

void motor_fault_monitor(void)

{
  float *pfVar1;
  float *pfVar2;
  int iVar3;
  int iVar4;
  float fVar5;
  float fVar6;
  
  iVar3 = DAT_1fff8400;
  pfVar2 = DAT_1fff83fc;
  pfVar1 = DAT_1fff83f8;
  fVar5 = DAT_1fff83fc[9];
  if (((uint)fVar5 < (uint)*DAT_1fff83f8) && (fVar5 != 0.0)) {
    *DAT_1fff83f8 = fVar5;
    pfVar1[1] = 1.4013e-45;
    *(undefined4 *)(iVar3 + 0x80) = 0xd;
  }
  iVar4 = DAT_1fff8404;
  if (*(float *)(DAT_1fff8404 + 0x40) <= pfVar2[2]) {
    if (pfVar1[1] == 0.0) {
      pfVar1[0x11] = 0.0;
    }
  }
  else {
    fVar5 = pfVar1[0x11];
    pfVar1[0x11] = (float)((int)fVar5 + 1U);
    if (8000 < (int)fVar5 + 1U) {
      *(undefined4 *)(iVar3 + 0x80) = 0xc;
      pfVar1[0x11] = 1.12104e-41;
      pfVar1[1] = 1.4013e-45;
    }
  }
  if (DAT_1fff8408 < *(int *)(iVar3 + 0x84)) {
    fVar5 = pfVar1[0x10];
    pfVar1[0x10] = (float)((int)fVar5 + 1U);
    if (8000 < (int)fVar5 + 1U) {
      *(undefined4 *)(iVar3 + 0x80) = 0xb;
      pfVar1[0x10] = 1.12104e-41;
      pfVar1[1] = 1.4013e-45;
    }
  }
  else if (pfVar1[1] == 0.0) {
    pfVar1[0x10] = 0.0;
  }
  if (ABS(*(float *)(iVar3 + 100)) <= pfVar2[3]) {
    if (pfVar1[1] == 0.0) {
      pfVar1[0xf] = 0.0;
    }
  }
  else {
    fVar5 = pfVar1[0xf];
    pfVar1[0xf] = (float)((int)fVar5 + 1U);
    if (5000 < (int)fVar5 + 1U) {
      *(undefined4 *)(iVar3 + 0x80) = 10;
      pfVar1[0xf] = 7.00649e-42;
      pfVar1[1] = 1.4013e-45;
    }
  }
  fVar5 = *(float *)(iVar3 + 0x68);
  if (*pfVar2 < fVar5) {
    if (pfVar1[1] == 0.0) {
      pfVar1[0xd] = 0.0;
    }
  }
  else {
    fVar6 = pfVar1[0xd];
    pfVar1[0xd] = (float)((int)fVar6 + 1U);
    if (5000 < (int)fVar6 + 1U) {
      *(undefined4 *)(iVar3 + 0x80) = 9;
      pfVar1[0xd] = 7.00649e-42;
      pfVar1[1] = 1.4013e-45;
    }
  }
  if (fVar5 <= pfVar2[0x1d]) {
    if (pfVar1[1] == 0.0) {
      pfVar1[0xe] = 0.0;
    }
  }
  else {
    fVar5 = pfVar1[0xe];
    pfVar1[0xe] = (float)((int)fVar5 + 1U);
    if (20000 < (int)fVar5 + 1U) {
      pfVar1[0xe] = 2.8026e-41;
      pfVar1[1] = 1.4013e-45;
      *(undefined4 *)(iVar3 + 0x80) = 8;
      goto LAB_1fff8128;
    }
  }
  if (*(uint *)(iVar3 + 0x80) < 8) {
    return;
  }
LAB_1fff8128:
  if (*(int *)(iVar4 + 0x38) == 2) {
    *(undefined4 *)(iVar4 + 0x38) = 0;
    *(undefined4 *)(iVar4 + 0x3c) = 1;
  }
  return;
}



/* ===== adc_foc_control_irq @ 1fff8136, 1170 bytes ===== */

void adc_foc_control_irq(void)

{
  float *pfVar1;
  float *pfVar2;
  ushort uVar3;
  int *piVar4;
  int iVar5;
  float *pfVar6;
  int iVar7;
  ushort *puVar8;
  undefined1 *puVar9;
  int iVar10;
  float *pfVar11;
  undefined4 *puVar12;
  uint in_fpscr;
  float fVar13;
  float fVar14;
  undefined4 uVar15;
  float fVar16;
  float fVar17;
  
  pfVar6 = DAT_1fff8400;
  DAT_1fff8400[0xe] = (float)((int)DAT_1fff8400[0xe] + 1);
  puVar8 = DAT_1fff8410;
  iVar7 = DAT_1fff840c;
  *DAT_1fff8410 = *(ushort *)(DAT_1fff840c + 0x50);
  puVar8[1] = *DAT_1fff8414;
  puVar8[2] = *DAT_1fff8418;
  puVar8[3] = *(ushort *)(iVar7 + 0x52);
  puVar8[6] = *DAT_1fff841c;
  puVar8[7] = *DAT_1fff8420;
  puVar8[4] = *(ushort *)(iVar7 + 0x54);
  puVar8[5] = *DAT_1fff8424;
  puVar8[8] = *DAT_1fff8428;
  iVar10 = DAT_1fff8430;
  puVar9 = DAT_1fff842c;
  iVar7 = DAT_1fff8404;
  pfVar6[0x21] = *(float *)(DAT_1fff8430 + ((puVar8[6] & 0xfff) >> 4) * 4);
  uVar3 = puVar8[7];
  *puVar9 = (char)(((uint)uVar3 << 0x14) >> 0x18);
  *(float *)(iVar7 + 0x40) =
       *(float *)(iVar7 + 0x40) * *(float *)(iVar7 + 0x74) +
       *(float *)(iVar7 + 0x78) * *(float *)(iVar10 + ((uVar3 & 0xfff) >> 4) * 4);
  fVar13 = (float)VectorUnsignedToFloat((uint)puVar8[3],(byte)(in_fpscr >> 0x16) & 3);
  fVar13 = fVar13 * DAT_1fff8434;
  pfVar6[0x1a] = fVar13;
  fVar16 = DAT_1fff8438;
  pfVar6[0x1b] = fVar13 * DAT_1fff8438;
  fVar13 = DAT_1fff843c;
  fVar14 = (float)VectorUnsignedToFloat((uint)*puVar8,(byte)(in_fpscr >> 0x16) & 3);
  fVar14 = (pfVar6[0x10] - fVar14) * DAT_1fff843c;
  pfVar6[0x13] = fVar14;
  fVar17 = (float)VectorUnsignedToFloat((uint)puVar8[1],(byte)(in_fpscr >> 0x16) & 3);
  pfVar6[0x14] = (pfVar6[0x11] - fVar17) * fVar13;
  fVar13 = (float)clampf(fVar14,0xbf800000,0x3f800000);
  pfVar6[0x13] = fVar13;
  fVar13 = (float)clampf(pfVar6[0x14],0xbf800000,0x3f800000);
  pfVar6[0x14] = fVar13;
  pfVar11 = DAT_1fff8440;
  pfVar1 = DAT_1fff8440 + 2;
  fVar13 = (float)VectorUnsignedToFloat((uint)puVar8[4],(byte)(in_fpscr >> 0x16) & 3);
  pfVar2 = DAT_1fff8440 + 3;
  fVar13 = *DAT_1fff8440 * *pfVar1 + fVar13 * *pfVar2;
  *DAT_1fff8440 = fVar13;
  fVar14 = (float)VectorUnsignedToFloat((uint)puVar8[5],(byte)(in_fpscr >> 0x16) & 3);
  fVar14 = pfVar11[1] * *pfVar1 + fVar14 * *pfVar2;
  pfVar11[1] = fVar14;
  pfVar11[4] = fVar13 - pfVar11[5];
  pfVar11[6] = (fVar14 - pfVar11[7]) * pfVar11[10];
  output_sensor_lookup_and_unwrap();
  iVar10 = DAT_1fff8444;
  if (*(int *)(DAT_1fff8444 + 4) != 0) {
    *(undefined4 *)(DAT_1fff8444 + 4) = 0;
    fVar14 = (float)VectorUnsignedToFloat
                              (*(undefined4 *)(iVar7 + 0x54),(byte)(in_fpscr >> 0x16) & 3);
    uVar15 = VectorFloatToUnsigned(*(float *)(iVar10 + 8) * *(float *)(iVar7 + 0x58),3);
    fVar13 = (float)VectorUnsignedToFloat(uVar15,(byte)(in_fpscr >> 0x16) & 3);
    *(float *)(iVar7 + 0xc) =
         (*(float *)(iVar10 + 8) * fVar14 - fVar13 * DAT_1fff8448) + *(float *)(iVar7 + 0x14);
    wrap_periodic_value(DAT_1fff8450,DAT_1fff844c,iVar7 + 0xc);
    fast_sincos_lut(*(undefined4 *)(iVar7 + 0xc),pfVar6 + 0x1e);
  }
  fVar13 = pfVar6[0x13];
  pfVar6[0x16] = fVar13;
  pfVar1 = DAT_1fff8454;
  iVar5 = DAT_1fff83fc;
  piVar4 = DAT_1fff83f8;
  fVar16 = (fVar13 + pfVar6[0x14] * 2.0) * fVar16;
  pfVar6[0x17] = fVar16;
  pfVar6[0x18] = pfVar6[0x1f] * fVar13 + pfVar6[0x1e] * fVar16;
  pfVar6[0x19] = pfVar6[0x1f] * fVar16 - pfVar6[0x1e] * fVar13;
  fVar16 = DAT_1fff8458;
  if (pfVar6[0xe] != 2.8026e-44) goto LAB_1fff84ee;
  fVar13 = *(float *)(iVar10 + 0x10) * DAT_1fff845c;
  *(float *)(iVar7 + 0x20) = fVar13;
  fVar13 = *(float *)(iVar7 + 4) * *(float *)(iVar7 + 0x68) + fVar13 * *(float *)(iVar7 + 0x6c);
  *(float *)(iVar7 + 4) = fVar13;
  *(float *)(iVar7 + 0x1c) = fVar13 * *(float *)(iVar7 + 0x5c);
  pfVar6[0xe] = 0.0;
  *(float *)(iVar10 + 0x10) = fVar16;
  piVar4[0xb] = 1;
  fVar14 = *pfVar6 - *(float *)(iVar7 + 0x18);
  pfVar1[0xc] = fVar14;
  fVar14 = pfVar1[10] * fVar14;
  pfVar1[0x10] = fVar14;
  fVar13 = pfVar6[0xf];
  if (fVar13 == 2.8026e-45 || fVar13 == 5.60519e-45) {
    fVar13 = (float)clampf(fVar14,-pfVar6[1]);
    pfVar6[7] = fVar13;
  }
  else if (fVar13 == 4.2039e-45) {
    fVar13 = (float)clampf(pfVar6[1],-*(float *)(iVar5 + 0x18));
    pfVar6[7] = fVar13;
  }
  fVar13 = pfVar6[7] - pfVar6[8];
  pfVar6[9] = fVar13;
  fVar14 = *(float *)(iVar5 + 0x10);
  if (fVar14 < fVar13) {
LAB_1fff848a:
    pfVar6[8] = pfVar6[8] + fVar14;
  }
  else {
    fVar14 = *(float *)(iVar5 + 0x14);
    if (fVar13 < fVar14) goto LAB_1fff848a;
    pfVar6[8] = pfVar6[7];
  }
  puVar12 = DAT_1fff8630;
  fVar14 = pfVar6[0x19];
  fVar13 = *(float *)(iVar7 + 0x68) * *(float *)(iVar7 + 0x70) + *(float *)(iVar7 + 0x6c) * fVar14;
  *(float *)(iVar7 + 0x70) = fVar13;
  *(float *)(iVar7 + 0x30) = *(float *)(iVar7 + 0x44) * fVar13;
  *puVar12 = *(undefined4 *)(iVar7 + 4);
  puVar12[1] = fVar14;
  motion_observer_step(puVar12);
  fVar13 = pfVar6[8];
  fVar14 = (float)puVar12[5];
  pfVar1[2] = fVar13 - fVar14;
  pfVar1[6] = *pfVar1 * (fVar13 - fVar14) - (float)puVar12[0xc];
LAB_1fff84ee:
  fVar13 = pfVar6[0xf];
  if (fVar13 == 1.4013e-45) {
    fVar13 = (*pfVar6 - *(float *)(iVar7 + 0x18)) * pfVar6[3] +
             pfVar6[4] * (pfVar6[1] - *(float *)(iVar7 + 0x1c)) + pfVar6[2];
    *(float *)(puVar9 + 4) = fVar13;
    fVar13 = *(float *)(iVar7 + 0x48) * fVar13;
    pfVar6[0xb] = fVar13;
    fVar13 = (float)clampf(fVar13,-*(float *)(iVar5 + 0xc));
    pfVar6[0xb] = fVar13;
  }
  else if (fVar13 == 2.8026e-45 || fVar13 == 4.2039e-45) {
    fVar13 = pfVar1[6];
    pfVar6[0xb] = fVar13;
    fVar13 = (float)clampf(fVar13,-*(float *)(iVar5 + 0xc));
    pfVar6[0xb] = fVar13;
  }
  else if (fVar13 == 5.60519e-45) {
    fVar13 = pfVar1[6];
    pfVar6[0xb] = fVar13;
    fVar13 = (float)clampf(fVar13,-pfVar6[0xc]);
    pfVar6[0xb] = fVar13;
  }
  else {
    pfVar6[0xb] = fVar16;
  }
  iVar10 = DAT_1fff8634;
  *(float *)(DAT_1fff8634 + 8) = pfVar6[0x18];
  *(float *)(iVar10 + 4) = fVar16;
  *(float *)(iVar10 + 0x54) = pfVar6[0x19];
  *(float *)(iVar10 + 0x50) = pfVar6[0xb];
  current_controller_step(iVar10);
  current_controller_step(iVar10 + 0x4c);
  motor_fault_monitor();
  if (*(int *)(iVar7 + 0x38) == 2) {
    *piVar4 = *piVar4 + 1;
  }
  else {
    *piVar4 = 0;
    reset_control_state();
  }
  limit_vector_magnitude(DAT_1fff8638,iVar10 + 0x40,iVar10 + 0x8c);
  fVar13 = *(float *)(iVar10 + 0x40);
  fVar16 = *(float *)(iVar10 + 0x8c);
  pfVar6[0x1c] = pfVar6[0x1f] * fVar13 - pfVar6[0x1e] * fVar16;
  pfVar6[0x1d] = pfVar6[0x1e] * fVar13 + pfVar6[0x1f] * fVar16;
  svpwm_write_compare();
  *(undefined1 *)(DAT_1fff863c + 0x46) = 1;
  DAT_e000e280 = 4;
  return;
}



/* ===== position_sensor_timer_irq @ 1fff8744, 44 bytes ===== */

void position_sensor_timer_irq(void)

{
  int iVar1;
  
  iVar1 = DAT_1fff8b40;
  if ((*(ushort *)(DAT_1fff8b40 + 0x58) & 0x800) != 0) {
    *DAT_1fff8b44 = 0;
  }
  *(ushort *)(iVar1 + 0x58) = *(ushort *)(iVar1 + 0x58) & 0xf5ff;
  DAT_e000e280 = 1;
  return;
}



/* ===== position_sensor_dma_irq @ 1fff8770, 328 bytes ===== */

void position_sensor_dma_irq(void)

{
  undefined4 uVar1;
  float *pfVar2;
  undefined2 *puVar3;
  uint *puVar4;
  uint uVar5;
  uint uVar6;
  uint in_fpscr;
  float fVar7;
  float fVar8;
  uint uVar9;
  float fVar10;
  float fVar11;
  
  puVar3 = DAT_1fff8b58;
  pfVar2 = DAT_1fff8b50;
  uVar1 = DAT_1fff8b48;
  if ((*DAT_1fff8b4c & 1) != 0) {
    if (DAT_1fff8b50[0xd] == 1.0) {
      uVar5 = 0x3fff - (*DAT_1fff8b54 >> 2);
    }
    else {
      uVar5 = (uint)(*DAT_1fff8b54 >> 2);
    }
    *DAT_1fff8b58 = (short)uVar5;
    uVar6 = uVar5 & 0xffff;
    uVar5 = (uVar5 & 0x3fff) >> 6;
    fVar8 = (float)VectorUnsignedToFloat(uVar6,(byte)(in_fpscr >> 0x16) & 3);
    fVar7 = *(float *)(DAT_1fff8b5c + uVar5 * 4);
    uVar9 = VectorFloatToUnsigned(fVar8 * DAT_1fff8b60,3);
    fVar11 = (float)VectorUnsignedToFloat(uVar9 & 0xffff,(byte)(in_fpscr >> 0x16) & 3);
    fVar10 = (float)VectorUnsignedToFloat(uVar6,(byte)(in_fpscr >> 0x16) & 3);
    fVar8 = (float)VectorUnsignedToFloat(uVar6,(byte)(in_fpscr >> 0x16) & 3);
    *(float *)(puVar3 + 4) =
         (fVar8 + fVar7 + (*(float *)(DAT_1fff8b5c + (uVar5 + 1 & 0xff) * 4) - fVar7) *
                          (fVar10 * DAT_1fff8b60 - fVar11)) * DAT_1fff8b64;
    fVar7 = DAT_1fff8b68;
    wrap_periodic_value(uVar1,DAT_1fff8b68,puVar3 + 4);
    fVar10 = *(float *)(puVar3 + 4);
    fVar8 = fVar10 - *(float *)(puVar3 + 6);
    if (DAT_1fff8b6c < (int)fVar8) {
      fVar8 = fVar8 - fVar7;
      pfVar2[2] = (float)((int)pfVar2[2] + -1);
    }
    if ((uint)DAT_1fff8b70 < (uint)fVar8) {
      fVar8 = fVar8 + fVar7;
      pfVar2[2] = (float)((int)pfVar2[2] + 1);
    }
    fVar11 = (float)VectorSignedToFloat(pfVar2[2],(byte)(in_fpscr >> 0x16) & 3);
    fVar7 = fVar10 + fVar11 * fVar7;
    *pfVar2 = fVar7;
    pfVar2[6] = fVar7 * pfVar2[0x17] - pfVar2[9];
    *(float *)(puVar3 + 6) = fVar10;
    *(float *)(puVar3 + 10) = fVar8;
    puVar4 = DAT_1fff8b74;
    *(float *)(puVar3 + 8) = *(float *)(puVar3 + 8) + fVar8;
    *(undefined4 *)(puVar3 + 2) = 1;
    *puVar4 = *puVar4 & 0xffff | 0x10000;
    *DAT_1fff8b78 = 1;
  }
  *DAT_1fff8b7c = 0xf000f;
  DAT_e000e280 = 2;
  return;
}



/* ===== mcan1_receive_irq @ 1fff88b8, 3664 bytes ===== */

void mcan1_receive_irq(void)

{
  byte *pbVar1;
  ushort uVar2;
  byte bVar3;
  undefined1 uVar4;
  byte bVar5;
  int iVar6;
  float fVar7;
  int iVar8;
  float *pfVar9;
  undefined4 *puVar10;
  ushort *puVar11;
  int iVar12;
  undefined4 *puVar13;
  ushort uVar14;
  undefined2 uVar15;
  int iVar16;
  float fVar17;
  float *pfVar18;
  float fVar19;
  uint uVar20;
  float fVar21;
  uint uVar22;
  uint uVar23;
  float fVar24;
  uint uVar25;
  uint in_fpscr;
  int iVar26;
  undefined4 uVar27;
  float fVar28;
  
  iVar6 = DAT_1fff8b8c;
  puVar10 = DAT_1fff8b88;
  pfVar9 = DAT_1fff8b84;
  iVar8 = DAT_1fff8b80;
  if ((*(uint *)(DAT_1fff8b80 + 0x50) & 1) == 0) goto switchD_1fff8dba_default;
  uVar23 = (*(uint *)(DAT_1fff8b80 + 0xa4) & 0x3fff) >> 8;
  iVar16 = uVar23 * 0x48;
  uVar25 = *(uint *)(&DAT_4002b080 + iVar16);
  *(uint *)(DAT_1fff8b8c + 0x5c) = uVar25;
  *(undefined4 *)(iVar6 + 0x60) = *(undefined4 *)(&DAT_4002b084 + iVar16);
  fVar21 = *(float *)(&DAT_4002b088 + iVar16);
  *(float *)(iVar6 + 100) = fVar21;
  puVar11 = DAT_1fff8b90;
  fVar19 = *(float *)(&DAT_4002b08c + iVar16);
  *(float *)(iVar6 + 0x68) = fVar19;
  *puVar11 = (ushort)((uVar25 << 3) >> 0x15);
  *(uint *)(iVar8 + 0xa8) = uVar23;
  pfVar18 = DAT_1fff8b94;
  fVar7 = DAT_1fff8b68;
  iVar6 = DAT_1fff8b50;
  fVar24 = DAT_1fff8b48;
  if ((float)((uVar25 & 0x1fffffff) >> 0x12 & 0xff) == pfVar9[8]) {
    if (puVar10[1] == 0) {
      *puVar10 = 0;
    }
    if (fVar21 == -NAN) {
      if (fVar19 == -2.6584558e+36) {
        puVar10[1] = 0;
        *puVar10 = 0;
        pfVar18[0x20] = 0.0;
        *(undefined4 *)(iVar6 + 0x38) = 0;
        *(undefined4 *)(iVar6 + 0x3c) = 1;
      }
      iVar12 = DAT_1fff8b98;
      iVar16 = DAT_1fff8b58;
      if (fVar19 == -1.06338233e+37) {
        if ((uint)pfVar18[0x20] < 2) {
          *(undefined4 *)(iVar6 + 0x38) = 2;
          pfVar18[0x20] = 1.4013e-45;
          *(undefined4 *)(iVar6 + 0x3c) = 1;
        }
      }
      else if (fVar19 == -4.2535293e+37) {
        *(undefined4 *)(iVar6 + 0x38) = 0;
        if (pfVar18[0x20] == 1.4013e-45) {
          pfVar18[0x20] = 0.0;
        }
        *(undefined4 *)(iVar6 + 0x3c) = 1;
      }
      else {
        if (fVar19 != -1.7014117e+38) goto LAB_1fff8978;
        DAT_e000e180 = 4;
        DataSynchronizationBarrier(0xf);
        InstructionSynchronizationBarrier(0xf);
        fVar19 = *(float *)(iVar6 + 0x5c);
        fVar17 = (float)VectorSignedToFloat(*(undefined4 *)(iVar6 + 8),(byte)(in_fpscr >> 0x16) & 3)
        ;
        fVar21 = (float)VectorSignedToFloat(*(undefined4 *)(iVar6 + 8),(byte)(in_fpscr >> 0x16) & 3)
        ;
        fVar17 = (float)VectorSignedToFloat((int)(fVar17 * fVar19),(byte)(in_fpscr >> 0x16) & 3);
        iVar26 = (int)(fVar21 - fVar17 * *(float *)(iVar6 + 0x4c));
        *(int *)(iVar6 + 8) = iVar26;
        *(undefined4 *)(iVar12 + 0x44) = 0;
        puVar13 = DAT_1fff8b9c;
        fVar21 = (float)VectorSignedToFloat(iVar26,(byte)(in_fpscr >> 0x16) & 3);
        fVar19 = (*(float *)(iVar16 + 8) + fVar21 * fVar7) * fVar19;
        *(float *)(iVar6 + 0x24) = fVar19;
        puVar13[1] = fVar19;
        *(undefined4 *)(iVar12 + 0x38) = *(undefined4 *)(iVar12 + 0x34);
        *puVar13 = *(undefined4 *)(iVar12 + 0x34);
        *(float *)(iVar6 + 0x18) = fVar24;
        *pfVar18 = fVar24;
        disableIRQinterrupts();
        flash_program_words(DAT_1fff8ba0,DAT_1fff8b9c,2);
        enableIRQinterrupts();
        *(undefined1 *)(DAT_1fff8ba4 + 0x46) = 1;
        *DAT_1fff8ba8 = 1;
        DAT_e000e280 = 4;
        DAT_e000e100 = 4;
      }
    }
    else {
LAB_1fff8978:
      if (*(int *)(iVar6 + 0x38) == 2) {
        uVar23 = (uVar25 & 0x1fffffff) >> 0x1a;
        fVar17 = DAT_1fff8b94[0xf];
        if (uVar23 == 0) {
          if (fVar17 == 1.4013e-45) {
            uVar23 = ((uint)fVar21 & 0xffff) >> 8 | ((uint)fVar21 & 0xff) << 8;
            puVar11[1] = (ushort)uVar23;
            uVar14 = (ushort)((uint)fVar21 >> 0x10);
            puVar11[2] = (ushort)((uint)fVar21 >> 0xc) & 0xff0 | uVar14 >> 0xc;
            puVar11[4] = uVar14 & 0xf00 | SUB42(fVar19,0) & 0xff;
            uVar14 = (ushort)((uint)fVar19 >> 8);
            puVar11[5] = (ushort)((uint)fVar19 >> 4) & 0xff0 | uVar14 >> 0xc;
            puVar11[3] = uVar14 & 0xf00 | (ushort)(byte)((uint)fVar19 >> 0x18);
            uVar27 = uint_to_float_packed(-pfVar9[0x15],uVar23,0x10);
            *(undefined4 *)(puVar11 + 8) = uVar27;
            uVar27 = uint_to_float_packed(-pfVar9[0x16],puVar11[2],0xc);
            *(undefined4 *)(puVar11 + 10) = uVar27;
            fVar21 = (float)uint_to_float_packed(-pfVar9[0x17],puVar11[3],0xc);
            *(float *)(puVar11 + 0xc) = fVar21;
            if (*(int *)(iVar6 + 0x34) == 0x40000000) {
              *(float *)(puVar11 + 8) = -*(float *)(puVar11 + 8);
              *(float *)(puVar11 + 10) = -*(float *)(puVar11 + 10);
              *(float *)(puVar11 + 0xc) = -fVar21;
            }
            *pfVar18 = *(float *)(puVar11 + 8);
            pfVar18[1] = *(float *)(puVar11 + 10);
            pfVar18[2] = *(float *)(puVar11 + 0xc);
            fVar21 = (float)uint_to_float_packed(fVar24,DAT_1fff8f8c,puVar11[4],0xc);
            pfVar18[3] = fVar21;
            fVar21 = (float)uint_to_float_packed(fVar24,0x40a00000,puVar11[5],0xc);
            pfVar18[4] = fVar21;
          }
        }
        else {
          fVar28 = DAT_1fff8b84[0x14];
          iVar16 = *(int *)(DAT_1fff8b50 + 0x34);
          if (uVar23 == 1) {
            if (fVar17 == 2.8026e-45) {
              if (iVar16 == 0x40000000) {
                fVar21 = -fVar21;
              }
              *pfVar18 = fVar21;
              pfVar18[1] = ABS(fVar19) * fVar28;
            }
          }
          else if (uVar23 == 2) {
            if (fVar17 == 4.2039e-45) {
              fVar21 = fVar21 * fVar28;
              *(float *)(puVar11 + 10) = fVar21;
              if (iVar16 == 0x40000000) {
                fVar21 = -fVar21;
                *(float *)(puVar11 + 10) = fVar21;
              }
              pfVar18[1] = fVar21;
            }
          }
          else if (uVar23 == 3 && fVar17 == 5.60519e-45) {
            if (iVar16 == 0x40000000) {
              fVar21 = -fVar21;
            }
            *pfVar18 = fVar21;
            puVar11[2] = SUB42(fVar19,0);
            if (10000 < ((uint)fVar19 & 0xffff)) {
              puVar11[2] = 10000;
            }
            fVar21 = (float)VectorUnsignedToFloat((uint)puVar11[2],(byte)(in_fpscr >> 0x16) & 3);
            pfVar18[1] = fVar21 * DAT_1fff8f94 * fVar28;
            puVar11[3] = (ushort)((uint)fVar19 >> 0x10);
            if (10000 < (uint)fVar19 >> 0x10) {
              puVar11[3] = 10000;
            }
            fVar21 = (float)VectorUnsignedToFloat((uint)puVar11[3],(byte)(in_fpscr >> 0x16) & 3);
            pfVar18[0xc] = fVar21 * DAT_1fff8f98;
          }
        }
      }
    }
    if (*(int *)(iVar6 + 0x34) == 0x3f800000) {
      uVar14 = float_to_uint_packed(*(float *)(iVar6 + 0x18),-pfVar9[0x15],0x10);
      puVar11[1] = uVar14;
      uVar14 = float_to_uint_packed(*(undefined4 *)(iVar6 + 0x1c),-pfVar9[0x16],0xc);
      puVar11[2] = uVar14;
      uVar14 = float_to_uint_packed(*(undefined4 *)(iVar6 + 0x30),-pfVar9[0x17],0xc);
      puVar11[3] = uVar14;
    }
    else {
      uVar14 = float_to_uint_packed(-*(float *)(iVar6 + 0x18),-pfVar9[0x15],0x10);
      puVar11[1] = uVar14;
      uVar14 = float_to_uint_packed(-*(float *)(iVar6 + 0x1c),-pfVar9[0x16],0xc);
      puVar11[2] = uVar14;
      uVar14 = float_to_uint_packed(-*(float *)(iVar6 + 0x30),-pfVar9[0x17],0xc);
      puVar11[3] = uVar14;
    }
    puVar13 = DAT_1fff8f90;
    pbVar1 = (byte *)(DAT_1fff8f90 + 2);
    *pbVar1 = *(byte *)(pfVar9 + 8) | *(char *)(pfVar18 + 0x20) << 4;
    uVar14 = puVar11[1];
    *(char *)((int)puVar13 + 9) = (char)(uVar14 >> 8);
    *(char *)((int)puVar13 + 10) = (char)uVar14;
    uVar14 = puVar11[2];
    *(char *)((int)puVar13 + 0xb) = (char)(uVar14 >> 4);
    uVar2 = puVar11[3];
    *(char *)(puVar13 + 3) = (char)uVar14 * '\x10' + (char)(uVar2 >> 8);
    *(char *)((int)puVar13 + 0xd) = (char)uVar2;
    *(char *)((int)puVar13 + 0xe) = (char)(int)pfVar18[0x21];
    *(char *)((int)puVar13 + 0xf) = (char)(int)*(float *)(iVar6 + 0x40);
    (*(code *)puVar13[1])(pbVar1,*(undefined2 *)(pfVar9 + 7),8);
  }
  iVar16 = DAT_1fff8f9c;
  puVar13 = DAT_1fff8f90;
  if (*puVar11 != 0x7ff) goto switchD_1fff8dba_default;
  uVar20 = DAT_1fff8f90[0x19];
  *(short *)(DAT_1fff8f9c + 0x1c) = (short)uVar20;
  fVar19 = (float)((uVar20 & 0xffffff) >> 0x10);
  bVar3 = (byte)(uVar20 >> 0x10);
  *(byte *)(iVar16 + 0x1e) = bVar3;
  uVar22 = uVar20 >> 0x18;
  uVar4 = (undefined1)(uVar20 >> 0x18);
  *(undefined1 *)(iVar16 + 0x1f) = uVar4;
  uVar25 = DAT_1fff9864;
  iVar12 = DAT_1fff9834;
  uVar23 = DAT_1fff93e4;
  iVar16 = DAT_1fff8f9c;
  fVar21 = pfVar9[8];
  bVar5 = SUB41(fVar21,0);
  if ((float)(uVar20 & 0xffff) != fVar21) {
    if (*(int *)(iVar6 + 0x38) == 0) {
      if ((uVar20 & 0xff0000ff) == 0x550000aa) {
        fVar21 = (float)((uVar20 & 0xffff) >> 8);
        pfVar9[7] = fVar19;
        pfVar9[8] = fVar21;
        *DAT_1fff9868 = uVar25 | (int)fVar21 << 0x10;
        *(undefined1 *)(puVar13 + 2) = 0xaa;
        *(byte *)((int)puVar13 + 9) = (byte)(uVar20 >> 8) & 0x7f;
        *(byte *)((int)puVar13 + 10) = bVar3 & 0x7f;
        *(undefined1 *)((int)puVar13 + 0xb) = 0x55;
        (*(code *)puVar13[1])(puVar13 + 2,0x7ff,4);
        puVar10[3] = 1;
      }
      else if (uVar20 == DAT_1fff9860) {
        *(undefined1 *)(puVar13 + 2) = 0x55;
        *(byte *)((int)puVar13 + 9) = bVar5 & 0x7f;
        *(byte *)((int)puVar13 + 10) = *(byte *)(pfVar9 + 7) & 0x7f;
        *(undefined1 *)((int)puVar13 + 0xb) = 0xaa;
        (*(code *)puVar13[1])(puVar13 + 2,0x7ff,4);
      }
      else {
        if ((uVar20 == DAT_1fff986c) && ((float)(uint)*(ushort *)(puVar13 + 0x1a) == fVar21)) {
          *(undefined1 *)(puVar13 + 2) = 0x41;
          *(undefined1 *)((int)puVar13 + 9) = 0x75;
          *(undefined1 *)((int)puVar13 + 10) = 0x70;
          *(undefined1 *)((int)puVar13 + 0xb) = 0x67;
          *(undefined1 *)(puVar13 + 3) = 0x72;
          *(undefined1 *)((int)puVar13 + 0xd) = 0x61;
          *(undefined1 *)((int)puVar13 + 0xe) = 100;
          *(undefined1 *)((int)puVar13 + 0xf) = 0x65;
          (*(code *)puVar13[1])(puVar13 + 2,0x7fe,8);
          select_configuration_bank_b();
          delay_ms(10);
          DataSynchronizationBarrier(0xf);
          *DAT_1fff9870 = *DAT_1fff9870 & 0x700 | DAT_1fff9874;
          DataSynchronizationBarrier(0xf);
          do {
                    /* WARNING: Do nothing block with infinite loop */
          } while( true );
        }
        if ((uVar20 & 0xff0000ff) == 0x110000ee) {
          *(undefined1 *)(puVar13 + 2) = 0x11;
          uVar15 = *(undefined2 *)(pfVar9 + 0xe);
          *(char *)((int)puVar13 + 9) = (char)uVar15;
          *(char *)((int)puVar13 + 10) = (char)((ushort)uVar15 >> 8);
          *(undefined1 *)((int)puVar13 + 0xb) = 0xee;
          (*(code *)puVar13[1])(puVar13 + 2,0x7ff,4);
        }
      }
    }
    goto switchD_1fff8dba_default;
  }
  if (fVar19 != 2.85865e-43) {
    if (fVar19 != 7.14662e-44) {
      if (fVar19 != 1.1911e-43) {
        if (fVar19 == 2.38221e-43) {
          iVar16 = *(int *)(iVar6 + 0x38);
        }
        if (fVar19 == 2.38221e-43 && iVar16 == 0) {
          puVar10[3] = 1;
          *(byte *)(puVar13 + 2) = bVar5;
          *(byte *)((int)puVar13 + 9) = (byte)((uint)((int)fVar21 << 0x14) >> 0x1c);
          *(undefined1 *)((int)puVar13 + 10) = 0xaa;
          *(undefined1 *)((int)puVar13 + 0xb) = 1;
          (*(code *)puVar13[1])(puVar13 + 2,*(undefined2 *)(pfVar9 + 7),4);
        }
        goto switchD_1fff8dba_default;
      }
      fVar19 = (float)puVar13[0x1a];
      fVar21 = fVar24;
      switch(uVar22) {
      case 0:
        if (DAT_1fff93dc < (int)fVar19) {
          *pfVar9 = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = *pfVar9;
        }
        break;
      case 1:
        uVar23 = in_fpscr & 0xfffffff | (uint)(fVar19 < 0.0) << 0x1f | (uint)(fVar19 == 0.0) << 0x1e
        ;
        bVar3 = (byte)(uVar23 >> 0x18);
        if ((bool)(bVar3 >> 6 & 1) || (bool)(bVar3 >> 7) != NAN(fVar19)) {
          pfVar9[1] = fVar24;
          fVar21 = (float)VectorUnsignedToFloat(pfVar9[0x10],(byte)(uVar23 >> 0x16) & 3);
          fVar21 = fVar21 * 1.5 * pfVar9[0x13] * DAT_1fff93e0 * pfVar9[0x14] * pfVar9[0x1e];
          *(float *)(iVar6 + 0x44) = fVar21;
        }
        else {
          pfVar9[1] = fVar19;
          fVar21 = fVar19 * DAT_1fff93e0;
LAB_1fff927e:
          *(float *)(iVar6 + 0x44) = fVar21;
          fVar24 = fVar19;
        }
LAB_1fff9282:
        *(float *)(iVar6 + 0x48) = 1.0 / fVar21;
        fVar21 = fVar24;
        break;
      case 2:
        if ((int)fVar19 + 0xbd600000U < 0xa80000) {
          pfVar9[2] = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[2];
        }
        break;
      case 3:
        if ((0x3f7fffff < (int)fVar19) || (fVar19 <= 0.0)) {
          fVar21 = pfVar9[3];
        }
        else {
          pfVar9[3] = fVar19;
          fVar21 = fVar19;
        }
        break;
      case 4:
        if (0.0 < fVar19) {
          pfVar9[4] = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[4];
        }
        break;
      case 5:
        if (fVar19 < 0.0) {
          pfVar9[5] = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[5];
        }
        break;
      case 6:
        if (0.0 < fVar19) {
          pfVar9[6] = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[6];
        }
        break;
      case 7:
        pfVar9[7] = (float)((uint)fVar19 & 0x7ff);
        fVar21 = fVar19;
        break;
      case 8:
        pfVar9[8] = (float)((uint)fVar19 & 0x7ff);
        *DAT_1fff93e8 = uVar23 | ((uint)fVar19 & 0x7ff) << 0x10;
        fVar21 = fVar19;
        break;
      case 9:
        pfVar9[9] = fVar19;
        fVar21 = fVar19;
        break;
      case 10:
        if ((int)fVar19 - 1U < 4) {
          fVar21 = fVar19;
          if (fVar19 != pfVar9[10]) {
            *pfVar18 = fVar24;
            pfVar18[1] = fVar24;
            pfVar18[2] = fVar24;
            pfVar18[3] = fVar24;
            pfVar18[4] = fVar24;
            pfVar9[10] = fVar19;
            pfVar18[0xf] = fVar19;
          }
        }
        else {
          fVar21 = pfVar9[10];
        }
        break;
      case 0xb:
      case 0xc:
      case 0xd:
      case 0xe:
      case 0xf:
      case 0x10:
      case 0x11:
      case 0x12:
      case 0x13:
      case 0x14:
        break;
      case 0x15:
        if (0.0 < fVar19) {
          pfVar9[0x15] = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[0x15];
        }
        break;
      case 0x16:
        if (0.0 < fVar19) {
          pfVar9[0x16] = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[0x16];
        }
        break;
      case 0x17:
        if (0.0 < fVar19) {
          pfVar9[0x17] = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[0x17];
        }
        break;
      case 0x18:
        if (DAT_1fff9830 < (int)fVar19 + 0xbd380000U) {
          fVar21 = pfVar9[0x18];
        }
        else {
          pfVar9[0x18] = fVar19;
          *(float *)(iVar12 + 0x38) = fVar19;
          derive_control_parameters();
          fVar21 = fVar19;
        }
        break;
      case 0x19:
        if (fVar19 < 0.0) {
          fVar21 = pfVar9[0x19];
        }
        else {
          pfVar9[0x19] = fVar19;
          pfVar18 = DAT_1fff9838;
LAB_1fff9478:
          *pfVar18 = fVar19;
          fVar21 = fVar19;
        }
        break;
      case 0x1a:
        if (fVar19 < 0.0) {
          fVar21 = pfVar9[0x1a];
        }
        else {
          pfVar9[0x1a] = fVar19;
          pfVar18 = DAT_1fff9838;
LAB_1fff9498:
          pfVar18[1] = fVar19;
          fVar21 = fVar19;
        }
        break;
      case 0x1b:
        if (0.0 <= fVar19) {
          pfVar9[0x1b] = fVar19;
          pfVar18 = DAT_1fff983c;
          goto LAB_1fff9478;
        }
        fVar21 = pfVar9[0x1b];
        break;
      case 0x1c:
        if (0.0 <= fVar19) {
          pfVar9[0x1c] = fVar19;
          pfVar18 = DAT_1fff983c;
          goto LAB_1fff9498;
        }
        fVar21 = pfVar9[0x1c];
        break;
      case 0x1d:
        if ((fVar19 <= *pfVar9) || (0x42000000 < (int)fVar19)) {
          fVar21 = pfVar9[0x1d];
        }
        else {
          pfVar9[0x1d] = fVar19;
          fVar21 = fVar19;
        }
        break;
      case 0x1e:
        if ((0.0 < fVar19) && ((int)fVar19 < 0x3f800001)) {
          pfVar9[0x1e] = fVar19;
          if (pfVar9[1] != 0.0) {
            fVar21 = pfVar9[1] * DAT_1fff93e0;
            goto LAB_1fff927e;
          }
          fVar21 = (float)VectorUnsignedToFloat
                                    (pfVar9[0x10],(byte)((in_fpscr & 0xfffffff) >> 0x16) & 3);
          fVar21 = fVar21 * 1.5 * pfVar9[0x13] * DAT_1fff93e0 * pfVar9[0x14] * fVar19;
          *(float *)(iVar6 + 0x44) = fVar21;
          fVar24 = fVar19;
          goto LAB_1fff9282;
        }
        fVar21 = pfVar9[0x1e];
        break;
      case 0x1f:
        if ((int)fVar19 + 0xc0800000U < 0x2700001) {
          pfVar9[0x1f] = fVar19;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[0x1f];
        }
        break;
      case 0x20:
        if ((fVar19 <= 0.0) || (DAT_1fff9840 <= (int)fVar19)) {
          if ((int)fVar19 < DAT_1fff9840) {
            fVar21 = pfVar9[0x20];
            break;
          }
          pfVar9[0x20] = fVar19;
          *(float *)(iVar6 + 0x68) = fVar24;
          fVar21 = 1.0;
        }
        else {
          pfVar9[0x20] = fVar19;
          fVar21 = DAT_1fff9844 / (DAT_1fff9844 + fVar19 * fVar7);
          *(float *)(iVar6 + 0x68) = fVar21;
          fVar21 = 1.0 - fVar21;
        }
        *(float *)(iVar6 + 0x6c) = fVar21;
        fVar21 = fVar19;
        break;
      case 0x21:
        if (DAT_1fff9830 < (int)fVar19 + 0xbd380000U) {
          fVar21 = pfVar9[0x21];
        }
        else {
          pfVar9[0x21] = fVar19;
          *(float *)(iVar12 + 0x3c) = fVar19;
LAB_1fff95fc:
          derive_control_parameters();
          fVar21 = fVar19;
        }
        break;
      case 0x22:
        if ((0.0 < fVar19) && ((int)fVar19 <= DAT_1fff9848)) {
          pfVar9[0x22] = fVar19;
          goto LAB_1fff95fc;
        }
        fVar21 = pfVar9[0x22];
        break;
      case 0x23:
        if ((uint)fVar19 < 0xc) {
          pfVar9[0x23] = fVar19;
          uVar27 = DAT_1fff984c;
          if (4 < (uint)fVar19) {
            uVar27 = DAT_1fff9850;
          }
          *puVar13 = uVar27;
          uVar27 = DAT_1fff9854;
          if (4 < (uint)fVar19) {
            uVar27 = DAT_1fff9858;
          }
          puVar13[1] = uVar27;
          fVar21 = fVar19;
        }
        else {
          fVar21 = pfVar9[0x23];
        }
      }
      uVar14 = *(ushort *)(pfVar9 + 8);
      *(char *)(puVar13 + 2) = (char)uVar14;
      *(byte *)((int)puVar13 + 9) = (byte)(((uint)uVar14 << 0x15) >> 0x1d);
      iVar6 = DAT_1fff985c;
      *(undefined1 *)((int)puVar13 + 10) = 0x55;
      *(undefined1 *)((int)puVar13 + 0xb) = *(undefined1 *)(iVar6 + 0x1f);
      *(char *)(puVar13 + 3) = SUB41(fVar21,0);
      *(char *)((int)puVar13 + 0xd) = (char)((uint)fVar21 >> 8);
      *(char *)((int)puVar13 + 0xe) = (char)((uint)fVar21 >> 0x10);
      *(char *)((int)puVar13 + 0xf) = (char)((uint)fVar21 >> 0x18);
      (*(code *)puVar13[1])(puVar13 + 2,*(undefined2 *)(pfVar9 + 7),8);
      if (*(char *)(iVar6 + 0x1f) == '#') {
        delay_ms(5);
        (*(code *)*puVar13)(*(undefined2 *)(pfVar9 + 0x23),*(undefined2 *)(pfVar9 + 8));
      }
      goto switchD_1fff8dba_default;
    }
    fVar24 = 0.0;
    switch(uVar22) {
    case 0:
      fVar24 = *pfVar9;
      break;
    case 1:
      fVar24 = pfVar9[1];
      break;
    case 2:
      fVar24 = pfVar9[2];
      break;
    case 3:
      fVar24 = pfVar9[3];
      break;
    case 4:
      fVar24 = pfVar9[4];
      break;
    case 5:
      fVar24 = pfVar9[5];
      break;
    case 6:
      fVar24 = pfVar9[6];
      break;
    case 7:
      fVar24 = pfVar9[7];
      break;
    case 8:
      fVar24 = fVar21;
      break;
    case 9:
      fVar24 = pfVar9[9];
      break;
    case 10:
      fVar24 = pfVar9[10];
      break;
    case 0xb:
      fVar24 = pfVar9[0xb];
      break;
    case 0xc:
      fVar24 = pfVar9[0xc];
      break;
    case 0xd:
      fVar24 = pfVar9[0xd];
      break;
    case 0xe:
      fVar24 = pfVar9[0xe];
      break;
    case 0xf:
      fVar24 = pfVar9[0xf];
      break;
    case 0x10:
      fVar24 = pfVar9[0x10];
      break;
    case 0x11:
      fVar24 = pfVar9[0x11];
      break;
    case 0x12:
      fVar24 = pfVar9[0x12];
      break;
    case 0x13:
      fVar24 = pfVar9[0x13];
      break;
    case 0x14:
      fVar24 = pfVar9[0x14];
      break;
    case 0x15:
      fVar24 = pfVar9[0x15];
      break;
    case 0x16:
      fVar24 = pfVar9[0x16];
      break;
    case 0x17:
      fVar24 = pfVar9[0x17];
      break;
    case 0x18:
      fVar24 = pfVar9[0x18];
      break;
    case 0x19:
      fVar24 = pfVar9[0x19];
      break;
    case 0x1a:
      fVar24 = pfVar9[0x1a];
      break;
    case 0x1b:
      fVar24 = pfVar9[0x1b];
      break;
    case 0x1c:
      fVar24 = pfVar9[0x1c];
      break;
    case 0x1d:
      fVar24 = pfVar9[0x1d];
      break;
    case 0x1e:
      fVar24 = pfVar9[0x1e];
      break;
    case 0x1f:
      fVar24 = pfVar9[0x1f];
      break;
    case 0x20:
      fVar24 = pfVar9[0x20];
      break;
    case 0x21:
      fVar24 = pfVar9[0x21];
      break;
    case 0x22:
      fVar24 = pfVar9[0x22];
      break;
    case 0x23:
      fVar24 = pfVar9[0x23];
      break;
    case 0x24:
      fVar24 = pfVar9[0x24];
      break;
    case 0x32:
      pfVar18 = DAT_1fff93d0;
      goto LAB_1fff91a4;
    case 0x33:
      fVar24 = DAT_1fff93d0[1];
      break;
    case 0x34:
      fVar24 = DAT_1fff93d0[2];
      break;
    case 0x35:
      fVar24 = DAT_1fff93d0[3];
      break;
    case 0x36:
      pfVar18 = DAT_1fff93d4;
LAB_1fff91a4:
      fVar24 = *pfVar18;
      break;
    case 0x37:
      fVar24 = *(float *)(iVar6 + 0x34);
      break;
    case 0x50:
      if (*(int *)(iVar6 + 0x34) == 0x3f800000) {
        fVar24 = *(float *)(iVar6 + 0x18);
      }
      else {
        fVar24 = -*(float *)(iVar6 + 0x18);
      }
      break;
    case 0x51:
      if (*(int *)(iVar6 + 0x34) == 0x3f800000) {
        fVar24 = *(float *)(DAT_1fff93d8 + 0x3c);
      }
      else {
        fVar24 = -*(float *)(DAT_1fff93d8 + 0x3c);
      }
    }
    *(byte *)(puVar13 + 2) = bVar5;
    *(char *)((int)puVar13 + 9) = (char)((uint)fVar21 >> 8);
    *(undefined1 *)((int)puVar13 + 10) = 0x33;
    *(undefined1 *)((int)puVar13 + 0xb) = uVar4;
    *(char *)(puVar13 + 3) = SUB41(fVar24,0);
    *(char *)((int)puVar13 + 0xd) = (char)((uint)fVar24 >> 8);
    *(char *)((int)puVar13 + 0xe) = (char)((uint)fVar24 >> 0x10);
    *(char *)((int)puVar13 + 0xf) = (char)((uint)fVar24 >> 0x18);
    (*(code *)puVar13[1])(puVar13 + 2,*(undefined2 *)(pfVar9 + 7),8);
    goto switchD_1fff8dba_default;
  }
  switch(uVar22) {
  case 0:
    if (*(int *)(iVar6 + 0x34) == 0x3f800000) {
      uVar15 = float_to_uint_packed(*(float *)(iVar6 + 0x18),-pfVar9[0x15],0x10);
      *(undefined2 *)(iVar16 + 2) = uVar15;
      uVar15 = float_to_uint_packed(*(undefined4 *)(iVar6 + 0x1c),-pfVar9[0x16],0xc);
      *(undefined2 *)(iVar16 + 4) = uVar15;
      uVar15 = float_to_uint_packed(*(undefined4 *)(iVar6 + 0x30),-pfVar9[0x17],0xc);
      *(undefined2 *)(iVar16 + 6) = uVar15;
    }
    else {
      uVar15 = float_to_uint_packed(-*(float *)(iVar6 + 0x18),-pfVar9[0x15],0x10);
      *(undefined2 *)(iVar16 + 2) = uVar15;
      uVar15 = float_to_uint_packed(-*(float *)(iVar6 + 0x1c),-pfVar9[0x16],0xc);
      *(undefined2 *)(iVar16 + 4) = uVar15;
      uVar15 = float_to_uint_packed(-*(float *)(iVar6 + 0x30),-pfVar9[0x17],0xc);
      *(undefined2 *)(iVar16 + 6) = uVar15;
    }
    *(byte *)(puVar13 + 2) = *(byte *)(pfVar9 + 8) | *(char *)(pfVar18 + 0x20) << 4;
    uVar15 = *(undefined2 *)(iVar16 + 2);
    *(char *)((int)puVar13 + 9) = (char)((ushort)uVar15 >> 8);
    *(char *)((int)puVar13 + 10) = (char)uVar15;
    uVar14 = *(ushort *)(iVar16 + 4);
    *(char *)((int)puVar13 + 0xb) = (char)(uVar14 >> 4);
    uVar15 = *(undefined2 *)(iVar16 + 6);
    *(char *)(puVar13 + 3) = (char)uVar14 * '\x10' + (char)((ushort)uVar15 >> 8);
    *(char *)((int)puVar13 + 0xd) = (char)uVar15;
    *(char *)((int)puVar13 + 0xe) = (char)(int)pfVar18[0x21];
    *(char *)((int)puVar13 + 0xf) = (char)(int)*(float *)(iVar6 + 0x40);
    (*(code *)puVar13[1])(puVar13 + 2,*(undefined2 *)(pfVar9 + 7),8);
    goto switchD_1fff8dba_default;
  case 1:
    *(byte *)(puVar13 + 2) = bVar5;
    *(undefined1 *)((int)puVar13 + 9) = *(undefined1 *)(pfVar18 + 0x20);
    *(char *)((int)puVar13 + 10) = (char)(int)pfVar18[0x21];
    *(char *)((int)puVar13 + 0xb) = (char)(int)*(float *)(iVar6 + 0x40);
    if (*(int *)(iVar6 + 0x34) == 0x3f800000) {
      fVar21 = *(float *)(iVar6 + 0x18);
    }
    else {
      fVar21 = -*(float *)(iVar6 + 0x18);
    }
    break;
  case 2:
    *(byte *)(puVar13 + 2) = bVar5;
    *(undefined1 *)((int)puVar13 + 9) = *(undefined1 *)(pfVar18 + 0x20);
    *(char *)((int)puVar13 + 10) = (char)(int)pfVar18[0x21];
    *(char *)((int)puVar13 + 0xb) = (char)(int)*(float *)(iVar6 + 0x40);
    if (*(int *)(iVar6 + 0x34) == 0x3f800000) {
      fVar21 = *(float *)(iVar6 + 0x1c);
    }
    else {
      fVar21 = -*(float *)(iVar6 + 0x1c);
    }
    break;
  case 3:
    *(byte *)(puVar13 + 2) = bVar5;
    *(undefined1 *)((int)puVar13 + 9) = *(undefined1 *)(pfVar18 + 0x20);
    *(char *)((int)puVar13 + 10) = (char)(int)pfVar18[0x21];
    *(char *)((int)puVar13 + 0xb) = (char)(int)*(float *)(iVar6 + 0x40);
    if (*(int *)(iVar6 + 0x34) == 0x3f800000) {
      fVar21 = *(float *)(iVar6 + 0x30);
    }
    else {
      fVar21 = -*(float *)(iVar6 + 0x30);
    }
    break;
  case 4:
    if (*(int *)(iVar6 + 0x34) == 0x3f800000) {
      fVar21 = *(float *)(iVar6 + 0x18);
      uVar15 = float_to_uint_packed(*(undefined4 *)(iVar6 + 0x1c),-pfVar9[0x16],0xc);
      *(undefined2 *)(iVar16 + 4) = uVar15;
      *(short *)(iVar16 + 0xc) = (short)(int)(pfVar18[0x19] * DAT_1fff93cc);
    }
    else {
      fVar21 = -*(float *)(iVar6 + 0x18);
      uVar15 = float_to_uint_packed(-*(float *)(iVar6 + 0x1c),-pfVar9[0x16],0xc);
      *(undefined2 *)(iVar16 + 4) = uVar15;
      *(short *)(iVar16 + 0xc) = (short)(int)(pfVar18[0x19] * DAT_1fff93c8);
    }
    *(char *)(puVar13 + 2) = SUB41(fVar21,0);
    *(char *)((int)puVar13 + 9) = (char)((uint)fVar21 >> 8);
    *(char *)((int)puVar13 + 10) = (char)((uint)fVar21 >> 0x10);
    *(char *)((int)puVar13 + 0xb) = (char)((uint)fVar21 >> 0x18);
    uVar15 = *(undefined2 *)(iVar16 + 4);
    *(char *)(puVar13 + 3) = (char)uVar15;
    *(char *)((int)puVar13 + 0xd) = (char)((ushort)uVar15 >> 8);
    uVar15 = *(undefined2 *)(iVar16 + 0xc);
    *(char *)((int)puVar13 + 0xe) = (char)uVar15;
    *(char *)((int)puVar13 + 0xf) = (char)((ushort)uVar15 >> 8);
    (*(code *)puVar13[1])(puVar13 + 2,*(undefined2 *)(pfVar9 + 7),8);
  default:
    goto switchD_1fff8dba_default;
  }
  *(char *)(puVar13 + 3) = SUB41(fVar21,0);
  *(char *)((int)puVar13 + 0xd) = (char)((uint)fVar21 >> 8);
  *(char *)((int)puVar13 + 0xe) = (char)((uint)fVar21 >> 0x10);
  *(char *)((int)puVar13 + 0xf) = (char)((uint)fVar21 >> 0x18);
  (*(code *)puVar13[1])(puVar13 + 2,*(undefined2 *)(pfVar9 + 7),8);
switchD_1fff8dba_default:
  if ((*(uint *)(iVar8 + 0x50) & 0x800000) != 0) {
    (*(code *)*DAT_1fff9878)(*(undefined2 *)(pfVar9 + 0x23),*(undefined2 *)(pfVar9 + 8));
    puVar10[0xc] = 1;
  }
  if ((*(uint *)(iVar8 + 0x50) & 0x2000000) != 0) {
    *(uint *)(iVar8 + 0x18) = *(uint *)(iVar8 + 0x18) & 0xfffffffe;
    puVar10[0xc] = 2;
  }
  *(undefined4 *)(iVar8 + 0x50) = 0xffffffff;
  DAT_e000e280 = 8;
  return;
}



/* ===== output_sensor_lookup_and_unwrap @ 1fff987c, 182 bytes ===== */

void output_sensor_lookup_and_unwrap(int param_1)

{
  int iVar1;
  uint in_fpscr;
  float fVar2;
  int iVar3;
  float fVar4;
  
  fVar2 = (float)thunk_FUN_00023be8(*(float *)(param_1 + 0x18) -
                                    *(float *)(param_1 + 0x10) * *(float *)(param_1 + 0x30),
                                    *(float *)(param_1 + 0x10) * *(float *)(param_1 + 0x2c));
  iVar1 = DAT_1fff9944;
  iVar3 = (int)(DAT_1fff9938 + fVar2 * DAT_1fff9934);
  if (iVar3 < 0) {
    iVar3 = iVar3 + 0x1000;
  }
  if (0xfff < iVar3) {
    iVar3 = iVar3 + -0x1000;
  }
  fVar2 = (float)VectorUnsignedToFloat
                           ((uint)*(ushort *)(DAT_1fff993c + iVar3 * 2),(byte)(in_fpscr >> 0x16) & 3
                           );
  fVar2 = (fVar2 - DAT_1fff9938) * DAT_1fff9940;
  *(float *)(param_1 + 0x34) = fVar2;
  fVar4 = fVar2 - *(float *)(param_1 + 0x40);
  if (iVar1 < (int)fVar4) {
    *(int *)(param_1 + 0x44) = *(int *)(param_1 + 0x44) + -1;
  }
  if ((uint)DAT_1fff9948 < (uint)fVar4) {
    *(int *)(param_1 + 0x44) = *(int *)(param_1 + 0x44) + 1;
  }
  *(float *)(param_1 + 0x40) = fVar2;
  fVar4 = (float)VectorSignedToFloat(*(undefined4 *)(param_1 + 0x44),(byte)(in_fpscr >> 0x16) & 3);
  *(float *)(param_1 + 0x3c) = (fVar2 + fVar4 * DAT_1fff994c) - *(float *)(param_1 + 0x38);
  return;
}



/* ===== flash_program_words @ 1fff9950, 232 bytes ===== */

void flash_program_words(undefined4 *param_1,undefined4 *param_2,uint param_3)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  int *piVar3;
  uint uVar4;
  undefined4 uVar5;
  uint uVar6;
  int *piVar7;
  undefined4 *puVar8;
  uint *puVar9;
  uint *puVar10;
  
  puVar1 = DAT_1fff9ac0;
  *DAT_1fff9ac0 = 0x123;
  *puVar1 = 0x3210;
  uVar6 = DAT_1fff9ac4;
  puVar1[1] = DAT_1fff9ac4;
  puVar1[1] = ~uVar6;
  puVar10 = DAT_1fff9ac0 + 6;
  uVar6 = *puVar10;
  *puVar10 = *puVar10 & 0xfff0ffff;
  puVar2 = DAT_1fff9ac0;
  puVar8 = DAT_1fff9ac0 + 9;
  *puVar8 = 0x3f;
  piVar7 = puVar2 + 8;
  do {
  } while (-1 < *piVar7 << 0x17);
  puVar9 = DAT_1fff9ac0 + 7;
  *puVar9 = (*puVar9 & 0xfffffff8) + 4;
  piVar3 = DAT_1fff9ac8;
  *DAT_1fff9ac8 = 1 << ((uint)param_1 >> 0xd & 0xff);
  *param_1 = 0;
  do {
  } while (-1 < *piVar7 << 0x17);
  *puVar8 = 0x10;
  *puVar9 = (*puVar9 & 0xfffffff8) + 3;
  for (uVar4 = 0; uVar4 < param_3; uVar4 = uVar4 + 1) {
    uVar5 = *param_2;
    param_2 = param_2 + 1;
    *param_1 = uVar5;
    param_1 = param_1 + 1;
    do {
    } while (-1 < *piVar7 << 0x1b);
    *puVar8 = 0x10;
  }
  *puVar9 = *puVar9 & 0xfffffff8;
  while (-1 < *piVar7 << 0x17) {
    *puVar8 = 0x10;
  }
  *piVar3 = 0;
  *puVar10 = *puVar10 | uVar6 & 0x70000;
  *puVar9 = *puVar9 | 0x10000;
  *puVar1 = 0x3210;
  return;
}



/* ===== flash_erase_sector @ 1fff9a38, 136 bytes ===== */

void flash_erase_sector(undefined4 *param_1)

{
  undefined4 *puVar1;
  undefined4 *puVar2;
  uint uVar3;
  uint *puVar4;
  uint *puVar5;
  undefined4 *puVar6;
  
  puVar1 = DAT_1fff9ac0;
  *DAT_1fff9ac0 = 0x123;
  *puVar1 = 0x3210;
  uVar3 = DAT_1fff9ac4;
  puVar1[1] = DAT_1fff9ac4;
  puVar1[1] = ~uVar3;
  puVar4 = DAT_1fff9ac0 + 6;
  uVar3 = *puVar4;
  *puVar4 = *puVar4 & 0xfff0ffff;
  puVar2 = DAT_1fff9ac0;
  puVar6 = DAT_1fff9ac0 + 9;
  *puVar6 = 0x3f;
  do {
  } while (-1 < (int)(puVar2[8] << 0x17));
  puVar5 = DAT_1fff9ac0 + 7;
  *puVar5 = (*puVar5 & 0xfffffff8) + 4;
  *DAT_1fff9ac8 = 1 << ((uint)param_1 >> 0xd & 0xff);
  *param_1 = 0;
  do {
  } while (-1 < (int)(puVar2[8] << 0x17));
  *puVar6 = 0x10;
  *puVar4 = *puVar4 | uVar3 & 0x70000;
  *puVar5 = *puVar5 | 0x10000;
  *puVar1 = 0x3210;
  return;
}



/* ===== mcan_write_classic_tx_element @ 1fff9acc, 66 bytes ===== */

void mcan_write_classic_tx_element(uint *param_1,int param_2,int param_3)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  
  iVar1 = DAT_1fff9c34;
  uVar3 = (*(uint *)(DAT_1fff9c34 + 0xc4) & 0x1fffff) >> 0x10;
  iVar2 = DAT_1fff9c38 + uVar3 * 0x48;
  *(int *)(iVar2 + 0x328) = param_2 << 0x12;
  *(int *)(iVar2 + 0x32c) = param_3 << 0x10;
  *(uint *)(iVar2 + 0x330) = *param_1 & 0xffffff | (uint)*(byte *)((int)param_1 + 3) << 0x18;
  *(uint *)(iVar2 + 0x334) = param_1[1] & 0xffffff | (uint)*(byte *)((int)param_1 + 7) << 0x18;
  *(int *)(iVar1 + 0xd0) = 1 << uVar3;
  return;
}



/* ===== mcan_write_fd_tx_element @ 1fff9b0e, 146 bytes ===== */

void mcan_write_fd_tx_element(int param_1,int param_2,uint param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint *puVar4;
  uint uVar5;
  
  iVar1 = DAT_1fff9c34;
  uVar5 = (*(uint *)(DAT_1fff9c34 + 0xc4) & 0x1fffff) >> 0x10;
  uVar3 = param_3;
  if (8 < param_3) {
    if (param_3 < 0xd) {
      uVar3 = 9;
    }
    else if (param_3 < 0x11) {
      uVar3 = 10;
    }
    else if (param_3 < 0x15) {
      uVar3 = 0xb;
    }
    else if (param_3 < 0x19) {
      uVar3 = 0xc;
    }
    else if (param_3 < 0x21) {
      uVar3 = 0xd;
    }
    else if (param_3 < 0x31) {
      uVar3 = 0xe;
    }
    else {
      uVar3 = param_4;
      if (param_3 < 0x41) {
        uVar3 = 0xf;
      }
    }
  }
  iVar2 = DAT_1fff9c38 + uVar5 * 0x48;
  *(int *)(iVar2 + 0x328) = param_2 << 0x12;
  *(uint *)(iVar2 + 0x32c) = uVar3 << 0x10 | 0x200000;
  puVar4 = (uint *)(DAT_1fff9c3c + uVar5 * 0x48);
  for (uVar3 = 0; uVar3 < param_3; uVar3 = uVar3 + 4 & 0xff) {
    *puVar4 = *(uint *)(param_1 + uVar3) & 0xffffff |
              (uint)*(byte *)((int)(param_1 + uVar3) + 3) << 0x18;
    puVar4 = puVar4 + 1;
  }
  *(int *)(iVar1 + 0xd0) = 1 << uVar5;
  return;
}



/* ===== mcan_write_fd_brs_tx_element @ 1fff9ba0, 146 bytes ===== */

void mcan_write_fd_brs_tx_element(int param_1,int param_2,uint param_3,uint param_4)

{
  int iVar1;
  int iVar2;
  uint uVar3;
  uint *puVar4;
  uint uVar5;
  
  iVar1 = DAT_1fff9c34;
  uVar5 = (*(uint *)(DAT_1fff9c34 + 0xc4) & 0x1fffff) >> 0x10;
  uVar3 = param_3;
  if (8 < param_3) {
    if (param_3 < 0xd) {
      uVar3 = 9;
    }
    else if (param_3 < 0x11) {
      uVar3 = 10;
    }
    else if (param_3 < 0x15) {
      uVar3 = 0xb;
    }
    else if (param_3 < 0x19) {
      uVar3 = 0xc;
    }
    else if (param_3 < 0x21) {
      uVar3 = 0xd;
    }
    else if (param_3 < 0x31) {
      uVar3 = 0xe;
    }
    else {
      uVar3 = param_4;
      if (param_3 < 0x41) {
        uVar3 = 0xf;
      }
    }
  }
  iVar2 = DAT_1fff9c38 + uVar5 * 0x48;
  *(int *)(iVar2 + 0x328) = param_2 << 0x12;
  *(uint *)(iVar2 + 0x32c) = uVar3 << 0x10 | 0x300000;
  puVar4 = (uint *)(DAT_1fff9c3c + uVar5 * 0x48);
  for (uVar3 = 0; uVar3 < param_3; uVar3 = uVar3 + 4 & 0xff) {
    *puVar4 = *(uint *)(param_1 + uVar3) & 0xffffff |
              (uint)*(byte *)((int)(param_1 + uVar3) + 3) << 0x18;
    puVar4 = puVar4 + 1;
  }
  *(int *)(iVar1 + 0xd0) = 1 << uVar5;
  return;
}



/* ===== svpwm_write_compare @ 1fff9c40, 260 bytes ===== */

void svpwm_write_compare(float param_1,float param_2)

{
  float fVar1;
  int iVar2;
  short sVar3;
  float fVar4;
  undefined4 uVar5;
  float local_10;
  float local_c;
  
  iVar2 = DAT_1fff9d54;
  fVar1 = DAT_1fff9d50;
  fVar4 = param_1 * DAT_1fff9d4c + param_2 * 0.5;
  local_10 = fVar4 - param_2;
  if (fVar4 <= 0.0) {
    sVar3 = 3;
  }
  else {
    sVar3 = 2;
  }
  if (0.0 < local_10) {
    sVar3 = sVar3 + -1;
  }
  if (param_2 < 0.0) {
    sVar3 = 7 - sVar3;
  }
  switch(sVar3) {
  default:
    local_10 = DAT_1fff9d58;
    fVar4 = DAT_1fff9d58;
    local_c = local_10;
    goto LAB_1fff9cbe;
  case 1:
  case 4:
    local_c = param_2 - local_10;
    local_10 = fVar4;
    break;
  case 2:
  case 5:
    local_10 = local_10 + fVar4;
    fVar4 = -param_2;
    local_c = param_2;
    goto LAB_1fff9cbe;
  case 3:
  case 6:
    fVar4 = param_2 + fVar4;
    local_c = -local_10;
  }
  fVar4 = -fVar4;
LAB_1fff9cbe:
  uVar5 = VectorFloatToUnsigned((1.0 - fVar4) * DAT_1fff9d50,3);
  *(short *)(DAT_1fff9d54 + 0x14) = (short)uVar5;
  uVar5 = VectorFloatToUnsigned((1.0 - local_c) * fVar1,3);
  *(short *)(iVar2 + 0xc) = (short)uVar5;
  uVar5 = VectorFloatToUnsigned((1.0 - local_10) * fVar1,3);
  *(short *)(iVar2 + 4) = (short)uVar5;
  return;
}



/* ===== reset_control_state @ 1fff9dc8, 124 bytes ===== */

void reset_control_state(void)

{
  undefined4 uVar1;
  int iVar2;
  int iVar3;
  
  iVar3 = DAT_1fffa134;
  iVar2 = DAT_1fffa11c;
  uVar1 = DAT_1fffa118;
  *(undefined4 *)(DAT_1fffa11c + 0x1c) = DAT_1fffa118;
  *(undefined4 *)(iVar2 + 0x20) = uVar1;
  *(undefined4 *)(iVar2 + 0x2c) = uVar1;
  iVar2 = DAT_1fffa130;
  *(undefined4 *)(DAT_1fffa130 + 0xc) = uVar1;
  *(undefined4 *)(iVar3 + 0xc) = uVar1;
  *(undefined4 *)(iVar2 + 0x14) = uVar1;
  *(undefined4 *)(iVar3 + 0x14) = uVar1;
  *(undefined4 *)(iVar2 + 0x18) = uVar1;
  *(undefined4 *)(iVar3 + 0x18) = uVar1;
  *(undefined4 *)(iVar2 + 0x34) = uVar1;
  *(undefined4 *)(iVar3 + 0x34) = uVar1;
  *(undefined4 *)(iVar2 + 0x38) = uVar1;
  *(undefined4 *)(iVar3 + 0x38) = uVar1;
  *(undefined4 *)(iVar2 + 0x40) = uVar1;
  *(undefined4 *)(iVar3 + 0x40) = uVar1;
  *(undefined4 *)(iVar2 + 0x3c) = uVar1;
  iVar2 = DAT_1fffa128;
  *(undefined4 *)(iVar3 + 0x3c) = uVar1;
  *(undefined4 *)(iVar2 + 8) = uVar1;
  *(undefined4 *)(iVar2 + 0x10) = uVar1;
  *(undefined4 *)(iVar2 + 0x1c) = uVar1;
  *(undefined4 *)(iVar2 + 0x14) = uVar1;
  *(undefined4 *)(iVar2 + 0x18) = uVar1;
  iVar2 = DAT_1fffa12c;
  *(undefined4 *)(DAT_1fffa12c + 8) = uVar1;
  *(undefined4 *)(iVar2 + 0x10) = uVar1;
  *(undefined4 *)(iVar2 + 0x1c) = uVar1;
  *(undefined4 *)(iVar2 + 0x14) = uVar1;
  *(undefined4 *)(iVar2 + 0x18) = uVar1;
  return;
}



/* ===== identification_filter_step @ 1fff9e44, 98 bytes ===== */

void identification_filter_step(float *param_1)

{
  float fVar1;
  float fVar2;
  
  fVar1 = *param_1 * param_1[2];
  param_1[3] = fVar1;
  fVar2 = param_1[4] + param_1[1] * fVar1 + param_1[7] * DAT_1fffa138;
  param_1[4] = fVar2;
  fVar1 = fVar1 + fVar2;
  param_1[5] = fVar1;
  fVar2 = param_1[8];
  if (param_1[8] <= fVar1) {
    fVar2 = param_1[9];
    if (fVar1 <= param_1[9]) {
      fVar2 = fVar1;
    }
  }
  param_1[6] = fVar2;
  param_1[7] = fVar2 - fVar1;
  return;
}



/* ===== configure_runtime_control_parameters_variant @ 1fff9ea6, 312 bytes ===== */

void configure_runtime_control_parameters_variant(void)

{
  uint uVar1;
  byte bVar2;
  float *pfVar3;
  float *pfVar4;
  float *pfVar5;
  float *pfVar6;
  float *pfVar7;
  int iVar8;
  uint in_fpscr;
  float fVar9;
  float fVar10;
  float fVar11;
  float fVar12;
  float fVar13;
  float fVar14;
  
  iVar8 = DAT_1fffa140;
  pfVar7 = DAT_1fffa13c;
  pfVar6 = DAT_1fffa134;
  pfVar5 = DAT_1fffa130;
  pfVar4 = DAT_1fffa124;
  pfVar3 = DAT_1fffa120;
  fVar11 = DAT_1fffa118;
  fVar12 = *(float *)(DAT_1fffa11c + 0x68);
  fVar10 = *DAT_1fffa13c;
  fVar9 = *(float *)(DAT_1fffa140 + 8);
  uVar1 = in_fpscr & 0xfffffff | (uint)(fVar12 < fVar10) << 0x1f | (uint)(fVar12 == fVar10) << 0x1e;
  bVar2 = (byte)(uVar1 >> 0x18);
  if ((bool)(bVar2 >> 6 & 1) || (bool)(bVar2 >> 7) != (NAN(fVar12) || NAN(fVar10))) {
    *DAT_1fffa120 = DAT_1fffa118;
    *pfVar4 = fVar11;
    pfVar3[1] = fVar11;
    pfVar4[1] = fVar11;
    *pfVar5 = fVar11;
    *pfVar6 = fVar11;
  }
  else {
    fVar13 = DAT_1fffa13c[0x18];
    fVar11 = DAT_1fffa13c[0x12];
    fVar10 = ((fVar13 * fVar11 * fVar9) / fVar12) * DAT_1fffa144;
    *DAT_1fffa120 = fVar10;
    *pfVar4 = fVar10;
    fVar10 = *(float *)(iVar8 + 0xc);
    fVar14 = (pfVar7[0x11] / fVar11) * fVar10;
    pfVar3[1] = fVar14;
    pfVar4[1] = fVar14;
    pfVar5[4] = fVar10;
    pfVar6[4] = fVar10;
    fVar11 = (fVar12 * DAT_1fffa148) / (fVar9 * fVar11);
    pfVar5[8] = fVar11;
    pfVar6[8] = fVar11;
    pfVar5[9] = 1.0 / fVar11;
    pfVar6[9] = 1.0 / fVar11;
    *pfVar5 = fVar13;
    *pfVar6 = fVar13;
    fVar11 = DAT_1fffa14c;
    pfVar5[7] = DAT_1fffa14c;
    pfVar6[7] = fVar11;
    fVar11 = DAT_1fffa150;
    pfVar5[0xb] = DAT_1fffa150;
    pfVar6[0xb] = fVar11;
    fVar11 = DAT_1fffa154;
    pfVar5[0xc] = DAT_1fffa154;
    pfVar6[0xc] = fVar11;
    pfVar5[0x11] = -1.0;
    pfVar6[0x11] = -1.0;
    pfVar5[0x12] = 1.0;
    pfVar6[0x12] = 1.0;
  }
  fVar11 = (float)VectorUnsignedToFloat(pfVar7[0x10],(byte)(uVar1 >> 0x16) & 3);
  if (pfVar7[1] == 0.0) {
    fVar9 = pfVar7[0x14] * fVar11 * 1.5 * pfVar7[0x13] * fVar9 * pfVar7[0x1e];
  }
  else {
    fVar9 = pfVar7[1] * fVar9;
  }
  *(float *)(DAT_1fffa158 + 0x44) = fVar9;
  return;
}



/* ===== motion_observer_step @ 1fff9fde, 156 bytes ===== */

void motion_observer_step(float *param_1)

{
  float fVar1;
  float fVar2;
  
  fVar1 = *param_1 - param_1[5];
  param_1[2] = fVar1;
  fVar2 = (fVar1 - param_1[3]) * DAT_1fffa15c;
  param_1[4] = fVar2;
  param_1[3] = fVar1;
  param_1[5] = param_1[5] +
               param_1[7] * (param_1[6] + param_1[8] * fVar1 + param_1[10] * param_1[1]);
  fVar1 = param_1[6] + param_1[7] * param_1[9] * (fVar2 + param_1[8] * fVar1);
  param_1[6] = fVar1;
  fVar1 = fVar1 * param_1[0xb];
  param_1[0xc] = fVar1;
  fVar2 = param_1[0xe];
  if (fVar2 < fVar1) {
    param_1[0xc] = fVar2;
    fVar1 = fVar2;
  }
  if (fVar1 < param_1[0xd]) {
    param_1[0xc] = param_1[0xd];
  }
  return;
}



/* ===== current_controller_step @ 1fffa07a, 158 bytes ===== */

void current_controller_step(float *param_1)

{
  float fVar1;
  float fVar2;
  
  fVar1 = param_1[2] - param_1[5];
  param_1[3] = fVar1;
  fVar2 = param_1[5] + param_1[4] * param_1[6] + param_1[0xb] * fVar1 + param_1[10] * param_1[0xf];
  param_1[5] = fVar2;
  fVar1 = param_1[6] + param_1[0xc] * fVar1;
  param_1[6] = fVar1;
  fVar2 = param_1[1] - fVar2;
  param_1[0xd] = fVar2;
  fVar2 = *param_1 * fVar2;
  param_1[0xe] = fVar2;
  fVar1 = (fVar2 - fVar1) * param_1[9];
  param_1[0xf] = fVar1;
  fVar2 = param_1[0x11];
  if (fVar1 < fVar2) {
    param_1[0xf] = fVar2;
    fVar1 = fVar2;
  }
  fVar2 = param_1[0x12];
  if (fVar2 < fVar1) {
    param_1[0xf] = fVar2;
    fVar1 = fVar2;
  }
  param_1[0x10] = fVar1;
  return;
}



/* ===== identification_rls2_step @ 1fffa160, 212 bytes ===== */

void identification_rls2_step(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  float fVar7;
  float fVar8;
  float fVar9;
  float fVar10;
  float fVar11;
  
  fVar2 = param_1[1];
  fVar8 = param_1[5];
  fVar1 = param_1[2];
  fVar7 = param_1[6];
  fVar6 = param_1[7];
  fVar3 = fVar8 * fVar2 + fVar7 * fVar1;
  fVar5 = param_1[8];
  fVar4 = fVar6 * fVar2 + fVar5 * fVar1;
  fVar9 = (*param_1 - fVar2 * param_1[3]) - fVar1 * param_1[4];
  fVar10 = 1.0 / (fVar2 * fVar3 + fVar1 * fVar4 + DAT_1fffa300);
  fVar3 = fVar3 * fVar10;
  fVar4 = fVar4 * fVar10;
  fVar11 = (1.0 - fVar3 * fVar2) * DAT_1fffa304;
  fVar10 = fVar3 * DAT_1fffa308 * fVar1;
  fVar2 = fVar4 * DAT_1fffa308 * fVar2;
  fVar1 = (1.0 - fVar4 * fVar1) * DAT_1fffa304;
  param_1[6] = fVar11 * fVar7 + fVar10 * fVar5;
  param_1[7] = fVar2 * fVar8 + fVar1 * fVar6;
  param_1[8] = fVar2 * fVar7 + fVar1 * fVar5;
  param_1[1] = *param_1;
  param_1[3] = param_1[3] + fVar3 * fVar9;
  param_1[4] = param_1[4] + fVar4 * fVar9;
  param_1[5] = fVar11 * fVar8 + fVar10 * fVar6;
  return;
}



/* ===== identification_flux_observer_step @ 1fffa234, 202 bytes ===== */

void identification_flux_observer_step(float *param_1)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  float fVar5;
  
  fVar3 = param_1[7];
  fVar1 = param_1[0x14];
  fVar4 = 1.0 - param_1[0xe] * fVar1;
  fVar2 = (param_1[0x10] * param_1[5] + fVar3 * param_1[9]) * fVar1 + fVar4 * param_1[8];
  param_1[8] = fVar2;
  fVar4 = ((param_1[0x10] * param_1[6] - fVar3 * fVar2) - param_1[0xf] * fVar3) * fVar1 +
          fVar4 * param_1[9];
  param_1[9] = fVar4;
  param_1[10] = param_1[3] - fVar2;
  fVar5 = param_1[4] - fVar4;
  param_1[0xb] = fVar5;
  fVar2 = param_1[0xc] + fVar1 * (fVar2 * (param_1[3] - fVar2) + fVar4 * fVar5);
  param_1[0xc] = fVar2;
  fVar3 = param_1[0xd] + fVar1 * fVar3 * fVar5;
  param_1[0xd] = fVar3;
  fVar1 = param_1[0x11] - param_1[1] * fVar2;
  param_1[0xe] = fVar1;
  fVar2 = param_1[0x12] - param_1[2] * fVar3;
  param_1[0xf] = fVar2;
  param_1[0x15] = *param_1 * fVar1;
  param_1[0x16] = *param_1 * fVar2;
  return;
}



/* ===== fast_sincos_lut @ 1fffa30c, 126 bytes ===== */

void fast_sincos_lut(float param_1,float *param_2,float *param_3)

{
  int iVar1;
  float *pfVar2;
  uint in_fpscr;
  uint uVar3;
  float fVar4;
  float fVar5;
  float fVar6;
  
  iVar1 = DAT_1fffa4c8;
  fVar5 = DAT_1fffa4c0 + param_1 * DAT_1fffa4bc;
  if ((fVar5 != 2048.0) && (fVar5 != 0.0)) {
    uVar3 = VectorFloatToUnsigned(fVar5,3);
    uVar3 = uVar3 & 0xffff;
    pfVar2 = (float *)(DAT_1fffa4c8 + uVar3 * 4);
    fVar4 = (float)VectorUnsignedToFloat(uVar3,(byte)((in_fpscr & 0xfffffff) >> 0x16) & 3);
    fVar6 = *pfVar2;
    *param_2 = fVar6 + (fVar5 - fVar4) * (pfVar2[1] - fVar6);
    pfVar2 = (float *)(iVar1 + (uVar3 + 0x200 & 0x7ff) * 4);
    fVar6 = *pfVar2;
    *param_3 = fVar6 + (fVar5 - fVar4) * (pfVar2[1] - fVar6);
    return;
  }
  *param_2 = DAT_1fffa4c4;
  *param_3 = -1.0;
  return;
}



/* ===== clampf @ 1fffa38a, 32 bytes ===== */

float clampf(float param_1,float param_2,float param_3)

{
  if ((param_1 <= param_3) && (param_3 = param_1, param_1 < param_2)) {
    return param_2;
  }
  return param_3;
}



/* ===== wrap_periodic_value @ 1fffa3aa, 52 bytes ===== */

void wrap_periodic_value(float param_1,float param_2,float *param_3)

{
  float fVar1;
  
  fVar1 = *param_3;
  if (param_2 < fVar1) {
    fVar1 = fVar1 - (param_2 - param_1);
    *param_3 = fVar1;
  }
  if (fVar1 < param_1) {
    *param_3 = fVar1 + (param_2 - param_1);
  }
  return;
}



/* ===== limit_vector_magnitude @ 1fffa414, 86 bytes ===== */

void limit_vector_magnitude(float param_1,float *param_2,float *param_3)

{
  float fVar1;
  
  fVar1 = (float)thunk_FUN_00023fd0(*param_2 * *param_2 + *param_3 * *param_3);
  if (param_1 < fVar1) {
    *param_2 = (*param_2 * param_1) / fVar1;
    *param_3 = (*param_3 * param_1) / fVar1;
  }
  return;
}



/* ===== float_to_uint_packed @ 1fffa46a, 40 bytes ===== */

int float_to_uint_packed(float param_1,float param_2,float param_3,uint param_4)

{
  uint in_fpscr;
  float fVar1;
  
  fVar1 = (float)VectorSignedToFloat((1 << (param_4 & 0xff)) + -1,(byte)(in_fpscr >> 0x16) & 3);
  return (int)(((param_1 - param_2) * fVar1) / (param_3 - param_2));
}



/* ===== uint_to_float_packed @ 1fffa492, 40 bytes ===== */

float uint_to_float_packed(float param_1,float param_2,undefined4 param_3,uint param_4)

{
  uint in_fpscr;
  float fVar1;
  float fVar2;
  
  fVar1 = (float)VectorSignedToFloat(param_3,(byte)(in_fpscr >> 0x16) & 3);
  fVar2 = (float)VectorSignedToFloat((1 << (param_4 & 0xff)) + -1,(byte)(in_fpscr >> 0x16) & 3);
  return (fVar1 * (param_2 - param_1)) / fVar2 + param_1;
}



/* ===== derive_control_parameters @ 1fffa4dc, 10 bytes ===== */

void derive_control_parameters(void)

{
  derive_control_parameters();
  return;
}



/* ===== delay_ms @ 1fffa4e6, 10 bytes ===== */

void delay_ms(void)

{
  delay_ms();
  return;
}



/* ===== select_configuration_bank_b @ 1fffa4f0, 10 bytes ===== */

void select_configuration_bank_b(void)

{
  select_configuration_bank_b();
  return;
}



/* ===== thunk_FUN_00023be8 @ 1fffa4fa, 10 bytes ===== */

void thunk_FUN_00023be8(void)

{
  FUN_00023be8();
  return;
}



/* ===== thunk_FUN_00023fd0 @ 1fffa504, 10 bytes ===== */

void thunk_FUN_00023fd0(void)

{
  FUN_00023fd0();
  return;
}


