CC := gcc
CFLAGS := -Wall -Wextra -std=c99 -O3 --pedantic -Wno-unused-parameter
LDFLAGS :=
SRC := ./eatmemory.c ./args/src/args.c
EXE := eatmemory
PREFIX := /usr/local
INSTALL_DIR := $(PREFIX)/bin

# Default target: Build the eatmemory program
all: $(EXE)

$(EXE): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# Install the executable to the specified PREFIX directory
install: $(EXE)
	mkdir -p $(INSTALL_DIR)
	install -m 755 $< $(INSTALL_DIR)

# Clean generated files
clean:
	rm -f $(EXE)

# Display help message
help:
	@echo "Usage: make [target] [PREFIX=/your/installation/path]"
	@echo "Targets:"
	@echo "  all (default) - Build the eatmemory program"
	@echo "  install       - Install the executable to PREFIX/bin"
	@echo "  clean         - Remove generated files"
	@echo "  help          - Display this help message"

.PHONY: all install clean help
