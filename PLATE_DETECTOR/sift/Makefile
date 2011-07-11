CC	= gcc
CFLAGS	+= -O3
BIN_DIR	= ./bin
SRC_DIR	= ./src
DOC_DIR	= ./docs
INC_DIR	= ./include
LIB_DIR	= ./lib

all: docs
	$(MAKE) -C $(SRC_DIR) $@

docs:
	doxygen Doxyfile

libfeat.a:
	mkdir -p $(LIB_DIR)
	$(MAKE) -C $(SRC_DIR) -I $(INC_DIR) $@

clean:
	make -C $(SRC_DIR) $@;

distclean: clean docsclean
	rm -rf $(LIB_DIR)
	rm -rf $(BIN_DIR)

docsclean:
	rm -rf $(DOC_DIR)/html/

.PHONY: docs clean docsclean libfeat.a
