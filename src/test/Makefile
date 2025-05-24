CC = g++
CFLAGS = -std=c++17 -O3 -Wall -fopenmp
INCLUDES = -I../ -I../../include

# Get the source directory
SRCDIR = ..

# Object files
OBJS = test_kl_refiner.o \
       $(SRCDIR)/KLRefiner.o \
       $(SRCDIR)/Hypergraph.o \
       $(SRCDIR)/Utilities.o \
       $(SRCDIR)/PriorityQueue.o \
       $(SRCDIR)/floorplan.o

# Targets
all: test_kl_refiner

test_kl_refiner: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ -fopenmp

test_kl_refiner.o: test_kl_refiner.cpp
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SRCDIR)/KLRefiner.o: $(SRCDIR)/KLRefiner.cpp $(SRCDIR)/KLRefiner.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SRCDIR)/Hypergraph.o: $(SRCDIR)/Hypergraph.cpp $(SRCDIR)/Hypergraph.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SRCDIR)/Utilities.o: $(SRCDIR)/Utilities.cpp $(SRCDIR)/Utilities.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SRCDIR)/PriorityQueue.o: $(SRCDIR)/PriorityQueue.cpp $(SRCDIR)/PriorityQueue.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

$(SRCDIR)/floorplan.o: $(SRCDIR)/floorplan.cpp $(SRCDIR)/floorplan.h
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	rm -f *.o $(SRCDIR)/*.o test_kl_refiner

.PHONY: all clean
