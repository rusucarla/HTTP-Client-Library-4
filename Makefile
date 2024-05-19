CXX=g++

CXXFLAGS=-Wall -std=c++11

TARGET=Client

all: $(TARGET)

$(TARGET): Client.o
	$(CXX) $(CXXFLAGS) -o $(TARGET) Client.o

Client.o: Client.cpp
	$(CXX) $(CXXFLAGS) -c Client.cpp

clean:
	rm -f $(TARGET) *.o

.PHONY: clean
