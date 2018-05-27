cdc : cdc.c
	gcc -g -O0 $^ -lm -o $@ -lgmp
