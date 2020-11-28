#include <math.h>
#include <float.h>
#include <limits.h>
#include <time.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/fb.h>
#include <linux/input.h>

// header file for facedetect
#include "cv.h"
#include "highgui.h"

#define RGB565(r,g,b)	((((r)>>3)<<11) | (((g)>>2)<<5) | ((b)>>3))
#define FBDEV_FILE	"/dev/fb0"
#define CAMERA_DEVICE	"/dev/camera"
#define FILE_NAME	"face_image.jpg"
#define P1_ENROLL	"P1_enroll.bmp"

static CvMemStorage* storage = 0;
static CvHaarClassifierCascade* cascade = 0;
struct fb_var_screeninfo fbvar;
struct fv_fix_screeninfo fbfix;
unsigned char* pfbmap;
unsigned short cis_rgb[320 * 240 * 2];

// LCD screen frame save
unsigned short frame[800 * 480];
unsigned short csframe[800 * 480];
unsigned short backframe[800 * 480];

const char *cascade_xml = "haarcascade_frontalface_alt2.xml";

#if 0
	"haarcascade_frontalface_alt.xml";
	"haarcascade_frontalface_alt2.xml";
	"haarcascade_frontalface_alt_tree.xml";
	"haarcascade_frontalface_default.xml";
	"haarcascade_fullbody.xml";
	"haarcascade_lowerbody.xml";
	"haarcascade_profileface.xml";
	"haarcascade_upperbody.xml";
#endif

static struct termios initial_settings, new_settings;
static int peek_character = -1;

void init_keyboard() {
	tcgetattr(0, &initial_settings);
	new_settings = initial_settings;
	new_settings.c_lflag &= ~ICANON;
	new_settings.c_lflag &= ~ECHO;
	new_settings.c_lflag &= ~ISIG;
	new_settings.c_cc[VMIN] = 1;
	new_settings.c_cc[VTIME] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
}

void close_keyboard() {
	tcsetattr(0, TCSANOW, &initial_settings);
}

int kbhit() {
	char ch;
	int nread;

	if(peek_character != -1) return 1;

	new_settings.c_cc[VMIN] = 0;
	tcsetattr(0, TCSANOW, &new_settings);
	nread = read(0, &ch, 1);
	new_settings.c_cc[VMIN] = 1;
	tcsetattr(0, TCSANOW, &new_settings);

	if(nread == 1) {
		peek_character = ch;
		return 1;
	}

	return 0;
}

int readch() {
	char ch;

	if(peek_character != -1) {
		ch = peek_character;
		peek_character = -1;
		return ch;
	}
	read(0, &ch, 1);
	return ch;
}

int fb_display(unsigned short* rgb, int sx, int sy, int szx, int szy) {
	int coor_x, coor_y;
	int screen_width, screen_height;
	unsigned short* ptr;

	screen_width = fbvar.xres;
	screen_height = fbvar.yres;
	bits_per _pixel = fbvar.bits_per_pixel;
	mem_size = screen_width * screen_height * 2;

	for (coor_y = 0; coor_y < szy; coor_y++) {
		ptr = (unsigned short*)pfbmap + (screen_width * sy + sx) + (screen_width * coor_y);
		for (coor_x = 0; coor_x < szx; coor_x++) {
			*ptr++ = rgb[coor_x + coor_y * szx];
		}
	}
	return 0;
}
void LCDdisplay(char* filename, int cols, int rows, int sx, int sy) {
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
	screen_width = fbvar.xres; // screen pixel width
	screen_height = fbvar.yres; // screen pixel height
	bits_per_pixel = fbvar.bits_per_pixel; // bits per pixel
	line_length = fbfix.line_length; // bytes per line
	mem_size = screen_width * screen_height * 2;
	fb_mapped = (unsigned char*)mmap(0, mem_size, PROT_READ | PROT_WRITE, MAP_SHARED, fb_fd, 0);

	if (stage == 0) {
		bmpfd = fopen("P1_enroll.bmp", "rb");// open with read mode
		if (bmpfd == NULL) {
			printf("파일 소환 실패\n");
			exit(1);
		}

		fseek(bmpfd, 54, SEEK_SET);

		lpImg = (char*)malloc(cols * rows * 3);

		tempImg = lpImg;

		fread(lpImg, sizeof(char), cols * rows * 3, bmpfd);

		for (i = 0; i < cols * rows; i++) {
			b = *lpImg++;
			g = *lpImg++;
			r = *lpImg++;

			frame[j] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
			j++;
		}
		for (i = 0; i < cols * rows; i++) {
			backframe[i] = frame[i];
		}

		j = 0;
		for (coor_y = 0; coor_y < rows; coor_y++) {
			ptr = (unsigned short*)fb_mapped + (screen_width * sy + sx) + (screen_width * coor_y);
			for (coor_x = 0; coor_x < cols; coor_x++)
				*ptr++ = csframe[coor_x + coor_y * cols];
		}
		fclose(bmpfd);
	}
	munmap(fb_mapped, mem_size);
	close(fb_fd);
}

void cvIMG2RGB565(IplImage *img, unsigned short *cv_rgb, int ex, int ey){
	int x, y;
	unsigned char r,g,b;

	for(y = 0; y < ey; y++){
		for(x=0; x < ex; x++){
			b = (img->imageData[(y*img->widthStep)+x*3]);
			g = (img->imageData[(y*img->widthStep)+x*3+1]);
			r = (img->imageData[(y*img->widthStep)+x*3+2]);
			cv_rgb[y*320 + x] = (unsigned short)RGB565(r,g,b);
		}
	}
}

void Fill_Background(unsigned short color){
	int x,y;

	for(y=0; y<480; y++){
		for(x=0; x<800; x++){
			*(unsigned short*)(pfbmap + (x)*2 + (y)*800*2) = color;
		}
	}
}

void RGB2cvIMG(IplImage *img, unsigned short *rgb, int ex, int ey){
	int x,y;

	for(y=0; y<ey; y++){
		for(x=0; x<ex; x++){
			(img->imageData[(y*img->widthStep)+x*3]) = (rgb[y*ex+x]&0x1F)<<3;	//b
			(img->imageData[(y*img->widthStep)+x*3+1]) = ((rgb[y*ex+x]&0x07E0)>>5)<<2;	//g
			(img->imageData[(y*img->widthStep)+x*3+2]) = ((rgb[y*ex+x]&0xF800)>>11)<<3;	//r
		}
	}
}

int detect_and_draw(IplImage *img){
	int ret = 0;

	static CvScalar colors[] = {
					{{0,0,255}},
					{{0,128,255}},
					{{0,255,255}},
					{{0,255,255}},
					{{255,128,0}},
					{{255,255,0}},
					{{255,0,0}},
					{{255,0,255}}
				};

	double scale = 1.3;
	CvSize x = cvSize(cvRound (img->width/scale), cvRound (img->height/scale));

	IplImage *gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
	IplImage *small_img = cvCreateImage(x, 8, 1);
	int i;

	cvCvtColor(img, gray, CV_BGR2GRAY);

	cvResize(gray, small_img, CV_INTER_LINEAR);

	cvEqualizeHist(small_img, small_img);

	cvClearMemStorage(storage);

	if(cascade){
		double t = (double)cvGetTickCount();

		CvSeq *faces = cvHaarDetectObjects(gray, cascade, storage, 1.1, 2, 0 /*CV_HAAR_DO_CANNY_PRUNING*/, cvSize(30,30));

		for(i=0; i<(faces ? faces->total : 0); i++){
			CvRect *r = (CvRect*)cvGetSeqElem(faces, i);
			CvPoint center;
			int radius;
			
			center.x = cvRound((r->x + r->width * 0.5) * scale) - 55;
			center.y = cvRound((r->y + r->width * 0.5) * scale) - 25;
			radius = cvRound((r->width + r->height) * 0x35 * scale);
			cvCircle(img, center, radius, colors[i%8], 3, 8, 0);
			ret++;
		}
	}

	cvIMG2RGB565(img, cis_rgb, img->width, img->height);
	if(ret){
		fb_display(cis_rgb, 435, 120, 320, 240);
	}

	cvReleaseImage(&gray);
	cvReleaseImage(&small_img);

	return ret;
}

int main(int argc, char** argv) {
	FILE* bmpfd;
	int fbfd, fd;
	int dev, ret = 0;
	int optlen = strlen("--cascade=");
	unsigned short ch = 0;
	CvCapture* capture = 0;
	IplImage* image = NULL;

	if ((fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		perror("mem open fail\n");
		exit(1);
	}
	if ((fbfd = open(FBDEV_FILE, O_RDWR)) < 0) {
		printf("Failed to open: %s\n", FBDEV_FILE);
		exit(-1);
	}
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fbvar) < 0) {
		printf("Failed to open: %s - FBIOGET_VSCREENINFO\n", FBDEV_FILE);
		exit(1);
	}
	if (ioctl(fbfd, FBIOGET_VSCREENINFO, &fbfix) < 0) {
		printf("Failed to open: %s - FBIOGET_VSCREENINFO\n", FBDEV_FILE);
		exit(1);
	}
	if (fbvar.bits_per_pixel != 16) {
		fprintf(stderr, "bpp is not 16\n");
		exit(1);
	}

	pfbmap = (unsigned char*)mmap(0, fbvar.xres * fbvar.yres * 2, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);

	if ((unsigned)pfbmap < 0) {
		perror("mmap failed...");
		exit(1);
	}

	if (argc > 1 && strncmp(argv[1], "--cascade=", optlen) == 0) {
		cascade_xml = argv[1] + optlen;
	}

	cascade = (CvHaarClassifierCascade*)cvLoad(cascade_xml, 0, 0, 0);

	if (!cascade) {
		fprintf(stderr, "ERROR: Could not load classifier cascade\n");
		fprintf(stderr, "Usage : ./facedetect -- cascade=[CASCADE_PATH/FILENAME]\n");
		return -1;
	}

	storage = cvCreateMemStorage(0);
	Fill_Background(0x0011);

	dev = open(CAMERA_DEVICE, O_RDWR);
	if (dev < 0) {
		printf("Error: cannot open %s.\n", CAMERA_DEVICE);
		exit(1);
	}

	image = cvCreateImage(cvSize(320, 240), IPL_DEPTH_8U, 3);

	printf("1.3M CIS Camera FaceDetect Program -ver.20100721\n");
	printf("Usage : \n");
	printf("	[d] CIS Camera Display\n");
	printf("	[f] face detect start\n");
	printf("	[s] face detect & image save(JPEG)\n");
	printf("	[q] EXIT\n\n");

	init_keyboard();
	if (kbhit()) {
		ch = readch();
	}
	LCDdisplay(P1_ENROLL, 800, 480, 0, 0);
	while (ch != 'q') {

		write(dev, NULL, 1);
		read(dev, cis_rgb, 320 * 240 * 2);

		fb_display(cis_rgb, 280, 120, 320, 240);

		if (ch == 'f' || ch == 's') {
			RGB2cvIMG(image, cis_rgb, 320, 240);
			if (image) {
				ret = detect_and_draw(image);
				if (ch == 's' && ret > 0) {
					cvSaveImage(FILE_NAME, image);
				}
			}
			if (ret) ch = 'd';
		}
	}

	cvReleaseImage(&image);
	close_keyboard();
	return 0;
}


