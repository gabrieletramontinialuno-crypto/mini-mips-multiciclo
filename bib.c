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

                if (valido) {
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
    int v=0; for(int i=0;i<16;i++) v=(v<<1)|(b[i]=='1'?1:0);
    if(v&0x8000) v|=~0xFFFF;
    return v;
}

// UNIT CONTROL
void decoder(memoria *mem){
    char *b = mem->bin;

    mem->opcode = separa_bits(b, 0, 4);

    if(mem->opcode == 0){ // tipo R
        mem->tipo = tipo_R;
        mem->rs    = separa_bits(b, 4, 3);
        mem->rt    = separa_bits(b, 7, 3);
        mem->rd    = separa_bits(b,10, 3);
        mem->funct = separa_bits(b,13, 3);
    }
    else if(mem->opcode == 2){ // jump
        mem->tipo = tipo_J;
        mem->addr = bits_jump(b);
    }
    else { // tipo I
        mem->tipo = tipo_I;
        mem->rs  = separa_bits(b, 4, 3);
        mem->rt  = separa_bits(b, 7, 3);
        mem->imm = bits_imm(b,10, 6);
    
}

}
sinais gera_sinais(int estado, int funct){

}

// ULA
int ula(int A, int B, int ctrl, int *ovf, int *zero){

}

// SALVA ESTADO PARA VOLTAR
void salvar_estado(CPU *cpu){

}

// EXECUCAO
int proximo_estado(int estado, int opcode){

}
void executa_ciclo(CPU *cpu){

    int est = cpu->estado_atual;
   memoria *ir = &cpu->inter.IR;

    printf("  [Ciclo %d] Estado %d (%s)", cpu->ciclos_clock, est, NOME_ESTADO[est]);

    switch(est){

        case 0: { // FETCH
            salvar_estado(cpu);

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

        case 2: { // MEM_ADDR
            int ovf, zero;
            cpu->inter.ULASaida = ula(cpu->inter.A, ir->imm, 0, &ovf, &zero);

            printf(" | ULASaida=A(%d)+imm(%d)=%d\n",
                cpu->inter.A, ir->imm, cpu->inter.ULASaida);
            break;
        }

        case 3: { // MEM_READ (lw)
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

        case 4: { // LW WB
            if(ir->rt != 0) // protege $0
                cpu->reg[ir->rt] = cpu->inter.MDR;

            printf(" | Reg[%d]=MDR=%d\n", ir->rt, cpu->inter.MDR);

            atualiza_estatisticas(cpu);
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

        case 7: { // R EXEC
            int ovf, zero;
            cpu->inter.ULASaida = ula(cpu->inter.A, cpu->inter.B, ir->funct, &ovf, &zero);

            if(ovf)
                printf(" | OVERFLOW!\n");
            else
                printf(" | ULASaida=%d\n", cpu->inter.ULASaida);

            break;
        }

        case 8: { // R WB
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
                printf(" | Branch TAKEN PC->%d\n", cpu->pc);
            } else {
                printf(" | Branch NOT taken\n");
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
    
}
void executa_programa(CPU *cpu){
    if(cpu->num_instrucoes==0){ 
        printf("Nenhuma instrucao carregada.\n"); 
        return; 
    }
    while(cpu->pc<MEM && cpu->instrucoes_exec<MAX_MEM){
        executa_instrucao(cpu);
    }
    printf("\nExecucao finalizada: %d instrucoes, %d ciclos de clock.\n", cpu->instrucoes_exec, cpu->ciclos_clock);
}

// ESTATISTICAS
void atualiza_estatisticas(CPU *cpu){

}

// VOLTAR
void volta_ciclo(CPU *cpu){

}
void volta_instrucao(CPU *cpu){

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

            // 👇 NÃO mostra dado pra instrução
            printf("        |\n");
        }
        else{
            printf(" %-26s |", "(dado)");

            // 👇 só aqui mostra valor
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

}
void print_est(CPU *cpu){

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