TARGETS = $(addprefix $(BIN_DIR)/,$(libdivsufsort_TARGETS))
GENERATED = $(libdivsufsort_GEN_HDR)
SRC = $(libdivsufsort_SOURCES)

CFLAGS = -Wall
LDFLAGS =
ifeq ($(DEBUG),)
CFLAGS += -O3
else
CFLAGS += -g
endif

CC_V := $(shell LANG=C $(CC) -v 2>&1)

libdivsufsort_EXAMPLES = bwt mksary sasearch suftest unbwt
libdivsufsort_TARGETS = libdivsufsort.a $(libdivsufsort_EXAMPLES)
libdivsufsort_CFLAGS = -fomit-frame-pointer -D__STRICT_ANSI__ -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -Ilibdivsufsort/include -I.
ifneq ($(findstring --enable-libgomp,$(CC_V))$(findstring --enable-gnu,$(CC_V)),)
libdivsufsort_CFLAGS += -fopenmp
libdivsufsort_LDFLAGS += -fopenmp
endif
libdivsufsort_CFLAGS += $(addprefix -D,HAVE_STDDEF_H HAVE_STDLIB_H HAVE_STRING_H HAVE_STRINGS_H HAVE_MEMORY_H HAVE_SYS_TYPES_H INLINE=inline PROJECT_VERSION_FULL=\"\")
divsufsort_SRCS = divsufsort.c sssort.c trsort.c utils.c
libdivsufsort_a_SRC = $(addprefix libdivsufsort/lib/, $(divsufsort_SRCS))
$(foreach prog,$(libdivsufsort_EXAMPLES),$(eval $(prog)_SRC = libdivsufsort/examples/$(prog).c libdivsufsort.a))
libdivsufsort_SOURCES = $(filter-out %.a, $(libdivsufsort_a_SRC) $(foreach prog,$(libdivsufsort_EXAMPLES),$(value $(prog)_SRC)))
libdivsufsort_GEN_HDR = divsufsort.h lfs.h

TARGETS += $(addprefix $(BIN_DIR)/,$(bwsearch_TARGETS))
SRC += $(bwsearch_SOURCES)
bwsearch_TARGETS = mkbws unbws bws
bwsearch_TARGETS += chkbws
bwsearch_CFLAGS = -ansi -pedantic -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE
bwsearch_CFLAGS += -g
ifneq ($(findstring --enable-libgomp,$(CC_V))$(findstring --enable-gnu,$(CC_V)),)
bwsearch_CFLAGS += -fopenmp
endif
uniq = $(if $1,$(firstword $1) $(call uniq,$(filter-out $(firstword $1),$1)))
bwsearch_SOURCES = $(call uniq,$(filter-out %.a, $(foreach prog,$(bwsearch_TARGETS),$(value $(prog)_SRC))))
mkbws_SRC = mkbws.c libdivsufsort.a
unbws_SRC = unbws.c bwslib.c bwslib2.c libdivsufsort.a
bws_SRC = bws.c bwslib.c
chkbws_SRC = chkbws.c bwslib.c bwslib2.c

DEP_DIR = deps
OBJ_DIR = objs
BIN_DIR = bin
src2obj = $(addprefix $(OBJ_DIR)/,$(filter %.o,$(1:.c=.o))) $(addprefix $(BIN_DIR)/,$(filter %.a,$1)) $(filter-out %.c %.a,$1)

ifneq ($(shell which gsed 2>/dev/null),)
SED = gsed
endif
SED ?= sed

BHOST := $(shell LANG=C $(CC) -v 2>&1 | $(SED) -n '/^Target: /{s/^.*: //;p}')
ifeq ($(BHOST),)
$(error Unrecognized compiler)
endif
ifeq ($(findstring mingw,$(BHOST)),)
DOT_EXE =
else
DOT_EXE = .exe
bwsearch_CFLAGS += -Wno-format #pedantic-ms-

TARGETS += $(addprefix $(BIN_DIR)/,$(mman_TARGETS))
SRC += $(mman_SOURCES)
GENERATED += $(mman_GEN_HDR)
mman_TARGETS = libmman.a
mman_CFLAGS = -fomit-frame-pointer
mman_SOURCES = $(libmman_a_SRC)
mman_GEN_HDR = mman.h
mman_DIR = mman-win32
libmman_a_SRC = $(mman_DIR)/mman.c
$(foreach prog,$(filter-out mkbws,$(bwsearch_TARGETS)),$(eval $(prog)_SRC += libmman.a))

ORIG_TARGETS := $(TARGETS)
TARGETS = $(addsuffix $(DOT_EXE),$(filter-out %.a,$(ORIG_TARGETS))) $(filter %.a,$(ORIG_TARGETS))
endif

all: $(TARGETS)

.PHONY: clean
clean:
	rm -rf $(TARGETS) $(DEP_DIR) $(OBJ_DIR) $(GENERATED)

$(libdivsufsort_SOURCES:%.c=$(DEP_DIR)/%.d): CFLAGS += $(libdivsufsort_CFLAGS)
$(libdivsufsort_SOURCES:%.c=$(DEP_DIR)/%.d): $(libdivsufsort_GEN_HDR)
$(libdivsufsort_GEN_HDR): %.h: libdivsufsort/include/%.h.cmake Makefile
	$(SED) 's/@INCFILE@/#include <inttypes.h>/g;s/@INLINE@/inline/g;s/@SAUCHAR_TYPE@/uint8_t/g;s/@SAINT32_TYPE@/int32_t/g;s/@SAINT32_PRId@/PRId32/g;s/@SAINT64_TYPE@/int64_t/g;s/@SAINT64_PRId@/PRId64/g;s/@DIVSUFSORT_IMPORT@//g;s/@DIVSUFSORT_EXPORT@//g;s/@W64BIT@//g;s/@SAINDEX_TYPE@/int32_t/g;s/@SAINDEX_PRId@/PRId32/g;s/@SAINT_PRId@/PRId32/g' $< > $@
$(bwsearch_SOURCES:%.c=$(DEP_DIR)/%.d): CFLAGS += $(bwsearch_CFLAGS)
$(bwsearch_SOURCES:%.c=$(DEP_DIR)/%.d): $(libdivsufsort_GEN_HDR)
$(bwsearch_SOURCES:%.c=$(OBJ_DIR)/%.o): Makefile
$(bwsearch_SOURCES:%.c=$(OBJ_DIR)/%.o): CFLAGS += $(bwsearch_CFLAGS)

$(BIN_DIR)/libdivsufsort.a: $(call src2obj, $(libdivsufsort_a_SRC))
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^

ifneq ($(findstring mingw,$(BHOST)),)
$(BIN_DIR)/libmman.a: CFLAGS += $(mman_CFLAGS)
$(BIN_DIR)/libmman.a: $(call src2obj, $(libmman_a_SRC))
	@mkdir -p $(@D)
	$(AR) $(ARFLAGS) $@ $^
$(bwsearch_SOURCES:%.c=$(DEP_DIR)/%.d): $(mman_GEN_HDR)
$(mman_GEN_HDR): %.h: $(mman_DIR)/%.h
	$(SED) '/^#ifndef.*\/\//s=//=\n&=' $< > $@
endif

$(addprefix $(BIN_DIR)/,$(addsuffix $(DOT_EXE),$(libdivsufsort_EXAMPLES))): CFLAGS += $(libdivsufsort_CFLAGS)

$(foreach prog,$(filter-out %.a,$(notdir $(if $(ORIG_TARGETS),$(ORIG_TARGETS),$(TARGETS)))),$(if $(filter libdivsufsort.a bwslib2.c,$(value $(prog)_SRC)),$(BIN_DIR)/$(prog)$(DOT_EXE))): LDFLAGS += $(libdivsufsort_LDFLAGS)
$(foreach prog,$(filter-out %.a,$(notdir $(if $(ORIG_TARGETS),$(ORIG_TARGETS),$(TARGETS)))),$(eval $(BIN_DIR)/$(prog)$(DOT_EXE): $(call src2obj, $(value $(prog)_SRC))))
$(filter-out %.a,$(TARGETS)):
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)

DEP = $(addprefix $(DEP_DIR)/,$(SRC:.c=.d))
findsrc = $(if $(filter $1,$(SRC)),$1,$(notdir $1))
src2tgt = $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.o,$(call findsrc,$1)))
src2tgt += $(addprefix $(OBJ_DIR)/,$(patsubst %.c,%.obj,$(call findsrc,$1)))
$(DEP_DIR)/%.d: %.c
	@mkdir -p $(@D)
	@$(CC) -MM $(CFLAGS) $< | $(SED) -n "H;$$ {g;s@.*:\(.*\)@$< := \$$\(wildcard\1\)\n$(call src2tgt,$<) $@: $$\($<\)@; p}" > $@
ifeq ($(filter clean,$(MAKECMDGOALS)),)
-include $(DEP)
endif
$(OBJ_DIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c -o $@ $(CFLAGS) $<

ifneq ($(shell which cl 2>/dev/null),)
.PHONY: compile_test
all: compile_test
compile_test: $(addprefix $(OBJ_DIR)/,$(addsuffix .obj,$(bwsearch_TARGETS)))
$(OBJ_DIR)/%.obj: %.c
	@mkdir -p $(@D)
	cl -nologo -c -Fo$@ -W3 -D_CRT_SECURE_NO_WARNINGS -DCOMPILE_TEST $<
endif
