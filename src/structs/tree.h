#ifndef TREE_H
#define TREE_H

#include "fat32.h"

// Estrutura de um nó da árvore de diretórios.
typedef struct TreeNode {
    FAT32DirectoryEntry entry;         // Informações do arquivo/diretório.
    struct TreeNode *parent;           // Ponteiro para o nó pai.
    struct TreeNode *firstChild;       // Ponteiro para o primeiro filho.
    struct TreeNode *nextSibling;      // Ponteiro para o próximo irmão.
} TreeNode;

// Funções principais para manipulação da árvore.
TreeNode *create_tree(FAT32Partition *partition);
TreeNode *create_node(FAT32DirectoryEntry *entry);
void build_tree(TreeNode *parent, FAT32Partition *partition);
void print_tree(TreeNode *root, int depth);
TreeNode *find_node(TreeNode *root, const char *path);
void delete_subtree(TreeNode *node);
void delete_tree(TreeNode **root);

#endif // TREE_H
