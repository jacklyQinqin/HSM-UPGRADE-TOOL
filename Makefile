TARGET = $(notdir $(CURDIR))
objs := $(patsubst %c, %o, $(shell ls *.c))
$(TARGET)_test:$(objs)
	aarch64-linux-gnu-gcc -o $@ $^ -lpthread
%.o:%.c
	aarch64-linux-gnu-gcc -c -o $@ $<
clean:
	rm -f  $(TARGET)_test *.all *.o
