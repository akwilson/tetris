# Makefile to build a shared object and/or a binary executable and/or unit test executable from C code
# Include this file in your own Makefile
# Places object, dependency files and output files in $(BUILDDIR)
# $(BUILDDIR) created if it does not exist
# $(INCLUDE_PATH) to specify header file search path; $(LIB_PATH) to specify library search path
# e.g. INCLUDE_PATH = -I/usr/local/include -I/some/path/include
#      LIB_PATH = -L/usr/local/lib -L/some/path/lib
# Compiles using full warnings and rebuilds when header files are changed
# `make clean` will remove all output from $(BUILDDIR)
# `make tests` will run the unit test binary
# `make install` will install the bin and lib targets in /usr/local

ifeq ($(BUILDDIR),)
	BUILDDIR = build
endif

LIB1_OUT = $(addprefix $(BUILDDIR)/, $(LIB1))
LIB1_OBJS = $(LIB1_SRCS:%.c=$(addprefix $(BUILDDIR)/, %.o))
LIB1_DEPS = $(LIB1_OBJS:%.o=%.d)

BIN1_OUT = $(addprefix $(BUILDDIR)/, $(BIN1))
BIN1_OBJS = $(BIN1_SRCS:%.c=$(addprefix $(BUILDDIR)/, %.o))
BIN1_DEPS = $(BIN1_OBJS:%.o=%.d)

TST1_OUT = $(addprefix $(BUILDDIR)/, $(TST1))
TST1_OBJS = $(TST1_SRCS:%.c=$(addprefix $(BUILDDIR)/, %.o))
TST1_DEPS = $(TST1_OBJS:%.o=%.d)

WASM1_OUT = $(addprefix $(BUILDDIR)/, $(WASM1))
WASM_CORE = $(basename $(WASM1))
WASM1_JS = $(addprefix $(BUILDDIR)/, $(WASM_CORE).js)
WASM1_WASM = $(addprefix $(BUILDDIR)/, $(WASM_CORE).wasm)
WASM1_DATA = $(addprefix $(BUILDDIR)/, $(WASM_CORE).data)
WASM1_HTML = $(addprefix $(BUILDDIR)/, $(WASM_CORE).html)
WASM_CC = emcc
ifeq ($(WASM1_USE_SDL2),Y)
	WASM_EXTRAS = -s USE_SDL=2 -s USE_SDL_IMAGE=2 -s SDL2_IMAGE_FORMATS='["png"]' -s USE_SDL_TTF=2
endif
ifneq ($(WASM1_ASSETS),)
	WASM1_ASSETS_ARGS = --preload-file $(WASM1_ASSETS)
endif

CFLAGS = -g -I. $(INCLUDE_PATH) -Wall -Wextra
#LDFLAGS = -linker_flags

$(BUILDDIR)/%.o : %.c
	$(COMPILE.c) -MMD -fpic -o $@ $<

all : directories $(LIB1_OUT) $(BIN1_OUT) $(TST1_OUT) $(WASM1_OUT)

$(LIB1_OUT) : $(LIB1_OBJS)
	$(LINK.c) $^ -shared -o $@

$(BIN1_OUT) : $(BIN1_OBJS)
	$(LINK.c) $^ $(LIB_PATH) $(LIBS) -o $@

$(TST1_OUT) : $(TST1_OBJS)
	$(LINK.c) $^ $(LIB_PATH) $(LIBS) -o $@

$(WASM1_OUT) : $(WASM1_SRCS)
	$(WASM_CC) $^ $(WASM_EXTRAS) $(WASM1_ASSETS_ARGS) -o $@

-include $(LIB1_DEPS) $(BIN1_DEPS) $(TST1_DEPS)

ifeq ($(PREFIX),)
	PREFIX = /usr/local
endif

.PHONY: directories clean test install
directories :
	@mkdir -p $(BUILDDIR)

clean :
	rm -f $(LIB1_OBJS) $(LIB1_DEPS) $(LIB1_OUT) \
		  $(BIN1_OBJS) $(BIN1_DEPS) $(BIN1_OUT) \
		  $(TST1_OBJS) $(TST1_DEPS) $(TST1_OUT) \
		  $(WASM1_HTML) $(WASM1_JS) $(WASM1_DATA) $(WASM1_WASM)

tests :
	@LD_LIBRARY_PATH=$(LIB_PATH) DYLD_LIBRARY_PATH=$(LIB_PATH) $(TST1_OUT)

INSTALL_H = $(HEADERS:%.h=$(addprefix $(DESTDIR)$(PREFIX)/include/, %.h))
INSTALL_L = $(addprefix $(DESTDIR)$(PREFIX)/lib/, $(LIB1))
INSTALL_B = $(addprefix $(DESTDIR)$(PREFIX)/bin/, $(BIN1))

install : $(INSTALL_H) $(INSTALL_L) $(INSTALL_B)

$(INSTALL_H) : $(HEADERS)
	install -d $(DESTDIR)$(PREFIX)/include
	install -m 644 $^ $(DESTDIR)$(PREFIX)/include

$(INSTALL_L) : $(LIB1_OUT)
	install -d $(DESTDIR)$(PREFIX)/lib
	install -m 644 $(LIB1_OUT) $(DESTDIR)$(PREFIX)/lib
	@if [ `uname` = "Linux" ]; then echo "*** Library installation complete. You may need to run 'sudo ldconfig' ***"; fi

$(INSTALL_B) : $(BIN1_OUT)
	install -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 $(BIN1_OUT) $(DESTDIR)$(PREFIX)/bin
