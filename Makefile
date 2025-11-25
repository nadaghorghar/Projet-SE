CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -Isrc
SRC_DIR = src
BUILD_DIR = build

# Fichiers source principaux
SRC = $(SRC_DIR)/main.c $(SRC_DIR)/menu.c $(SRC_DIR)/process.c

# Fichiers objets (uniquement les fichiers centraux)
OBJ = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

EXEC = ordonnanceur

all: $(BUILD_DIR) $(EXEC)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Compilation du programme principal
$(EXEC): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ -ldl

# Compilation des .o du dossier src/
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Nettoyage
clean:
	rm -rf $(BUILD_DIR) $(EXEC)

# Installation facultative
install: $(EXEC)
	@if [ -w /usr/local/bin ]; then \
		cp $(EXEC) /usr/local/bin/; \
		echo "Installé dans /usr/local/bin"; \
	else \
		echo "Installé dans le répertoire courant (pas de droits admin)"; \
	fi

.PHONY: all clean install
