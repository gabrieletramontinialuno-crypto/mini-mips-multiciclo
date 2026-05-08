#ifndef BIB_h
#define BIB_h

#define MAX_MEM 256
#define MAX_REG 8
#define MEM 128


typedef enum { 
    tipo_R, tipo_J, tipo_I, tipo_dado, tipo_outros 
} classe_inst;

typedef enum {
    S_FETCH=0, S_DECODE=1, S_MEM_ADDR=2, S_MEM_READ=3, S_MEM_WB=4,
    S_MEM_WRITE=5, S_ADDI_WB=6, S_R_EXEC=7, S_R_WB=8, S_BEQ=9, S_JUMP=10
} estado;



typedef struct {
    classe_inst tipo;
    char bin[17];
    int opcode, rs, rt, rd, funct, imm, addr;
    int dado;
} memoria;

typedef struct {
    memoria IR;
    int MDR, A, B, ULASaida;
} regs_inter;

typedef struct {
    int PCEsc, IouD, EscMem, IREsc, MemParaReg, EscReg;
    int ULAFonteA, ULAFonteB, ControleULA, PCFonte, RegDst, Branch;
    int le_mem;
} sinais;

typedef struct {
    int total_inst, add, sub, and_op, or_op;
    int addi, sw, lw, beq, jump;
    int total_r, total_i, total_j;
    int ciclos_clock;
} estatisticas;

typedef struct {
    int pc, estado, instrucoes_exec;
    char reg[MAX_REG];
    int dados[MEM];
    regs_inter inter;
    estatisticas est;
} salva_cpu;

typedef struct {
    int pc, estado_atual, ciclos_clock, instrucoes_exec, i_hist;
    memoria memoria[MAX_MEM];
    int num_instrucoes;
    char reg[MAX_REG];
    char *NOME_REG[MAX_REG];
    regs_inter inter;
    estatisticas est;
    salva_cpu historico[MAX_MEM];
} CPU;


void limpa_buffer();
void carrega_mem(CPU *cpu);
void inicializa_cpu(CPU *cpu);

int separa_bits(char *b, int ini, int nBits);
int bits_imm(char *b, int ini, int nBits);
int bits_jump(char *b);
int bin_to_int16(char *b);
void decoder(memoria *mem);
sinais gera_sinais(int estado, int funct); 

int ula(int A, int B, int ctrl, int *ovf, int *zero);
void salvar_estado(CPU *cpu); 
int proximo_estado(int estado, int opcode);
void executa_ciclo(CPU *cpu);
void executa_instrucao(CPU *cpu);
void executa_programa(CPU *cpu);
void atualiza_estatisticas(CPU *cpu);
void volta_ciclo(CPU *cpu);
void volta_instrucao(CPU *cpu);
void reset_cpu(CPU *cpu);

void disassembla(memoria *mem, char *buf);
void print_mem(CPU *cpu);
void print_regs(CPU *cpu);
void print_inter(CPU *cpu);
void print_est(CPU *cpu);
void print_complete(CPU *cpu);

void salva_asm(CPU *cpu);
void salva_dat(CPU *cpu);

#endif