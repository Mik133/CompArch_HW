CXX=g++
CXXLINK=$(CXX)
CXXFLAGS=-std=c++11
OBJS=cacheSim.o
EXEC=cacheSim

cacheSim:cacheSim.cpp cacheData.h
	$(CXX) $(CXXFLAGS) cacheSim.cpp -o cacheSim


.PHONY: clean
clean:
	rm -f *.o
	rm -f cacheSim
