CXX ?= g++
CXXFLAGS = -O2 -std=c++17 -Wall

program: program.cpp
	$(CXX) $(CXXFLAGS) program.cpp -o program

run: program
	./program kanto.txt

clean:
	rm -f program resultado.js
