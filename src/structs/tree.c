#include "tree.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Cria um nó para a árvore baseado em uma entrada de diretório.
TreeNode *create_node(FAT32DirectoryEntry *entry) {
    TreeNode *node = (TreeNode *)malloc(sizeof(TreeNode));
    if (!node) {
        perror("Erro ao alocar memória para TreeNode");
        return NULL;
    }
    node->entry = *entry; // Copia os dados do diretório/arquivo.
    node->parent = NULL;
    node->firstChild = NULL;
    node->nextSibling = NULL;
    return node;
}

// Constrói a árvore recursivamente a partir de um diretório pai.
void build_tree(TreeNode *parent, FAT32Partition *partition) {
    FAT32DirectoryEntry entries[256];
    uint32_t firstCluster = (parent->entry.firstClusterHigh << 16) | parent->entry.firstClusterLow;
    int count = fat32_read_directory(partition, firstCluster, entries);

    if (count < 0) {
        printf("Erro ao ler o diretório: %s\n", parent->entry.name);
        return;
    }

    TreeNode *lastChild = NULL;
    for (int i = 0; i < count; ++i) {
        if (entries[i].attributes & FAT32_ATTR_VOLUME_ID) {
            continue; // Ignora volumes.
        }

        TreeNode *child = create_node(&entries[i]);
        child->parent = parent;

        if (!lastChild) {
            parent->firstChild = child;
        } else {
            lastChild->nextSibling = child;
        }
        lastChild = child;

        // Se for um diretório, constrói seus filhos.
        if (entries[i].attributes & FAT32_ATTR_DIRECTORY) {
            build_tree(child, partition);
        }
    }
}

// Cria a árvore a partir do diretório raiz.
TreeNode *create_tree(FAT32Partition *partition) {
    FAT32DirectoryEntry rootEntry;
    if (fat32_get_root_directory(partition, &rootEntry) != 0) {
        printf("Erro ao obter o diretório raiz.\n");
        return NULL;
    }

    TreeNode *root = create_node(&rootEntry);
    build_tree(root, partition);
    return root;
}

// Exibe a árvore de forma textual.
void print_tree(TreeNode *root, int depth) {
    if (!root) return;

    for (int i = 0; i < depth; ++i) {
        printf("  ");
    }
    printf("%s\n", root->entry.name);

    print_tree(root->firstChild, depth + 1);
    print_tree(root->nextSibling, depth);
}

// Busca um nó pelo caminho absoluto.
TreeNode *find_node(TreeNode *root, const char *path) {
    if (!root || strcmp(path, "/") == 0) {
        return root;
    }

    char *pathCopy = strdup(path);
    char *token = strtok(pathCopy, "/");
    TreeNode *current = root->firstChild;

    while (token) {
        while (current && strcmp(current->entry.name, token) != 0) {
            current = current->nextSibling;
        }

        if (!current) {
            free(pathCopy);
            return NULL; // Diretório não encontrado.
        }

        token = strtok(NULL, "/");
        if (token) {
            current = current->firstChild;
        }
    }

    free(pathCopy);
    return current;
}

// Exclui uma subárvore recursivamente.
void delete_subtree(TreeNode *node) {
    if (!node) return;

    delete_subtree(node->firstChild);
    delete_subtree(node->nextSibling);
    free(node);
}

// Exclui toda a árvore e libera a memória.
void delete_tree(TreeNode **root) {
    if (!root || !*root) return;

    delete_subtree(*root);
    *root = NULL;
}
