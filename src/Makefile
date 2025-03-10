CC=clang
CFLAGS=-g -Wall -Wextra -pedantic -I../include -fPIC -fprofile-arcs -ftest-coverage
LDFLAGS=-g -L../build/src -L../build/test --coverage -fPIC
LDLIBS=
RM=rm
BUILD_DIR=../build
DIST_DIR=../dist

ROOT=/usr/local

PROJECT_NAME=vgc

LIB_NAME=lib$(PROJECT_NAME)
STATIC_LIBRARY=$(LIB_NAME).a
DYNAMIC_LIBRARY=$(LIB_NAME).so

STATIC_LIBRARY_PATH=$(DIST_DIR)/lib/$(STATIC_LIBRARY)
DYNAMIC_LIBRARY_PATH=$(DIST_DIR)/lib/$(DYNAMIC_LIBRARY)

INSTALL_STATIC_LIBRARY_PATH=$(ROOT)/lib/$(STATIC_LIBRARY)
INSTALL_DYNAMIC_LIBRARY_PATH=$(ROOT)/lib/$(DYNAMIC_LIBRARY)

INSTALL_INCLUDE_DIR=$(ROOT)/include/vgc

.PHONY: all
all: clean $(STATIC_LIBRARY_PATH) $(DYNAMIC_LIBRARY_PATH)

$(BUILD_DIR)/obj/%.o: %.c
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -MMD -c $< -o $@

SRCS=vgc.c
OBJS=$(SRCS:%.c=$(BUILD_DIR)/obj/%.o)
DEPS=$(OBJS:%.o=%.d)

$(STATIC_LIBRARY_PATH): $(OBJS)
	mkdir -p $(@D)
	ar rcs $@ $^

$(DYNAMIC_LIBRARY_PATH): $(OBJS)
	mkdir -p $(@D)
	$(CC) $(LDFLAGS) $(LDLIBS) -shared -fPIC $^ -o $@

clean:
	$(RM) -f $(OBJS) $(DEPS)

distclean: clean
	$(RM) -f $(DIST_DIR)/lib/$(STATIC_LIBRARY)
	$(RM) -f $(DIST_DIR)/lib/$(DYNAMIC_LIBRARY)
	$(RM) -f $(DIST_DIR)/lib/*gcda
	$(RM) -f $(DIST_DIR)/lib/*gcno

install:
	sudo cp -f $(STATIC_LIBRARY_PATH) $(INSTALL_STATIC_LIBRARY_PATH)
	sudo cp -f $(DYNAMIC_LIBRARY_PATH) $(INSTALL_DYNAMIC_LIBRARY_PATH)
	rm -rf $(INSTALL_INCLUDE_DIR)
	mkdir -p $(INSTALL_INCLUDE_DIR)
	sudo cp -f *.h $(INSTALL_INCLUDE_DIR)

uninstall:
	sudo rm -f $(INSTALL_STATIC_LIBRARY_PATH)
	sudo rm -f $(INSTALL_DYNAMIC_LIBRARY_PATH)
	sudo rm -rf $(INSTALL_INCLUDE_DIR)
