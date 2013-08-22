namespace symbiosis {
  // INTEL: Prefixes
  constexpr uchar I_REX_64_BIT_48 = 0x48;
  constexpr uchar I_REX_REGISTER_EXT_41 = 0x41;
  constexpr uchar I_REX_RM_EXT_44 = 0x44;

  // INTEL: Opcodes
  constexpr uchar I_PUSH_r64_50 = 0x50;
  constexpr uchar I_POP_r64_58 = 0x58;
  constexpr uchar I_LEA_8d = 0x8d;
  constexpr uchar I_IMUL_r64_r64_AF = 0xAF;
  constexpr uchar I_IMUL_r64_r64_imm32_69 = 0x69;
  constexpr uchar I_IMUL_r64_r64_imm8_6B = 0x6B;
  constexpr uchar I_MOV_r64_imm64_b8 = 0xb8;

  constexpr uchar I_JMP_e9 = 0xe9;
  constexpr uchar I_CALL_e8 = 0xe8;
  constexpr uchar I_XOR_30 = 0x30;

  // Intel: Mod/RM
  constexpr uchar I_MOD_INDEX_00 = 0x00;
  constexpr uchar I_MOD_INDEX_P_SBYTE_40 = 0x40;
  constexpr uchar I_MOD_INDEX_P_SWORD_80 = 0x80;
  constexpr uchar I_MOD_REGISTER_C0 = 0xC0;

  constexpr int I_REG_A = 0x0;
  constexpr int I_REG_C = 0x1;
  constexpr int I_REG_D = 0x2;
  constexpr int I_REG_B = 0x3;
  constexpr int I_REG_SP = 0x4;
  constexpr int I_REG_SIB = I_REG_SP;
  constexpr int I_REG_BP = 0x5;
  constexpr int I_REG_RIP_REL = I_REG_BP;
  constexpr int I_REG_SI = 0x6;
  constexpr int I_REG_DI = 0x7;
  constexpr int I_REG_R8 = 0x8;
  constexpr int I_REG_R9 = 0x9;
  constexpr int I_REG_R10 = 0xa;
  constexpr int I_REG_R11 = 0xb;
  constexpr int I_REG_R12 = 0xc;
  constexpr int I_REG_R13 = 0xd;
  constexpr int I_REG_R14 = 0xe;
  constexpr int I_REG_R15 = 0xf;

  // ARM
  constexpr uchar A_LDR_e5 = 0xe5;
  constexpr uchar A_PUSH_e9 = 0xe9;
  constexpr uchar A_BL_eb = 0xeb;
  constexpr uchar A_B_ea = 0xea;
}
