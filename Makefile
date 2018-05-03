CC=g++
LD = g++

SRCDIR = src
BINDIR = bin
OBJDIR = obj

OPENCV   = `pkg-config opencv --cflags --libs`
CFLAGS  += -Wall -Wextra -pedantic -Wno-unused-parameter -c $(OPENCV)
LDFLAGS += $(OPENCV)

ifeq ($(DEBUG),1)
CFLAGS += -O0 -g -DDEBUG
LDFLAGS += -O0 -g -DDEBUG
else
CFLAGS += -O3 -flto -DRELEASE
LDFLAGS += -O3 -flto -DRELEASE
endif

SOURCES = $(wildcard $(SRCDIR)/*.cpp)
HEADERS = $(wildcard $(SRCDIR)/*.h)
OBJECTS = $(patsubst $(SRCDIR)/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

EXECUTABLE = cvgame
TARGET = $(BINDIR)/$(EXECUTABLE)

all: $(SOURCES) $(TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BINDIR) $(OBJDIR):
	mkdir -p $@

module = uvcvideo
run: $(TARGET)
	lsmod | grep $(module) &>/dev/null || sudo modprobe $(module)
	$(TARGET) ${ARGS}

clean:
	rm -rf $(BINDIR) $(OBJDIR)

.PHONY:
	clean run all

