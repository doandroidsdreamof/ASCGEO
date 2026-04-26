# ascgeo - ASCII Geolocation Network Monitor

CC = gcc
# CFLAGS = -Wall -Wextra -Werror -pedantic -std=c17 -g -Iinclude
CFLAGS = -Wall -Wextra -pedantic -std=c17 -g -Iinclude
LDFLAGS = -lncurses

SRC_DIR = src
OBJ_DIR = build
INC_DIR = include

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

TARGET = ascgeo

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)

.PHONY: all clean run