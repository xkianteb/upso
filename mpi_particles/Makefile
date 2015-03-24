include Defs.$(shell uname)

CXXFLAGS += -Wall

# set to -O for optimized, -g for debug; also can use 'make OPT=-g'
OPT = -O

PROG = mpi

CC = CC
MPCC = mpicc
CFLAGS = -O3
LIBS =

CXX = c++

#GLOBJS = Cylinder Generic Geometry Image Input Shader Sphere Textures View Xform
GLOBJS = Cylinder.o Generic.o Geometry.o Image.o Input.o Shader.o Sphere.o Textures.o View.o Xform.o
GLOBJS_FULL = Cylinder.cpp Generic.cpp Geometry.cpp Image.cpp Input.cpp Shader.cpp Sphere.cpp Textures.cpp View.cpp Xform.cpp

TARGETS = mpi

all: $(TARGETS)


mpi: $(GLOBJS) mpi.o common.o gl.o
	$(MPCC) $(OPT) -o $@  $(LDFLAGS) $(LDLIBS) mpi.o common.o gl.o $(GLOBJS)

gl.o: $(GLOBJS)
	$(CXX) $(OPT) -c gl.cpp $(GLOBJS_FULL)

mpi.o: mpi.cpp common.h
	$(MPCC) -c $(CFLAGS) mpi.cpp

common.o: common.cpp common.h
	$(CC) -c $(CFLAGS) common.cpp

# .o from .c or .cxx, also generating dependency file
%.o: %.cpp
	$(CXX) $(OPT) -c -o $@ $< $(CXXFLAGS)
	$(CXX) -MM -o $*.d $<

clean:
	rm -f *.o $(TARGETS) *~ *.d