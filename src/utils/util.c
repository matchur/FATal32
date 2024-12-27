#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "tree.h"
#include "fat32.h"

TreeNode* assimilate_tree(const char *imagePath) {
    // Inicializa a partição FAT32
    FAT32Partition partition;
    if (fat32_initialize(&partition, imagePath) != 0) {
        fprintf(stderr, "Erro ao inicializar a partição FAT32 a partir do arquivo: %s\n", imagePath);
        return NULL;
    }

    // Cria a árvore de diretórios a partir do diretório raiz
    TreeNode *root = create_tree(&partition);
    if (!root) {
        fprintf(stderr, "Erro ao criar a árvore de diretórios.\n");
        return NULL;
    }
    fat32_free_partition(&partition);
    printf("Árvore de diretórios criada com sucesso.\n");
    return root;
}

void free_tree(TreeNode *root) {
    if (root == NULL) return;

    // Libera recursivamente todos os nós filhos
    for (size_t i = 0; i < root->childCount; ++i) {
        free_tree(root->children[i]);
    }

    // Libera a memória alocada para o nó atual
    free(root->children); // Libera o array de filhos
    free(root);
}
