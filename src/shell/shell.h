#ifndef SHELL_H
#define SHELL_H


#include "../commands/cmd.h"
#include "../menu/menu.h"
#include "../utils/utils.h"
#include "../structs/fat32.h"

void start_shell(const char *image_path);
void ctrl_terminal();
const char* validate_arguments(int argc, char *argv[]);

#endif // SHELL_H
