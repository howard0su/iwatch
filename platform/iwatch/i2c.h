#ifndef _I2C_H_
#define _I2C_H_
extern void I2C_Init();
extern void I2C_readbytes(unsigned char reg, unsigned char *data, uint8_t len);
extern void I2C_write(unsigned char reg, unsigned char write_word);
extern void I2C_addr(unsigned char address);

#define I2C_read8(reg, data) I2C_readbytes(reg, data, 1)
#define I2C_read16(reg, data) I2C_readbytes(reg, (unsigned char*)data, 2); \
                              __swap_bytes(*data);

#endif