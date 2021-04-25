# EXECUTABLE := xx
# CU_FILES   := water-gpu.cu
# CU_DEPS    :=
# CC_FILES   := water-mpi.cpp
# TARGET = water-mpi
# OBJS += water-mpi.o

# CC = mpicc
# #CFLAGS = -Wall -Werror -DDEBUG -g # debug flags
# CFLAGS = -std=gnu99 -Wall -Werror -g -O3 # release flags
# CFLAGS += -MMD -MP
# LDFLAGS += $(LIBS)
# LDFLAGS+=-L/usr/local/depot/cuda-10.2/lib64/ -lcudart

# default:	$(TARGET)
# all:		$(TARGET)

# $(TARGET):	$(OBJS)
# 	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

# %.o: %.c
# 	$(CC) $(CFLAGS) -c -o $@ $<

# DEPS = $(OBJS:%.o=%.d)
# -include $(DEPS)

# clean:
# 	rm $(TARGET) $(OBJS) $(DEPS) || true
EXECUTABLE := watermpi
CU_FILES   := water-gpu.cu
CU_DEPS    :=
CC_FILES   := main.cpp


all: $(EXECUTABLE)

###########################################################

OBJDIR=objs
CXX=mpic++ -m64
CXXFLAGS=-O3 -Wall
# LDFLAGS=-L/usr/local/depot/cuda-10.2/lib64/ -lcudart
LDFLAGS=-L/usr/local/cuda/lib64/ -lcudart
NVCC=nvcc
NVCCFLAGS=-O3 -m64 --gpu-architecture compute_61 -ccbin /usr/bin/gcc


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