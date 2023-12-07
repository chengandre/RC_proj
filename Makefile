# Define the compiler
CXX = g++

# Flags for compilation
CXXFLAGS = -std=c++17 -Wall

# Source files
AS_SRC = AS.cpp common.cpp
USER_SRC = User.cpp common.cpp

# Header files
AS_HDR = AS.hpp common.hpp
USER_HDR = User.hpp common.hpp

# Targets
all: AS user

AS: $(AS_SRC)
	$(CXX) $(CXXFLAGS) -o AS $(AS_SRC) -pthread

user: $(USER_SRC)
	$(CXX) $(CXXFLAGS) -o user $(USER_SRC)

# Clean rule
clean:
	rm -f AS user