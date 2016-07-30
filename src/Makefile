CC = g++

CPPFLAGS = -Wall -c -O2 $(shell pkg-config gtksourceviewmm-3.0 --cflags)
LDLIBS = $(shell pkg-config gtksourceviewmm-3.0 gthread-2.0 gmodule-2.0 --libs) -lsqlite3

SOURCES = $(wildcard *.cpp) 
APP = hiasm5

all: $(APP)

clean:
	rm $(APP) *.o

$(APP): $(SOURCES:%.cpp=%.o)
	$(LINK.o) $^ $(LDLIBS) -o $(APP)
	strip $@

