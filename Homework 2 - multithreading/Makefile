CXX = g++
CXXFLAGS = -std=c++11 -g -Wall -Werror -pedantic-errors -DNDEBUG -pthread
CXXLINK = $(CXX)
OBJS = Bank.o
RM = rm -f
# Creating the  executable
Bank: $(OBJS)
	$(CXXLINK) $(CXXFLAGS) -o Bank $(OBJS)
# Creating the object files
Bank.o: Bank.cpp Bank.h
# Cleaning old files before new make
clean:
	$(RM) $(TARGET) *.o *~ "#"* core.* Bank