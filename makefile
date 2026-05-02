compila:
	gcc bib.c -c
	gcc main.c bib.o -o main
executa:
	./main
apaga:
	rm bib.o
	rm main