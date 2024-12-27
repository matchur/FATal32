#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <time.h>
#include "src/shell/shell.h"
#include "src/utils/util.h"

// Declaração do ponteiro global para a árvore de diretórios
TreeNode *rootNode = NULL;

int main(int argc, char *argv[]) {
    const char *imagePath = validate_arguments(argc, argv);
    if (imagePath == NULL) {
        fprintf(stderr, "Erro: Arquivo FAT32 não especificado ou inválido.\n");
        printf("Passe o nome do arquivo com /.fatal32 <imagem.iso>\n");
        printf("Ou especifique o caminho absoluto /.fatal32 -r <caminho absoluto/imagem.iso> \n");
        return EXIT_FAILURE;
    }

    printf("Carregando FATal32... %s\n", imagePath);
    
    rootNode = assimilate_tree(imagePath);
    if (rootNode == NULL) {
        fprintf(stderr, "Erro ao assimilar a árvore de diretórios.\n");
        return EXIT_FAILURE;
    }


    // Chama o shell com a imagem carregada.
    start_shell(imagePath);

    free_tree(rootNode);
    return EXIT_SUCCESS;
}
