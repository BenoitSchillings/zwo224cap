ver = debug
platform = mac64 

CC = g++
#INCLIB = /usr/local/include
#LDLIB = /usr/local/lib
OPENCV = $(shell pkg-config --cflags opencv) $(shell pkg-config --libs opencv)
USB =  -I../libusb/include  -L../libusb/$(platform) -lusb-1.0  



LIBSPATH = -L../lib/$(platform) -I../include



ifeq ($(ver), debug)
DEFS = -D_LIN -D_DEBUG 
CFLAGS = -g  -I $(INCLIB) -L $(LDLIB) $(DEFS) $(COMMON) $(LIBSPATH)  -lpthread  $(USB) -DGLIBC_20
else
DEFS = -D_LIN 
CFLAGS =  -O3 -I $(INCLIB) -L $(LDLIB) $(DEFS) $(COMMON) $(LIBSPATH)  -lpthread  $(USB) -DGLIBC_20
endif

ifeq ($(platform), mac32)
CC = g++
CFLAGS += -D_MAC -m32
OPENCV = -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_ts -lopencv_video -lopencv_videostab -I/usr/local/include/opencv
endif

ifeq ($(platform), mac64)
CC = g++
CFLAGS += -D_MAC -m64 -O4
OPENCV = -lopencv_calib3d -lopencv_contrib -lopencv_core -lopencv_features2d -lopencv_flann -lopencv_highgui -lopencv_imgproc -lopencv_legacy -lopencv_ml -lopencv_objdetect -lopencv_photo -lopencv_stitching -lopencv_ts -lopencv_video -lopencv_videostab -I/usr/local/include/opencv
endif

ifeq ($(platform), x86)
CFLAGS += -m32
CFLAGS += -lrt
endif


ifeq ($(platform), x64)
CFLAGS += -m64
CFLAGS += -lrt
endif

ifeq ($(platform), armv5)
CC = arm-none-linux-gnueabi-g++
AR= arm-nonelinux-gnueabi-ar
CFLAGS += -march=armv5
CFLAGS += -lrt
endif


ifeq ($(platform), armv6)
CC = arm-bcm2708hardfp-linux-gnueabi-g++
AR= arm-bcm2708hardfp-linux-gnueabi-ar
CFLAGS += -march=armv6
CFLAGS += -lrt
endif

#ifeq ($(platform), armhf)
#CC = arm-linux-gnueabihf-g++
#AR= arm-linux-gnueabihf-ar
#CFLAGS += -march=armv5
#LDLIB += -lrt
#endif


all:cvt moon
moon: moon.cpp
	$(CC) -march=native -O4 moon.cpp -o moon $(CFLAGS)  $(OPENCV) -lASICamera

cvt: cvt.cpp
	$(CC) -march=native -O4 cvt.cpp -o cvt $(CFLAGS)  $(OPENCV) -lASICamera

clean:
	rm -f moon cvt 
#pkg-config libusb-1.0 --cflags --libs
#pkg-config opencv --cflags --libs

