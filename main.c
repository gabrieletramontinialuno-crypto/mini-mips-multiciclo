#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "bib.h"

int main() {
  CPU cpu;
  inicializa_cpu(&cpu);
  int menu;
  do {
    printf("\n-------------------  MINI MIPS 8 BITS MULTICICLO  "
           "-------------------\n"
           " 1. Carregar memoria (.mem)\n"
           " 2. Executar programa (Run)\n"
           " 3. Executar uma instrucao completa (Step Instrucao)\n"
           " 4. Executar um ciclo de clock (Step Ciclo)\n"
           " 5. Voltar um ciclo (Back Step)\n"
           " 6. Resetar programa\n"
           " 7. Imprimir memoria\n"
           " 8. Imprimir banco de registradores\n"
           " 9. Imprimir registradores intermediarios\n"
           "10. Imprimir todo o simulador\n"
           "11. Imprimir Estatisticas\n"
           "12. Salvar .asm\n"
           "13. Salvar .dat\n"
           " 0. Sair\n"
           "Opcao: ");
    if (scanf("%d", &menu) != 1) {
      printf("Opcao invalida.\n");
      while (getchar() != '\n')
        ;
      menu = -1;
      continue;
    }
    switch (menu) {
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
      if (cpu.num_instrucoes == 0) {
        printf("Nenhuma instrucao carregada.\n");
        break;
      }
      if (cpu.estado_atual == 0 && cpu.memoria[cpu.pc].opcode == 5) {
        printf("Instrucao de parada no PC %d.\n", cpu.pc);
        break;
      }
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
  } while (menu != 0);
  return 0;
}