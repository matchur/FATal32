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

# Arquivos de origem
SRC_FILES = main.c \
            $(COMMANDS_DIR)/cmd.c \
            $(MENU_DIR)/menu.c \
            $(FAT32_DIR)/fat32.c \
            $(UTILS_DIR)/util.c

# Arquivos objeto
OBJ_FILES = $(SRC_FILES:.c=.o)
OBJ_FILES = $(OBJ_FILES:%=$(BUILD_DIR)/%)

# Regra principal
all: $(BUILD_DIR)/$(EXEC)

# Como compilar os arquivos .o a partir dos .c
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(BUILD_DIR)/$(*D)
	$(CC) $(CFLAGS) -c $< -o $@

# Como compilar o executável final
$(BUILD_DIR)/$(EXEC): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $@

# Limpeza
clean:
	rm -rf $(BUILD_DIR)

# Recompilar tudo
rebuild: clean all
