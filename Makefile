CC = gcc
CFLAGS  = -Wall -Wextra -std=c99 -Isrc -Ipolitiques \
          $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = -ldl -lm -rdynamic \
          $(shell pkg-config --libs gtk+-3.0)

SRC_DIR = src
POLITIQUES_DIR = politiques
BUILD_DIR = build
TARGET = ordonnanceur

OBJECTS = \
	$(BUILD_DIR)/main.o \
	$(BUILD_DIR)/menu.o \
	$(BUILD_DIR)/process.o \
	$(BUILD_DIR)/gui.o \
	$(BUILD_DIR)/gui_globals.o \
	$(BUILD_DIR)/fifo.o \
	$(BUILD_DIR)/round_robin.o \
	$(BUILD_DIR)/priorite.o \
	$(BUILD_DIR)/multi_level.o \
	$(BUILD_DIR)/multi_level_static.o

all: $(BUILD_DIR) $(TARGET)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS)

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/menu.o: $(SRC_DIR)/menu.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/process.o: $(SRC_DIR)/process.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gui.o: $(SRC_DIR)/gui.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/gui_globals.o: $(SRC_DIR)/gui_globals.c
	$(CC) $(CFLAGS) -fPIC -c $< -o $@

$(BUILD_DIR)/fifo.o: $(POLITIQUES_DIR)/fifo.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/round_robin.o: $(POLITIQUES_DIR)/round_robin.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/priorite.o: $(POLITIQUES_DIR)/priorite.c
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/multi_level.o: $(POLITIQUES_DIR)/multi_level.c
	$(CC) $(CFLAGS) -c $< -o $@
	
$(BUILD_DIR)/multi_level_static.o: $(POLITIQUES_DIR)/multi_level_static.c
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm -rf $(BUILD_DIR) $(TARGET) politiques/*.so

rebuild: clean all

.PHONY: all clean rebuild
