#ifndef CMD_H
#define CMD_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Funções para comandos do sistema de arquivos
void cmd_info();                // Exibe informações do disco e da FAT.
void cmd_cluster(int num);      // Exibe o conteúdo do bloco especificado no formato texto.
void cmd_pwd();                 // Exibe o diretório corrente (caminho absoluto).
void cmd_attr(const char *path); // Exibe os atributos de um arquivo ou diretório.
void cmd_cd(const char *path);  // Altera o diretório corrente para o especificado.
void cmd_touch(const char *file); // Cria um arquivo vazio.
void cmd_mkdir(const char *dir); // Cria um diretório vazio.
void cmd_rm(const char *file);   // Remove o arquivo especificado.
void cmd_rmdir(const char *dir); // Remove o diretório especificado, se estiver vazio.
void cmd_cp(const char *src, const char *dst); // Copia um arquivo de src para dst.
void cmd_mv(const char *src, const char *dst); // Move um arquivo de src para dst.
void cmd_rename(const char *file, const char *newname); // Renomeia um arquivo.
void cmd_ls();                  // Lista os arquivos e diretórios do diretório corrente.

#endif // CMD_H
