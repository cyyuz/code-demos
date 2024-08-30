INCLUDE = -Isrc/public/

all:test test1

test:test.cpp
	g++ -o test -g test.cpp

test1:test1.cpp test1.cpp
	g++ -o test1 -g test1.cpp

clean:
	rm -f test test1  