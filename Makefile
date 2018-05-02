TARGET = main.out

# XXX: Don't forget backslash at the end of any line except the last one

SRCS = \
       src/simpletun.cpp\
       src/rwr.cpp\
       src/nonblock.cpp

.PHONY: all clean

all: $(SRCS)
	$(CXX) -std=gnu++17 $(addprefix -I,$(HDRS)) -o $(TARGET) $(CFLAGS) $(SRCS)

clean:
	rm -rf $(TARGET)
