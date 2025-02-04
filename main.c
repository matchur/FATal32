#include <stdio.h>
#include <stdlib.h>
#include "src/shell/shell.h"
#include "src/utils/utils.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: %s <caminho_absoluto_para_imagem_fat32>\n", argv[0]);
        return EXIT_FAILURE;
    }

    const char *image_path = argv[1];

    if (!file_exists(image_path)) {
        fprintf(stderr, "ERRO FATAL: Arquivo %s não encontrado. Verifique o caminho e tente novamente.\n", image_path);
        return EXIT_FAILURE;
    }

    // Chama o shell com o caminho da imagem.
    start_shell(image_path);

    return EXIT_SUCCESS;
}