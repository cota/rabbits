#include <sys/mman.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <global.h>

#include "io.h"

#include <init.h>
#include <jpgdec_regs.h>
#include <jpgblock.h>

#define PAGE_SIZE	(4 * 1024)
#define LEN		16
#define DEV_PATH "/dev/uio0"

#define NOF_ADAPTER_REG_WR_DONE		0 /* R/O */
#define NOF_ADAPTER_REG_WR_SEND		1 /* W/O */
#define NOF_ADAPTER_REG_RD_EMPTY	2 /* W/O */
#define NOF_ADAPTER_REG_RD_FULL		3 /* R/O */

static uint32_t *mp, *regs, *acc_regs;
static unsigned char bmp_received[RGB_NUM][IMAGE_SIZE];

static void *map_area(const char *dev, int n)
{
	void *iomem;
	int fd;

	fd = open(dev, O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	iomem = mmap(0, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, n * PAGE_SIZE);
	if (iomem == MAP_FAILED) {
		perror("mmap");
		exit(EXIT_FAILURE);
	}
	return iomem;
}

static inline int can_write(void)
{
	return ioread32(regs + NOF_ADAPTER_REG_WR_DONE);
}

static inline int send_write(void)
{
	iowrite32(1, regs + NOF_ADAPTER_REG_WR_SEND);
}

static void write_block(uint32_t *mp, uint32_t *data, size_t len)
{
	while (!can_write())
		usleep(1);

	for (int i = 0; i < len; i++)
		iowrite32(data[i], mp + i);
	send_write();
}

static void write_input(void)
{
	JpgBlock<unsigned char> data;
	int block_num = (JPEG_FILE_SIZE / DCTSIZE2) + 1;

	for (int i = 0; i < block_num; i++) {
		for (int j = 0; j < DCTSIZE2; j++) {
			int k = i * DCTSIZE2 + j;

			/* The decoder ignores bytes beyond this point */
			if (k < JPEG_FILE_SIZE)
				data.array[j] = input_jpg[k];
			else
				data.array[j] = 0;
		}
		write_block(mp, (uint32_t *)&data.array[0], LEN);
	}
}

static void read_block(uint32_t *data, size_t len)
{
	for (int i = 0; i < len; i++)
		data[i] = ioread32(mp + i);
}

static inline int poll_read(void)
{
	if (!ioread32(regs + NOF_ADAPTER_REG_RD_FULL))
		usleep(1);
}

static inline int mark_read(void)
{
	iowrite32(1, regs + NOF_ADAPTER_REG_RD_EMPTY);
}

static void read_output(void)
{
	int bmp_block_num = ((WIDTH + DCTSIZE - 1) / DCTSIZE) *
		((HEIGHT + DCTSIZE - 1) / DCTSIZE);
	int bnum = 0;
	int hpos = 0;
	int vpos = 0;
	JpgBlock<unsigned char> data;

	while (bnum < RGB_NUM * bmp_block_num) {
		for (int c = 0; c < RGB_NUM; c++) {
			for (int i = 0; i < 4; i++) {
				poll_read();
				read_block((uint32_t *)&data.array[0], LEN);
				mark_read();
				bnum++;
				//cout << "BLOCK " << bnum << " - comp " << c << " - hpos, vpos " << hpos << ", " << vpos << " i " << i << endl;
				for (int j=0; j<DCTSIZE; j++) {
					int vind = (vpos + ( i/2 ) ) * DCTSIZE + j;
					if (vind >= HEIGHT) break;
					vind = vind * WIDTH;

					for (int k=0; k<DCTSIZE; k++) {
						int hind = (hpos + ( i%2 ) )*DCTSIZE + k;
						if (hind >= WIDTH) break;
						//cout << "Actually written @ " << vind << " + " << hind << " = " << vind + hind << endl;
						bmp_received[c][vind + hind] = data.array[DCTSIZE*j + k];
					}
				}
			}
		}
		hpos += 2;
		if (hpos * DCTSIZE > WIDTH) {
			hpos = 0;
			vpos += 2;
		}
	}
}

int main(int argc, char *argv[])
{
	size_t addr;
	pid_t pid;
	int fd;

	mp		= (uint32_t *)map_area(DEV_PATH, 0);
	regs		= (uint32_t *)map_area(DEV_PATH, 1);
	acc_regs 	= (uint32_t *)map_area(DEV_PATH, 2);

	iowrite32(JPEG_FILE_SIZE, acc_regs + (JPGDEC_REG_FILE_SIZE >> 2));

	mark_read();

	pid = fork();
	if (pid < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0)
		write_input();
	else
		read_output();

	return 0;
}
