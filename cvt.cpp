#include "stdio.h"
#include "highgui.h"
#include "ASICamera.h"
#include <sys/time.h>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <time.h>



#define	SX	1280
#define	SY	960

#define	ushort	unsigned short

//--------------------------------------------------------------------------

double	sum_r[SX*SY];
double	sum_g[SX*SY];
double	sum_b[SX*SY];

double	count_r[SX*SY];
double	count_g[SX*SY];
double	count_b[SX*SY];

double   bias[SX*SY];
double	dev[SX*SY];
double  flat[SX*SY];
//--------------------------------------------------------------------------

void zero_all()
{
    for (int i = 0; i < SX*SY; i++) {
        sum_r[i] = 0.0;
        sum_g[i] = 0.0;
        sum_b[i] = 0.0;
        count_r[i] = 0.0;
        count_g[i] = 0.0;
        count_b[i] = 0.0;
        bias[i] = 0;
        dev[i] = 0;
        flat[i] = 0;
    }
}

//--------------------------------------------------------------------------


char	in_range(int x, int y)
{
    if (x < 0 || y < 0) return 0;
    if (x > (SX-1) || (y > (SY-1))) return 0;
    
    return 1;
}

//--------------------------------------------------------------------------


void add_channel(ushort *src, int dx, int dy)
{
    int	x, y;
    float	red_bias;
    float	green_bias;
    float	blue_bias;
    float	MSTD = 830;
    red_bias = 0;
    
    for (int i = 0; i < SX; i+=2) {
        red_bias += src[i];
    }
    red_bias /= (SX/2.0);
    
    green_bias = 0;
    
    for (int i = 0; i < SX; i+=2) {
        green_bias += src[i + 1];
    }
    green_bias /= (SX/2.0);
    
    blue_bias = 0;
    
    for (int i = 0; i < SX; i+=2) {
        blue_bias += src[i + SX];
    }
    blue_bias /= (SX/2.0);
    
    long add = 0;
    
    for (y = 0; y < SY; y += 2) {
        for (x = 0; x < SX; x += 2) {
            ushort	value;
            int	dest_x, dest_y;
            
            if (dev[y * SX + x] < MSTD) {
                value = src[y * SX + x];  //red
                
                dest_x = x + dx;
                dest_y = y + dy;
                
                if (in_range(dest_x, dest_y)) {
                    sum_r[dest_y * SX + dest_x] += (value - red_bias);
                    count_r[dest_y * SX + dest_x] += 1;
                    add++;
                }
            }
            
            
            if (dev[(y+1) * SX + (x+1)] < (MSTD * 1.8)) {
                value = src[(y+1) * SX + (x+1)]; //blue
                
                dest_x = x + dx + 1;
                dest_y = y + dy + 1;
                
                if (in_range(dest_x, dest_y)) {
                    sum_b[dest_y * SX + dest_x] += (value - blue_bias);
                    count_b[dest_y * SX + dest_x] += 1;
                    add++;
                }
            }
            
            if (dev[(y) * SX + (x+1)] < MSTD) {
                value = src[(y) * SX + (x+1)]; //green
                
                dest_x = x + dx + 1;
                dest_y = y + dy;
                
                if (in_range(dest_x, dest_y)) {
                    sum_g[dest_y * SX + dest_x] += (value - green_bias);
                    count_g[dest_y * SX + dest_x] += 1;
                    add++;
                }
            }
            
            if (dev[(y+1) * SX + x] < MSTD) {
                value = src[(y + 1) * SX + (x)]; //green
                
                dest_x = x + dx;
                dest_y = y + dy + 1;
                
                if (in_range(dest_x, dest_y)) {
                    sum_g[dest_y * SX + dest_x] += (value - green_bias);
                    count_g[dest_y * SX + dest_x] += 1;
                    add++;
                }
            }
            
        }
    }
}

ushort rggb_array[SX*SY];
ushort tmp_array[SX*SY];


void find_centroid(float hint_x, float hint_y, float*cx, float*cy)
{
    float	binned[SX/2][SY/2];
    
    int	x,y;
    
    for (y = 0; y < SY/2; y++) {
        for (x = 0; x < SX/2; x++) {
            binned[y][x] = 0;
        }
    }
    
    
    for (y = 0; y < SY; y++) {
        for (x = 0; x < SX; x++) {
            binned[y/2][x/2] += rggb_array[y * SX + x] * 0.25;
        }
    }
    
    int	max_x;
    int	max_y;
    float	max_v = 0;
    float	offset = 0;
    
    for (y = 0; y < SY/2; y++) {
        for (x = 0; x < SX/2; x++) {
            offset += binned[y][x];
            if (binned[y][x] > max_v) {
                max_v = binned[y][x];
                max_x = x;
                max_y = y;
            }
        }
    }
    
    if (hint_x > 0) {
        max_v = 0;
        for (y = hint_y/2 - 50; y < hint_y/2 + 50; y++) {
            for (x = hint_x/2 - 50; x < hint_x/2 + 50; x++) {
                if (binned[y][x] > max_v) {
                    max_v = binned[y][x];
                    max_x = x;
                    max_y = y;
                }
            }
        }
    }
    
    offset = offset / (SY*SX/4.0);
    float	sum_x = 0;
    float	sum_y = 0;
    float	total = 0;
    
    for (y = max_y - 4; y <= max_y + 4; y++) {
        for (x = max_x - 4; x <= max_x + 4; x++) {
            if ((binned[y][x] - offset) > 0) {
                sum_x = sum_x + ((float)x*(binned[y][x] - offset));
                sum_y = sum_y + ((float)y*(binned[y][x] - offset));
                total = total + (binned[y][x] - offset);
            }
        }
    }
    
    sum_x = sum_x / total;
    sum_y = sum_y / total;
    *cx = sum_x * 2.0;
    *cy = sum_y * 2.0;
    
    printf("%d %d <%f %f> %f\n", max_x, max_y, sum_x, sum_y, offset);
}

void calc_flat()
{
    FILE    *flat_file = fopen("./flat10001000.raw", "rb");
    
    for (int i = 0; i < 1000; i++) {
        fread(tmp_array, 1, SX*SY*sizeof(ushort), flat_file);
        for (int j = 0; j < SX*SY; j++) {
            flat[j] = flat[j] + (tmp_array[j] / (1000.0*10000.0));
        }
    }
    
    //skip 200
    for (int i = 0; i < 200; i++) {
        fread(tmp_array, 1, SX*SY*sizeof(ushort), flat_file);
    }
    
    //sub 800 darks
    for (int i = 0; i < 800; i++) {
        fread(tmp_array, 1, SX*SY*sizeof(ushort), flat_file);
        for (int j = 0; j < SX*SY; j++) {
            flat[j] = flat[j] - (tmp_array[j] / (800.0*10000.0));
        }
    }
    
    for (int j = 0; j < SX*SY; j++) {
	//flat[j] /= 2.0;
   	printf("%f\n", flat[j]); 
    }
    fclose(flat_file);

}

void get_dk()
{
    FILE *bias_file = fopen("./dk.raw", "rb");
    
    for (int i = 0; i < 2000; i++) {
        fread(tmp_array, 1, SX*SY*sizeof(ushort), bias_file);
        for (int j = 0;  j < SX*SY; j++)
            bias[j] += (float)tmp_array[j];
    }
    
    for (int j = 0; j < SX*SY; j++) {
        bias[j] /= 2000.0;
    }
    
    fclose(bias_file);
}

void get_std()
{
    FILE *bias_file = fopen("./dk.raw", "rb");
    
    for (int i = 0; i < 2000; i++) {
        fread(tmp_array, 1, SX*SY*sizeof(ushort), bias_file);
        for (int j = 0;  j < SX*SY; j++) {
            float v = bias[j] - (float)tmp_array[j];
            dev[j] += v * v;
        }
    }
    
    for (int j = 0; j < SX*SY; j++) {
        dev[j] /= 2000.0;
        dev[j] = sqrt(dev[j]);
        
        if (bias[j] > 12000)
            dev[j] = 5000;
    }
    
    fclose(bias_file);
}

void save_result()
{
    
    FILE *out_red = fopen("./color_red.raw", "wb");
    FILE *out_green = fopen("./color_green.raw", "wb");
    FILE *out_blue = fopen("./color_blue.raw", "wb");
    
    int	x, y;
    
    for (y = 0; y < SY; y++) {
        for (x = 0; x < SX; x++) {
            float	red;
            float	green;
            float	blue;
            
            int	idx = y * SX + x;
            
            red = sum_r[idx] / (0.1+count_r[idx]);
            green = sum_g[idx] / (0.1+count_g[idx]);
            blue = sum_b[idx] / (0.1+count_b[idx]);
            
            
            fwrite(&red, 1, sizeof(red), out_red);
            fwrite(&green, 1, sizeof(green), out_green);
            fwrite(&blue, 1, sizeof(blue), out_blue);
        }
    }
    fclose(out_red);
    fclose(out_blue);
    fclose(out_green);
    
}

int main()
{
    zero_all();
    
    
    get_dk();
    
    get_std();
    calc_flat();
    
    FILE *input = fopen("./m57.raw", "rb");
    
    float	cur_x;
    float	cur_y;
    float	hint_x = 0;
    float	hint_y = 0;
    int     frame = 0;
    float	last_x;
    float	last_y;
    
    //fseek(input, SX*SY*sizeof(ushort)*8000,SEEK_SET);
    do {
        int cnt = fread(rggb_array, 1, SX * SY * sizeof(ushort), input);
        if (cnt == (SX * SY * sizeof(ushort))) {
            
            for (int i = 0; i < SX*SY; i++) {
                float val = rggb_array[i];
                val += 2000;
                val -= 1.0*bias[i];
                val = val / (flat[i] + 0.23);
                if (val < 0) val = 0;	
                rggb_array[i] = val;
            }
            
            find_centroid(0, 0, &cur_x, &cur_y);
            if (hint_x == 0) {
                hint_x = cur_x + 0.5;
                hint_y = cur_y + 0.5;
                last_x = cur_x;
                last_y = cur_y;	
            }
            
            if (fabs(cur_x - last_x) < 20 && (fabs(cur_y - last_y) < 20)) {	
                add_channel(rggb_array, (hint_x - cur_x), (hint_y - cur_y));	
                last_x = cur_x;
                last_y = cur_y;	
            }
            else
                printf("too far\n");	
        }	
        else
            break;
        frame++;
        if (frame > 13500)
            break;	
    } while (1); 
    
    save_result();
    
    return 0;
}
