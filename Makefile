all:
	clang-format -i *.c
	gcc parser.c -o parser.bin
	./parser.bin /bin/cat
clean:
	rm -vf *.bin
