SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

EXE := $(BIN_DIR)/tetris
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

CFLAGS = -Wall -Iinclude
LFLAGS = -lglfw3 -framework Cocoa -framework OpenGL -framework IOKit

.PHONY: all clean run

all: $(EXE)

$(EXE): $(OBJ) | $(BIN_DIR)
	$(CC) $^ -o $@  $(LFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) -c $< -o $@ $(CFLAGS)

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

clean: 
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR)

run:
	@./$(EXE)
