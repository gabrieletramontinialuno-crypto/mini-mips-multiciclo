#include "bib.h"
#include <stdio.h>
#include <string.h>

const char *NOME_ESTADO[] = {
    "FETCH",   "DECODE", "MEM_ADDR", "MEM_READ", "LW_WB", "MEM_WRITE",
    "ADDI_WB", "R_EXEC", "R_WB",     "BEQ",      "JUMP"};

void limpa_buffer() {
  int c;
  while ((c = getchar()) != '\n' && c != EOF)
    ;
}

// MEMORIA

void carrega_mem(CPU *cpu) {
  char arq[100];
  printf("Nome do arquivo .mem: ");
  limpa_buffer();
  scanf("%s", arq);
  strcat(arq, ".mem");
  FILE *f = fopen(arq, "r");
  if (!f) {
    printf("Erro ao abrir %s\n", arq);
    return;
  }

  // Reset memoria
  for (int i = 0; i < MAX_MEM; i++) {
    memset(cpu->memoria[i].bin, '0', 16);
    cpu->memoria[i].bin[16] = '\0';
    cpu->memoria[i].tipo = tipo_dado;
    cpu->memoria[i].dado = 0;
    cpu->memoria[i].opcode = 0;
  }
  cpu->num_instrucoes = 0;

  char linha[100];
  int addr = 0;
  int modo_data = 0;

  while (fgets(linha, sizeof(linha), f)) {
    linha[strcspn(linha, "\r\n")] = '\0';
    if (strncmp(linha, ".data", 5) == 0) {
      modo_data = 1;
      if (addr < INST_END)
        addr = INST_END;
      continue;
    }
    if (strlen(linha) == 0) {
      addr++;
      continue;
    }
    char bits[20];
    if (sscanf(linha, "%16s", bits) != 1 || strlen(bits) != 16) {
      addr++;
      continue;
    }
    int valido = 1;
    for (int i = 0; i < 16; i++)
      if (bits[i] != '0' && bits[i] != '1') {
        valido = 0;
        break;
      }
    if (!valido) {
      addr++;
      continue;
    }

    if (addr >= MAX_MEM)
      break;

    if (addr < INST_END && !modo_data) {
      // Instrucao
      strcpy(cpu->memoria[addr].bin, bits);
      decode(&cpu->memoria[addr]);
      cpu->memoria[addr].dado = 0;
      cpu->num_instrucoes = addr + 1;
    } else {
      // Dado
      strcpy(cpu->memoria[addr].bin, bits);
      cpu->memoria[addr].tipo = tipo_dado;
      cpu->memoria[addr].dado = bin_to_int16(bits);
    }
    addr++;
  }

  fclose(f);
  printf("Instrucoes e dados carregados com sucesso.\n");
}

void inicializa_cpu(CPU *cpu) {
  cpu->pc = 0;
  cpu->estado_atual = 0;
  cpu->ciclos_clock = 0;
  cpu->instrucoes_exec = 0;
  cpu->num_instrucoes = 0;
  cpu->i_hist = 0;
  memset(&cpu->est, 0, sizeof(Estatisticas));
  memset(&cpu->inter, 0, sizeof(Regs_inter));
  for (int i = 0; i < MAX_REG; i++)
    cpu->reg[i] = 0;
  cpu->NOME_REG[0] = "$0";
  cpu->NOME_REG[1] = "$r1";
  cpu->NOME_REG[2] = "$r2";
  cpu->NOME_REG[3] = "$r3";
  cpu->NOME_REG[4] = "$r4";
  cpu->NOME_REG[5] = "$r5";
  cpu->NOME_REG[6] = "$r6";
  cpu->NOME_REG[7] = "$r7";
  for (int i = 0; i < MAX_MEM; i++) {
    memset(cpu->memoria[i].bin, '0', 16);
    cpu->memoria[i].bin[16] = '\0';
    cpu->memoria[i].tipo = tipo_dado;
    cpu->memoria[i].opcode = cpu->memoria[i].rs = cpu->memoria[i].rt = 0;
    cpu->memoria[i].rd = cpu->memoria[i].funct = cpu->memoria[i].imm =
        cpu->memoria[i].addr = 0;
    cpu->memoria[i].dado = 0;
  }
}

int separa_bits(char *b, int ini, int n) {
  int v = 0;
  for (int i = 0; i < n; i++)
    v = (v << 1) | (b[ini + i] == '1' ? 1 : 0);
  return v;
}
int bits_imm(char *b, int ini, int n) {
  int v = separa_bits(b, ini, n);
  if (v & (1 << (n - 1)))
    return v | ((int)(~0u << n));
  return v;
}
int bits_jump(char *b) { return separa_bits(b, 4, 12) & 0xFF; }
int bin_to_int16(char *b) {
  int v = 0;
  for (int i = 0; i < 16; i++)
    v = (v << 1) | (b[i] == '1' ? 1 : 0);
  if (v & 0x8000)
    v |= ~0xFFFF;
  return v;
}

// UNIT CONTROL
void decode(Mem_in *e) {
  char *b = e->bin;
  e->opcode = separa_bits(b, 0, 4);
  switch (e->opcode) {
  case 0: // Tipo R
    e->tipo = tipo_R;
    e->rs = separa_bits(b, 4, 3);
    e->rt = separa_bits(b, 7, 3);
    e->rd = separa_bits(b, 10, 3);
    e->funct = separa_bits(b, 13, 3);
    e->imm = 0;
    e->addr = 0;
    break;
  case 2: // Jump
    e->tipo = tipo_J;
    e->addr = bits_jump(b);
    e->rs = e->rt = e->rd = e->funct = e->imm = 0;
    break;
  case 4:
  case 8:
  case 11:
  case 15: // Tipo I
    e->tipo = tipo_I;
    e->rs = separa_bits(b, 4, 3);
    e->rt = separa_bits(b, 7, 3);
    e->imm = bits_imm(b, 10, 6);
    e->rd = e->funct = e->addr = 0;
    break;
  case 5: // HALT
    e->tipo = tipo_outros;
    e->rs = e->rt = e->rd = e->funct = e->imm = e->addr = 0;
    break;
  default:
    e->tipo = tipo_outros;
    break;
  }
}
Sinais gera_sinais(int estado, int funct) {
  Sinais s = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  switch (estado) {
  case 0: // FETCH
    s.IREsc = 1;
    s.PCEsc = 1;
    s.ULAFonteB = 1; // PC+1
    s.ControleULA = 0;
    s.PCFonte = 0;
    s.RegDst = 1;
    break;
  case 1: // DECODE
    s.ULAFonteA = 0;
    s.ULAFonteB = 2;
    s.ControleULA = 0;
    s.RegDst = 1;
    break;
  case 2: // MEM ADDR / IMM
    s.ULAFonteA = 1;
    s.ULAFonteB = 2;
    s.ControleULA = 0;
    break;
  case 3: // MEM READ (LW)
    s.IouD = 1;
    s.ULAFonteA = 1;
    s.ULAFonteB = 2;
    break;
  case 4: // WB LW
    s.EscReg = 1;
    s.MemParaReg = 1;
    s.RegDst = 0;
    s.IouD = 1;
    s.ULAFonteA = 1;
    s.ULAFonteB = 2;
    break;
  case 5: // MEM WRITE (SW)
    s.EscMem = 1;
    s.IouD = 1;
    s.ULAFonteA = 1;
    s.ULAFonteB = 2;
    break;
  case 6: // WB ADDI
    s.EscReg = 1;
    s.RegDst = 0;
    s.MemParaReg = 0;
    s.ULAFonteA = 1;
    s.ULAFonteB = 2;
    break;
  case 7: // R EXEC
    s.ULAFonteA = 1;
    s.ULAFonteB = 0;
    s.ControleULA = funct;
    s.RegDst = 1;
    break;
  case 8: // R WB
    s.RegDst = 1;
    s.EscReg = 1;
    s.MemParaReg = 0;
    break;
  case 9: // BEQ
    s.ULAFonteA = 1;
    s.ULAFonteB = 0;
    s.ControleULA = 1;
    s.Branch = 1;
    s.PCFonte = 1;
    break;
  case 10: // JUMP
    s.PCEsc = 1;
    s.PCFonte = 2;
    break;
  }
  return s;
}

int ula(int A, int B, int ctrl, int *ovf, int *zero) {
  int r = 0;
  *ovf = 0;
  *zero = 0;
  switch (ctrl) {
  case 0:
    r = A + B;
    {
      char r8 = (char)r;
      if ((A > 0 && B > 0 && r8 < 0) || (A < 0 && B < 0 && r8 > 0))
        *ovf = 1;
      r = r8;
    }
    break;
  case 1:
    r = A - B;
    if ((A > 0 && B < 0 && r < 0) || (A < 0 && B > 0 && r > 0))
      *ovf = 1;
    break;
  case 2:
    r = A & B;
    break;
  case 3:
    r = A | B;
    break;
  default:
    printf("Operacao ULA invalida\n");
    break;
  }
  if (r == 0)
    *zero = 1;
  return r;
}

void salvar_estado(CPU *cpu) {
  int idx = cpu->ciclos_clock % MAX_MEM;
  Salva_CPU *h = &cpu->historico[idx];
  h->pc = cpu->pc;
  h->estado = cpu->estado_atual;
  memcpy(h->reg, cpu->reg, MAX_REG);
  for (int i = INST_END; i < MAX_MEM; i++)
    h->dados[i] = cpu->memoria[i].dado;
  h->inter = cpu->inter;
  h->est = cpu->est;
  h->instrucoes_exec = cpu->instrucoes_exec;
  if (cpu->i_hist < MAX_MEM)
    cpu->i_hist++;
}

int proximo_estado(int estado, int opcode) {
  switch (estado) {
  case 0:
    return 1;
  case 1:
    if (opcode == 11 || opcode == 15 || opcode == 4)
      return 2; // lw,sw,addi
    if (opcode == 0)
      return 7; // tipo R
    if (opcode == 8)
      return 9; // beq
    if (opcode == 2)
      return 10; // jump
    return 0;    // Desconhecido
  case 2:
    if (opcode == 11)
      return 3; // lw
    if (opcode == 15)
      return 5; // sw
    if (opcode == 4)
      return 6; // addi
    return 0;
  case 3:
    return 4; // lw
  case 4:
    return 0;
  case 5:
    return 0;
  case 6:
    return 0;
  case 7:
    return 8; // tipo R
  case 8:
    return 0;
  case 9:
    return 0;
  case 10:
    return 0;
  default:
    return 0;
  }
}
void executa_ciclo(CPU *cpu) {
  int est = cpu->estado_atual;
  Mem_in *ir = &cpu->inter.IR;
  Sinais s = gera_sinais(est, ir->funct);
  (void)s; // sinais gerados para referencia/debug OBS

  salvar_estado(cpu); // salva o estado ANTES de executar o ciclo

  printf("  [Ciclo %d] Estado %d (%s)", cpu->ciclos_clock, est,
         NOME_ESTADO[est]);

  switch (est) {
  case 0: { // FETCH: IR=Mem[PC], PC=PC+1
    if (cpu->pc >= 0 && cpu->pc < MAX_MEM) {
      cpu->inter.IR = cpu->memoria[cpu->pc];
      decode(&cpu->inter.IR);
    }
    int ovf, zero;
    cpu->inter.ULASaida = ula(cpu->pc, 1, 0, &ovf, &zero);
    cpu->pc = cpu->inter.ULASaida;
    printf(" | IR=Mem[%d] PC->%d\n", cpu->pc - 1, cpu->pc);
    break;
  }
  case 1: { // DECODE: A=Reg[rs], B=Reg[rt], ULASaida=PC+ext(imm)
    cpu->inter.A = (int)cpu->reg[ir->rs];
    cpu->inter.B = (int)cpu->reg[ir->rt];
    int ovf, zero;
    int imm_ext = ir->imm;
    cpu->inter.ULASaida = ula(cpu->pc, imm_ext, 0, &ovf, &zero);
    printf(" | A=Reg[%d]=%d B=Reg[%d]=%d ULASaida=%d\n", ir->rs, cpu->inter.A,
           ir->rt, cpu->inter.B, cpu->inter.ULASaida);
    break;
  }
  case 2: { // MEM ADDR: ULASaida = A + ext(imm)
    int ovf, zero;
    cpu->inter.ULASaida = ula(cpu->inter.A, ir->imm, 0, &ovf, &zero);
    printf(" | ULASaida=A(%d)+imm(%d)=%d\n", cpu->inter.A, ir->imm,
           cpu->inter.ULASaida);
    break;
  }
  case 3: { // MEM READ: MDR=Mem[ULASaida]
    int addr = cpu->inter.ULASaida;
    if (addr >= 0 && addr < MAX_MEM)
      cpu->inter.MDR = cpu->memoria[addr].dado;
    else {
      printf(" | Endereco invalido: %d\n", addr);
      cpu->inter.MDR = 0;
      break;
    }
    printf(" | MDR=Mem[%d]=%d\n", addr, cpu->inter.MDR);
    break;
  }
  case 4: { // LW WB: Reg[rt]=MDR
    cpu->reg[ir->rt] = (char)cpu->inter.MDR;
    printf(" | Reg[%d]=MDR=%d\n", ir->rt, cpu->inter.MDR);
    atualiza_Estatisticas(cpu);
    cpu->instrucoes_exec++;
    break;
  }
  case 5: { // SW: Mem[ULASaida]=B
    int addr = cpu->inter.ULASaida;
    if (addr >= 0 && addr < MAX_MEM)
      cpu->memoria[addr].dado = cpu->inter.B;
    else {
      printf(" | Endereco invalido: %d\n", addr);
      break;
    }
    printf(" | Mem[%d]=B=%d\n", addr, cpu->inter.B);
    atualiza_Estatisticas(cpu);
    cpu->instrucoes_exec++;
    break;
  }
  case 6: { // ADDI WB: Reg[rt]=ULASaida
    cpu->reg[ir->rt] = (char)cpu->inter.ULASaida;
    printf(" | Reg[%d]=ULASaida=%d\n", ir->rt, cpu->inter.ULASaida);
    atualiza_Estatisticas(cpu);
    cpu->instrucoes_exec++;
    break;
  }
  case 7: { // R EXEC: ULASaida=A op B
    int ovf, zero;
    cpu->inter.ULASaida =
        ula(cpu->inter.A, cpu->inter.B, ir->funct, &ovf, &zero);
    if (ovf)
      printf(" | OVERFLOW! A=%d B=%d res=%d\n", cpu->inter.A, cpu->inter.B,
             cpu->inter.ULASaida);
    else
      printf(" | ULASaida=A(%d) op(%d) B(%d)=%d\n", cpu->inter.A, ir->funct,
             cpu->inter.B, cpu->inter.ULASaida);
    break;
  }
  case 8: { // R WB: Reg[rd]=ULASaida
    cpu->reg[ir->rd] = (char)cpu->inter.ULASaida;
    printf(" | Reg[%d]=ULASaida=%d\n", ir->rd, cpu->inter.ULASaida);
    atualiza_Estatisticas(cpu);
    cpu->instrucoes_exec++;
    break;
  }
  case 9: { // BEQ: if(A==B) PC=ULASaida
    int ovf, zero;
    ula(cpu->inter.A, cpu->inter.B, 1, &ovf, &zero);
    if (zero) {
      cpu->pc = cpu->inter.ULASaida;
      printf(" | Branch TAKEN PC->%d\n", cpu->pc);
    } else
      printf(" | Branch NOT taken\n");
    atualiza_Estatisticas(cpu);
    cpu->instrucoes_exec++;
    break;
  }
  case 10: { // JUMP: PC=IR[7:0]
    cpu->pc = ir->addr;
    printf(" | PC->%d (jump)\n", cpu->pc);
    atualiza_Estatisticas(cpu);
    cpu->instrucoes_exec++;
    break;
  }
  }
  cpu->estado_atual = proximo_estado(est, ir->opcode);
  cpu->ciclos_clock++;
  cpu->est.ciclos_clock = cpu->ciclos_clock;
}

void executa_instrucao(CPU *cpu) {
  if (cpu->num_instrucoes == 0) {
    printf("Nenhuma instrucao carregada.\n");
    return;
  }
  int pc_ini = cpu->pc;
  if (cpu->memoria[pc_ini].opcode == 5) {
    printf("Instrucao de parada no PC %d.\n", pc_ini);
    return;
  }
  printf("\n---------- PC = %d ----------\n", pc_ini);

  // disassembla a instrucao antes de executar
  char buf[64];
  Mem_in temp = cpu->memoria[pc_ini];
  decode(&temp);
  disassembla(&temp, buf);
  printf("Instrucao: %s\n", buf);

  do {
    executa_ciclo(cpu);
  } while (cpu->estado_atual != 0);
  printf("\n");
}
void executa_programa(CPU *cpu) {
  if (cpu->num_instrucoes == 0) {
    printf("Nenhuma instrucao carregada.\n");
    return;
  }
  while (cpu->pc < INST_END && cpu->instrucoes_exec < MAX_MEM) {
    if (cpu->memoria[cpu->pc].opcode == 5) {
      printf("\nInstrucao de parada no PC %d. Encerrando.\n", cpu->pc);
      break;
    }
    executa_instrucao(cpu);
  }
  printf("\nExecucao finalizada: %d instrucoes, %d ciclos de clock.\n",
         cpu->instrucoes_exec, cpu->ciclos_clock);
}

void atualiza_Estatisticas(CPU *cpu) {
  Mem_in *ir = &cpu->inter.IR;
  cpu->est.total_inst++;
  if (ir->opcode == 0) {
    cpu->est.total_r++;
    switch (ir->funct) {
    case 0:
      cpu->est.add++;
      break;
    case 1:
      cpu->est.sub++;
      break;
    case 2:
      cpu->est.and_op++;
      break;
    case 3:
      cpu->est.or_op++;
      break;
    }
  } else if (ir->opcode == 4) {
    cpu->est.total_i++;
    cpu->est.addi++;
  } else if (ir->opcode == 8) {
    cpu->est.total_i++;
    cpu->est.beq++;
  } else if (ir->opcode == 11) {
    cpu->est.total_i++;
    cpu->est.lw++;
  } else if (ir->opcode == 15) {
    cpu->est.total_i++;
    cpu->est.sw++;
  } else if (ir->opcode == 2) {
    cpu->est.total_j++;
    cpu->est.jump++;
  }
}

void volta_ciclo(CPU *cpu) {
  if (cpu->ciclos_clock > 0 && cpu->i_hist > 0) {
    cpu->ciclos_clock--;
    cpu->i_hist--;
    int idx = cpu->ciclos_clock % MAX_MEM;
    Salva_CPU *h = &cpu->historico[idx];
    cpu->pc = h->pc;
    cpu->estado_atual = h->estado;
    memcpy(cpu->reg, h->reg, MAX_REG);
    for (int i = INST_END; i < MAX_MEM; i++)
      cpu->memoria[i].dado = h->dados[i];
    cpu->inter = h->inter;
    cpu->est = h->est;
    cpu->instrucoes_exec = h->instrucoes_exec;
    printf("Voltou para ciclo %d, estado %d, PC=%d\n", cpu->ciclos_clock,
           cpu->estado_atual, cpu->pc);
  } else
    printf("Limite do historico atingido!\n");
}
void volta_instrucao(CPU *cpu) {
  if (cpu->i_hist == 0) {
    printf("Nao ha estado anterior para voltar.\n");
    return;
  }

  do {
    if (cpu->i_hist == 0)
      break;
    cpu->i_hist--;
    Salva_CPU *h = &cpu->historico[cpu->i_hist];
    cpu->pc = h->pc;
    cpu->estado_atual = h->estado;
    cpu->instrucoes_exec = h->instrucoes_exec;
    memcpy(cpu->reg, h->reg, sizeof(cpu->reg));
    cpu->inter = h->inter;
    cpu->est = h->est;
    cpu->ciclos_clock--;
    cpu->est.ciclos_clock = cpu->ciclos_clock;
    for (int i = INST_END; i < MAX_MEM; i++)
      cpu->memoria[i].dado = h->dados[i - INST_END];
  } while (cpu->estado_atual != 0);

  printf("Voltou 1 instrucao. Estado atual: %d (%s), PC=%d\n",
         cpu->estado_atual, NOME_ESTADO[cpu->estado_atual], cpu->pc);
}
void reset_cpu(CPU *cpu) {
  cpu->pc = 0;
  cpu->estado_atual = 0;
  cpu->ciclos_clock = 0;
  cpu->instrucoes_exec = 0;
  cpu->i_hist = 0;
  memset(&cpu->est, 0, sizeof(Estatisticas));
  memset(&cpu->inter, 0, sizeof(Regs_inter));
  for (int i = 0; i < MAX_REG; i++)
    cpu->reg[i] = 0;
  for (int i = INST_END; i < MAX_MEM; i++)
    cpu->memoria[i].dado = 0;
  printf("Simulador resetado!\n");
}

void disassembla(Mem_in *e, char *buf) {
  switch (e->opcode) {
  case 0:
    switch (e->funct) {
    case 0:
      sprintf(buf, "add $r%d, $r%d, $r%d", e->rd, e->rs, e->rt);
      break;
    case 1:
      sprintf(buf, "sub $r%d, $r%d, $r%d", e->rd, e->rs, e->rt);
      break;
    case 2:
      sprintf(buf, "and $r%d, $r%d, $r%d", e->rd, e->rs, e->rt);
      break;
    case 3:
      sprintf(buf, "or  $r%d, $r%d, $r%d", e->rd, e->rs, e->rt);
      break;
    default:
      sprintf(buf, "??? funct=%d", e->funct);
      break;
    }
    break;
  case 2:
    sprintf(buf, "j %d", e->addr);
    break;
  case 4:
    sprintf(buf, "addi $r%d, $r%d, %d", e->rt, e->rs, e->imm);
    break;
  case 5:
    sprintf(buf, "HALT");
    break;
  case 8:
    sprintf(buf, "beq $r%d, $r%d, %d", e->rs, e->rt, e->imm);
    break;
  case 11:
    sprintf(buf, "lw $r%d, %d($r%d)", e->rt, e->imm, e->rs);
    break;
  case 15:
    sprintf(buf, "sw $r%d, %d($r%d)", e->rt, e->imm, e->rs);
    break;
  default:
    sprintf(buf, "data %s", e->bin);
    break;
  }
}

void print_mem(CPU *cpu) {
  printf("\n+------+------------------+----------------------------++------+---"
         "-----+\n");
  printf("| Addr | Binario          | Assembly                   || Addr |  "
         "Dado  |\n");
  printf("| inst |                  |                            ||  dat |     "
         "   |\n");
  printf("+------+------------------+----------------------------++------+-----"
         "---+\n");

  for (int i = 0; i < INST_END; i++) {
    // Coluna de instrucoes: sempre desassembla (zeros = add $r0, $r0, $r0)
    char buf[64];
    Mem_in temp = cpu->memoria[i];
    decode(&temp);
    disassembla(&temp, buf);
    printf("| %4d | %-16s | %-26s |", i, cpu->memoria[i].bin, buf);
    // Coluna de dados (sempre mostra addr INST_END+i)
    int addr_d = INST_END + i;
    printf("| %4d | %6d |\n", addr_d, cpu->memoria[addr_d].dado);
  }

  printf("+------+------------------+----------------------------++------+-----"
         "---+\n");
}
void print_regs(CPU *cpu) {
  printf("\nBanco de Registradores:\n");
  printf("+------+--------+\n| Reg  |  Valor |\n+------+--------+\n");
  for (int i = 0; i < MAX_REG; i++)
    printf("| %4s | %6d |\n", cpu->NOME_REG[i], cpu->reg[i]);
  printf("+------+--------+\n");
}
void print_inter(CPU *cpu) {
  printf("\nRegistradores Intermediarios:\n");
  printf("+----------+--------+\n");
  printf("| Reg      |  Valor |\n");
  printf("+----------+--------+\n");
  printf("| PC       | %6d |\n", cpu->pc);
  printf("| Estado   | %6d |\n", cpu->estado_atual);
  printf("| IR.opc   | %6d |\n", cpu->inter.IR.opcode);
  printf("| A        | %6d |\n", cpu->inter.A);
  printf("| B        | %6d |\n", cpu->inter.B);
  printf("| ULASaida | %6d |\n", cpu->inter.ULASaida);
  printf("| MDR      | %6d |\n", cpu->inter.MDR);
  printf("+----------+--------+\n");
}
void print_est(CPU *cpu) {
  printf("\n===== Estatisticas =====\n");
  printf("Total de instrucoes: %d\n", cpu->est.total_inst);
  printf("Total ciclos de clock: %d\n", cpu->est.ciclos_clock);
  if (cpu->est.total_inst > 0)
    printf("CPI medio: %.2f\n",
           (float)cpu->est.ciclos_clock / cpu->est.total_inst);
  printf("\nTipo R: %d\n", cpu->est.total_r);
  printf("  ADD: %d | SUB: %d | AND: %d | OR: %d\n", cpu->est.add, cpu->est.sub,
         cpu->est.and_op, cpu->est.or_op);
  printf("Tipo I: %d\n", cpu->est.total_i);
  printf("  ADDI: %d | BEQ: %d | LW: %d | SW: %d\n", cpu->est.addi,
         cpu->est.beq, cpu->est.lw, cpu->est.sw);
  printf("Tipo J: %d\n", cpu->est.total_j);
  printf("  JUMP: %d\n", cpu->est.jump);
}
void print_complete(CPU *cpu) {
  printf("\n=========== ESTADO DO SIMULADOR MULTICICLO ===========\n");
  print_regs(cpu);
  print_inter(cpu);
  print_mem(cpu);
}

void salva_asm(CPU *cpu) {
  if (cpu->num_instrucoes == 0) {
    printf("Nenhuma instrucao carregada.\n");
    return;
  }
  char arq[100];
  printf("Nome do arquivo .asm: ");
  limpa_buffer();
  scanf("%s", arq);
  strcat(arq, ".asm");
  FILE *f = fopen(arq, "w");
  if (!f) {
    printf("Erro ao criar arquivo.\n");
    return;
  }
  for (int i = 0; i < cpu->num_instrucoes; i++) {
    char buf[64];
    Mem_in temp = cpu->memoria[i];
    decode(&temp);
    disassembla(&temp, buf);
    fprintf(f, "%s\n", buf);
  }
  fclose(f);
  printf("Arquivo '%s' salvo!\n", arq);
}

void salva_dat(CPU *cpu) {
  char arq[100];
  printf("Nome do arquivo .dat: ");
  limpa_buffer();
  scanf("%s", arq);
  strcat(arq, ".dat");
  FILE *f = fopen(arq, "w");
  if (!f) {
    printf("Erro ao criar arquivo.\n");
    return;
  }
  for (int i = INST_END; i < MAX_MEM; i++)
    fprintf(f, "%d\n", cpu->memoria[i].dado);
  fclose(f);
  printf("Arquivo '%s' salvo!\n", arq);
}