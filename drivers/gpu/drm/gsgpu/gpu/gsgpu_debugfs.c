#include <linux/kthread.h>
#include <drm/drmP.h>
#include <linux/debugfs.h>
#include "gsgpu.h"

/**
 * gsgpu_debugfs_add_files - Add simple debugfs entries
 *
 * @adev:  Device to attach debugfs entries to
 * @files:  Array of function callbacks that respond to reads
 * @nfiles: Number of callbacks to register
 *
 */
int gsgpu_debugfs_add_files(struct gsgpu_device *adev,
			     const struct drm_info_list *files,
			     unsigned nfiles)
{
	unsigned i;

	for (i = 0; i < adev->debugfs_count; i++) {
		if (adev->debugfs[i].files == files) {
			/* Already registered */
			return 0;
		}
	}

	i = adev->debugfs_count + 1;
	if (i > GSGPU_DEBUGFS_MAX_COMPONENTS) {
		DRM_ERROR("Reached maximum number of debugfs components.\n");
		DRM_ERROR("Report so we increase "
			  "GSGPU_DEBUGFS_MAX_COMPONENTS.\n");
		return -EINVAL;
	}
	adev->debugfs[adev->debugfs_count].files = files;
	adev->debugfs[adev->debugfs_count].num_files = nfiles;
	adev->debugfs_count = i;
#if defined(CONFIG_DEBUG_FS)
	drm_debugfs_create_files(files, nfiles,
				 adev->ddev->primary->debugfs_root,
				 adev->ddev->primary);
#endif
	return 0;
}

#if defined(CONFIG_DEBUG_FS)

/**
 * gsgpu_debugfs_regs_read - Callback for reading MMIO registers
 */
static ssize_t gsgpu_debugfs_regs_read(struct file *f, char __user *buf,
					size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = file_inode(f)->i_private;

	dev_err(adev->dev, "%s Not Implement\n", __func__);

	return -EINVAL;
}

/**
 * gsgpu_debugfs_regs_write - Callback for writing MMIO registers
 */
static ssize_t gsgpu_debugfs_regs_write(struct file *f, const char __user *buf,
					 size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = file_inode(f)->i_private;

	dev_err(adev->dev, "%s Not Implement\n", __func__);

	return -EINVAL;
}

/**
 * gsgpu_debugfs_gca_config_read - Read from gfx config data
 *
 * @f: open file handle
 * @buf: User buffer to store read data in
 * @size: Number of bytes to read
 * @pos:  Offset to seek to
 *
 * This file is used to access configuration data in a somewhat
 * stable fashion.  The format is a series of DWORDs with the first
 * indicating which revision it is.  New content is appended to the
 * end so that older software can still read the data.
 */

static ssize_t gsgpu_debugfs_gca_config_read(struct file *f, char __user *buf,
					size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = file_inode(f)->i_private;
	ssize_t result = 0;
	int r;
	uint32_t *config, no_regs = 0;

	if (size & 0x3 || *pos & 0x3)
		return -EINVAL;

	config = kmalloc_array(256, sizeof(*config), GFP_KERNEL);
	if (!config)
		return -ENOMEM;

	/* version, increment each time something is added */
	config[no_regs++] = 3;
	config[no_regs++] = adev->gfx.config.max_shader_engines;
	config[no_regs++] = adev->gfx.config.max_tile_pipes;
	config[no_regs++] = adev->gfx.config.max_cu_per_sh;
	config[no_regs++] = adev->gfx.config.max_sh_per_se;
	config[no_regs++] = adev->gfx.config.max_backends_per_se;
	config[no_regs++] = adev->gfx.config.max_texture_channel_caches;
	config[no_regs++] = adev->gfx.config.max_gprs;
	config[no_regs++] = adev->gfx.config.max_gs_threads;
	config[no_regs++] = adev->gfx.config.max_hw_contexts;
	config[no_regs++] = adev->gfx.config.sc_prim_fifo_size_frontend;
	config[no_regs++] = adev->gfx.config.sc_prim_fifo_size_backend;
	config[no_regs++] = adev->gfx.config.sc_hiz_tile_fifo_size;
	config[no_regs++] = adev->gfx.config.sc_earlyz_tile_fifo_size;
	config[no_regs++] = adev->gfx.config.num_tile_pipes;
	config[no_regs++] = adev->gfx.config.backend_enable_mask;
	config[no_regs++] = adev->gfx.config.mem_max_burst_length_bytes;
	config[no_regs++] = adev->gfx.config.mem_row_size_in_kb;
	config[no_regs++] = adev->gfx.config.shader_engine_tile_size;
	config[no_regs++] = adev->gfx.config.num_gpus;
	config[no_regs++] = adev->gfx.config.multi_gpu_tile_size;
	config[no_regs++] = adev->gfx.config.mc_arb_ramcfg;
	config[no_regs++] = adev->gfx.config.gb_addr_config;
	config[no_regs++] = adev->gfx.config.num_rbs;

	/* rev==2 */
	config[no_regs++] = adev->family;

	/* rev==3 */
	config[no_regs++] = adev->pdev->device;
	config[no_regs++] = adev->pdev->revision;
	config[no_regs++] = adev->pdev->subsystem_device;
	config[no_regs++] = adev->pdev->subsystem_vendor;

	while (size && (*pos < no_regs * 4)) {
		uint32_t value;

		value = config[*pos >> 2];
		r = put_user(value, (uint32_t *)buf);
		if (r) {
			kfree(config);
			return r;
		}

		result += 4;
		buf += 4;
		*pos += 4;
		size -= 4;
	}

	kfree(config);
	return result;
}

/**
 * gsgpu_debugfs_sensor_read - Read from the powerplay sensors
 *
 * @f: open file handle
 * @buf: User buffer to store read data in
 * @size: Number of bytes to read
 * @pos:  Offset to seek to
 */
static ssize_t gsgpu_debugfs_sensor_read(struct file *f, char __user *buf,
					size_t size, loff_t *pos)
{
	return -EINVAL;
}

/** gsgpu_debugfs_wave_read - Read WAVE STATUS data
 *
 * @f: open file handle
 * @buf: User buffer to store read data in
 * @size: Number of bytes to read
 * @pos:  Offset to seek to
 *
 * The offset being sought changes which wave that the status data
 * will be returned for.  The bits are used as follows:
 *
 * Bits 0..6: 	Byte offset into data
 * Bits 7..14:	SE selector
 * Bits 15..22:	SH/SA selector
 * Bits 23..30: CU/{WGP+SIMD} selector
 * Bits 31..36: WAVE ID selector
 * Bits 37..44: SIMD ID selector
 *
 * The returned data begins with one DWORD of version information
 * Followed by WAVE STATUS registers relevant to the GFX IP version
 * being used.  See gfx_read_wave_data() for an example output.
 */
static ssize_t gsgpu_debugfs_wave_read(struct file *f, char __user *buf,
					size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = f->f_inode->i_private;
	int r, x;
	ssize_t result = 0;
	uint32_t offset, se, sh, cu, wave, simd, data[32];

	if (size & 3 || *pos & 3)
		return -EINVAL;

	/* decode offset */
	offset = (*pos & GENMASK_ULL(6, 0));
	se = (*pos & GENMASK_ULL(14, 7)) >> 7;
	sh = (*pos & GENMASK_ULL(22, 15)) >> 15;
	cu = (*pos & GENMASK_ULL(30, 23)) >> 23;
	wave = (*pos & GENMASK_ULL(36, 31)) >> 31;
	simd = (*pos & GENMASK_ULL(44, 37)) >> 37;

	/* switch to the specific se/sh/cu */
	mutex_lock(&adev->grbm_idx_mutex);

	x = 0;
	if (adev->gfx.funcs->read_wave_data)
		adev->gfx.funcs->read_wave_data(adev, simd, wave, data, &x);

	mutex_unlock(&adev->grbm_idx_mutex);

	if (!x)
		return -EINVAL;

	while (size && (offset < x * 4)) {
		uint32_t value;

		value = data[offset >> 2];
		r = put_user(value, (uint32_t *)buf);
		if (r)
			return r;

		result += 4;
		buf += 4;
		offset += 4;
		size -= 4;
	}

	return result;
}

/** gsgpu_debugfs_gpr_read - Read wave gprs
 *
 * @f: open file handle
 * @buf: User buffer to store read data in
 * @size: Number of bytes to read
 * @pos:  Offset to seek to
 *
 * The offset being sought changes which wave that the status data
 * will be returned for.  The bits are used as follows:
 *
 * Bits 0..11:	Byte offset into data
 * Bits 12..19:	SE selector
 * Bits 20..27:	SH/SA selector
 * Bits 28..35: CU/{WGP+SIMD} selector
 * Bits 36..43: WAVE ID selector
 * Bits 37..44: SIMD ID selector
 * Bits 52..59: Thread selector
 * Bits 60..61: Bank selector (VGPR=0,SGPR=1)
 *
 * The return data comes from the SGPR or VGPR register bank for
 * the selected operational unit.
 */
static ssize_t gsgpu_debugfs_gpr_read(struct file *f, char __user *buf,
					size_t size, loff_t *pos)
{
	struct gsgpu_device *adev = f->f_inode->i_private;
	int r;
	ssize_t result = 0;
	uint32_t offset, se, sh, cu, wave, simd, thread, bank, *data;

	if (size & 3 || *pos & 3)
		return -EINVAL;

	/* decode offset */
	offset = *pos & GENMASK_ULL(11, 0);
	se = (*pos & GENMASK_ULL(19, 12)) >> 12;
	sh = (*pos & GENMASK_ULL(27, 20)) >> 20;
	cu = (*pos & GENMASK_ULL(35, 28)) >> 28;
	wave = (*pos & GENMASK_ULL(43, 36)) >> 36;
	simd = (*pos & GENMASK_ULL(51, 44)) >> 44;
	thread = (*pos & GENMASK_ULL(59, 52)) >> 52;
	bank = (*pos & GENMASK_ULL(61, 60)) >> 60;

	data = kcalloc(1024, sizeof(*data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	/* switch to the specific se/sh/cu */
	mutex_lock(&adev->grbm_idx_mutex);

	if (bank == 0) {
		if (adev->gfx.funcs->read_wave_vgprs)
			adev->gfx.funcs->read_wave_vgprs(adev, simd, wave, thread, offset, size>>2, data);
	} else {
		if (adev->gfx.funcs->read_wave_sgprs)
			adev->gfx.funcs->read_wave_sgprs(adev, simd, wave, offset, size>>2, data);
	}

	mutex_unlock(&adev->grbm_idx_mutex);

	while (size) {
		uint32_t value;

		value = data[offset++];
		r = put_user(value, (uint32_t *)buf);
		if (r) {
			result = r;
			goto err;
		}

		result += 4;
		buf += 4;
		size -= 4;
	}

err:
	kfree(data);
	return result;
}

static const struct file_operations gsgpu_debugfs_regs_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_debugfs_regs_read,
	.write = gsgpu_debugfs_regs_write,
	.llseek = default_llseek
};

static const struct file_operations gsgpu_debugfs_gca_config_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_debugfs_gca_config_read,
	.llseek = default_llseek
};

static const struct file_operations gsgpu_debugfs_sensors_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_debugfs_sensor_read,
	.llseek = default_llseek
};

static const struct file_operations gsgpu_debugfs_wave_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_debugfs_wave_read,
	.llseek = default_llseek
};
static const struct file_operations gsgpu_debugfs_gpr_fops = {
	.owner = THIS_MODULE,
	.read = gsgpu_debugfs_gpr_read,
	.llseek = default_llseek
};

static const struct file_operations *debugfs_regs[] = {
	&gsgpu_debugfs_regs_fops,
	&gsgpu_debugfs_gca_config_fops,
	&gsgpu_debugfs_sensors_fops,
	&gsgpu_debugfs_wave_fops,
	&gsgpu_debugfs_gpr_fops,
};

static const char *debugfs_regs_names[] = {
	"gsgpu_regs",
	"gsgpu_gca_config",
	"gsgpu_sensors",
	"gsgpu_wave",
	"gsgpu_gpr",
};

/**
 * gsgpu_debugfs_regs_init -	Initialize debugfs entries that provide
 * 								register access.
 *
 * @adev: The device to attach the debugfs entries to
 */
int gsgpu_debugfs_regs_init(struct gsgpu_device *adev)
{
	struct drm_minor *minor = adev->ddev->primary;
	struct dentry *ent, *root = minor->debugfs_root;
	unsigned i, j;

	for (i = 0; i < ARRAY_SIZE(debugfs_regs); i++) {
		ent = debugfs_create_file(debugfs_regs_names[i],
					  S_IFREG | S_IRUGO, root,
					  adev, debugfs_regs[i]);
		if (IS_ERR(ent)) {
			for (j = 0; j < i; j++) {
				debugfs_remove(adev->debugfs_regs[i]);
				adev->debugfs_regs[i] = NULL;
			}
			return PTR_ERR(ent);
		}

		if (!i)
			i_size_write(ent->d_inode, adev->rmmio_size);
		adev->debugfs_regs[i] = ent;
	}

	return 0;
}

void gsgpu_debugfs_regs_cleanup(struct gsgpu_device *adev)
{
	unsigned i;

	for (i = 0; i < ARRAY_SIZE(debugfs_regs); i++) {
		if (adev->debugfs_regs[i]) {
			debugfs_remove(adev->debugfs_regs[i]);
			adev->debugfs_regs[i] = NULL;
		}
	}
}

static int gsgpu_debugfs_test_ib(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;
	int r = 0, i;

	/* hold on the scheduler */
	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		struct gsgpu_ring *ring = adev->rings[i];

		if (!ring || !ring->sched.thread)
			continue;
		kthread_park(ring->sched.thread);
	}

	seq_printf(m, "run ib test:\n");
	r = gsgpu_ib_ring_tests(adev);
	if (r)
		seq_printf(m, "ib ring tests failed (%d).\n", r);
	else
		seq_printf(m, "ib ring tests passed.\n");

	/* go on the scheduler */
	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		struct gsgpu_ring *ring = adev->rings[i];

		if (!ring || !ring->sched.thread)
			continue;
		kthread_unpark(ring->sched.thread);
	}

	return 0;
}

static int gsgpu_debugfs_test_ring(struct seq_file *m, void *data)
{

	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;
	struct gsgpu_ring *ring;
	int r = 0, i;

	/* hold on the scheduler */
	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		ring = adev->rings[i];

		if (!ring || !ring->sched.thread)
			continue;
		kthread_park(ring->sched.thread);
	}

	seq_printf(m, "run ring test:\n");
	ring = &adev->gfx.gfx_ring[0];
	r = gsgpu_ring_test_ring(ring);
	if (r)
		seq_printf(m, "%s ring tests failed (%d).\n", ring->name, r);
	else
		seq_printf(m, "%s ring tests passed.\n", ring->name);


	for (i = 0; i < adev->xdma.num_instances; i++) {
		ring = &adev->xdma.instance[i].ring;
		r = gsgpu_ring_test_ring(ring);
		if (r)
			seq_printf(m, "%s ring tests failed (%d).\n", ring->name, r);
		else
			seq_printf(m, "%s ring tests passed.\n", ring->name);
	}

	/* go on the scheduler */
	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		struct gsgpu_ring *ring = adev->rings[i];

		if (!ring || !ring->sched.thread)
			continue;
		kthread_unpark(ring->sched.thread);
	}

	return 0;
}

static int gsgpu_debugfs_test_xdma(struct seq_file *m, void *data)
{

	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;
	struct gsgpu_ring *ring;
	int r = 0, i;

	/* hold on the scheduler */
	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		ring = adev->rings[i];

		if (!ring || !ring->sched.thread)
			continue;
		kthread_park(ring->sched.thread);
	}

	seq_printf(m, "run xdma test:\n");
	for (i = 0; i < adev->xdma.num_instances; i++) {
		ring = &adev->xdma.instance[i].ring;
		r = gsgpu_ring_test_xdma(ring, msecs_to_jiffies(5000));
		if (r)
			seq_printf(m, "%s ring tests failed (%d).\n", ring->name, r);
		else
			seq_printf(m, "%s ring tests passed.\n", ring->name);
	}

	/* go on the scheduler */
	for (i = 0; i < GSGPU_MAX_RINGS; i++) {
		struct gsgpu_ring *ring = adev->rings[i];

		if (!ring || !ring->sched.thread)
			continue;
		kthread_unpark(ring->sched.thread);
	}

	return 0;
}

static int gsgpu_debugfs_get_vbios_dump(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *) m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;

	seq_write(m, adev->bios, adev->bios_size);
	return 0;
}

static int gsgpu_debugfs_evict_vram(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;

	seq_printf(m, "(%d)\n", gsgpu_bo_evict_vram(adev));
	return 0;
}

static int gsgpu_debugfs_evict_gtt(struct seq_file *m, void *data)
{
	struct drm_info_node *node = (struct drm_info_node *)m->private;
	struct drm_device *dev = node->minor->dev;
	struct gsgpu_device *adev = dev->dev_private;

	seq_printf(m, "(%d)\n", ttm_bo_evict_mm(&adev->mman.bdev, TTM_PL_TT));
	return 0;
}

static const struct drm_info_list gsgpu_debugfs_list[] = {
	{"gsgpu_vbios", gsgpu_debugfs_get_vbios_dump},
	{"gsgpu_test_ib", &gsgpu_debugfs_test_ib},
	{"gsgpu_test_ring", &gsgpu_debugfs_test_ring},
	{"gsgpu_test_xdma", &gsgpu_debugfs_test_xdma},
	{"gsgpu_evict_vram", &gsgpu_debugfs_evict_vram},
	{"gsgpu_evict_gtt", &gsgpu_debugfs_evict_gtt},
};

int gsgpu_debugfs_init(struct gsgpu_device *adev)
{
	return gsgpu_debugfs_add_files(adev, gsgpu_debugfs_list,
					ARRAY_SIZE(gsgpu_debugfs_list));
}

#else
int gsgpu_debugfs_init(struct gsgpu_device *adev)
{
	return 0;
}
int gsgpu_debugfs_regs_init(struct gsgpu_device *adev)
{
	return 0;
}
void gsgpu_debugfs_regs_cleanup(struct gsgpu_device *adev) { }
#endif
