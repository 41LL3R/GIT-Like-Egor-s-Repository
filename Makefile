all: mygit

clean:
	rm mygit main.o repository.o tools.o

mygit: main.o repository.o tools.o
	gcc main.o repository.o tools.o -o mygit

main.o: main.c repository.h tools.h
	gcc -c main.c -o main.o

repository.o: repository.c repository.h tools.h
	gcc -c repository.c -o repository.o

tools.o: tools.c tools.h
	gcc -c tools.c -o tools.o