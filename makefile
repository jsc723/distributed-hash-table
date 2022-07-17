# tool macros
CC := g++ # FILL: the compiler
CCFLAGS := --std=c++11# FILL: compile flags
DBGFLAGS := -g
CCOBJFLAGS := $(CCFLAGS) -c
LIBRA := -lboost_system -lboost_thread -pthread -lprotobuf

# path macros
BIN_PATH := bin
OBJ_PATH := obj
SRC_PATH := src
DBG_PATH := debug
CLIENT_SRC_PATH := src/client
CLIENT_OBJ_PATH := obj/client

# compile macros
TARGET_NAME := dhserver# FILL: target name
ifeq ($(OS),Windows_NT)
	TARGET_NAME := $(addsuffix .exe,$(TARGET_NAME))
endif
TARGET := $(BIN_PATH)/$(TARGET_NAME)
TARGET_DEBUG := $(DBG_PATH)/$(TARGET_NAME)
CLIENT_TARGET := $(BIN_PATH)/client

# src files & obj files
SRC := $(foreach x, $(SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
OBJ := $(addprefix $(OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))
OBJ_DEBUG := $(addprefix $(DBG_PATH)/, $(addsuffix .o, $(notdir $(basename $(SRC)))))

CLIENT_SRC := $(foreach x, $(CLIENT_SRC_PATH), $(wildcard $(addprefix $(x)/*,.c*)))
CLIENT_OBJ := $(addprefix $(CLIENT_OBJ_PATH)/, $(addsuffix .o, $(notdir $(basename $(CLIENT_SRC)))))

PCH_SRC = $(SRC_PATH)/headers.hpp
PCH_OUT = $(SRC_PATH)/headers.hpp.gch

# clean files list
DISTCLEAN_LIST := $(OBJ) \
                  $(OBJ_DEBUG) \
				  $(CLIENT_OBJ)
CLEAN_LIST := $(TARGET) \
			  $(TARGET_DEBUG) \
			  $(CLIENT_TARGET) \
			  $(DISTCLEAN_LIST)

# default rule
default: makedir proto all

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

$(CLIENT_TARGET): $(CLIENT_OBJ) $(OBJ)
	$(CC) $(CCFLAGS) -o $@ $(CLIENT_OBJ) obj/messages.pb.o obj/serializer.o $(LIBRA)

$(CLIENT_OBJ_PATH)/%.o: $(SRC_PATH)/%.c* $(PCH_OUT)
	$(CC) $(CCOBJFLAGS) -H -include $(PCH_SRC) -c -o $@ $<


.PHONY: client
client: $(CLIENT_TARGET)


# phony rules
.PHONY: makedir
makedir:
	@mkdir -p $(BIN_PATH) $(OBJ_PATH) $(DBG_PATH) $(CLIENT_SRC_PATH) $(CLIENT_OBJ_PATH)

.PHONY: proto
proto:
	protoc -I=$(SRC_PATH) --cpp_out=$(SRC_PATH) $(SRC_PATH)/messages.proto

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