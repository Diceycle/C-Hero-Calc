CC = gcc
CXX = g++
RM = rm -f
CPPFLAGS = -Wall -O3 -std=c++11

SRCS = main.cpp cosmosClasses.cpp inputProcessing.cpp battleLogic.cpp cosmosDefines.cpp base64.cpp
OBJS = $(subst .cpp,.o,$(SRCS))

all: CosmosQuest

CosmosQuest: $(OBJS)
	$(CXX) $(LDFLAGS) -o CosmosQuest $(OBJS) $(LDLIBS)
	
$(OBJS) : cosmosClasses.h

main.o: main.cpp
cosmosClasses.o: cosmosClasses.cpp
inputProcessing.o: inputProcessing.cpp
battleLogic.o: battleLogic.cpp
cosmosDefines.o: cosmosDefines.cpp
base64.o : base64.cpp

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) CosmosQuest

rebuild: distclean all

run: all
	./CosmosQuest
