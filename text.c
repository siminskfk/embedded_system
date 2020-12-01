

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/ioctl.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <unistd.h>

#include <linux/input.h>

#include <termios.h>

#include <time.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <unistd.h> // open/close

#include <fcntl.h> // O_RDWR

#include <sys/ioctl.h> // ioctl

#include <sys/mman.h> // mmap PROT_

#include <linux/fb.h>

#include <sys/poll.h>

#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <sys/ioctl.h>

#include <sys/types.h>

#include <sys/stat.h>

#include <fcntl.h>

#include <unistd.h>

#include <linux/input.h>





// touch 관련 변수 정의

#define EVENT_BUF_NUM 2

#define Y_WIDTH 1275

#define X_WIDTH 797.5

#define X_OFFSET 4920-X_WIDTH/2

#define Y_OFFSET 13150+Y_WIDTH/2



int x_detected, y_detected;        // touch 받아서 detecting한 좌표 저장하는 변수



#define FBDEV_FILE "/dev/fb0"





#define TURN     1



#define STONE_CURSOR    4



#define BOARD_WIDTH     9

#define BOARD_HEIGHT    9



// LCD 관련 정의

#define LCD_FINIT 0

#define LCD_SINIT 1

#define LCD_PRINT 2



// LCD 화면 프레임 저장 배열

unsigned short frame[384000];

unsigned short csframe[384000];

unsigned short backframe[384000];



// 바둑판 각 좌표별 상태 저장 이중 배열

unsigned int board_status[BOARD_HEIGHT][BOARD_WIDTH] = { { 0 } };





// 커서의 현재 좌표 저장용 배열

unsigned int cursor[2] = { 0,0 };



unsigned int turn = TURN;



unsigned int clock_count;

unsigned int sec_saved;

unsigned int sec_present;

struct timeval val;

struct tm* ptm;



// 터치 받을 때 시간 넘어가면 통과할 때 사용

int poll_state;

struct pollfd    poll_events;



// TextLCD 에 표시되는 값들에 관한 정보 저장 구조체

struct strcommand_varible strcommand;



struct strcommand_varible {

    char rows;

    char nfonts;

    char display_enable;

    char cursor_enable;

    char nblink;

    char set_screen;

    char set_rightshit;

    char increase;

    char nshift;

    char pos;

    char command;

    char strlength;

    char buf[16];

};



struct strcommand_varible strcommand;



// file descriptors

int key_fd = -1;

int textlcd_fd = -1;

int dot_fd = -1;

int seg_fd = -1;

int led_fd = -1;

int event_fd = -1;

int buz_fd = -1;



struct input_event event_buf[EVENT_BUF_NUM];



// ms delay 함수

void m_delay(int num) {

    volatile int i, j;

    for (i = 0;i < num;i++)

        for (j = 0;j < 16384;j++);

}





int GetTouch(void) {

    unsigned int  buzzer_sound;

    int i;

    size_t read_bytests;

    struct input_event event_bufts[EVENT_BUF_NUM];

    int x, y;



    if ((event_fd = open("/dev/input/event2", O_RDONLY)) < 0) {

        printf("open error");

        exit(1);

    }



    poll_events.fd = event_fd;

    poll_events.events = POLLIN | POLLERR;          // 수신된 자료가 있는지, 에러가 있는지

    poll_events.revents = 0;



    poll_state = poll(                                    // poll()을 호출하여 event 발생 여부 확인

    (struct pollfd*) & poll_events,       // event 등록 변수

        1,   // 체크할 pollfd 개수

        10    // time out 시간

        );



    if (poll_events.revents == POLLIN) {

        read_bytests = read(event_fd, event_bufts, ((sizeof(struct input_event)) * EVENT_BUF_NUM));



        for (i = 0; i < (read_bytests / sizeof(struct input_event)); i++) {

            switch (event_bufts[i].type) {

            case EV_ABS:

                switch (event_bufts[i].code) {

                case ABS_X:

                    y = event_bufts[i].value;

                    break;

                case ABS_Y:

                    x = event_bufts[i].value;

                    break;

                default:

                    break;

                }

                break;

            default:

                break;

            }

        }



        if (x < X_OFFSET)

            x_detected = -1;

        else if (x < X_OFFSET + X_WIDTH)

            x_detected = 0;

        else if (x < X_OFFSET + 2 * X_WIDTH)

            x_detected = 1;

        else if (x < X_OFFSET + 3 * X_WIDTH)

            x_detected = 2;

        else if (x < X_OFFSET + 4 * X_WIDTH)

            x_detected = 3;

        else if (x < X_OFFSET + 5 * X_WIDTH)

            x_detected = 4;

        else if (x < X_OFFSET + 6 * X_WIDTH)

            x_detected = 5;

        else if (x < X_OFFSET + 7 * X_WIDTH)

            x_detected = 6;

        else if (x < X_OFFSET + 8 * X_WIDTH)

            x_detected = 7;

        else if (x < X_OFFSET + 9 * X_WIDTH)

            x_detected = 8;

        else

            x_detected = -1;



        if (y > Y_OFFSET)

            y_detected = -1;

        else if (y > Y_OFFSET - Y_WIDTH)

            y_detected = 0;

        else if (y > Y_OFFSET - 2 * Y_WIDTH)

            y_detected = 1;

        else if (y > Y_OFFSET - 3 * Y_WIDTH)

            y_detected = 2;

        else if (y > Y_OFFSET - 4 * Y_WIDTH)

            y_detected = 3;

        else if (y > Y_OFFSET - 5 * Y_WIDTH)

            y_detected = 4;

        else if (y > Y_OFFSET - 6 * Y_WIDTH)

            y_detected = 5;

        else if (y > Y_OFFSET - 7 * Y_WIDTH)

            y_detected = 6;

        else if (y > Y_OFFSET - 8 * Y_WIDTH)

            y_detected = 7;

        else if (y > Y_OFFSET - 9 * Y_WIDTH)

            y_detected = 8;

        else

            x_detected = -1;



        buzzer_sound = 0xff;

        write(buz_fd, &buzzer_sound, 2);

        m_delay(300);

        buzzer_sound = 0x00;

        write(buz_fd, &buzzer_sound, 2);



        close(event_fd);

    }

    else {

        close(event_fd);

        return -1;

    }



    return 0;

}



void cs(int x, int y) {

    FILE* bmpfdb;

    char* lpImgb;

    char r, g, b;

    unsigned short temp_frame[1600];

    int j = 0;

    int i = 0;

    int xpos = 0;

    int ypos = 0;

    int mov = 0;

    int k = 0;



    switch (turn) {

    case TURN:

        bmpfdb = fopen("size.bmp", "rb");//파일을 읽기 모드로 엶

        if (bmpfdb == NULL) {

            printf("파일 소환 실패\n");

            exit(1);

        }

        break;

    default:

        break;

    }



    fseek(bmpfdb, 54, SEEK_SET);



    lpImgb = (char*)malloc(3072);



    fread(lpImgb, sizeof(char), 3072, bmpfdb);



    for (i = 0; i < 1024; i++) {

        b = *lpImgb++;

        g = *lpImgb++;

        r = *lpImgb++;



        temp_frame[j] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);

        j++;

    }

    xpos = x * 40.625 + 220;

    ypos = y * 40.75 + 60;

    mov = xpos + ypos * 800;



    for (i = 0; i < 384000; i++) {

        csframe[i] = frame[i];

    }



    for (j = 0; j < 32; j++) {

        for (i = 0; i < 32; i++) {

            csframe[mov + i] = temp_frame[k];

            k++;

        }

        mov = mov + 800;

    }

    fclose(bmpfdb);

}



void LCDinit(int inp) {

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





    if (inp == 0) {

        bmpfd = fopen("P2_enroll1.bmp", "rb");//파일을 읽기 모드로 엶

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

    }





    if (inp == 1) {

        for (i = 0; i < 384000; i++) {

            frame[i] = backframe[i];

            csframe[i] = backframe[i];

        }

        for (coor_y = 0; coor_y < rows; coor_y++) {

            ptr = (unsigned short*)fb_mapped + (screen_width * coor_y);

            for (coor_x = 0; coor_x < cols; coor_x++)

                *ptr++ = csframe[coor_x + coor_y * cols];

        }

    }



    if (inp == 2) {

        for (coor_y = 0; coor_y < rows; coor_y++) {

            ptr = (unsigned short*)fb_mapped + (screen_width * coor_y);

            for (coor_x = 0; coor_x < cols; coor_x++)

                *ptr++ = csframe[coor_x + coor_y * cols];

        }

    }



    munmap(fb_mapped, mem_size);

    close(fb_fd);

}



// device driver들 로드

void init_devices() {



    strcommand.rows = 0;

    strcommand.nfonts = 0;

    strcommand.display_enable = 1;

    strcommand.cursor_enable = 0;

    strcommand.nblink = 0;

    strcommand.set_screen = 0;

    strcommand.set_rightshit = 1;

    strcommand.increase = 1;

    strcommand.nshift = 0;

    strcommand.pos = 10;

    strcommand.command = 1;

    strcommand.strlength = 16;



    if ((buz_fd = open("/dev/buzzer", O_WRONLY)) < 0) {

        printf("application : buzzer driver open fail!\n");

        exit(1);

    }



}





void cursorProgram() {

    char key;

    unsigned int status_backup = 10;

    int time_over = 0;

    int put_stone = 0;

    unsigned int cursor_check[2];



    cursor[0] = 4;

    cursor[1] = 4;



    ioctl(seg_fd, 0, NULL, NULL);



    status_backup = board_status[cursor[0]][cursor[1]];

    board_status[cursor[0]][cursor[1]] = STONE_CURSOR;



    gettimeofday(&val, NULL);

    ptm = localtime(&val.tv_sec);



    sec_saved = ptm->tm_hour * 60 * 24 + ptm->tm_min * 60 + ptm->tm_sec;





    while (1) {

        cs(cursor[1], cursor[0]);

        LCDinit(LCD_PRINT);



        if (status_backup != 10)

            board_status[cursor[0]][cursor[1]] = status_backup;



        while (1) {

            if (GetTouch() != -1) {

                if (x_detected != -1 && y_detected != -1) {

                    cursor[0] = y_detected;

                    cursor[1] = x_detected;



                    cs(cursor[1], cursor[0]);

                    LCDinit(LCD_PRINT);

                }

            }



        }



        status_backup = board_status[cursor[0]][cursor[1]];

        board_status[cursor[0]][cursor[1]] = STONE_CURSOR;

        cs(cursor[1], cursor[0]);



    }



    return;

}



int main(void) {

    int i = 0, j = 0;

    unsigned int led_cnt;

    unsigned int seg_count;



    printf("***TOUCH & BUZZER***\n");



    init_devices();

    LCDinit(LCD_FINIT);



    while (1) {



        for (i = 0; i < BOARD_HEIGHT; i++)

            for (j = 0; j < BOARD_WIDTH; j++)

                board_status[i][j] = 0;



        while (1) {

            cursorProgram();

            LCDinit(LCD_PRINT);

        }





        LCDinit(LCD_SINIT);

    }



    return 0;

}