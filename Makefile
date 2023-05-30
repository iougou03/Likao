CC:= gcc
CFLAGS:= -Wall -I src/lib/ -Wno-uninitialized `pkg-config --cflags gtk+-3.0`
LDFLAGS = `pkg-config --libs gtk+-3.0`
RM:= rm -rf
OUT_DIR:= dist

LIB_DIR := src/lib
LIB_SRC:= $(wildcard $(LIB_DIR)/*.c)
LIB_OBJ:= $(patsubst $(LIB_DIR)/%.c,$(OUT_DIR)/%.o,$(LIB_SRC))

SERVER_LIBS:= -lpthread -ljson-c
SERVER_DIR:= src/server
SERVER_SRC:= $(wildcard $(SERVER_DIR)/*.c)
SERVER_OBJ:= $(patsubst $(SERVER_DIR)/%.c,$(OUT_DIR)/server/%.o,$(SERVER_SRC))

CLIENT_LIBS:= -lpthread -ljson-c
CLIENT_DIR:= src/client
CLIENT_SRC:= $(wildcard $(CLIENT_DIR)/*.c)
CLIENT_OBJ:= $(patsubst $(CLIENT_DIR)/%.c,$(OUT_DIR)/client/%.o,$(CLIENT_SRC))
CLIENT_PUBLIC:= src/client/public

.PHONY: all clean public

all: $(OUT_DIR)/server/server $(OUT_DIR)/client/client public

$(OUT_DIR)/%.o: $(LIB_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR)/server/%.o: $(SERVER_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR)/client/%.o: $(CLIENT_DIR)/%.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OUT_DIR)/server/server: $(SERVER_OBJ) $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(SERVER_LIBS)

$(OUT_DIR)/client/client: $(CLIENT_OBJ) $(LIB_OBJ)
	$(CC) $(CFLAGS) $^ -o $@ $(CLIENT_LIBS) $(LDFLAGS) 

public:
	cp -r $(CLIENT_PUBLIC) $(OUT_DIR)/client
clean:
	$(RM) $(OUT_DIR)