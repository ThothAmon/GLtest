PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))
ECORE_CFLAGS = -I/usr/include/ecore-x-1 -I/usr/include/eina-1 -I/usr/include/eina-1/eina -I/usr/include/efl-1
ECORE_X_LIBS = -lecore_x
EGL_LIBS     = -lEGL -lGLESv2
#libEGL_nvidia.so.0 libGLESv2_nvidia libnvidia-eglcore
OBJS = GLtest.o
CFLAGS += -fpermissive
#-Xlinker -v 
#-print-search-dirs

ifeq ($(BUILD_MODE),debug)
	CFLAGS += -g
else ifeq ($(BUILD_MODE),run)
	CFLAGS += -O2
else
	$(error Build mode $(BUILD_MODE) not supported by this Makefile)
endif

all:	GLtest

GLtest:	$(OBJS) $(ECORE_X_LIBS) $(EGL_LIBS)
	$(CXX) -o $@ $^

%.o:	$(PROJECT_ROOT)%.cpp
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(ECORE_CFLAGS) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $(ECORE_CFLAGS) -o $@ $<

clean:
	rm -fr GLtest $(OBJS)
