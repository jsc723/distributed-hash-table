# tool macros
CC := g++ # FILL: the compiler
CCFLAGS := --std=c++11# FILL: compile flags
DBGFLAGS := -g
CCOBJFLAGS := $(CCFLAGS) -c
LIBRA := -lboost_system -lboost_thread -pthread

# path macros
BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
DBG_PATH := debug

# compile macros
TARGET_NAME := dh# FILL: target name
ifeq ($(OS),Windows_NT)
	TARGET_NAME := $(addsuffix .exe,$(TARGET_NAME))
endif
TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)

# src files & obj files
SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEBUG := $(addprefix $(DBG_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

PCH_SRC = $(SRC_PATH)/headers.hpp
PCH_OUT = $(SRC_PATH)/headers.hpp.gch

# clean files list
DISTCLEAN_LIST := $(OBJ) \
                  $(OBJ_DEBUG)
CLEAN_LIST := $(TARGET) \
			  $(TARGET_DEBUG) \
			  $(DISTCLEAN_LIST)

# default rule
default: makedir all

# non-phony targets
$(TARGET): $(OBJ)
	$(CC) $(CCFLAGS) -o $@ $(OBJ) $(LIBRA)

$(OBJ_PATH)/%.o: $(SRC_PATH)/%.c* $(PCH_OUT)
	$(CC) $(CCOBJFLAGS) -H -include $(PCH_SRC) -c -o $@ $<

$(DBG_PATH)/%.o: $(SRC_PATH)/%.c* $(PCH_OUT)
	$(CC) $(CCOBJFLAGS) $(DBGFLAGS) -H -c -o $@ $<

$(TARGET_DEBUG): $(OBJ_DEBUG)
	$(CC) $(CCFLAGS) $(DBGFLAGS) $(OBJ_DEBUG) -o $@ $(LIBRA)

$(PCH_OUT): $(PCH_SRC)
	$(CC) $(CCFLAGS) -o $@ $<

.PHONY: headers
headers:
	$(CC) $(CCFLAGS) $(PCH_SRC)

# phony rules
.PHONY: makedir
makedir:
	@mkdir -p $(BIN_PATH) $(OBJ_PATH) $(DBG_PATH)

.PHONY: all
all: $(TARGET)

.PHONY: debug
debug: $(TARGET_DEBUG)

.PHONY: clean
clean:
	@echo CLEAN $(CLEAN_LIST)
	@rm -f $(CLEAN_LIST)

.PHONY: distclean
distclean:
	@echo CLEAN $(DISTCLEAN_LIST)
	@rm -f $(DISTCLEAN_LIST)