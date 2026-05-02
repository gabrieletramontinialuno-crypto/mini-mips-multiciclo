#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bib.h"

int main(){
    CPU cpu;
    inicializa_cpu(&cpu);
    int menu = -1;
    
    do {
        printf("\n-------------------  MINI MIPS 8 BITS MULTICICLO  -------------------\n"
            "1. Carregar memoria (.mem)\n"
            "2. Executar programa (Run)\n"
            "3. Executar uma instrucao completa (Step Instrucao)\n"
            "4. Executar um ciclo de clock (Step Ciclo)\n"
            "5. Voltar um ciclo (Back Step)\n"
            "6. Voltar instrução"
            "7. Resetar programa\n"
            "8. Imprimir memoria\n"
            "9. Imprimir banco de registradores\n"
            "10. Imprimir registradores intermediarios\n"
            "11. Imprimir todo o simulador\n"
            "12. Imprimir estatisticas\n"
            "13. Salvar .asm\n"
            "14. Salvar .dat\n"
            "0. Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &menu);

        switch(menu){
            case 1: 
                carrega_mem(&cpu); 
            break;
            case 2: 
                executa_programa(&cpu); 
            break;
            case 3: 
                executa_instrucao(&cpu); 
            break;
            case 4:
                executa_ciclo(&cpu); 
            break;
            case 5: 
                volta_ciclo(&cpu); 
            break;
            case 6: 
                volta_instrucao(&cpu); 
            break;
            case 7: 
                reset_cpu(&cpu); 
            break;
            case 8: 
                print_mem(&cpu); 
            break;
            case 9: 
                print_regs(&cpu); 
            break;
            case 10: 
                print_inter(&cpu); 
            break;
            case 11: 
                print_complete(&cpu); 
            break;
            case 12: 
                print_est(&cpu); 
            break;
            case 13: 
                salva_asm(&cpu); 
            break;
            case 14: 
                salva_dat(&cpu); 
            break;
            case 0: 
                printf("Encerrando.\n"); 
            break;
            default: 
                printf("Opcao invalida!\n"); 
            break;
        }
    } while(menu!=0);
    return 0;
}