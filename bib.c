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

}
sinais gera_sinais(int estado, int funct){
   sinais s = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
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