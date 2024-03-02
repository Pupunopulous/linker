# Compiler to use
CXX = g++

# Compiler flags
CXXFLAGS = -w -std=c++2a

# Linker flags
LDFLAGS =

# Source files
SOURCES = linker.cpp Parser.cpp

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Executable name
EXECUTABLE = linker

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS) 
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJECTS) $(EXECUTABLE)