#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char* argv[])
{
	const char* dev_path = "/dev/deca_adc";
	short words_count;
	int fd;
	long tmp;
	int main_ret = -1;
	int ret;
	short *buf;
	int i;

	if (argc != 2) {
		printf("Wrong arguments. Pass number of words to get in decimal"
		       " format\n");
		return -1;
	}

	fd = open(dev_path, O_RDWR);
	if (fd < 0) {
		perror("open()");
		return -1;
	}

	tmp = strtol(argv[1], NULL, 10);
	if (tmp < 1 || tmp > 512) {
		printf("Wrong input, should be in range of 1 to 512\n");
		goto exit;
	}

	words_count = (short)tmp;

	ret = write(fd, &words_count, sizeof(words_count));
	if (ret != sizeof(words_count)) {
		perror("write()");
		goto exit;
	}

	sleep(1);

	buf = (short *)malloc(sizeof(short) * words_count);
	if (!buf) {
		perror("malloc()");
		goto exit;
	}

	ret = read(fd, (char *) buf, sizeof(short) * words_count);
	if (ret != sizeof(short) * words_count) {
		perror("read()");
		printf("ret = %d\n", ret);
		goto exit_1;
	}

	for (i = 0; i < words_count; i++) {
		printf("0x%04x ", buf[i]);
	}
	printf("\n");

	main_ret = 0;
exit_1:
	free(buf);
exit:
	close(fd);
	return main_ret;
}
