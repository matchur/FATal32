#ifndef UTILS_H
#define UTILS_H

#include "../structs/fat32.h"
#include "../structs/tree.h"

// Constrói a árvore de diretórios a partir de uma imagem FAT32.
TreeNode* assimilate_tree(const char *imagePath);

// Libera a memória alocada para a árvore de diretórios.
void free_tree(TreeNode *root);

#endif // UTILS_H
