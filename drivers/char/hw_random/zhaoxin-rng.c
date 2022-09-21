/*
 * RNG driver for Zhaoxin RNGs
 */

#include <crypto/padlock.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/hw_random.h>
#include <linux/delay.h>
#include <asm/cpu_device_id.h>
#include <asm/io.h>
#include <asm/msr.h>
#include <asm/cpufeature.h>
#include <asm/fpu/api.h>




enum {
	ZHAOXIN_STRFILT_CNT_SHIFT	= 16,
	ZHAOXIN_STRFILT_FAIL	= (1 << 15),
	ZHAOXIN_STRFILT_ENABLE	= (1 << 14),
	ZHAOXIN_RAWBITS_ENABLE	= (1 << 13),
	ZHAOXIN_RNG_ENABLE		= (1 << 6),
	ZHAOXIN_NOISESRC1		= (1 << 8),
	ZHAOXIN_NOISESRC2		= (1 << 9),
	ZHAOXIN_XSTORE_CNT_MASK	= 0x0F,

	ZHAOXIN_RNG_CHUNK_8		= 0x00,	/* 64 rand bits, 64 stored bits */
	ZHAOXIN_RNG_CHUNK_4		= 0x01,	/* 32 rand bits, 32 stored bits */
	ZHAOXIN_RNG_CHUNK_4_MASK	= 0xFFFFFFFF,
	ZHAOXIN_RNG_CHUNK_2		= 0x02,	/* 16 rand bits, 32 stored bits */
	ZHAOXIN_RNG_CHUNK_2_MASK	= 0xFFFF,
	ZHAOXIN_RNG_CHUNK_1		= 0x03,	/* 8 rand bits, 32 stored bits */
	ZHAOXIN_RNG_CHUNK_1_MASK	= 0xFF,
};

/*
 * Investigate using the 'rep' prefix to obtain 32 bits of random data
 * in one insn.  The upside is potentially better performance.  The
 * downside is that the instruction becomes no longer atomic.  Due to
 * this, just like familiar issues with /dev/random itself, the worst
 * case of a 'rep xstore' could potentially pause a cpu for an
 * unreasonably long time.  In practice, this condition would likely
 * only occur when the hardware is failing.  (or so we hope :))
 *
 * Another possible performance boost may come from simply buffering
 * until we have 4 bytes, thus returning a u32 at a time,
 * instead of the current u8-at-a-time.
 *
 * Padlock instructions can generate a spurious DNA fault, but the
 * kernel doesn't use CR0.TS, so this doesn't matter.
 */

static inline u32 xstore(u32 *addr, u32 edx_in)
{
	u32 eax_out;

	asm(".byte 0x0F,0xA7,0xC0 /* xstore %%edi (addr=%0) */"
		: "=m" (*addr), "=a" (eax_out), "+d" (edx_in), "+D" (addr));

	return eax_out;
}

static int zhaoxin_rng_data_present(struct hwrng *rng, int wait)
{
	char buf[16 + PADLOCK_ALIGNMENT - STACK_ALIGN] __attribute__
		((aligned(STACK_ALIGN)));
	u32 *zhaoxin_rng_datum = (u32 *)PTR_ALIGN(&buf[0], PADLOCK_ALIGNMENT);
	u32 bytes_out;
	int i;

	/* We choose the recommended 1-byte-per-instruction RNG rate,
	 * for greater randomness at the expense of speed.  Larger
	 * values 2, 4, or 8 bytes-per-instruction yield greater
	 * speed at lesser randomness.
	 *
	 * If you change this to another ZHAOXIN_CHUNK_n, you must also
	 * change the ->n_bytes values in rng_vendor_ops[] tables.
	 * ZHAOXIN_CHUNK_8 requires further code changes.
	 *
	 * A copy of MSR_ZHAOXIN_RNG is placed in eax_out when xstore
	 * completes.
	 */

	for (i = 0; i < 20; i++) {
		*zhaoxin_rng_datum = 0; /* paranoia, not really necessary */
		bytes_out = xstore(zhaoxin_rng_datum, ZHAOXIN_RNG_CHUNK_1);
		bytes_out &= ZHAOXIN_XSTORE_CNT_MASK;
		if (bytes_out || !wait)
			break;
		udelay(10);
	}
	rng->priv = *zhaoxin_rng_datum;
	return bytes_out ? 1 : 0;
}

static int zhaoxin_rng_data_read(struct hwrng *rng, u32 *data)
{
	u32 zhaoxin_rng_datum = (u32)rng->priv;

	*data = zhaoxin_rng_datum;

	return 1;
}

static int zhaoxin_rng_init(struct hwrng *rng)
{
	struct cpuinfo_x86 *c = &cpu_data(0);

	/* Zhaoxin CPUs don't have the MSR_ZHAOXIN_RNG anymore.  The RNG
	 * is always enabled if CPUID rng_en is set.  There is no
	 * RNG configuration like it used to be the case in this
	 * register */
	if (c->x86 > 6) {
		if (!boot_cpu_has(X86_FEATURE_XSTORE_EN)) {
			pr_err(PFX "can't enable hardware RNG "
				"if XSTORE is not enabled\n");
			return -ENODEV;
		}
		return 0;
	}
	return 0;
}


static struct hwrng zhaoxin_rng = {
	.name		= "zhaoxin",
	.init		= zhaoxin_rng_init,
	.data_present	= zhaoxin_rng_data_present,
	.data_read	= zhaoxin_rng_data_read,
};

static struct x86_cpu_id zhaoxin_rng_ids[] = {
	{ X86_VENDOR_CENTAUR, 7, X86_MODEL_ANY, X86_FEATURE_XSTORE },
	{ X86_VENDOR_ZHAOXIN, 7, X86_MODEL_ANY, X86_FEATURE_XSTORE },
	{}
};
MODULE_DEVICE_TABLE(x86cpu, zhaoxin_rng_ids);

static int __init mod_init(void)
{
	int err;

	if (!x86_match_cpu(zhaoxin_rng_ids))
		return -ENODEV;

	pr_info("RNG detected\n");
	err = hwrng_register(&zhaoxin_rng);
	if (err) {
		pr_err(PFX "RNG registering failed (%d)\n",
		       err);
		goto out;
	}
out:
	return err;
}
module_init(mod_init);

static void __exit mod_exit(void)
{
	hwrng_unregister(&zhaoxin_rng);
}
module_exit(mod_exit);

static struct x86_cpu_id __maybe_unused zhaoxin_rng_cpu_id[] = {
	X86_MATCH_FEATURE(X86_FEATURE_XSTORE, NULL),
	{}
};
MODULE_DEVICE_TABLE(x86cpu, zhaoxin_rng_cpu_id);

MODULE_DESCRIPTION("H/W RNG driver for Zhaoxin CPU with PadLock");
MODULE_LICENSE("GPL");
