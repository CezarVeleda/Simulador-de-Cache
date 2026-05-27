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
    
    // Variáveis para leitura
    unsigned char bytes[4];

    unsigned int endereco;

    unsigned int tag;
    unsigned int indice;
    unsigned int offset;

    // Leitura do binario
    while (fread(bytes, 1, 4, arquivo) == 4) {

        /*
            Monta int de 32 bits tratando big endian
        */

        endereco = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

        // Extrai offset
        offset = endereco & ((1 << bits_offset) - 1);

        // Extrai índice
        indice = (endereco >> bits_offset) & ((1 << bits_indice) - 1);

        // Extrai tag
        tag = endereco >> (bits_offset + bits_indice);

        // Mostra resultados
        printf("Endereco: 0x%08X\n", endereco);

        printf("Tag: %u\n", tag);

        printf("Indice: %u\n", indice);

        printf("Offset: %u\n", offset);

    }


    fclose(arquivo);
    return 0;
}