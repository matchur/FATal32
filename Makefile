CC = gcc
CFLAGS = -Wall -Wextra -g
SRC_DIR = src
BUILD_DIR = build
EXEC = fatal32

COMMANDS_DIR = commands
MENU_DIR = menu
UTILS_DIR = utils
SHELL_DIR = shell
STRUCTS_DIR = structs

SRC_FILES = main.c \
			$(SRC_DIR)/$(COMMANDS_DIR)/cmd.c \
			$(SRC_DIR)/$(MENU_DIR)/menu.c \
			$(SRC_DIR)/$(STRUCTS_DIR)/fat32.c \
			$(SRC_DIR)/$(UTILS_DIR)/utils.c \
			$(SRC_DIR)/$(SHELL_DIR)/shell.c

OBJ_FILES = $(BUILD_DIR)/main.o \
            $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(filter-out main.c, $(SRC_FILES)))

all: $(BUILD_DIR) $(EXEC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(EXEC): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $(EXEC) $(OBJ_FILES)

$(BUILD_DIR)/main.o: main.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(EXEC)

.PHONY: all clean
