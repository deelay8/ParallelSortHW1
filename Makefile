all:
	g++ -Iinclude -I. -O3 -DNDEBUG mergesort.cpp -o mergesort

debug:
	g++ -Iinclude -I. -fomit-frame-pointer -g -Wall mergesort.cpp -o mergesort_debug