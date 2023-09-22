#ifndef __DC_I2C_H__
#define __DC_I2C_H__

#define DC_I2C_ADDR 0x1f00
#define DC_GPIO_CFG_OFFSET 0x1660
#define DC_GPIO_IN_OFFSET 0x1650
#define DC_GPIO_OUT_OFFSET 0x1650
#define DC_I2C_TON 10

#define DC_I2C_PRER_LO_REG	0x0
#define DC_I2C_PRER_HI_REG	0x1
#define DC_I2C_CTR_REG		0x2
#define DC_I2C_TXR_REG		0x3
#define DC_I2C_RXR_REG		0x3
#define DC_I2C_CR_REG		0x4
#define DC_I2C_SR_REG		0x4

#define CTR_EN			0x80
#define CTR_IEN			0x40

#define CR_START		0x81
#define CR_STOP			0x41
#define CR_READ			0x21
#define CR_WRITE		0x11
#define CR_ACK			0x8
#define CR_IACK			0x1

#define SR_NOACK		0x80
#define SR_BUSY			0x40
#define SR_AL			0x20
#define SR_TIP			0x2
#define SR_IF			0x1

#define dc_readb(addr) readb(i2c->reg_base + addr)
#define dc_writeb(val, addr) writeb(val, i2c->reg_base + addr)

struct gsgpu_dc_i2c {
	struct gsgpu_device *adev;
	struct i2c_adapter adapter;
	struct i2c_client *ddc_client;
	struct completion cmd_complete;
	void __iomem *reg_base;
	u32 data, clock;
};

int gsgpu_i2c_init(struct gsgpu_device *adev, uint32_t link_index);
void gsgpu_dc_i2c_irq(struct gsgpu_dc_i2c *i2c);

#endif /* __DC_I2C_H__ */
