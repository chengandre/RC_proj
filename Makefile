# Define the compiler
CXX = g++

# Flags for compilation
CXXFLAGS = -std=c++11 -Wall

# Source files
AS_SRC = AS.cpp
USER_SRC = User.cpp

# Header files
AS_HDR = AS.hpp
USER_HDR = User.hpp

# Object files
AS_OBJ = $(AS_SRC:.cpp=.o)
USER_OBJ = $(USER_SRC:.cpp=.o)

# Targets
all: AS user

AS: $(AS_OBJ)
	$(CXX) $(CXXFLAGS) -o AS $(AS_OBJ)

user: $(USER_OBJ)
	$(CXX) $(CXXFLAGS) -o user $(USER_OBJ)

# Object file rules
%.o: %.cpp %.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

# Clean rule
clean:
	rm -f AS user $(AS_OBJ) $(USER_OBJ)

