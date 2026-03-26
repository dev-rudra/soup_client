CXX      = g++
CXXFLAGS = -std=c++11 -Wall -Wextra
INCLUDES = -I include

SOURCES  = src/socket.cpp \
		   src/config_parser.cpp \
		   src/frame_encoder.cpp \
           src/main.cpp

TARGET   = ouch_client

$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $(SOURCES) -o $(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: clean
