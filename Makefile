all: guard_size-gcc guard_size-xlc

guard_size-gcc: guard_size.c proc_maps_parser/pmparser.c
	gcc -g -O0 $^ -o $@ -lpthread
guard_size-xlc: guard_size.c proc_maps_parser/pmparser.c
	xlc -g -O0 $^ -o $@ -lpthread

clean:
	rm guard_size-gcc guard_size-xlc
