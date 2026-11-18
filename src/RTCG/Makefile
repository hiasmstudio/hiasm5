CC = g++

CPPFLAGS = -fPIC --std=c++17
LDLIBS = -shared

SOURCES = $(wildcard *.cpp)
APP = ../codegen

all: $(APP)

clean:
	rm ../libCodeGen.so *.o

$(APP): $(SOURCES:%.cpp=%.o)
	$(LINK.o) $^ $(LDLIBS) -o ../libCodeGen.so
