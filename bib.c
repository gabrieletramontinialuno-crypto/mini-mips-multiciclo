#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bib.h"

const char *NOME_ESTADO[] = {
    "FETCH", "DECODE", "MEM_ADDR", "MEM_READ", "LW_WB",
    "MEM_WRITE", "ADDI_WB", "R_EXEC", "R_WB", "BEQ", "JUMP"
};

void limpa_buffer(){int c; while((c=getchar())!='\n'&&c!=EOF);}

// MEMORIA

void carrega_mem(CPU *cpu){
    // Reinicializa CPU ao carregar novo programa
    inicializa_cpu(cpu);

    char arq[50];

    printf("Nome do arquivo .mem: ");
    limpa_buffer();
    scanf("%s", arq);

    FILE *arquivo = fopen(arq, "r");
    if (!arquivo){
        printf("Erro ao abrir arquivo.\n");
        return;
    }

    char linha_str[100];
    int linha = 0;
    int i = 0;       // instruções
    int j = MEM;     // dados começam em 128

    while (fgets(linha_str, sizeof(linha_str), arquivo)) {
        linha++;

        char *instrucao = strtok(linha_str, ";");
        char *dado = strtok(NULL, ";");

        // ===== INSTRUÇÃO =====
        if (instrucao != NULL) {

            while (*instrucao == ' ') instrucao++;

            int len = strlen(instrucao);
            if (len > 0 && instrucao[len-1] == '\n')
                instrucao[len-1] = '\0';

            len = strlen(instrucao);

            if (len == 16 && i < MEM) {

                int valido = 1;
                for (int k = 0; k < 16; k++) {
                    if (instrucao[k] != '0' && instrucao[k] != '1') {
                        valido = 0;
                        break;
                    }
                }

                if (valido!=0) {
                    strcpy(cpu->memoria[i].bin, instrucao);
                    cpu->memoria[i].tipo = tipo_outros; // decoder ajusta depois
                    i++;
                } else {
                    printf("Linha %d: instrucao invalida\n", linha);
                }
            }
        }

        // ===== DADO =====
        if (dado != NULL) {

            while (*dado == ' ') dado++;

            int valor = atoi(dado);

            if (j < MAX_MEM) {
                cpu->memoria[j].dado = valor;
                cpu->memoria[j].tipo = tipo_dado;
                j++;
            }
        }
    }

    cpu->num_instrucoes = i;

    printf("%d instrucoes carregadas.\n", i);

    fclose(arquivo);
}


void inicializa_cpu(CPU *cpu){
    cpu->pc = cpu->estado_atual = cpu->ciclos_clock=0;
    cpu->instrucoes_exec = cpu->num_instrucoes = cpu->i_hist=0;
    memset(&cpu->est, 0, sizeof(estatisticas));
    memset(&cpu->inter, 0, sizeof(regs_inter));
    for(int i=0;i<MAX_REG;i++) cpu->reg[i]=0;
    cpu->NOME_REG[0]="$0"; 
    cpu->NOME_REG[1]="$r1"; 
    cpu->NOME_REG[2]="$r2";
    cpu->NOME_REG[3]="$r3"; 
    cpu->NOME_REG[4]="$r4"; 
    cpu->NOME_REG[5]="$r5";
    cpu->NOME_REG[6]="$r6"; 
    cpu->NOME_REG[7]="$r7";
    for(int i=0;i<MAX_MEM;i++){
        memset(cpu->memoria[i].bin,'0',16);
        cpu->memoria[i].bin[16]='\0';
        cpu->memoria[i].tipo=tipo_dado;
        cpu->memoria[i].opcode=cpu->memoria[i].rs=cpu->memoria[i].rt=0;
        cpu->memoria[i].rd=cpu->memoria[i].funct=cpu->memoria[i].imm=cpu->memoria[i].addr=0;
        cpu->memoria[i].dado=0;
    }
}

int separa_bits(char *b, int ini, int nBits){
    int n = 0;
    for (int i = 0; i < nBits; i++) {
        n = (n << 1) | (b[ini + i] == '1' ? 1 : 0);
    }
    return n;
}
int bits_imm(char *b, int ini, int nBits){
    int val = separa_bits(b, ini, nBits);
    signed char sinal = (signed char)(val << 2);
    return (int)(sinal >> 2);
}
int bits_jump(char *b){
    return separa_bits(b,4,12)&0xFF;
}
int bin_to_int16(char *b){
    int v=0; 
    for(int i=0;i<16;i++) v=(v<<1)|(b[i]=='1'?1:0);
    if(v&0x8000) v|=~0xFFFF;
    return v;
}

// UNIT CONTROL
void decoder(memoria *mem){
    char *b = mem->bin;

    mem->opcode = separa_bits(b, 0, 4);

    switch(mem->opcode){
        case 0: // Tipo R
            mem->tipo = tipo_R;
            mem->rs = separa_bits(b, 4, 3);
            mem->rt = separa_bits(b, 7, 3);
            mem->rd = separa_bits(b, 10, 3);
            mem->funct = separa_bits(b, 13, 3);
            mem->imm = mem->addr = 0;
        break;
        case 2: // Jump
            mem->tipo = tipo_J;
            mem->addr = bits_jump(b);
            mem->rs = mem->rt = mem->rd = mem->funct = mem->imm = 0;
        break;
        case 4: case 8: case 11: case 15: // Tipo I
            mem->tipo = tipo_I;
            mem->rs = separa_bits(b, 4, 3);
            mem->rt = separa_bits(b, 7, 3);
            mem->imm = bits_imm(b, 10, 6);
            mem->rd = mem->funct = mem->addr = 0;
        break;
    }
}
sinais gera_sinais(int estado, int funct){
    
    sinais s;

    // zera todos sinais
    memset(&s, 0, sizeof(sinais));

    switch(estado){

        // Estado 0 [0000] - FETCH (Busca da instrução)
        case 0:
            s.EscMem = 0;        // não escreve na memória
            s.ULAFonteA = 0;     // usa PC
            s.IouD = 0;          // endereço vem do PC
            s.IREsc = 1;         // escreve IR
            s.ULAFonteB = 1;     // constante 1 (PC+1)
            s.ControleULA = 0;   // soma (ADD)
            s.PCEsc = 1;         // atualiza PC
            s.PCFonte = 0;       // saída da ULA
        break;

        // Estado 1 [0001] - DECODE (Decodificação / Leitura Rs e Rt)
        case 1:
            s.ULAFonteA = 0;     // PC
            s.ULAFonteB = 2;     // imediato (10 binário)
            s.ControleULA = 0;   // soma (ADD)
            s.RegDst = 1;
        break;

        // Estado 2 [0010] - Cálculo do endereço de acesso à memória/imediato
        case 2:
            s.ULAFonteA = 1;     // registrador A
            s.ULAFonteB = 2;     // imediato (10 binário)
            s.ControleULA = 0;   // soma (ADD)
        break;

        // Estado 3 [0011] - Acesso à memória (leitura LW)
        case 3:
            s.EscMem = 0;        // não escreve (leitura)
            s.IouD = 1;          // endereço vem da ULA
            s.ULAFonteA = 1;     // registrador A
            s.ULAFonteB = 2;     // imediato (10 binário)
        break;

        // Estado 4 [0100] - Escrita no registrador Rt (LW Write Back)
        case 4:
            s.EscReg = 1;
            s.MemParaReg = 1;
            s.RegDst = 0;
            s.ULAFonteA = 1;     // registrador A
            s.ULAFonteB = 2;     // imediato (10 binário)
        break;

        // Estado 5 [0101] - Escrita à memória (SW)
        case 5:
            s.EscMem = 1;        // escreve na memória
            s.IouD = 1;          // endereço vem da ULA
            s.ULAFonteA = 1;     // registrador A
            s.ULAFonteB = 2;     // imediato (10 binário)
        break;

        // Estado 6 [0110] - Término da instrução tipo I (ADDI WB)
        case 6:
            s.EscMem = 0;
            s.EscReg = 1;
            s.RegDst = 0;
            s.MemParaReg = 0;
            s.ULAFonteA = 1;     // registrador A
            s.ULAFonteB = 2;     // imediato (10 binário)
        break;

        // Estado 7 [0111] - Execução tipo R
        case 7:
            s.ULAFonteA = 1;     // registrador A
            s.ULAFonteB = 0;     // registrador B (00 binário)
            s.ControleULA = funct; // ULAOp = Funct
            s.RegDst = 1;
        break;

        // Estado 8 [1000] - Término da instrução tipo R (R Write Back)
        case 8:
            s.RegDst = 1;
            s.EscReg = 1;
            s.MemParaReg = 0;
        break;

        // Estado 9 [1001] - Término do desvio condicional (BEQ)
        case 9:
            s.ULAFonteA = 1;     // registrador A
            s.ULAFonteB = 0;     // registrador B (00 binário)
            s.ControleULA = 1;   // subtração (para comparação)
            s.Branch = 1;
            s.PCEsc = 0;
            s.PCFonte = 1;       // FontePC = 01
        break;

        // Estado 10 [1010] - Término do desvio incondicional (JUMP)
        case 10:
            s.PCEsc = 1;
            s.PCFonte = 2;       // FontePC = 10
        break;
    }

    return s;
}

// ULA
int ula(int A, int B, int ctrl, int *ovf, int *zero){
    int resultado = 0;
    *ovf = 0;
    *zero = 0;

    switch(ctrl){
        case 0: // ADD
            resultado = A + B;
            // overflow: sinais iguais na entrada, sinal diferente na saida
            if(((A > 0 && B > 0) && resultado < 0) || ((A < 0 && B < 0) && resultado > 0))
                *ovf = 1;
        break;
        case 1: // SUB
            resultado = A - B;
            if(((A > 0 && B < 0) && resultado < 0) || ((A < 0 && B > 0) && resultado > 0))
                *ovf = 1;
        break;
        case 2: // AND
            resultado = A & B;
        break;
        case 3: // OR
            resultado = A | B;
        break;
        default:
            resultado = 0;
        break;
    }

    *zero = (resultado == 0) ? 1 : 0;
    return resultado;
}

// SALVA ESTADO PARA VOLTAR
void salvar_estado(CPU *cpu){
    if(cpu->i_hist >= MAX_MEM) return; // historico cheio
    salva_cpu *h = &cpu->historico[cpu->i_hist];
    h->pc = cpu->pc;
    h->estado = cpu->estado_atual;
    h->instrucoes_exec = cpu->instrucoes_exec;
    memcpy(h->reg, cpu->reg, sizeof(cpu->reg));
    h->inter = cpu->inter;
    h->est = cpu->est;
    // salva dados
    for(int i = MEM; i < MAX_MEM; i++)
        h->dados[i - MEM] = cpu->memoria[i].dado;
    cpu->i_hist++;
}

// EXECUCAO
int proximo_estado(int estado, int opcode){
    switch(estado){
        case 0: return 1;
        case 1:
            if(opcode==11||opcode==15||opcode==4) return 2; // lw,sw,addi
            if(opcode==0) return 7; // tipo R
            if(opcode==8) return 9; // beq
            if(opcode==2) return 10; // jump
            return 0; // desconhecido
        case 2:
            if(opcode==11) return 3; // lw
            if(opcode==15) return 5; // sw
            if(opcode==4) return 6;  // addi
            return 0;
        case 3: return 4;
        case 4: return 0;
        case 5: return 0;
        case 6: return 0;
        case 7: return 8;
        case 8: return 0;
        case 9: return 0;
        case 10: return 0;
        default: return 0;
    }
}
void executa_ciclo(CPU *cpu){
    if(cpu->num_instrucoes == 0){
        printf("Nenhuma instrucao carregada.\n");
        return;
    }

    salvar_estado(cpu);

    int est = cpu->estado_atual;
    memoria *ir = &cpu->inter.IR;
    sinais s = gera_sinais(est, ir->funct);
    printf("  [Ciclo %d] Estado %d (%s)", cpu->ciclos_clock, est, NOME_ESTADO[est]);

    switch(est){

        case 0: { // FETCH

            if(cpu->pc >= 0 && cpu->pc < MAX_MEM){
                cpu->inter.IR = cpu->memoria[cpu->pc];
                decoder(&cpu->inter.IR);
            }

            int ovf, zero;
            cpu->inter.ULASaida = ula(cpu->pc, 1, 0, &ovf, &zero);
            cpu->pc = cpu->inter.ULASaida;

            printf(" | IR=Mem[%d] PC->%d\n", cpu->pc-1, cpu->pc);
            break;
        }

        case 1: { // DECODE
            cpu->inter.A = cpu->reg[ir->rs];
            cpu->inter.B = cpu->reg[ir->rt];

            int ovf, zero;
            cpu->inter.ULASaida = ula(cpu->pc, ir->imm, 0, &ovf, &zero);

            printf(" | A=Reg[%d]=%d B=Reg[%d]=%d ULASaida=%d\n",
                ir->rs, cpu->inter.A, ir->rt, cpu->inter.B, cpu->inter.ULASaida);
            break;
        }

        case 2: { // MEM_ADDR (SW e LW)
            int ovf, zero;
            cpu->inter.ULASaida = ula(cpu->inter.A, ir->imm, 0, &ovf, &zero);

            printf(" | ULASaida=A(%d)+imm(%d)=%d\n",
                cpu->inter.A, ir->imm, cpu->inter.ULASaida);
            break;
        }

        case 3: { // MEM_READ (Leitura memoria)
            int addr = cpu->inter.ULASaida;

            if(addr >= MEM && addr < MAX_MEM){
                cpu->inter.MDR = cpu->memoria[addr].dado;
                printf(" | MDR=Mem[%d]=%d\n", addr, cpu->inter.MDR);
            } else {
                printf(" | Endereco invalido: %d\n", addr);
                cpu->inter.MDR = 0;
            }
            break;
        }

        case 4: { // LW White Back
            if(ir->rt != 0) // protege $0
                cpu->reg[ir->rt] = cpu->inter.MDR;

            printf(" | Reg[%d]=MDR=%d\n", ir->rt, cpu->inter.MDR);

            atualiza_estatisticas(cpu); //ainda não implementada nesse codigo
            cpu->instrucoes_exec++;
            break;
        }

        case 5: { // SW
            int addr = cpu->inter.ULASaida;

            if(addr >= MEM && addr < MAX_MEM){
                cpu->memoria[addr].dado = cpu->inter.B;
                printf(" | Mem[%d]=B=%d\n", addr, cpu->inter.B);
            } else {
                printf(" | Endereco invalido: %d\n", addr);
            }

            atualiza_estatisticas(cpu);
            cpu->instrucoes_exec++;
            break;
        }

        case 6: { // ADDI WB
            if(ir->rt != 0)
                cpu->reg[ir->rt] = cpu->inter.ULASaida;

            printf(" | Reg[%d]=ULASaida=%d\n", ir->rt, cpu->inter.ULASaida);

            atualiza_estatisticas(cpu);
            cpu->instrucoes_exec++;
            break;
        }

        case 7: { // tipo R
            int ovf, zero;
            cpu->inter.ULASaida = ula(cpu->inter.A, cpu->inter.B, ir->funct, &ovf, &zero);

            if(ovf)
                printf(" | OVERFLOW!\n");
            else
                printf(" | ULASaida=%d\n", cpu->inter.ULASaida);

            break;
        }

        case 8: { // R Write Back
            if(ir->rd != 0)
                cpu->reg[ir->rd] = cpu->inter.ULASaida;

            printf(" | Reg[%d]=ULASaida=%d\n", ir->rd, cpu->inter.ULASaida);

            atualiza_estatisticas(cpu);
            cpu->instrucoes_exec++;
            break;
        }

        case 9: { // BEQ
            if(cpu->inter.A == cpu->inter.B){
                cpu->pc = cpu->inter.ULASaida;
                printf(" | Branch FEITO PC->%d\n", cpu->pc);
            } else {
                printf(" | Branch NAO FEITO\n");
            }

            atualiza_estatisticas(cpu);
            cpu->instrucoes_exec++;
            break;
        }

        case 10: { // JUMP
            cpu->pc = ir->addr;

            printf(" | PC->%d (jump)\n", cpu->pc);

            atualiza_estatisticas(cpu);
            cpu->instrucoes_exec++;
            break;
        }
    }

    cpu->estado_atual = proximo_estado(est, ir->opcode);
    cpu->ciclos_clock++;
    cpu->est.ciclos_clock = cpu->ciclos_clock;
}
 
void executa_instrucao(CPU *cpu){
    if(cpu->num_instrucoes == 0){
        printf("Nenhuma instrucao carregada.\n");
        return;
    }
    if(cpu->pc >= MEM){
        printf("PC fora da area de instrucoes (PC=%d).\n", cpu->pc);
        return;
    }

    printf("\n--- Executando instrucao (PC=%d) ---\n", cpu->pc);

    // executa ciclos ate completar uma instrucao inteira
    // uma instrucao termina quando o proximo estado eh FETCH (estado 0)
    do {
        executa_ciclo(cpu);
    } while(cpu->estado_atual != 0);

    printf("--- Instrucao completa (%d ciclos totais) ---\n", cpu->ciclos_clock);
}
void executa_programa(CPU *cpu){
    if(cpu->num_instrucoes == 0){ 
        printf("Nenhuma instrucao carregada.\n"); 
        return; 
    }

    printf("\n========== EXECUTANDO PROGRAMA ==========\n");

    int limite = MAX_MEM * 5; // limite de seguranca contra loops infinitos
    int ciclos_ini = cpu->ciclos_clock;

    while(cpu->pc < MEM && cpu->instrucoes_exec < MAX_MEM && limite > 0){
        executa_ciclo(cpu);
        limite--;
    }

    if(limite == 0)
        printf("\nATENCAO: Limite de ciclos atingido (possivel loop infinito).\n");

    printf("\n========== EXECUCAO FINALIZADA ==========\n");
    printf("Instrucoes executadas: %d\n", cpu->instrucoes_exec);
    printf("Ciclos de clock: %d\n", cpu->ciclos_clock - ciclos_ini);
    printf("CPI medio: %.2f\n", 
        cpu->instrucoes_exec > 0 ? (float)(cpu->ciclos_clock - ciclos_ini) / cpu->instrucoes_exec : 0.0);
}

// ESTATISTICAS
void atualiza_estatisticas(CPU *cpu){
    memoria *ir = &cpu->inter.IR;
    cpu->est.total_inst++;

    switch(ir->tipo){
        case tipo_R:
            cpu->est.total_r++;
            switch(ir->funct){
                case 0: cpu->est.add++; break;
                case 1: cpu->est.sub++; break;
                case 2: cpu->est.and_op++; break;
                case 3: cpu->est.or_op++; break;
            }
        break;
        case tipo_I:
            cpu->est.total_i++;
            switch(ir->opcode){
                case 4:  cpu->est.addi++; break;
                case 8:  cpu->est.beq++; break;
                case 11: cpu->est.lw++; break;
                case 15: cpu->est.sw++; break;
            }
        break;
        case tipo_J:
            cpu->est.total_j++;
            cpu->est.jump++;
        break;
        default: break;
    }
}

// VOLTAR
void volta_ciclo(CPU *cpu){
    if(cpu->i_hist == 0){
        printf("Nao ha estado anterior para voltar.\n");
        return;
    }
    cpu->i_hist--;
    salva_cpu *h = &cpu->historico[cpu->i_hist];
    cpu->pc = h->pc;
    cpu->estado_atual = h->estado;
    cpu->instrucoes_exec = h->instrucoes_exec;
    memcpy(cpu->reg, h->reg, sizeof(cpu->reg));
    cpu->inter = h->inter;
    cpu->est = h->est;
    cpu->ciclos_clock--;
    cpu->est.ciclos_clock = cpu->ciclos_clock;
    for(int i = MEM; i < MAX_MEM; i++)
        cpu->memoria[i].dado = h->dados[i - MEM];
    printf("Voltou 1 ciclo. Estado atual: %d (%s), PC=%d\n", 
        cpu->estado_atual, NOME_ESTADO[cpu->estado_atual], cpu->pc);
}
void volta_instrucao(CPU *cpu){
    if(cpu->i_hist == 0){
        printf("Nao ha estado anterior para voltar.\n");
        return;
    }
    // volta ciclos ate encontrar um estado FETCH (estado 0)
    // ou ate acabar o historico
    do {
        if(cpu->i_hist == 0) break;
        cpu->i_hist--;
        salva_cpu *h = &cpu->historico[cpu->i_hist];
        cpu->pc = h->pc;
        cpu->estado_atual = h->estado;
        cpu->instrucoes_exec = h->instrucoes_exec;
        memcpy(cpu->reg, h->reg, sizeof(cpu->reg));
        cpu->inter = h->inter;
        cpu->est = h->est;
        cpu->ciclos_clock--;
        cpu->est.ciclos_clock = cpu->ciclos_clock;
        for(int i = MEM; i < MAX_MEM; i++)
            cpu->memoria[i].dado = h->dados[i - MEM];
    } while(cpu->estado_atual != 0);

    printf("Voltou 1 instrucao. Estado atual: %d (%s), PC=%d\n",
        cpu->estado_atual, NOME_ESTADO[cpu->estado_atual], cpu->pc);
}
void reset_cpu(CPU *cpu){
    cpu->pc=0; cpu->estado_atual=0; cpu->ciclos_clock=0;
    cpu->instrucoes_exec=0; cpu->i_hist=0;
    memset(&cpu->est,0,sizeof(estatisticas));
    memset(&cpu->inter,0,sizeof(regs_inter));
    for(int i=0;i<MAX_REG;i++){
        cpu->reg[i]=0;
    }
    for(int i=MEM;i<MAX_MEM;i++){
        cpu->memoria[i].dado=0;
    }
    printf("Simulador resetado!\n");
}

void disassembla(memoria *mem, char *buf){
    switch(mem->opcode){
        case 0:
            switch(mem->funct){
                case 0: sprintf(buf,"add $r%d, $r%d, $r%d",mem->rd,mem->rs,mem->rt); break;
                case 1: sprintf(buf,"sub $r%d, $r%d, $r%d",mem->rd,mem->rs,mem->rt); break;
                case 2: sprintf(buf,"and $r%d, $r%d, $r%d",mem->rd,mem->rs,mem->rt); break;
                case 3: sprintf(buf,"or  $r%d, $r%d, $r%d",mem->rd,mem->rs,mem->rt); break;
                default: sprintf(buf,"Instrução desconhecida"); break;
            } break;
        case 2:  sprintf(buf,"j %d",mem->addr); break;
        case 4:  sprintf(buf,"addi $r%d, $r%d, %d",mem->rt,mem->rs,mem->imm); break;
        case 8:  sprintf(buf,"beq $r%d, $r%d, %d",mem->rs,mem->rt,mem->imm); break;
        case 11: sprintf(buf,"lw $r%d, %d($r%d)",mem->rt,mem->imm,mem->rs); break;
        case 15: sprintf(buf,"sw $r%d, %d($r%d)",mem->rt,mem->imm,mem->rs); break;
        default: sprintf(buf,"Instrução desconhecida"); break;
    }
}

// PRINT
void print_mem(CPU *cpu){

    printf("\n+------+------------------+----------------------------+--------+\n");
    printf("|                      Memoria Unificada                        |\n");
    printf("+------+------------------+----------------------------+--------+\n");
    printf("| Addr | Binario          | Assembly                   |  Dado  |\n");
    printf("+------+------------------+----------------------------+--------+\n");

    for(int i = 0; i < MAX_MEM; i++){

        printf("| %4d | %-16s |", i, cpu->memoria[i].bin);

        if(cpu->memoria[i].tipo != tipo_dado){
            char buf[64];

            memoria temp = cpu->memoria[i];
            decoder(&temp);
            disassembla(&temp, buf);

            printf(" %-26s |", buf);

            // NÃO mostra dado pra instrução
            printf("        |\n");
        }
        else{
            printf(" %-26s |", "(dado)");

            // só aqui mostra valor
            printf(" %6d |\n", cpu->memoria[i].dado);
        }
    }

    printf("+------+------------------+----------------------------+--------+\n");
}
void print_regs(CPU *cpu){
    printf("\nBanco de Registradores:\n");
    printf("+------+--------+\n| Reg  |  Valor |\n+------+--------+\n");
    for(int i=0;i<MAX_REG;i++)
        printf("| %4s | %6d |\n",cpu->NOME_REG[i],cpu->reg[i]);
    printf("+------+--------+\n");
}
void print_inter(CPU *cpu){
    printf("\nRegistradores Intermediarios:\n");
    printf("+----------------+--------+\n");
    printf("| Registrador    |  Valor |\n");
    printf("+----------------+--------+\n");
    printf("| PC             | %6d |\n", cpu->pc);
    printf("| Estado Atual   | %6d |\n", cpu->estado_atual);
    printf("| IR.opcode      | %6d |\n", cpu->inter.IR.opcode);
    printf("| IR.rs          | %6d |\n", cpu->inter.IR.rs);
    printf("| IR.rt          | %6d |\n", cpu->inter.IR.rt);
    printf("| IR.rd          | %6d |\n", cpu->inter.IR.rd);
    printf("| IR.funct       | %6d |\n", cpu->inter.IR.funct);
    printf("| IR.imm         | %6d |\n", cpu->inter.IR.imm);
    printf("| IR.addr        | %6d |\n", cpu->inter.IR.addr);
    printf("| MDR            | %6d |\n", cpu->inter.MDR);
    printf("| A              | %6d |\n", cpu->inter.A);
    printf("| B              | %6d |\n", cpu->inter.B);
    printf("| ULASaida       | %6d |\n", cpu->inter.ULASaida);
    printf("+----------------+--------+\n");
}
void print_est(CPU *cpu){
    printf("\n============ ESTATISTICAS ============\n");
    printf("Ciclos de clock:     %d\n", cpu->ciclos_clock);
    printf("Instrucoes executadas: %d\n", cpu->instrucoes_exec);
    if(cpu->instrucoes_exec > 0)
        printf("CPI medio:           %.2f\n", (float)cpu->ciclos_clock / cpu->instrucoes_exec);
    printf("\n--- Por tipo ---\n");
    printf("Tipo R: %d  (add=%d, sub=%d, and=%d, or=%d)\n",
        cpu->est.total_r, cpu->est.add, cpu->est.sub, cpu->est.and_op, cpu->est.or_op);
    printf("Tipo I: %d  (addi=%d, lw=%d, sw=%d, beq=%d)\n",
        cpu->est.total_i, cpu->est.addi, cpu->est.lw, cpu->est.sw, cpu->est.beq);
    printf("Tipo J: %d  (jump=%d)\n", cpu->est.total_j, cpu->est.jump);
    printf("Total:  %d\n", cpu->est.total_inst);
    printf("======================================\n");
}
void print_complete(CPU *cpu){
    printf("\n=========== ESTADO DO SIMULADOR MULTICICLO ===========\n");
    print_regs(cpu);
    print_inter(cpu);
    print_mem(cpu);
}

// SALVAR ARQUIVOS
void salva_asm(CPU *cpu){
    if(cpu->num_instrucoes==0){ 
        printf("Nenhuma instrucao carregada.\n"); 
        return; 
    }
    char arq[50]; 
    printf("Nome do arquivo .asm: "); 
    limpa_buffer(); 
    scanf("%s",arq);
    strcat(arq,".asm");

    FILE *f=fopen(arq,"w");
    if(!f){ 
        printf("Erro ao criar arquivo.\n"); 
        return;
    }
    for(int i=0; i < cpu->num_instrucoes; i++){
        char linha_asm[64];
        memoria temp=cpu->memoria[i];
        decoder(&temp);
        disassembla(&temp,linha_asm);
        fprintf(f,"%s\n",linha_asm);
    }
    fclose(f); 
    printf("Arquivo '%s' salvo!\n",arq);
}

void salva_dat(CPU *cpu){
    char arq[50]; 
    printf("Nome do arquivo .dat: "); 
    limpa_buffer(); 
    scanf("%s",arq);
    strcat(arq,".dat");
    FILE *f=fopen(arq,"w");
    if(!f){ 
        printf("Erro ao criar arquivo.\n"); 
        return; 
    }
    for(int i=MEM;i<MAX_MEM;i++){
        fprintf(f,"%d\n",cpu->memoria[i].dado);
    }
    fclose(f); printf("Arquivo '%s' salvo!\n",arq);
}