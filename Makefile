CC = gcc
CFLAGS = -Wall -Wextra -g
SRC_DIR = src
BUILD_DIR = build
EXEC = fatal32

# Diretórios de origem
COMMANDS_DIR = $(SRC_DIR)/commands
MENU_DIR = $(SRC_DIR)/menu
FAT32_DIR = $(SRC_DIR)/fat32
UTILS_DIR = $(SRC_DIR)/utils
SHELL_DIR = $(SRC_DIR)/shell

# Arquivos de origem
SRC_FILES = main.c \
            $(COMMANDS_DIR)/cmd.c \
            $(MENU_DIR)/menu.c \
            $(FAT32_DIR)/fat32.c \
            $(UTILS_DIR)/util.c \
			$(SHELL_DIR)/shell.c

# Arquivos objeto
OBJ_FILES = $(addprefix $(BUILD_DIR)/, $(SRC_FILES:.c=.o))

# Regra principal
all: $(EXEC)

# Como compilar os arquivos .o a partir dos .c
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)/$(*D)
	$(CC) $(CFLAGS) -c $< -o $@

# Como compilar o executável final
$(EXEC): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(EXEC)

# Limpeza
clean:
	rm -rf $(BUILD_DIR) $(EXEC)

# Recompilar tudo
rebuild: clean all

