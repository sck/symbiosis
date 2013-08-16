namespace symbiosis {
  // INTEL: Prefixes
  constexpr uchar I_REX_B_41 = 0x41;
  constexpr uchar I_REX_W_48 = 0x48;
  constexpr uchar I_REX_WXB_4b = 0x4b;
  constexpr uchar I_REX_WRXB_4f = 0x4f;

  // INTEL: Opcodes
  constexpr uchar I_PUSH_BP_55 = 0x55;
  constexpr uchar I_LEA_8d = 0x8d;
  constexpr uchar I_MOV_r8_rm8_8a = 0x8a;
  constexpr uchar I_MOV_r64_r64_8b = 0x8b;

  constexpr uchar I_LEA_bf = 0xbf;
  constexpr uchar I_JMP_e9 = 0xe9;
  constexpr uchar I_CALL_e8 = 0xe8;
  constexpr uchar I_XOR_30 = 0x30;

  // Intel: Mod/RM
  constexpr uchar I_RM_BITS_07 = 0x07;
  constexpr uchar I_MOD_BITS_c0 = 128 + 64;
  constexpr uchar I_MOD_SDWORD_RAX_SDWORD_AL_80 = 0x80;
  constexpr uchar I_MOD_SDWORD_RDI_SDWORD_BH_bf = 0xBF;
  constexpr uchar I_BP_MOD_00 = 0x00;
  constexpr uchar I_BP_RM_RIP_DISP32_05 = 0x05;

  // ARM
  constexpr uchar A_LDR_e5 = 0xe5;
  constexpr uchar A_PUSH_e9 = 0xe9;
  constexpr uchar A_BL_eb = 0xeb;
  constexpr uchar A_B_ea = 0xea;
}
