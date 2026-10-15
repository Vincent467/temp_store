SRC_CONFUSION_DIR := src/confusion_algo
SRC_NORMAL_DIR := src/algo
BUILD_DIR := lib
LIB_DPU_ALGORITHM := libdpualgorithm.so
INSTALL_LIB_PATH := $(BUILD_DIR)/$(LIB_DPU_ALGORITHM)
SRC_CONFUSION := $(shell find $(SRC_CONFUSION_DIR) -maxdepth 5 -name '*.c')
SRC_NORMAL := $(shell find $(SRC_NORMAL_DIR) -maxdepth 5 -name '*.c')
OBJ_CONFUSION := $(patsubst $(SRC_CONFUSION_DIR)/%.c,$(BUILD_DIR)/$(SRC_CONFUSION_DIR)/%.o,$(SRC_CONFUSION))
OBJ_NORMAL := $(patsubst $(SRC_NORMAL_DIR)/%.c,$(BUILD_DIR)/$(SRC_NORMAL_DIR)/%.o,$(SRC_NORMAL))

CONFUSION_INC_DIR := $(shell find $(shell pwd)/$(SRC_CONFUSION_DIR)/ -maxdepth 3 -type d)
NORMAL_INC_DIR := $(shell find $(shell pwd)/$(SRC_NORMAL_DIR)/ -maxdepth 3 -type d)

INCLUDES = -I ./../libdrm/include -I ./../driver/include/uapi/drm/
CONFUSION_INCLUDES = -I $(foreach dir, $(CONFUSION_INC_DIR), -I$(dir))
NORMAL_INCLUDES = -I $(foreach dir, $(NORMAL_INC_DIR), -I$(dir))

CC=$(CROSS_COMPILE)gcc
CFLAGS:= -fPIC

#Confusion algo and normal algo use same common function, will multiple-definition,
#Here we make sure that functions with the same name are same implement.
#After confused the SRC_CONFUSION_DIR, the common function will confused with
#special symbols, will not has multiple-definition.
LDFLAGS := -shared -Wl,--allow-multiple-definition

.PHONY: all clean

all: $(LIB_DPU_ALGORITHM)

$(LIB_DPU_ALGORITHM):$(INSTALL_LIB_PATH)

$(INSTALL_LIB_PATH): $(OBJ_CONFUSION) $(OBJ_NORMAL)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) -o $@ $^

$(BUILD_DIR)/$(SRC_CONFUSION_DIR)/%.o:$(SRC_CONFUSION_DIR)/%.c | $(BUILD_DIR)/$(SRC_CONFUSION_DIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) $(CONFUSION_INCLUDES) -c $< -o $@

$(BUILD_DIR)/$(SRC_NORMAL_DIR)/%.o:$(SRC_NORMAL_DIR)/%.c | $(BUILD_DIR)/$(SRC_NORMAL_DIR)
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) $(INCLUDES) $(NORMAL_INCLUDES) -c $< -o $@

$(BUILD_DIR)/$(SRC_CONFUSION_DIR):
	mkdir -p $@

$(BUILD_DIR)/$(SRC_NORMAL_DIR):
	mkdir -p $@

clean:
	@rm -rf $(BUILD_DIR)

.PHONY:
	clean
