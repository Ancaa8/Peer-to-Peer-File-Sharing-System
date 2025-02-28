CXX = mpic++
CXXFLAGS = -pthread -Wall -std=c++11
TARGET = tema2
SRC = tema2.cpp peer.cpp tracker.cpp utils.cpp
OBJ = $(SRC:.cpp=.o)
ZIPNAME = tema2.zip

all: build

build: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)
	rm -f $(OBJ)

tema2.o: tema2.cpp utils.h
peer.o: peer.cpp utils.h
tracker.o: tracker.cpp utils.h
utils.o: utils.cpp utils.h

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJ)

zip:
	zip -r $(ZIPNAME) $(SRC) *.h Makefile README.md include

.PHONY: all build clean zip
