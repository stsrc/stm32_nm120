#include "nm120.h"
#include "i2c.h"

#define WRADDR 0xCEU
#define RDADDR WRADDR | 1U

#define READ_REG_VER_CMD "\xfb\xd9"
#define READ_REG_VER_CMD_SIZE 2U

void nm120_init() {
	i2c_init();
}

unsigned char nm120_version()
{
	unsigned char version;
	i2c_write(WRADDR,
		  (const unsigned char *) READ_REG_VER_CMD,
		  READ_REG_VER_CMD_SIZE);
	i2c_read(RDADDR, &version, sizeof(version));
	return version;
}

