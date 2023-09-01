SRC_DIR = src
CFILES   = ${SRC_DIR}/Bitmap.c 
CFILES += ${SRC_DIR}/Quantize.c 
CFILES += ${SRC_DIR}/Dither.c 
CFILES += ${SRC_DIR}/Qualetize.c 
CFILES += ${SRC_DIR}/Tiles.c 
CFILES += ${SRC_DIR}/tilequant.c
OUT_DIR = bin
OUT_SO  = tilequant
OUT_BIN = ${OUT_DIR}/${OUT_SO}

.PHONY: prepare dll clean

all: prepare ${OUT_BIN}
dynamic: prepare solink

prepare:
	@mkdir -p ${OUT_DIR}

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
IS_UNIX = true
endif
ifeq ($(UNAME), Darwin)
IS_UNIX = true
endif
ifdef IS_UNIX
OUT_SO_EXT = ".so"
else
OUT_SO_EXIT = ".dll"
endif

${OUT_BIN}: ${CFILES}
	@echo "Generating $@ ..."
	@$(CC) -lm -g -Wall -Wextra $^ -o $@

solink: ${CFILES}
	@echo "Linking daynic archive $@ ... "
	@$(CC) -shared -o ${OUT_DIR}/$(OUT_SO)$(OUT_SO_EXT) -lm -g -Wall -fPIC -Wextra $(CFILES) -DDECLSPEC="$(DDECLSPEC)"

clean:
	@rm -rf ${OUT_DIR}/*
