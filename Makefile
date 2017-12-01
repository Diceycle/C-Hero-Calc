CC = gcc
CXX = g++
RM = rm -f
CPPFLAGS = -Wall -Ofast -std=c++11

SRCS = main.cpp cosmosData.cpp inputProcessing.cpp battleLogic.cpp base64.cpp
OBJS = $(subst .cpp,.o,$(SRCS))

all: CosmosQuest

CosmosQuest: $(OBJS)
	$(CXX) $(LDFLAGS) -o CosmosQuest $(OBJS) $(LDLIBS)

main.o: main.cpp
cosmosData.o: cosmosData.cpp
inputProcessing.o: inputProcessing.cpp
battleLogic.o: battleLogic.cpp
base64.o : base64.cpp

clean:
	$(RM) $(OBJS)

distclean: clean
	$(RM) CosmosQuest

rebuild: distclean all

run: all
	./CosmosQuest
