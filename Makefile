CC=g++
LD = g++

SRCDIR = src
BINDIR = bin
OBJDIR = obj

LIBS   = `pkg-config opencv4 --cflags --libs` -lpthread
CFLAGS  += -Wall -Wextra -pedantic -Wno-unused-parameter -c $(LIBS)
LDFLAGS += $(LIBS)

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

EXECUTABLE2 = estimate_camera_fps
TARGET2 = $(BINDIR)/$(EXECUTABLE2)

all: $(SOURCES) $(TARGET) $(TARGET2)

$(TARGET): $(filter-out obj/$(EXECUTABLE2).o,$(OBJECTS)) | $(BINDIR)
	$(CC) $(LDFLAGS) $^ -o $@

$(TARGET2): obj/$(EXECUTABLE2).o | $(BINDIR)
	$(CC) $(LDFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp | $(OBJDIR)
	$(CC) $(CFLAGS) -o $@ $<

$(BINDIR) $(OBJDIR):
	mkdir -p $@

module = uvcvideo
module:
	lsmod | grep $(module) &>/dev/null || sudo modprobe $(module)

run: module $(TARGET)
	$(TARGET) ${ARGS}

clean:
	rm -rf $(BINDIR) $(OBJDIR)

.PHONY:
	clean run all

