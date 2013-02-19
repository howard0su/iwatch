#ifndef _I2C_H_
#define _I2C_H_
extern void I2C_Init();
extern unsigned char I2C_read(unsigned char reg);
extern void I2C_write(unsigned char reg, unsigned char write_word);
extern void I2C_addr(unsigned char address);
#endif