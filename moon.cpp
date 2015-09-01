#include "stdio.h"
#include "highgui.h"
#include "ASICamera.h"
#include <sys/time.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <time.h>


//--------------------------------------------------------------------------------------

using namespace cv;
using namespace std;

#define ushort unsigned short
#define uchar unsigned char
#define PTYPE unsigned short
//--------------------------------------------------------------------------------------
void convert(ushort *src, ushort *dst, int xsize, int ysize);
//--------------------------------------------------------------------------------------

void cvText(Mat img, const char* text, int x, int y)
{
    putText(img, text, Point2f(x, y), FONT_HERSHEY_PLAIN, 3, CV_RGB(65000, 15000, 15000), 5, 8);
}

//--------------------------------------------------------------------------------------

double GetTickCount()
{
    return 3.0*clock()/1000000.0;
}

//--------------------------------------------------------------------------------------

void move(float x, float y)
{
    int time = 100;
   
  
    if (fabs(x) < 2) x = 0;
    if (fabs(y) < 2) y = 0;
    
    if (fabs(x) > 30) return;
    if (fabs(y) > 30) return;
    
    if (x > 0) {
        pulseGuide(guideEast, time);
    }
    else {
        pulseGuide(guideWest, time);
    }
    
    if (y > 0) {
        pulseGuide(guideNorth, time);
    }
    else {
        pulseGuide(guideSouth, time);
    }
}


//--------------------------------------------------------------------------------------

void init_cam(int width, int height)
{
    int CamNum=0;
    bool bresult;
    
    int numDevices = getNumberOfConnectedCameras();
    if(numDevices <= 0) {
        exit(-1);
    }
    
    CamNum = 0;
    
    bresult = openCamera(CamNum);
    if(!bresult) {
        printf("OpenCamera error,are you root?,press any key to exit\n");
        exit(-1);
    }
    
    
    initCamera(); //this must be called before camera operation. and it only need init once
    printf("sensor temperature:%02f\n", getSensorTemp());
    setImageFormat(width, height, 1, IMG_RAW16);
    setValue(CONTROL_BRIGHTNESS, 100, false);
    setValue(CONTROL_GAIN, 0, false);
    setValue(CONTROL_BANDWIDTHOVERLOAD, getMin(CONTROL_BANDWIDTHOVERLOAD), false); //lowest transfer speed
    setValue(CONTROL_EXPOSURE, 50, false);
    setValue(CONTROL_HIGHSPEED, 0, false);
}

//--------------------------------------------------------------------------------------

int  main()
{
    int width;
    int height;
    int i;
    char c;
    
    int time1,time2;
    int count=0;
    int exp0, gain0;
    int recording = 0;
    int cap = 0;
    float q = 0;
    
    char buf[128] = {0};
    
    width = 1280;
    height = 960;
    
    //width = 1936;
    //height = 1216;
    
    
    Mat    image(Size(width, height), CV_16UC1);
    Mat    image_color(Size(width, height), CV_16UC3);
    Mat	   image_add(Size(width, height), CV_16UC1); 
    Mat	   tmp;
 
    printf("%d\n", CLOCKS_PER_SEC);
    
    cvNamedWindow("video", 1);
    createTrackbar("gain", "video", 0, 600, 0);
    createTrackbar("exp", "video", 0, 256, 0);
    createTrackbar("mult", "video", 0, 100, 0);
    createTrackbar("Sub", "video", 0, 500, 0);
    
    setTrackbarPos("gain", "video", 374);
    setTrackbarPos("exp", "video", 50);
    setTrackbarPos("mult", "video", 10.0);
    
    init_cam(width, height);
    startCapture();
    
    time1 = GetTickCount();
    
    FILE *output;
    
    output = fopen("./out.raw", "wb");
    
    printf("t0\n");
    int max_x = -1;
    int max_y = -1;
    int guide_count = 0;
 
    bool guiding = false;
    char dark_mode = 0;
 
    while(1) {
       	bool got_it;
        char c;
        c = 0;
        do {
            got_it = getImageData(image.ptr<uchar>(0), width * height * sizeof(PTYPE), 30);
            if (c == 0) c = cvWaitKey(1);
        } while(!got_it);
        
        if (recording) {
            fwrite(image.ptr<uchar>(0), 1, width * height * sizeof(PTYPE), output);
        }
        
        cap = cap + 1;
        
        double t1 = GetTickCount();
        
        
        count++;
        time2 = GetTickCount();
        int gain = getTrackbarPos("gain", "video");
        float exp = getTrackbarPos("exp", "video");
        exp = exp / 256.0;
        exp = exp * exp;
        float mult = 0.1 + getTrackbarPos("mult", "video");
        
        if (exp0 != exp || gain0 != gain) {
            setValue(CONTROL_GAIN, gain, false);
            setValue(CONTROL_EXPOSURE, exp*1000000, false);
            setValue(CONTROL_BRIGHTNESS, 100, false);
            gain0 = gain;
            exp0 = exp;
       	    printf("%f %ld\n", exp, gain); 
	}
        
        switch(c) {
            case 27:
                goto END;
            case ' ':
                recording^=1;
                break;
            case 'r':
            case 'R':
                break;
	    case 'g':
	    case 'G':
		guiding^=1;
		if (guiding == 0) {
				guide_count = 0;
				max_x = 0;
				max_y = 0;	
		}	
		break;
	   case 'D':
	   case 'd':
		fclose(output);
		output  = fopen("./dark.raw", "wb"); 
       		dark_mode = 1;
		break; 
	  }
        
        double minVal;
        double maxVal;
        Point  minLoc;
        Point  maxLoc;
        
        minMaxLoc(image(Rect(50, 50, width - 100, height - 100)), &minVal, &maxVal, &minLoc, &maxLoc);
        
        Mat tmp;
        

	if (count % 3 == 0) {
            image = image - Scalar(getTrackbarPos("Sub", "video") * 256);
            image = image.mul(mult/10.0);
            convert(image.ptr<ushort>(0), image_color.ptr<ushort>(0), width, height);
            char buf[512];
 
	    sprintf(buf, "rec=%d, dk=%d, cap = %d", recording, dark_mode, cap);
            cvText(image_color, buf, 40,40);
	    sprintf(buf, "exp =%f, gain=%d", exp, gain);
            cvText(image_color, buf, 40,65);
            sprintf(buf, "%d %d, %d %d = %f", max_x, max_y, maxLoc.x, maxLoc.y, maxVal);
            cvText(image_color, buf, 40,80);
 
	    imshow("video", image_color);
        
	}
        
        
        if (guiding) {

#define	RATIO	15	
 
	    if ((guide_count % RATIO) == 0) {
	   	image_add = Scalar(0); 
	   	guide_count = 0; 
	    }
	    
	    image_add = image_add + image * 0.1;
           	
            guide_count++;

	    if (guide_count == (RATIO-1)) {
        	
		resize(image_add, tmp, image_add.size(), 0.5, 0.5, INTER_AREA);	
		minMaxLoc(tmp(Rect(75, 75, width/2 - 100, height/2 - 100)), &minVal, &maxVal, &minLoc, &maxLoc);
		imshow("guide", tmp);
	    	if (max_x < 0) {
                	max_x = maxLoc.x;
                	max_y = maxLoc.y;
            	}
            	else {
                	int dx, dy;
               
                	dx = maxLoc.x - max_x;
                	dy = maxLoc.y - max_y;
                
                    	move(dx, dy);
            	}
	    }
        }
        
        
        if (recording) {
            char buf[512];
            //convert(image.ptr<ushort>(0), image_color.ptr<ushort>(0), width, height);
        }
    }
    
END:
    stopCapture();
    fclose(output);
    return 1;
}



static void
_bilinearRGGB( void* source, void* target, int xSize, int ySize )
{
    int row, col, lastX, lastY;
    unsigned short* s;
    unsigned short* t;
    
    // FIX ME -- handle first row/column
    // FIX ME -- handle last row/column
    
    // odd rows
    // odd pixels, R avg(diagonals), G avg(vert/horiz), B direct copy
    // even pixels, R avg(verticals), G direct copy, B avg(horizontals)
    
    lastX = xSize - 2;
    lastY = ySize - 1;
    for ( row = 1; row < lastY; row += 2 ) {
        s = (( unsigned short* ) source) + row * xSize + 1;
        t = (( unsigned short* ) target) + ( row * xSize + 1 ) * 3;
        for ( col = 1; col < lastX; col += 2 ) {
            *t++ = ( *( s - xSize - 1 ) + *( s - xSize + 1 ) + *( s + xSize - 1 ) +
                    *( s + xSize + 1 )) / 4;  // R
            *t++ = ( *( s - xSize ) + *( s - 1 ) + *( s + 1 ) +
                    *( s + xSize )) / 4;  // G
            *t++ = *s;  // B
            s++;
            *t++ = ( *( s - xSize ) + *( s + xSize )) / 2;  // R
            *t++ = *s;  // G
            *t++ = ( *( s - 1 ) + *( s + 1 )) / 2;  // B
            s++;
        }
    }
    
    // even rows
    // odd pixels, R avg(horizontals), G direct copy, B avg(verticals)
    // even pixels, R direct copy, G avg(vert/horiz), B avg(diagonals)
    
    for ( row = 2; row < lastY; row += 2 ) {
        s = (( unsigned short* ) source) + row * xSize + 1;
        t = (( unsigned short* ) target) + ( row * xSize + 1 ) * 3;
        for ( col = 1; col < lastX; col += 2 ) {
            *t++ = ( *( s - 1 ) + *( s + 1 )) / 2;  // R
            *t++ = *s;  // G
            *t++ = ( *( s - xSize ) + *( s + xSize )) / 2;  // B
            s++;
            *t++ = *s;  // R
            *t++ = ( *( s - xSize ) + *( s - 1 ) + *( s + 1 ) +
                    *( s + xSize )) / 4;  // G
            *t++ = ( *( s - xSize - 1 ) + *( s - xSize + 1 ) + *( s + xSize - 1 ) +
                    *( s + xSize + 1 )) / 4;  // B
            s++;
        }
    }
}



static void
_bilinearBGGR8 ( void* source, void* target, int xSize, int ySize )
{
    int row, col, lastX, lastY;
    unsigned short* s;
    unsigned short* t;
    
    // FIX ME -- handle first row/column
    // FIX ME -- handle last row/column
    
    // odd rows
    // odd pixels, R direct copy, G avg(vert/horiz), B avg(diagonals)
    // even pixels, R avg(horizontals), G direct copy, B avg(verticals)
    
    lastX = xSize - 2;
    lastY = ySize - 1;
    for ( row = 1; row < lastY; row += 2 ) {
        s = ( unsigned short* ) source + row * xSize + 1;
        t = ( unsigned short* ) target + ( row * xSize + 1 ) * 3;
        for ( col = 1; col < lastX; col += 2 ) {
            *t++ = *s;  // R
            *t++ = ( *( s - xSize ) + *( s - 1 ) + *( s + 1 ) +
                    *( s + xSize )) / 4;  // G
            *t++ = ( *( s - xSize - 1 ) + *( s - xSize + 1 ) + *( s + xSize - 1 ) +
                    *( s + xSize + 1 )) / 4;  // B
            s++;
            *t++ = ( *( s - 1 ) + *( s + 1 )) / 2;  // R
            *t++ = *s;  // G
            *t++ = ( *( s - xSize ) + *( s + xSize )) / 2;  // B
            s++;
        }
    }
    
    // even rows
    // odd pixels, R avg(verticals), G direct copy, B avg(horizontals)
    // even pixels, R avg(diagonals), G avg(vert/horiz), B direct copy
    
    for ( row = 2; row < lastY; row += 2 ) {
        s = ( unsigned short* ) source + row * xSize + 1;
        t = ( unsigned short* ) target + ( row * xSize + 1 ) * 3;
        for ( col = 1; col < lastX; col += 2 ) {
            *t++ = ( *( s - xSize ) + *( s + xSize )) / 2;  // R
            *t++ = *s;  // G
            *t++ = ( *( s - 1 ) + *( s + 1 )) / 2;  // B
            s++;
            *t++ = ( *( s - xSize - 1 ) + *( s - xSize + 1 ) + *( s + xSize - 1 ) +
                    *( s + xSize + 1 )) / 4;  // R
            *t++ = ( *( s - xSize ) + *( s - 1 ) + *( s + 1 ) +
                    *( s + xSize )) / 4;  // G
            *t++ = *s;  // B
            s++;
        }
    }
}

void convert(ushort *src, ushort *dst, int xsize, int ysize)
{
    _bilinearBGGR8(src, dst, xsize, ysize);
}
