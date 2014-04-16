#ifndef _I2C_H_
#define _I2C_H_
extern void I2C_Init_();
extern int I2C_readbytes(I2C_TypeDef *i2c,uint8_t addr, unsigned char reg, unsigned char *data, uint16_t len);
extern int I2C_writebytes(I2C_TypeDef *i2c,uint8_t addr, unsigned char reg, const unsigned char *data, uint8_t len);
extern int I2C_write(I2C_TypeDef *i2c, uint8_t addr, unsigned char reg, const unsigned char data);

extern void I2C_addr(unsigned char address);
extern void I2C_done(); // move I2c module to power-save mode

#define I2C_read8(reg, data) I2C_readbytes(reg, data, 1)

#endif