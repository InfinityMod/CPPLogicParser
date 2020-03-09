run:
	./sample

sample: sample.cpp
	g++ -std=c++17 -O2 -Wall -pedantic -pthread -g sample.cpp -o sample

build: sample