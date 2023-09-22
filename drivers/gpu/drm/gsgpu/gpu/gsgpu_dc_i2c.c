#include <linux/i2c.h>
#include <linux/i2c-algo-bit.h>

#include "gsgpu.h"
#include "gsgpu_dc.h"
#include "gsgpu_dc_connector.h"
#include "gsgpu_dc_crtc.h"
#include "gsgpu_dc_encoder.h"
#include "gsgpu_dc_i2c.h"
#include "gsgpu_dc_hdmi.h"
#include "gsgpu_dc_reg.h"

static void dc_gpio_set_dir(struct gsgpu_device *adev,
			    unsigned int pin, int input)
{
	u32 temp;

	temp = dc_readl_locked(adev, DC_GPIO_CFG_OFFSET);
	if (input)
		temp |= 1UL << pin;
	else
		temp &= ~(1UL << pin);
	dc_writel_locked(adev, DC_GPIO_CFG_OFFSET, temp);
}

static void dc_gpio_set_val(struct gsgpu_device *adev,
			    unsigned int pin, int high)
{
	u32 temp;

	temp = dc_readl_locked(adev, DC_GPIO_OUT_OFFSET);
	if (high)
		temp |= 1UL << pin;
	else
		temp &= ~(1UL << pin);
	dc_writel_locked(adev, DC_GPIO_OUT_OFFSET, temp);
}

static void gsgpu_gpio_set_data(void *i2c, int value)
{
	struct gsgpu_dc_i2c *li2c = i2c;
	struct gsgpu_device *adev = li2c->adev;
	unsigned int pin = li2c->data;

	if (value)
		dc_gpio_set_dir(adev, pin, 1);
	else {
		dc_gpio_set_val(adev, pin, 0);
		dc_gpio_set_dir(adev, pin, 0);
	}
}

static void gsgpu_gpio_set_clock(void *i2c, int value)
{
	struct gsgpu_dc_i2c *li2c = i2c;
	struct gsgpu_device *adev = li2c->adev;
	unsigned int pin = li2c->clock;

	if (value)
		dc_gpio_set_dir(adev, pin, 1);
	else {
		dc_gpio_set_val(adev, pin, 0);
		dc_gpio_set_dir(adev, pin, 0);
	}
}

static int gsgpu_gpio_get_data(void *i2c)
{
	int val;
	struct gsgpu_dc_i2c *li2c = i2c;
	struct gsgpu_device *adev = li2c->adev;
	unsigned int pin = li2c->data;

	val = dc_readl_locked(adev, DC_GPIO_IN_OFFSET);

	return (val >> pin) & 1;
}

static int gsgpu_gpio_get_clock(void *i2c)
{
	int val;
	struct gsgpu_dc_i2c *li2c = i2c;
	struct gsgpu_device *adev = li2c->adev;
	unsigned int pin = li2c->clock;

	val = dc_readl_locked(adev, DC_GPIO_IN_OFFSET);

	return (val >> pin) & 1;
}

void gsgpu_dc_i2c_irq(struct gsgpu_dc_i2c *i2c)
{
	unsigned char int_flag;

	int_flag = dc_readb(DC_I2C_SR_REG);

	if (int_flag & SR_IF) {
		dc_writeb(CR_IACK, DC_I2C_CR_REG);
		if (!(int_flag & SR_TIP))
			complete(&i2c->cmd_complete);
	}
}

static int i2c_stop(struct gsgpu_dc_i2c *i2c)
{
	unsigned long complete;

again:
	dc_writeb(CR_STOP, DC_I2C_CR_REG);
	complete = wait_for_completion_timeout(&i2c->cmd_complete,
			(&i2c->adapter)->timeout);
	if (!complete) {
		DRM_ERROR("Timeout abort message cmd\n");
		return -1;
	}

	dc_readb(DC_I2C_SR_REG);
	while (dc_readb(DC_I2C_SR_REG) & SR_BUSY)
		goto again;

	return 0;
}

static int i2c_start(struct gsgpu_dc_i2c *i2c, int dev_addr, int flags)
{
	unsigned long complete;
	unsigned char addr = (dev_addr & 0x7f) << 1;
	int retry = 5;

	addr |= (flags & I2C_M_RD) ? 1:0;
start:
	mdelay(1);
	dc_writeb(addr, DC_I2C_TXR_REG);
	dc_writeb((CR_START | CR_WRITE), DC_I2C_CR_REG);

	complete = wait_for_completion_timeout(&i2c->cmd_complete,
			(&i2c->adapter)->timeout);
	if (!complete) {
		DRM_ERROR("Timeout abort message cmd\n");
		return -1;
	}

	if (dc_readb(DC_I2C_SR_REG) & SR_NOACK) {
		if (i2c_stop(i2c) < 0)
			return -1;
		while (retry--)
			goto start;
		return 0;
	}

	return 1;
}
static int i2c_read(struct gsgpu_dc_i2c *i2c, unsigned char *buf, int count)
{
	int i;
	unsigned long complete;

	for (i = 0; i < count; i++) {
		dc_writeb((i == count - 1) ? (CR_READ | CR_ACK) : CR_READ,
			    DC_I2C_CR_REG);
		complete = wait_for_completion_timeout(&i2c->cmd_complete,
				(&i2c->adapter)->timeout);
		if (!complete) {
			DRM_ERROR("Timeout abort message cmd\n");
			return -1;
		}

		buf[i] = dc_readb(DC_I2C_RXR_REG);
	}

	return i;
}

static int i2c_write(struct gsgpu_dc_i2c *i2c, unsigned char *buf, int count)
{
	int i;
	unsigned long complete;

	for (i = 0; i < count; i++) {
		dc_writeb(buf[i], DC_I2C_TXR_REG);
		dc_writeb(CR_WRITE, DC_I2C_CR_REG);

		complete = wait_for_completion_timeout(&i2c->cmd_complete,
				(&i2c->adapter)->timeout);
		if (!complete) {
			DRM_ERROR("Timeout abort message cmd\n");
			return -1;
		}

		if (dc_readb(DC_I2C_SR_REG) & SR_NOACK) {
			DRM_ERROR("gsgpu dc i2c device no ack\n");
			if (i2c_stop(i2c) < 0)
				return -1;
			return 0;
		}
	}

	return i;
}

static int i2c_doxfer(struct gsgpu_dc_i2c *i2c,	struct i2c_msg *msgs, int num)
{
	struct i2c_msg *msg = msgs;
	int ret;
	int i;

	for (i = 0; i < num; i++) {
		reinit_completion(&i2c->cmd_complete);
		ret = i2c_start(i2c, msg->addr, msg->flags);
		if (ret <= 0)
			return ret;

		if (msg->flags & I2C_M_RD) {
			if (i2c_read(i2c, msg->buf, msg->len) < 0)
				return -1;
		} else {
			if (i2c_write(i2c, msg->buf, msg->len) < 0)
				return -1;
		}

		++msg;
		if (i2c_stop(i2c) < 0)
			return -1;
	}

	if (i2c_stop(i2c) < 0)
		return -1;

	return i;
}

static int gsgpu_dc_i2c_xfer(struct i2c_adapter *adapter,
			      struct i2c_msg *msgs, int num)
{
	struct gsgpu_dc_i2c *i2c = i2c_get_adapdata(adapter);
	int retry;
	int ret;

	for (retry = 0; retry < adapter->retries; retry++) {
		ret = i2c_doxfer(i2c, msgs, num);
		if (ret != -EAGAIN)
			return ret;

		udelay(100);
	}

	return -EREMOTEIO;
}

static u32 gsgpu_dc_i2c_func(struct i2c_adapter *adapter)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm gsgpu_dc_i2c_algo = {
	.master_xfer = gsgpu_dc_i2c_xfer,
	.functionality = gsgpu_dc_i2c_func,
};

static int gsgpu_dc_i2c_init(struct gsgpu_device *adev,
			     int i2c_addr, uint32_t link_index)
{
	struct gsgpu_dc_i2c *i2c;
	struct i2c_client *ddc_client;
	int ret = 0;
	int value = 0;
	const struct i2c_board_info ddc_info = {
		.type = "ddc-dev",
		.addr = i2c_addr,
		.flags = I2C_CLASS_DDC,
	};

	i2c = kzalloc(sizeof(struct gsgpu_dc_i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.class = I2C_CLASS_DDC;
	i2c->adapter.algo = &gsgpu_dc_i2c_algo;
	i2c->adapter.dev.parent = adev->ddev->dev;
	i2c->adapter.nr = -1;
	i2c->adapter.retries = 5;
	i2c->adapter.timeout = msecs_to_jiffies(100);
	i2c->reg_base = adev->loongson_dc_rmmio + DC_I2C_ADDR + link_index*0x10;
	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name), "DC-I2C:%d", link_index);

	init_completion(&i2c->cmd_complete);

	/* use dc i2c */
	value = dc_readl(adev, CURRENT_REG(DC_HDMI_CTRL_REG, link_index));
	value |= (1 << 8);
	dc_writel(adev, CURRENT_REG(DC_HDMI_CTRL_REG, link_index), value);

	/* config PRER and interrupt */
	dc_writeb(dc_readb(DC_I2C_CTR_REG) & ~0x80, DC_I2C_CTR_REG);
	dc_writeb(0x2c, DC_I2C_PRER_LO_REG);
	dc_writeb(0x1, DC_I2C_PRER_HI_REG);
	dc_writeb(dc_readb(DC_I2C_CTR_REG) | 0xc0, DC_I2C_CTR_REG);

	/* enable i2c interrupt */
	value = dc_readl(adev, DC_INT_REG);
	value |= (1 << (link_index + 27));
	dc_writel(adev, DC_INT_REG, value);

	i2c_set_adapdata(&i2c->adapter, i2c);
	ret = i2c_add_adapter(&i2c->adapter);
	if (ret) {
		DRM_ERROR("Failed to register hw i2c %d\n", link_index);
		goto out_free;
	}

	ddc_client = i2c_new_device(&i2c->adapter, &ddc_info);
	if (IS_ERR(ddc_client)) {
		ret = PTR_ERR(ddc_client);
		DRM_ERROR("Failed to create standard ddc client\n");
		goto out_free;
	}

	i2c->ddc_client = ddc_client;
	adev->i2c[link_index] = i2c;

out_free:
	if (ret) {
		kfree(i2c);
		adev->i2c[link_index] = NULL;
	}

	return ret;
}

static int gsgpu_dc_gpio_init(struct gsgpu_device *adev,
			      int i2c_addr, uint32_t link_index)
{
	struct gsgpu_dc_i2c *i2c;
	struct i2c_client *ddc_client;
	struct i2c_algo_bit_data *i2c_algo_data;
	int ret = 0;
	int value = 0;
	const struct i2c_board_info ddc_info = {
		.type = "ddc-dev",
		.addr = i2c_addr,
		.flags = I2C_CLASS_DDC,
	};

	i2c = kzalloc(sizeof(struct gsgpu_dc_i2c), GFP_KERNEL);
	if (!i2c)
		return -ENOMEM;

	i2c_algo_data = kzalloc(sizeof(struct i2c_algo_bit_data), GFP_KERNEL);
	if (!i2c_algo_data) {
		ret = -ENOMEM;
		goto out_free;
	}

	i2c->adapter.owner = THIS_MODULE;
	i2c->adapter.class = I2C_CLASS_DDC;
	i2c->adapter.algo_data = i2c_algo_data;
	i2c->adapter.dev.parent = adev->ddev->dev;
	i2c->adapter.nr = -1;
	snprintf(i2c->adapter.name, sizeof(i2c->adapter.name),
		 "DC-GPIO-I2C:%d", link_index);

	i2c->data = link_index * 2;
	i2c->clock = link_index * 2 + 1;
	i2c_algo_data->setsda = gsgpu_gpio_set_data;
	i2c_algo_data->setscl = gsgpu_gpio_set_clock;
	i2c_algo_data->getsda = gsgpu_gpio_get_data;
	i2c_algo_data->getscl = gsgpu_gpio_get_clock;
	i2c_algo_data->udelay = DC_I2C_TON;
	i2c_algo_data->timeout = usecs_to_jiffies(2200);

	/* use dc gpio */
	value = dc_readl(adev, CURRENT_REG(DC_HDMI_CTRL_REG, link_index));
	value &= ~(1 << 8);
	dc_writel(adev, CURRENT_REG(DC_HDMI_CTRL_REG, link_index), value);

	ret = i2c_bit_add_numbered_bus(&i2c->adapter);
	if (ret)
		goto free_algo_data;

	i2c_algo_data->data = i2c;
	i2c_set_adapdata(&i2c->adapter, i2c);

	ddc_client = i2c_new_device(&i2c->adapter, &ddc_info);
	if (IS_ERR(ddc_client)) {
		ret = PTR_ERR(ddc_client);
		DRM_ERROR("Failed to create standard ddc client\n");
		goto free_algo_data;
	}

	i2c->ddc_client = ddc_client;
	i2c->adev = adev;
	adev->i2c[link_index] = i2c;

	return ret;

free_algo_data:
	kfree(i2c_algo_data);
out_free:
	if (ret) {
		kfree(i2c);
		adev->i2c[link_index] = NULL;
	}

	return ret;
}

int gsgpu_i2c_init(struct gsgpu_device *adev, uint32_t link_index)
{
	int ret;
	int i2c_addr;
	struct gsgpu_link_info *link_info = &adev->dc->link_info[link_index];

	i2c_addr = link_info->encoder->resource->chip_addr;
	if (i2c_addr == 0 || i2c_addr == 0xff)
		i2c_addr = DDC_ADDR;

	if (adev->dc_revision != 2 && adev->dc_revision != 0x10) {
		ret = gsgpu_dc_i2c_init(adev, i2c_addr, link_index);
		if (ret)
			return ret;
		DRM_INFO("GSGPU DC init i2c %d addr 0x%x finish\n",
			 link_index, i2c_addr);
	} else {
		ret = gsgpu_dc_gpio_init(adev, i2c_addr, link_index);
		if (ret)
			return ret;
		DRM_INFO("GSGPU DC init gpio %d addr 0x%x finish\n",
			 link_index, i2c_addr);
	}

	return 0;
}
