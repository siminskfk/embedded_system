#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <linux/input.h>
#include <linux/fb.h>


#define FBDEV_FILE "/dev/fb0"

// LCD 화면 프레임 저장 배열
unsigned short frame[384000];
unsigned short csframe[384000];
unsigned short backframe[384000];

// ms delay 함수

void m_delay(int num) {
    volatile int i, j;
    for (i = 0;i < num;i++)
        for (j = 0;j < 16384;j++);
}


void LCDinit(char* filename, int cols, int rows, int sx, int sy) {
    int screen_width;
    int screen_height;
    int bits_per_pixel;
    int line_length;
    int fb_fd;
    struct fb_var_screeninfo fbvar;
    struct fb_fix_screeninfo fbfix;
    unsigned char* fb_mapped;
    int mem_size;
    unsigned short* ptr;
    int coor_y;
    int coor_x;
    int i;
    FILE* bmpfd;
    char* lpImg, * tempImg;
    char r, g, b;
    int j = 0;

    int cols = 800, rows = 480;

    if (access(FBDEV_FILE, F_OK)) {
        printf("%s: access error\n", FBDEV_FILE);
        exit(1);
    }
    if ((fb_fd = open(FBDEV_FILE, O_RDWR)) < 0) {
        printf("%s: open error\n", FBDEV_FILE);
        exit(1);
    }
    if (ioctl(fb_fd, FBIOGET_VSCREENINFO, &fbvar)) {
        printf("%s: ioctl error - FBIOGET_VSCREENINFO \n",
            FBDEV_FILE);
        exit(1);
    }
    if (ioctl(fb_fd, FBIOGET_FSCREENINFO, &fbfix)) {
        printf("%s: ioctl error - FBIOGET_FSCREENINFO \n", FBDEV_FILE);
        exit(1);
    }
    screen_width = fbvar.xres; // 스크린의 픽셀 폭
    screen_height = fbvar.yres; // 스크린의 픽셀 높이
    bits_per_pixel = fbvar.bits_per_pixel; // 픽셀 당 비트 개수
    line_length = fbfix.line_length; // 한개 라인 당 바이트 개수
    mem_size = screen_width * screen_height * 2;
    fb_mapped = (unsigned char*)mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);

    bmpfd = fopen(filename, "rb");//파일을 읽기 모드로 엶
    if (bmpfd == NULL) {
        printf("파일 소환 실패\n");
        exit(1);
    }

    fseek(bmpfd, 54, SEEK_SET);

    lpImg = (char*)malloc(1152000);

    tempImg = lpImg;

    fread(lpImg, sizeof(char), 1152000, bmpfd);

    for (i = 0; i < 384000; i++) {
        b = *lpImg++;
        g = *lpImg++;
        r = *lpImg++;

        frame[j] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        j++;
    }

    for (i = 0; i < 384000; i++) {
        backframe[i] = frame[i];
    }

    j = 0;

    for (coor_y = 0; coor_y < rows; coor_y++) {
        ptr = (unsigned short*)fb_mapped + (screen_width * coor_y);
        for (coor_x = 0; coor_x < cols; coor_x++)
            *ptr++ = csframe[coor_x + coor_y * cols];
    }
    fclose(bmpfd);

    munmap(fb_mapped, mem_size);
    close(fb_fd);
}


int main(void) {
    char*name="P2_enroll1.bmp"
    printf("Start\n");
    LCDinit(LCD_FINIT);
    return 0;
}