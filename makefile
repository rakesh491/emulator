cc:
	rm -f log.txt
	gcc -c main.c -o ./output/main.o -I ./include -Wall
	gcc -c cpu.c -o ./output/cpu.o -I ./include -Wall
	gcc -c files.c -o ./output/files.o -I ./include -Wall
	gcc -c mapper.c -o ./output/mapper.o -I ./include -Wall
	gcc -c ppu.c -o ./output/ppu.o -I ./include -Wall
	gcc ./output/main.o ./output/cpu.o ./output/files.o ./output/mapper.o ./output/ppu.o -o ./output/vivek.o -lSDL2

rr:
	./output/vivek.o
