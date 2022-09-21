#include <linux/kernel.h>
#include <linux/module.h>
#include "phytium_scto.h"
#include <linux/init.h>
#include <asm/io.h>
#include <linux/miscdevice.h>
#include <linux/err.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/acpi.h>
#include "sm2.h"
#include "sm3.h"
#include "sm4.h"
#include "trng.h"

#define ENABLE_INTERRUPT

static DEFINE_SPINLOCK(scto_lock);

static long scto_misc_ioctl(struct file *fp, unsigned int cmd, unsigned long arg)
{
        SM2_KEY st_sm2_key;
        SM2_ENCRYPT st_sm2_encrypt;
        SM2_DECRYPT st_sm2_decrypt;
        SM2_GETE    st_sm2_gete;
        SM2_GETZ    st_sm2_getz;
        SM2_SIGN    st_sm2_sign;
        SM2_VERIFY  st_sm2_verify;

        SM3_HASH_CONTEXT    st_sm3_init;
        SM3_HASH_PROCESS    st_sm3_process;
        SM3_HASH_DONE       st_sm3_done;
        SM3_HASH            st_sm3_hash;

        SM3_DMA_HASH_CONTEXT    st_sm3_dma_init;
        SM3_DMA_HASH_PROCESS    st_sm3_dma_process;
        SM3_DMA_HASH_DONE       st_sm3_dma_done;
        SM3_DMA_HASH            st_sm3_dma_hash;
        SM4_INIT                st_sm4_init;
        SM4_INIT                st_sm4_dma_init;
        SM4_CRYPTO              st_sm4_crypto;
        SM4_DMA_CRYPTO          st_sm4_dma_crypto;
        uint8_t ret;
        unsigned long flags;
        spin_lock_irqsave(&scto_lock, flags);
        switch(cmd)
	{
            case SCTO_SM2_KEYGET:{
                memset(&st_sm2_key,0, sizeof(st_sm2_encrypt));
                ret = sm2_keyget(st_sm2_key.priKey, st_sm2_key.pubKey);
                if( ret != 0 ){
                    printk("sm2_keyget failed with:%d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }
                if( copy_to_user((SM2_KEY *)arg, (SM2_KEY *)&st_sm2_key, sizeof(SM2_KEY)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }
                break;
            }
            case SCTO_SM2_ENCRYPT:{
                memset(&st_sm2_encrypt, 0, sizeof(st_sm2_encrypt));
                if( copy_from_user((SM2_ENCRYPT *)&st_sm2_encrypt, (SM2_ENCRYPT *)arg, sizeof(SM2_ENCRYPT))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }
                ret = sm2_encrypt(st_sm2_encrypt.M, st_sm2_encrypt.MByteLen, st_sm2_encrypt.pubKey,st_sm2_encrypt.order, st_sm2_encrypt.C, &st_sm2_encrypt.CByteLen);
                if( ret != 0 ){
                    printk("sm2_encrypt failed with:%d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }
                if( copy_to_user((SM2_ENCRYPT *)arg, &st_sm2_encrypt, sizeof(SM2_ENCRYPT)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }
                break;
            }
            case SCTO_SM2_DECRYPT:{
                memset(&st_sm2_decrypt, 0, sizeof(st_sm2_decrypt));
                if( copy_from_user((SM2_DECRYPT *)&st_sm2_decrypt, (SM2_DECRYPT *)arg, sizeof(SM2_DECRYPT))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }
                ret = sm2_decrypt(st_sm2_decrypt.C, st_sm2_decrypt.CByteLen, st_sm2_decrypt.priKey, st_sm2_decrypt.order, st_sm2_decrypt.M, &st_sm2_decrypt.MByteLen);
                if( ret != 0 ){
                    printk("sm2_decrypt failed with:%d\n",ret); 
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }
                if( copy_to_user((SM2_DECRYPT *)arg, &st_sm2_decrypt, sizeof(SM2_DECRYPT)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                  return -EFAULT;
                }
                break;
            }
            case SCTO_SM2_GETE:{
                memset(&st_sm2_gete, 0, sizeof(st_sm2_gete));
                if( copy_from_user((SM2_GETE *)&st_sm2_gete, (SM2_GETE *)arg, sizeof(SM2_GETE))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm2_getE(st_sm2_gete.M, st_sm2_gete.byteLen, st_sm2_gete.Z, st_sm2_gete.E);
                if( ret != 0 ){
                    printk("sm2_getE failed with:%d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM2_GETE *)arg, &st_sm2_gete, sizeof(SM2_GETE)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM2_GETZ:{
                memset(&st_sm2_getz, 0, sizeof(st_sm2_getz));
                if( copy_from_user((SM2_GETZ *)&st_sm2_getz, (SM2_GETZ *)arg, sizeof(SM2_GETZ))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm2_getZ(st_sm2_getz.ID, st_sm2_getz.byteLenofID, st_sm2_getz.pubKey, st_sm2_getz.Z);
                if( ret != 0 ){
                    printk("sm2_getZ failed with:%d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM2_GETZ *)arg, &st_sm2_getz, sizeof(SM2_GETZ)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM2_SIGN:{
                memset(&st_sm2_sign, 0, sizeof(st_sm2_sign));
                if( copy_from_user((SM2_SIGN *)&st_sm2_sign, (SM2_SIGN *)arg, sizeof(SM2_SIGN))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm2_sign(st_sm2_sign.E, st_sm2_sign.priKey, st_sm2_sign.signature);
                if( ret != 0 ){
                    printk("sm2_sign failed with:%d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM2_SIGN *)arg, &st_sm2_sign, sizeof(SM2_SIGN)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM2_VERIFY:{
                memset(&st_sm2_verify, 0, sizeof(st_sm2_verify));
                if( copy_from_user((SM2_VERIFY *)&st_sm2_verify, (SM2_VERIFY *)arg, sizeof(SM2_VERIFY))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm2_verify(st_sm2_verify.E, st_sm2_verify.pubKey, st_sm2_verify.signature);
                if( ret != 0 ){
                    printk("sm2_verify failed with:%d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM2_VERIFY *)arg, &st_sm2_verify, sizeof(SM2_VERIFY)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                     return -EFAULT;
                }
                break;
            }
            case SCTO_SM3_INIT:{
                memset(&st_sm3_init, 0, sizeof(st_sm3_init));
                if( copy_from_user((SM3_HASH_CONTEXT *)&st_sm3_init, (SM3_HASH_CONTEXT *)arg, sizeof(SM3_HASH_CONTEXT))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm3_init((hash_context_t *)&st_sm3_init);
                if( ret != 0 ){
                    printk("sm3_init failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM3_HASH_CONTEXT *)arg, &st_sm3_init, sizeof(SM3_HASH_CONTEXT)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM3_PROCESS:{
                memset(&st_sm3_process, 0, sizeof(st_sm3_process));
                if( copy_from_user((SM3_HASH_PROCESS *)&st_sm3_process, (SM3_HASH_PROCESS *)arg, sizeof(SM3_HASH_PROCESS))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm3_process((hash_context_t *)&st_sm3_process.hash_context, st_sm3_process.input, st_sm3_process.byteLen);
                if( ret != 0 ){
                    printk("sm3_process failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM3_HASH_PROCESS *)arg, &st_sm3_process, sizeof(SM3_HASH_PROCESS)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM3_DONE:{
                memset(&st_sm3_done, 0, sizeof(st_sm3_done));
                if( copy_from_user((SM3_HASH_DONE *)&st_sm3_done, (SM3_HASH_DONE *)arg, sizeof(SM3_HASH_DONE))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm3_done((hash_context_t *)&st_sm3_done.hash_context, st_sm3_done.digest);
                if( ret != 0 ){
                    printk("sm3_done failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM3_HASH_DONE *)arg, &st_sm3_done, sizeof(SM3_HASH_DONE)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM3_HASH:{
                memset(&st_sm3_hash, 0, sizeof(st_sm3_hash));
                if( copy_from_user((SM3_HASH*)&st_sm3_hash, (SM3_HASH *)arg, sizeof(SM3_HASH))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm3_hash(st_sm3_hash.message, st_sm3_hash.byteLen, st_sm3_hash.digest);
                if( ret != 0 ){
                    printk("sm3_hash failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM3_HASH *)arg, &st_sm3_hash, sizeof(SM3_HASH)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }
                break;
            }
            case SCTO_SM3_DMA_INIT:{
                memset(&st_sm3_dma_init, 0, sizeof(st_sm3_dma_init));
                if( copy_from_user((SM3_DMA_HASH_CONTEXT *)&st_sm3_dma_init, (SM3_DMA_HASH_CONTEXT *)arg, sizeof(SM3_DMA_HASH_CONTEXT))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm3_dma_init((hash_dma_context_t *)&st_sm3_dma_init);
                if( ret != 0 ){
                    printk("sm3_dma_init failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM3_DMA_HASH_CONTEXT *)arg, &st_sm3_dma_init, sizeof(SM3_DMA_HASH_CONTEXT)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM3_DMA_PROCESS:{
                memset(&st_sm3_dma_process, 0, sizeof(st_sm3_dma_process));
                if( copy_from_user((SM3_DMA_HASH_PROCESS *)&st_sm3_dma_process, (SM3_DMA_HASH_PROCESS *)arg, sizeof(SM3_DMA_HASH_PROCESS))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm3_dma_process((hash_dma_context_t *)&st_sm3_dma_process.dma_hash_context, st_sm3_dma_process.input, st_sm3_dma_process.wordLen, st_sm3_dma_process.output);
                if( ret != 0 ){
                    printk("sm3_dma_process failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM3_DMA_HASH_PROCESS *)arg, &st_sm3_dma_process, sizeof(SM3_DMA_HASH_PROCESS)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM3_DMA_DONE:{
                memset(&st_sm3_dma_done, 0, sizeof(st_sm3_dma_done));
                if( copy_from_user((SM3_DMA_HASH_DONE *)&st_sm3_dma_done, (SM3_DMA_HASH_DONE *)arg, sizeof(SM3_DMA_HASH_DONE))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm3_dma_last_process((hash_dma_context_t *)&st_sm3_dma_done.dma_hash_context, st_sm3_dma_done.input, st_sm3_dma_done.byteLen, st_sm3_dma_done.output);
                if( ret != 0 ){
                    printk("sm3_dma_last_process failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM3_DMA_HASH_DONE *)arg, &st_sm3_dma_done, sizeof(SM3_DMA_HASH_DONE)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM3_DMA_HASH:{
                memset(&st_sm3_dma_hash, 0, sizeof(st_sm3_dma_hash));
                if( copy_from_user((SM3_DMA_HASH*)&st_sm3_dma_hash, (SM3_DMA_HASH *)arg, sizeof(SM3_DMA_HASH))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                uint32_t *message = kmalloc(sizeof(st_sm3_dma_hash.message), GFP_KERNEL);
                memset(message, 0, sizeof(st_sm3_dma_hash.message));
                memcpy(message, st_sm3_dma_hash.message,sizeof(st_sm3_dma_hash.message)/sizeof(st_sm3_dma_hash.message[0]));

                uint32_t *digest = kmalloc(sizeof(st_sm3_dma_hash.digest), GFP_KERNEL);
                memset(digest, 0, sizeof(st_sm3_dma_hash.digest));
                ret = sm3_dma_hash((const uint32_t *)message, st_sm3_dma_hash.byteLen, digest);
                if( ret != 0 ){
                    printk("sm3_dma_hash failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                kfree(message);
                kfree(digest);

                if( copy_to_user((SM3_DMA_HASH *)arg, &st_sm3_dma_hash, sizeof(SM3_DMA_HASH)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM4_INIT:{
                memset(&st_sm4_init, 0, sizeof(st_sm4_init));
                if( copy_from_user((SM4_INIT*)&st_sm4_init, (SM4_INIT *)arg, sizeof(SM4_INIT))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm4_init(st_sm4_init.mode, st_sm4_init.crypto, st_sm4_init.key, st_sm4_init.iv);
                if( ret != 0 ){
                    printk("sm4_init failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM4_INIT *)arg, &st_sm4_init, sizeof(SM4_INIT)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM4_CRYPTO:{
                memset(&st_sm4_crypto, 0, sizeof(st_sm4_crypto));
                if( copy_from_user((SM4_CRYPTO*)&st_sm4_crypto, (SM4_CRYPTO *)arg, sizeof(SM4_CRYPTO))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm4_crypto(st_sm4_crypto.in, st_sm4_crypto.out, st_sm4_crypto.byteLen);
                if( ret != 0 ){
                    printk("sm4_crypto failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM4_CRYPTO *)arg, &st_sm4_crypto, sizeof(SM4_CRYPTO)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM4_DMA_INIT:{
                memset(&st_sm4_dma_init, 0, sizeof(st_sm4_dma_init));
                if( copy_from_user((SM4_INIT*)&st_sm4_dma_init, (SM4_INIT *)arg, sizeof(SM4_INIT))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm4_init(st_sm4_dma_init.mode, st_sm4_dma_init.crypto, st_sm4_dma_init.key, st_sm4_dma_init.iv);
                if( ret != 0 ){
                    printk("sm4_dma_init failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM4_INIT *)arg, &st_sm4_dma_init, sizeof(SM4_INIT)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            case SCTO_SM4_DMA_CRYPTO:{
                memset(&st_sm4_dma_crypto, 0, sizeof(st_sm4_dma_crypto));
                if( copy_from_user((SM4_DMA_CRYPTO*)&st_sm4_dma_crypto, (SM4_DMA_CRYPTO *)arg, sizeof(SM4_DMA_CRYPTO))){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                ret = sm4_dma_crypto(st_sm4_dma_crypto.in, st_sm4_dma_crypto.out, st_sm4_dma_crypto.byteLen);
                if( ret != 0 ){
                    printk("sm4_dma_crypto failed with %d\n",ret);
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return ret;
                }

                if( copy_to_user((SM4_DMA_CRYPTO *)arg, &st_sm4_dma_crypto, sizeof(SM4_DMA_CRYPTO)) ){
                    spin_unlock_irqrestore(&scto_lock, flags);
                    return -EFAULT;
                }

                break;
            }
            default:
		printk("unsupport test case\n");
                break;
	}
        spin_unlock_irqrestore(&scto_lock, flags);

        return 0;
}

static irqreturn_t scto_interrupt(int irq, void *dev_id)
{
    int ret = 1;
    int data = 0;
    unsigned long flags;
    spin_lock_irqsave(&scto_lock, flags);
    data = get_smx_irq_stat();
    if(data & 0x1 ){
        smx_irq_clear();
    }

    if( data & 0x2){
        smx_irq_clear();
    }

    if( data & 0x4 ){
        smx_irq_clear();
    }

    data = get_ske_irq_stat();
    if( data){
        ske_irq_clear();
    }

    data = get_hash_irq_stat();
    if( data){
        hash_irq_clear();
    }

    spin_unlock_irqrestore(&scto_lock, flags);

    return IRQ_RETVAL(ret);
}

static int scto_probe(struct platform_device *pdev)
{
    int irq;
    irq = platform_get_irq(pdev, 0);
    if(irq < 0 ){
        printk(KERN_ERR "scto probe get irq failed\n");
        return -1;
    }

    if( devm_request_irq(&pdev->dev, irq, scto_interrupt, IRQF_SHARED, "sctoManager", pdev) < 0){
        printk(KERN_ERR "scto probe devm_reuest_irq failed\n");
        return -1;
    }

    return 0;
}

static const struct file_operations scto_misc_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = scto_misc_ioctl,
};

static struct miscdevice scto_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = "phytium-scto",
	.fops = &scto_misc_fops,
};

static const struct acpi_device_id scto_acpi_match[] = {
    {"SCTO0001", 0},
    {}
};

static struct platform_driver scto_driver = {
    .driver = {
        .name = "scto_driver",
        .acpi_match_table = ACPI_PTR(scto_acpi_match),
    },
    .probe = scto_probe,
};

static int __init ft_scto_init(void)
{
	int ret;

#ifdef ENABLE_INTERRUPT
        init_ske_reg();
        init_smx_reg();
        trng_interrupt_init();
        enable_global_interrupt();
        ret = platform_driver_register(&scto_driver);
        if( ret ){
            printk(KERN_ERR "scto register failed with ret:0x%x\n",ret);
            goto Err0;
        }
#endif
        init_dma_config();
	ret = misc_register(&scto_misc_device);
	if( ret ){
		printk("unable to register scto driver\n");
		goto Err0;
	}

	return 0;
Err0:
        uninit_ske_reg();
        uninit_smx_reg();

        return ret;

}

static void __exit ft_scto_exit(void)
{
    platform_driver_unregister(&scto_driver);
}

module_init(ft_scto_init);
module_exit(ft_scto_exit);

MODULE_DESCRIPTION("phytium scto");
MODULE_VERSION("0.1");
MODULE_LICENSE("GPL");
