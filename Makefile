.PHONY: clean

TARGET := obfs0
CC := gcc
LIBS := 

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,obj/%.o,$(SRC))

$(TARGET): obj $(OBJ)
	$(CC) $(LIBS) -o $(TARGET) $(OBJ)

obj/%.o: src/%.c
	$(CC) -c $< -o $@

obj:
	mkdir -p obj

clean:
	rm -rf obj $(TARGET)