#include <stdio.h>
#include <stdlib.h>

typedef struct {
    int valido;
    unsigned int tag;

    // Para FIFO/LRU
    unsigned long tempo;
} LinhaCache;

typedef struct {
    LinhaCache *linhas;
} Conjunto;

typedef struct {
    Conjunto *conjuntos;

    int nsets;
    int bsize;
    int assoc;
} Cache;

int log2_int(int valor) { // para os bits de offset e indice, offset=log2​(bsize) e indice=log2​(nsets) 
    int resultado = 0;

    while (valor > 1) {
        valor = valor / 2;
        resultado++;
    }

    return resultado;
}


int main(int argc, char *argv[]) {

    // Verifica argumentos
    if (argc != 7) {

        printf("Numero de argumentos incorreto.\n");
        printf("Use:\n");
        printf("./cache_simulator <nsets> <bsize> <assoc> <substituicao> <flag_saida> arquivo_de_entrada\n");

        return 1;
    }
    // Leitura dos argumentos da linha de comando 
    int nsets = atoi(argv[1]);
    int bsize = atoi(argv[2]);
    int assoc = atoi(argv[3]);
    char *subst = argv[4];
    int flagOut = atoi(argv[5]);
    char *arquivoEntrada = argv[6];

    // calculo da quantidade de bits
    int bits_offset = log2_int(bsize);

    int bits_indice = log2_int(nsets);

    int bits_tag = 32 - bits_offset - bits_indice;

    FILE *arquivo = fopen(arquivoEntrada, "rb");

    if (arquivo == NULL) {

        printf("Erro ao abrir arquivo.\n");

        return 1;
    }



    
    return 0;
}