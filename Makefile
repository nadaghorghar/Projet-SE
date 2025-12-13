CC = gcc
CFLAGS  = -Wall -Wextra -std=c99 -Isrc -Ipolitiques -pthread \
          $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = -pthread -rdynamic
LIBS = -ldl -lm $(shell pkg-config --libs gtk+-3.0)

SRC_DIR = src
POLITIQUES_DIR = politiques
BUILD_DIR = build
TARGET = ordonnanceur

SRC_FILES = $(wildcard $(SRC_DIR)/*.c) $(wildcard $(POLITIQUES_DIR)/*.c)
OBJECTS = $(patsubst %.c,$(BUILD_DIR)/%.o,$(notdir $(SRC_FILES)))

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

# Règle générique pour les fichiers .c dans src/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Règle générique pour les fichiers .c dans politiques/
$(BUILD_DIR)/%.o: $(POLITIQUES_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR) $(TARGET) politiques/*.so

rebuild: clean all

.PHONY: all clean rebuild
