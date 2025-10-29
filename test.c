// ...existing code...
/*
 * Linux 用户空间 I2C demo（使用 ioctl I2C_RDWR）
 *
 * 功能：
 *  - 打开 /dev/i2c-N
 *  - 使用 I2C_RDWR ioctl 实现读写寄存器（支持组合读写）
 *
 * 编译：
 *   make
 *
 * 运行示例（需要权限）：
 *   sudo ./test 1 0x50 r 0x00
 *   sudo ./test 1 0x50 w 0x10 0xAB
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

static int open_i2c_bus(int bus)
{
    char filename[32];
    int fd;

    snprintf(filename, sizeof(filename), "/dev/i2c-%d", bus);
    fd = open(filename, O_RDWR);
    if (fd < 0) {
        fprintf(stderr, "打开 %s 失败: %s\n", filename, strerror(errno));
        return -1;
    }
    return fd;
}

/* 使用 I2C_RDWR ioctl 向从设备 addr 的寄存器 reg 写一个字节 */
static int i2c_write_reg_ioctl(int fd, int addr, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = { reg, val };
    struct i2c_msg msgs[1];
    struct i2c_rdwr_ioctl_data rdwr = {
        .msgs = msgs,
        .nmsgs = 1,
    };

    msgs[0].addr  = addr;
    msgs[0].flags = 0;
    msgs[0].len   = sizeof(buf);
    msgs[0].buf   = buf;

    if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
        fprintf(stderr, "ioctl I2C_RDWR 写失败: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}

/* 使用 I2C_RDWR ioctl 先写寄存器地址再读一个字节（组合消息）*/
static int i2c_read_reg_ioctl(int fd, int addr, uint8_t reg, uint8_t *out)
{
    uint8_t reg_buf = reg;
    uint8_t val_buf = 0;
    struct i2c_msg msgs[2];
    struct i2c_rdwr_ioctl_data rdwr = {
        .msgs = msgs,
        .nmsgs = 2,
    };

    /* 写寄存器地址 */
    msgs[0].addr  = addr;
    msgs[0].flags = 0;
    msgs[0].len   = 1;
    msgs[0].buf   = &reg_buf;

    /* 读回数据 */
    msgs[1].addr  = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len   = 1;
    msgs[1].buf   = &val_buf;

    if (ioctl(fd, I2C_RDWR, &rdwr) < 0) {
        fprintf(stderr, "ioctl I2C_RDWR 读失败: %s\n", strerror(errno));
        return -1;
    }

    *out = val_buf;
    return 0;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "用法: %s <bus> <addr> <r|w> <reg> [value]\n"
        " 示例:\n"
        "  %s 1 0x50 r 0x00          # 读寄存器 0x00\n"
        "  %s 1 0x50 w 0x10 0xAB     # 写寄存器 0x10 = 0xAB\n",
        prog, prog, prog);
}

int main(int argc, char **argv)
{
    if (argc < 5) {
        usage(argv[0]);
        return 1;
    }

    int bus = atoi(argv[1]);
    int addr = (int)strtol(argv[2], NULL, 0) & 0x7F;
    char op = argv[3][0];
    uint8_t reg = (uint8_t)strtol(argv[4], NULL, 0);

    int fd = open_i2c_bus(bus);
    if (fd < 0) return 2;

    /* 可选：设置 I2C_SLAVE（部分驱动需要） */
    if (ioctl(fd, I2C_SLAVE, addr) < 0) {
        /* 若失败也不立即退出，后续 I2C_RDWR 会使用 msgs[].addr */
        fprintf(stderr, "设置 I2C_SLAVE 0x%02X 失败（忽略）: %s\n", addr, strerror(errno));
    }

    int ret = 0;
    if (op == 'w') {
        if (argc < 6) {
            fprintf(stderr, "写操作需要提供 value 参数。\n");
            ret = 3;
            goto out;
        }
        uint8_t val = (uint8_t)strtol(argv[5], NULL, 0);
        if (i2c_write_reg_ioctl(fd, addr, reg, val) < 0) ret = 4;
        else printf("写入: addr=0x%02X reg=0x%02X val=0x%02X\n", addr, reg, val);
    } else if (op == 'r') {
        uint8_t val;
        if (i2c_read_reg_ioctl(fd, addr, reg, &val) < 0) ret = 5;
        else printf("读取: addr=0x%02X reg=0x%02X => 0x%02X\n", addr, reg, val);
    } else {
        fprintf(stderr, "未知操作: %c\n", op);
        ret = 6;
    }

out:
    close(fd);
    return ret;
}
// ...existing code...