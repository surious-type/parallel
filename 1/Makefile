CC = gcc
override CFLAGS += -fopenmp -lm

SRC_DIR = src
BIN_DIR = bin

%: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $(BIN_DIR)/$@

%.run: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $< -o $(BIN_DIR)/$* && ./$(BIN_DIR)/$*

clean:
	rm -rf $(BIN_DIR)/

.PHONY: all clean
