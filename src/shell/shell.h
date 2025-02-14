/*  
===============================================================================  
Nome do Projeto : FATal32 
Descrição       : Esse é o cabeçalho do shell.c
Autor           : Matheus V. Costa  
Data de Criação : 25/12/2024  
Última Alteração: 14/02/2024  
===============================================================================  
*/
#ifndef SHELL_H
#define SHELL_H


#include "../commands/cmd.h"
#include "../menu/menu.h"
#include "../utils/utils.h"
#include "../structs/fat32.h"

extern FAT32Partition *root;
extern char absolute_image_path[256];
extern char program_path[256];
extern char current_path[256];

void start_shell(const char *image_path);
void ctrl_terminal();
const char* validate_arguments(int argc, char *argv[]);

#endif // SHELL_H
