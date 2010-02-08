TARGET=aout-tool

$(TARGET): main.c a.out.h
	gcc -o $@ -s main.c

test: $(TARGET) test.o
	./$(TARGET) test.o

test.o: test.c
	i386-pc-minix-gcc -c test.c

install: $(TARGET)
	cp $(TARGET) /usr/local/bin

clean:
	rm -f $(TARGET) *.o *core
