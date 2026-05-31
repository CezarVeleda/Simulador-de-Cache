#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

int log2_int(int valor) { // para os bits de offset e indice
    int resultado = 0;
    while (valor > 1) {
        valor = valor / 2;
        resultado++;
    }
    return resultado;
}

Cache *criarCache(int nsets, int bsize, int assoc) {
    Cache *cache = malloc(sizeof(Cache));
    cache->nsets = nsets;
    cache->bsize = bsize;
    cache->assoc = assoc;

    cache->conjuntos = malloc(nsets * sizeof(Conjunto));

    for (int i = 0; i < nsets; i++) {
        cache->conjuntos[i].linhas = malloc(assoc * sizeof(LinhaCache));
        for (int j = 0; j < assoc; j++) {
            cache->conjuntos[i].linhas[j].valido = 0;
            cache->conjuntos[i].linhas[j].tag = 0;
            cache->conjuntos[i].linhas[j].tempo = 0;
        }
    }
    return cache;
}

int escolherVitima(Cache *cache, int indice, char politica) {
    if (politica == 'R') {
        return rand() % cache->assoc;
    }

    if (politica == 'F' || politica == 'L') {
        int vitima = 0;
        unsigned long menorTempo = cache->conjuntos[indice].linhas[0].tempo;
        
        for (int i = 1; i < cache->assoc; i++) {
            if (cache->conjuntos[indice].linhas[i].tempo < menorTempo) {
                menorTempo = cache->conjuntos[indice].linhas[i].tempo;
                vitima = i;
            }
        }
        return vitima;
    }
    return 0;
}

void liberarCache(Cache *cache) {
    for (int i = 0; i < cache->nsets; i++) {
        free(cache->conjuntos[i].linhas);
    }
    free(cache->conjuntos);
    free(cache);
}

int main(int argc, char *argv[]) {
    // Semente fixa para garantir o determinismo no corretor automático
    srand(0);

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
    
    // Calculo da quantidade de bits
    int bits_offset = log2_int(bsize);
    int bits_indice = log2_int(nsets);
    int bits_tag = 32 - bits_offset - bits_indice;
    
    if (nsets <= 0 || bsize <= 0 || assoc <= 0) {
        printf("Parametros invalidos\n");
        return 1;
    }
    if (subst[0] != 'R' && subst[0] != 'F' && subst[0] != 'L') {
        printf("Politica invalida\n");
        return 1;
    }

    Cache *cache = criarCache(nsets, bsize, assoc);

    FILE *arquivo = fopen(arquivoEntrada, "rb");
    if (arquivo == NULL) {
        perror("Erro ao abrir arquivo");
        return 1;
    }
    
    // Variáveis para leitura
    unsigned char bytes[4];
    unsigned int endereco;
    unsigned int tag;
    unsigned int indice;
    unsigned int offset;

    unsigned long acessos = 0;
    unsigned long hits = 0;
    unsigned long misses = 0;
    unsigned long relogio = 0;

    unsigned long missCompulsorio = 0;
    unsigned long missCapacidade = 0;
    unsigned long missConflito = 0;

    // Lógica global para desempate de Capacidade x Conflito
    int blocos_validos = 0;
    int max_blocos = nsets * assoc;
    
    /* Leitura do binário */
    while (fread(bytes, 1, 4, arquivo) == 4) {

        endereco = (bytes[0] << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];

        /* Extrai offset, indice e tag */
        offset = endereco & ((1 << bits_offset) - 1);
        indice = (endereco >> bits_offset) & ((1 << bits_indice) - 1);
        tag = endereco >> (bits_offset + bits_indice);
        
        acessos++;
        relogio++;

        int hit = 0;
        
        /* Procura HIT */
        for (int i = 0; i < assoc; i++) {
            LinhaCache *linha = &cache->conjuntos[indice].linhas[i];

            if (linha->valido && linha->tag == tag) {
                hit = 1;
                hits++;

                // Apenas LRU atualiza o tempo de acesso no HIT
                if (subst[0] == 'L') {
                    linha->tempo = relogio;
                }
                break;
            }
        }

        /* MISS */
        if (!hit) {
            misses++;
            int inserido = 0;
            
            /* 1. Procura linha vazia (Miss Compulsório) */
            for (int i = 0; i < assoc; i++) {
                LinhaCache *linha = &cache->conjuntos[indice].linhas[i];

                if (!linha->valido) {
                    linha->valido = 1;
                    linha->tag = tag;
                    linha->tempo = relogio;
                    inserido = 1;
                    
                    missCompulsorio++;
                    blocos_validos++; // Aumenta a lotação global da cache
                    break;
                }
            }

            /* 2. Se não inseriu (conjunto estava cheio), precisa substituir e classificar */
            if (!inserido) {
                
                // Se a cache inteira lotou, é falta por Capacidade. Caso contrário, Conflito.
                if (blocos_validos == max_blocos) {
                    missCapacidade++;
                } else {
                    missConflito++;
                }

                // Sorteia/Calcula a vítima e a sobrescreve
                int vitima = escolherVitima(cache, indice, subst[0]);
                cache->conjuntos[indice].linhas[vitima].tag = tag;
                cache->conjuntos[indice].linhas[vitima].tempo = relogio;
            }
        }
    }

    double taxaHit = (double)hits / acessos;
    double taxaMiss = (double)misses / acessos;
    double taxaCompulsorio = 0;
    double taxaCapacidade = 0;
    double taxaConflito = 0;

    if (misses > 0) {
        taxaCompulsorio = (double)missCompulsorio / misses;
        taxaCapacidade = (double)missCapacidade / misses;
        taxaConflito = (double)missConflito / misses;
    }

    if (flagOut == 1) {
        printf("%lu %.4f %.4f %.4f %.4f %.4f\n",
            acessos, taxaHit, taxaMiss, taxaCompulsorio, taxaCapacidade, taxaConflito);
    } else {
        printf("Total de acessos: %lu\n", acessos);
        printf("Hits: %lu\n", hits);
        printf("Misses: %lu\n", misses);
        printf("Taxa de hit: %.4f\n", taxaHit);
        printf("Taxa de miss: %.4f\n", taxaMiss);
        printf("Misses compulsorios: %lu\n", missCompulsorio);
        printf("Misses capacidade: %lu\n", missCapacidade);
        printf("Misses conflito: %lu\n", missConflito);
    }

    liberarCache(cache);
    fclose(arquivo);
    return 0;
}