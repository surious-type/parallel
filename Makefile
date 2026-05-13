CC ?= gcc
MPICC ?= mpicc
MPIRUN ?= mpirun

CFLAGS ?= -O2
OMPFLAGS ?= -fopenmp
LDLIBS ?= -lm

NP ?= 4

SRC_DIR := src
BIN_DIR := bin
BIN_STAMP := $(BIN_DIR)/.dir

SOURCES := $(wildcard $(SRC_DIR)/*.c)
PROGRAMS := $(patsubst $(SRC_DIR)/%.c,%,$(SOURCES))

RUN_TARGETS := $(addsuffix .run,$(PROGRAMS))
MPI_TARGETS := $(addsuffix .mpicc,$(PROGRAMS))
MPI_RUN_TARGETS := $(addsuffix .mpirun,$(PROGRAMS))

all: $(PROGRAMS)

$(PROGRAMS): %: $(SRC_DIR)/%.c | $(BIN_STAMP)
	$(CC) $(CFLAGS) $(OMPFLAGS) $< -o $(BIN_DIR)/$@ $(LDLIBS)

$(RUN_TARGETS): %.run: $(SRC_DIR)/%.c | $(BIN_STAMP)
	$(CC) $(CFLAGS) $(OMPFLAGS) $< -o $(BIN_DIR)/$* $(LDLIBS)
	./$(BIN_DIR)/$*

$(MPI_TARGETS): %.mpicc: $(SRC_DIR)/%.c | $(BIN_STAMP)
	$(MPICC) $(CFLAGS) $< -o $(BIN_DIR)/$@ $(LDLIBS)

$(MPI_RUN_TARGETS): %.mpirun: $(SRC_DIR)/%.c | $(BIN_STAMP)
	$(MPICC) $(CFLAGS) $< -o $(BIN_DIR)/$*.mpi $(LDLIBS)
	$(MPIRUN) -np $(NP) ./$(BIN_DIR)/$*.mpi

$(BIN_STAMP):
	mkdir -p $(BIN_DIR)
	touch $(BIN_STAMP)

list:
	@echo "C programs:"
	@printf "  %s\n" $(PROGRAMS)
	@echo
	@echo "Run targets:"
	@printf "  %s\n" $(RUN_TARGETS)
	@echo
	@echo "MPI build targets:"
	@printf "  %s\n" $(MPI_TARGETS)
	@echo
	@echo "MPI run targets:"
	@printf "  %s\n" $(MPI_RUN_TARGETS)

clean:
	rm -rf $(BIN_DIR)

.PHONY: all clean list
