CC=gcc
CXX=g++
RM=rm -f
CPPFLAGS=-Wall -O2 -std=c++11

SRCS=main.cpp cosmosClasses.cpp inputProcessing.cpp battleLogic.cpp cosmosDefines.cpp
OBJS=$(subst .cpp,.o,$(SRCS))

all: CosmosQuest

CosmosQuest: $(OBJS)
	$(CXX) $(LDFLAGS) -o CosmosQuest $(OBJS) $(LDLIBS)

CosmosQuest.o: main.cpp

cosmosClasses.o: cosmosClasses.cpp

inputProcessing.o: inputProcessing.cpp

battleLogic.o: battleLogic.cpp

cosmosDefines.o: cosmosDefines.cpp



clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) CosmosQuest

rebuild: distclean all

run: all
	./CosmosQuest
