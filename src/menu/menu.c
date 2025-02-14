/*  
===============================================================================  
Nome do Projeto : FATal32 
Descrição       : Esse é o código menu.c responsável por gerar as telas gráficas 
                  para o usuário.
Autor           : Matheus V. Costa  
Data de Criação : 25/12/2024  
Última Alteração: 14/02/2024  
===============================================================================  
*/

#include "menu.h"

void print_ascii_art() {
    printf("8888888888     d8888 88888888888       888  .d8888b.   .d8888b.  \n");
    printf("888           d88888     888           888 d88P  Y88b d88P  Y88b \n");
    printf("888          d88P888     888           888      .d88P        888 \n");
    printf("8888888     d88P 888     888   8888b.  888     8888\"       .d88P \n");
    printf("888        d88P  888     888      \"88b 888      \"Y8b.  .od888P\"  \n");
    printf("888       d88P   888     888  .d888888 888 888    888 d88P\"      \n");
    printf("888      d8888888888     888  888  888 888 Y88b  d88P 888\"       \n");
    printf("888     d88P     888     888  \"Y888888 888  \"Y8888P\"  888888888  \n");
    printf("                                                                 \n");
    printf("                                                                 \n");
}
void print_opcoes() {
    printf("---------------------- Help Sheet ----------------------\n");
    printf("                                                         \n");
    printf("info               - Exibe informações do disco e da FAT.\n");
    printf("cluster <num>      - Exibe o conteúdo do bloco <num> em texto.\n");
    printf("pwd                - Exibe o diretório corrente (caminho absoluto).\n");
    printf("attr <file | dir>  - Exibe atributos de um arquivo ou diretório.\n");
    printf("cd <path>          - Altera o diretório corrente para <path>.\n");
    printf("touch <file>       - Cria o arquivo <file> vazio.\n");
    printf("mkdir <dir>        - Cria o diretório <dir> vazio.\n");
    printf("rm <file>          - Remove o arquivo <file>.\n");
    printf("rmdir <dir>        - Remove o diretório <dir> se estiver vazio.\n");
    printf("cp <src> <dst>     - Copia um arquivo de <src> para <dst>.\n");
    printf("mv <src> <dst>     - Move um arquivo de <src> para <dst>.\n");
    printf("rename <file> <new>- Renomeia <file> para <new>.\n");
    printf("ls                 - Lista arquivos e diretórios do corrente.\n");
    printf("help               - Abre: Help Sheet.\n");
    printf("extra              - Abre: Extra Sheet.\n");
    printf("exit               - Sair do programa.\n");
    printf("------------------------------------------------------\n");
}

void print_extra()
{
    printf("-------------------- Extra Sheet ---------------------\n");
    printf("------------------------------------------------------\n");
    printf("------------------------------------------------------\n");
    printf("------------------------------------------------------\n");
    printf("------------------------------------------------------\n");
    printf("------------------------------------------------------\n");   
    printf("------------------------------------------------------\n");
}
                                                      


void gerar_menu() {

        print_ascii_art();
        print_opcoes();

}
