CC = g++
CMAKE ?= cmake
SQLITE3 ?= sqlite3

# Set GTK4=1 for GTK4/gtkmm4. On Linux, src/GsvCompat.h wraps the
# GtkSourceView 5 C API because a gtkmm4 wrapper is unavailable; Windows
# keeps the plain Gtk::TextView path.
GTK4 ?= 0

ifeq ($(GTK4),1)
GTKMM_MODULE ?= gtkmm-4.0
GTKSOURCEMM_MODULE ?= gtksourceview-5
CXXFLAGS_STD ?= -std=c++17
DEFINES = -DHIASM_GTK4=1
GTK4_LIBS =
else
GTKMM_MODULE ?= gtkmm-3.0
GTKSOURCEMM_MODULE ?= gtksourceviewmm-3.0
CXXFLAGS_STD ?= -std=c++11
DEFINES = -DHIASM_GTK3=1
GTK4_LIBS =
endif

ROOT_DIR := .
SRC_DIR := src
BUILD_DIR ?= build/Windows_MinGW-x64-Debug
RESOURCES_DIR ?= resources
PACKS_DIR ?= packs
OBJ_DIR ?= $(BUILD_DIR)/obj
ELEMENTS_DIR = $(BUILD_DIR)/elements
APP = $(BUILD_DIR)/hiasm

PKG_MODULES = $(GTKMM_MODULE) $(GTKSOURCEMM_MODULE) gthread-2.0 gmodule-2.0
CPPFLAGS = -Wall -O2 $(CXXFLAGS_STD) $(DEFINES) -I$(SRC_DIR) $(shell pkg-config $(PKG_MODULES) --cflags)
LDLIBS = $(shell pkg-config $(PKG_MODULES) --libs) $(GTK4_LIBS) -lsqlite3

SOURCES = $(wildcard $(SRC_DIR)/*.cpp)
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

ifeq ($(GTK4),1)
THEME_FILE = hiasm.css
else
THEME_FILE = gtkrc
endif

all: stage

$(APP): $(OBJECTS)
	$(CMAKE) -E make_directory $(BUILD_DIR)
	$(CC) -o $@ $(OBJECTS) $(LDLIBS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CMAKE) -E make_directory $(OBJ_DIR)
	$(CC) $(CPPFLAGS) -MMD -MP -c -o $@ $<

stage: $(APP)
	$(CMAKE) -E copy_directory $(RESOURCES_DIR)/int $(BUILD_DIR)/int
	$(CMAKE) -E copy_if_different $(RESOURCES_DIR)/$(THEME_FILE) $(BUILD_DIR)/$(THEME_FILE)
	$(CMAKE) -E copy_if_different $(RESOURCES_DIR)/splash.png $(BUILD_DIR)/splash.png
	$(CMAKE) -E copy_directory $(PACKS_DIR) $(ELEMENTS_DIR)
	$(CMAKE) -E rm -f $(ELEMENTS_DIR)/_base/elements.db
	$(SQLITE3) $(ELEMENTS_DIR)/_base/elements.db ".read $(ELEMENTS_DIR)/_base/elements.sql"

clean:
	-$(CMAKE) -E rm -rf $(OBJ_DIR) $(APP)

-include $(OBJECTS:.o=.d)

.PHONY: all stage clean
