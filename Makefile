all: rle gen_img

gen_img: gen_img.c
	mkdir -p bin	
	clang -o bin/gen_img gen_img.c

rle: main.c rle_img.c
	mkdir -p bin	
	clang -o bin/rle rle_img.c main.c -lcurses
