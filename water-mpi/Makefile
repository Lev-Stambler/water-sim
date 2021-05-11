OBJDIR=objs
CXX=mpic++ -m64
CXXFLAGS=-O3 -Wall 
LDFLAGS=-L/usr/local/depot/cuda-10.2/lib64/ -lcudart 

OBJS=$(OBJDIR)/main.o  $(OBJDIR)/water-gpu.o

.PHONY: dirs clean

all: $(EXECUTABLE)

default: $(EXECUTABLE)

dirs:
		mkdir -p $(OBJDIR)/

clean:
		rm -rf $(OBJDIR) *.ppm *~ $(EXECUTABLE)

check_scan: default
				./checker.pl scan

check_find_repeats: default
				./checker.pl find_repeats

$(EXECUTABLE): dirs $(OBJS)
		$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)



$(OBJDIR)/%.o: %.cpp
		$(CXX) $< $(CXXFLAGS) -c -o $@

$(OBJDIR)/%.o: %.cu
		$(NVCC) $< $(NVCCFLAGS) -c -o $@
