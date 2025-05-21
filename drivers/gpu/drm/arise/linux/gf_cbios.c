/*
 * Copyright © 2021 Glenfly Tech Co., Ltd.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */
#include "gf_disp.h"
#include "gf_vip.h"
#include "gf_wb.h"
#include "gf_cbios.h"
#include "gf_drmfb.h"
#include "gf_modifies.h"
#include "gf_trace.h"

/* cbios call back functions */
void disp_dbg_print(unsigned int debug_level, char *debug_message)
{
    gf_cb_printk(debug_message);
}

void disp_delay_micro_seconds(unsigned int usecs)
{
    if(usecs <= 20)
    {
        gf_udelay(usecs);
    }
    else if(usecs < 20000)
    {
        gf_usleep_range(usecs, usecs + (usecs >> 3));
    }
    else
    {
        gf_msleep(usecs/1000 + 1);
    }
}

unsigned char disp_read_uchar(void *data, unsigned int port)
{
    disp_info_t *disp_info = data;
    adapter_info_t* adapter = disp_info->adp_info;

    return gf_read8(adapter->mmio + port);
}

unsigned short disp_read_ushort(void *data, unsigned int port)
{
    disp_info_t *disp_info = data;
    adapter_info_t *adapter = disp_info->adp_info;

    return gf_read16(adapter->mmio + port);
}

unsigned int disp_read_ulong(void *data, unsigned int port)
{
    disp_info_t *disp_info = data;
    adapter_info_t *adapter = disp_info->adp_info;

    return gf_read32(adapter->mmio + port);
}

void disp_write_uchar(void *data, unsigned int port, unsigned char value)
{
    disp_info_t *disp_info = data;
    adapter_info_t *adapter = disp_info->adp_info;

    gf_write8(adapter->mmio + port, value);
}

void disp_write_ushort(void *data, unsigned int port, unsigned short value)
{
    disp_info_t *disp_info = data;
    adapter_info_t *adapter = disp_info->adp_info;

    gf_write16(adapter->mmio + port, value);
}

void disp_write_ulong(void *data, unsigned int port, unsigned int value)
{
    disp_info_t *disp_info = data;
    adapter_info_t *adapter = disp_info->adp_info;

    gf_write32(adapter->mmio + port, value);
}

void* disp_alloc_memory(unsigned int size)
{
    return gf_calloc(size);
}

void disp_free_pool(void *pool)
{
    gf_free(pool);
}

unsigned long long disp_acquire_spin_lock(void *spin_lock)
{
    struct os_spinlock *spin = (struct os_spinlock *)spin_lock;

    if (spin_lock == NULL)
    {
        gf_error("In func %s :first param spin_lock is NULL.\n", GF_FUNC_NAME(__func__));

        return 0;
    }

    return (unsigned long long)gf_spin_lock_irqsave(spin);
}

void disp_release_spin_lock(void *spin_lock, unsigned long long irq_status)
{
    struct os_spinlock *spin = (struct os_spinlock *)spin_lock;

    if (spin_lock == NULL)
    {
        gf_error("In func %s :first param spin_lock is NULL.\n", GF_FUNC_NAME(__func__));

        return;
    }

    gf_spin_unlock_irqrestore(spin, (unsigned long)irq_status);
}

void disp_acquire_mutex(void *mutex)
{
    struct os_mutex *m = (struct os_mutex *)mutex;

    if (m == NULL)
    {
        gf_error("In func %s :first param mutex is NULL.\n", GF_FUNC_NAME(__func__));

        return;
    }

    gf_mutex_lock(m);
}

void disp_release_mutex(void *mutex)
{
    struct os_mutex *m = (struct os_mutex *)mutex;

    if (m == NULL)
    {
        gf_error("In func %s :first param mutex is NULL.\n", GF_FUNC_NAME(__func__));

        return;
    }

    gf_mutex_unlock(m);
}

unsigned char disp_read_port_uchar(unsigned char *port)
{
    return gf_inb((unsigned short)(long)port);
}

void disp_write_port_uchar(unsigned char *port, unsigned char value)
{
    return gf_outb((unsigned short)(long)port, value);
}

void disp_write_log_file(unsigned int dbgleverl, unsigned char * dbgmsg, void * buffer, unsigned int size)
{
    //util_dump_memory_to_file(buffer, size, dbgmsg, CBIOS_LOG_FILE);
}

unsigned int disp_get_platform_config(void *data, const char* config_name, int *buffer, int length)
{
    disp_info_t *disp_info = data;
    gf_card_t*  gf_card = disp_info->gf_card;
    unsigned int ret = FALSE;

    if (!gf_get_platform_config(gf_card->pdev, config_name, buffer, length))
    {
        ret = TRUE;
    }
    else
    {
        ret = FALSE;
    }

    return ret;
}

void disp_query_sys_time(unsigned long long *u64_time)
{
    unsigned long sec = 0, usec = 0;

    if(u64_time == NULL)
    {
        return ;
    }

    gf_getsecs(&sec, &usec);

    *u64_time = (unsigned long long)sec * 1000000 + usec;
}

int  disp_get_output_num(int  outputs)
{
    int  num = 0;
    while(outputs)
    {
        if(outputs & 1)
        {
            num++;
        }
        outputs >>= 1;
    }
    return  num;
}

int  disp_biosmonitor_to_output(int bios_monitor)
{
    int  output = 0;
    if(bios_monitor & CBIOS_MONITOR_TYPE_CRT)
    {
        output |= UT_OUTPUT_TYPE_CRT;
    }
    if(bios_monitor & CBIOS_MONITOR_TYPE_TV)
    {
        output |= UT_OUTPUT_TYPE_TV;
    }
    if(bios_monitor & CBIOS_MONITOR_TYPE_HDTV)
    {
        output |= UT_OUTPUT_TYPE_HDTV;
    }
    if(bios_monitor & CBIOS_MONITOR_TYPE_PANEL)
    {
        output |= UT_OUTPUT_TYPE_PANEL;
    }
    if(bios_monitor & CBIOS_MONITOR_TYPE_DVI)
    {
        output |= UT_OUTPUT_TYPE_DVI;
    }
    if(bios_monitor & CBIOS_MONITOR_TYPE_HDMI)
    {
        output |= UT_OUTPUT_TYPE_HDMI;
    }
    if(bios_monitor & CBIOS_MONITOR_TYPE_DP)
    {
        output |= UT_OUTPUT_TYPE_DP;
    }
    if(bios_monitor & CBIOS_MONITOR_TYPE_MHL)
    {
        output |= UT_OUTPUT_TYPE_MHL;
    }

    return output;
}

int  disp_output_to_biosmonitor(int output)
{
    int  bios_monitor = 0;
    if(output & UT_OUTPUT_TYPE_CRT)
    {
        bios_monitor |= CBIOS_MONITOR_TYPE_CRT;
    }
    if(output & UT_OUTPUT_TYPE_TV)
    {
        bios_monitor |= CBIOS_MONITOR_TYPE_TV;
    }
    if(output & UT_OUTPUT_TYPE_HDTV)
    {
        bios_monitor |= CBIOS_MONITOR_TYPE_HDTV;
    }
    if(output & UT_OUTPUT_TYPE_PANEL)
    {
        bios_monitor |= CBIOS_MONITOR_TYPE_PANEL;
    }
    if(output & UT_OUTPUT_TYPE_DVI)
    {
        bios_monitor |= CBIOS_MONITOR_TYPE_DVI;
    }
    if(output & UT_OUTPUT_TYPE_HDMI)
    {
        bios_monitor |= CBIOS_MONITOR_TYPE_HDMI;
    }
    if(output & UT_OUTPUT_TYPE_DP)
    {
        bios_monitor |= CBIOS_MONITOR_TYPE_DP;
    }
    if(output & UT_OUTPUT_TYPE_MHL)
    {
        bios_monitor |= CBIOS_MONITOR_TYPE_MHL;
    }

    return bios_monitor;
}

CHIPID_HW disp_chipid_to_cbios_chipid(unsigned int chip_id)
{
     CHIPID_HW cb_chip_id = 0;

     switch(chip_id)
     {
     case CHIP_ARISE:
        cb_chip_id = CHIPID_E3K;
         break;
     case CHIP_ARISE1020:
        cb_chip_id = CHIPID_ARISE1020;
         break;
     case CHIP_ARISE1040:
        cb_chip_id = CHIPID_ARISE1040;
         break;
     case CHIP_ARISE1010:
        cb_chip_id = CHIPID_ARISE1010;
         break;
     case CHIP_ARISE10C0T:
        cb_chip_id = CHIPID_ARISE10C0T;
         break;
     case CHIP_ARISE2030:
        cb_chip_id = CHIPID_ARISE2030;
         break;
     case CHIP_ARISE2020:
        cb_chip_id = CHIPID_ARISE2020;
         break;
     default:
         gf_assert(0, "Invalid chip id.");
         break;
     }

     return cb_chip_id;
}

int disp_get_shadow_rom_image(disp_info_t *disp_info)
{
    void *shadow_rom_image = NULL;
    void *src_image = NULL;
    unsigned char cr_a0_c, sr_1f;
    unsigned int mm850c = 0, vid_boundary = 0, bound_reg = 0;
    adapter_info_t*  adapter_info = disp_info->adp_info;
    gf_map_argu_t    map  = {0};
    gf_vm_area_t     *vma = NULL;

    if((adapter_info->run_on_qt))
    {
        return 0;
    }

    vid_boundary = ((adapter_info->fb_total_size >> 24) -1 ) & 0xFF;
    bound_reg = gf_read32(adapter_info->mmio + 0x490a0);
    gf_write32(adapter_info->mmio + 0x490a0, (bound_reg & 0xFFFFFF00) | vid_boundary);

    shadow_rom_image = gf_calloc(GF_SHADOW_VBIOS_SIZE);
    if(!shadow_rom_image)
    {
        gf_error("malloc shadow rom memory failed\n");
        goto fail;
    }

    map.flags.cache_type = GF_MEM_UNCACHED;
    map.flags.mem_space  = GF_MEM_KERNEL;
    map.flags.mem_type   = GF_SYSTEM_IO;
    map.phys_addr        = adapter_info->fb_bus_addr + adapter_info->fb_total_size - GF_SHADOW_VBIOS_SIZE;
    map.size             = GF_SHADOW_VBIOS_SIZE;

    vma = gf_map_io_memory(NULL, &map);

    src_image = vma->virt_addr;
    if(!src_image)
    {
        gf_error("map shadow vbios failed\n");
        goto fail;
    }

    /* enable eclk */
    sr_1f = gf_read8(adapter_info->mmio + 0x861F);
    gf_write8(adapter_info->mmio + 0x861F, sr_1f & 0xfe);

    /* enable linear address */
    cr_a0_c = gf_read8(adapter_info->mmio + 0x8AA0);
    gf_write8(adapter_info->mmio + 0x8AA0, cr_a0_c | 0x10);

    mm850c = gf_read32(adapter_info->mmio + 0x850c);
    if(!(mm850c & 0x2))
    {
        gf_write8(adapter_info->mmio + 0x850c, mm850c | 0x2);
    }

    gf_memcpy(shadow_rom_image, src_image, GF_SHADOW_VBIOS_SIZE);

    gf_unmap_io_memory(vma);

    /* restore cr_a0_c */
    gf_write8(adapter_info->mmio + 0x8AA0, cr_a0_c);
    gf_write32(adapter_info->mmio + 0x850c, mm850c);

    if(*(unsigned short*)shadow_rom_image != 0xAA55)
    {
        gf_error("Invalid shadow rom_image \n");
        goto fail;
    }

    disp_info->rom_image = shadow_rom_image;

    return  GF_SHADOW_VBIOS_SIZE;

fail:
    if(shadow_rom_image)
    {
        gf_free(shadow_rom_image);
        shadow_rom_image = NULL;
    }

    return  0;
}

unsigned int disp_get_rom_bar_size(disp_info_t *disp_info)
{
    gf_card_t        *gf_card = disp_info->gf_card;
    unsigned int rom_save = 0, temp = 0;

    gf_get_rom_save_addr(gf_card->pdev, &rom_save);

    temp = 0xFFFFFFFF;
    gf_write_rom_save_addr(gf_card->pdev, temp);

    gf_get_rom_save_addr(gf_card->pdev, &temp);
    temp &= 0xFFFFFFFE;
    temp = ~temp;
    temp = temp + 1;

    gf_write_rom_save_addr(gf_card->pdev, rom_save);

    return temp;
}

unsigned int disp_get_rom_bar_image(disp_info_t *disp_info)
{
    gf_card_t        *gf_card = disp_info->gf_card;
    adapter_info_t   *adapter_info = disp_info->adp_info;
    void             *rom_bar_image = NULL;
    unsigned int     rom_len = 0, i;
    unsigned char    *pbyte = NULL;
    int              ret = -1;

    rom_bar_image = gf_calloc(GF_VBIOS_ROM_SIZE);
    if (!rom_bar_image)
    {
        gf_error("malloc rom image memory failed!\n");
        disp_info->rom_image = NULL;
        return 0;
    }

    ret = gf_pci_get_rom(gf_card->pdev, rom_bar_image, GF_VBIOS_ROM_SIZE);

    if (ret == 0)
    {
        rom_len = GF_VBIOS_ROM_SIZE;
    }
    else
    {
        gf_info("invalid rom head(0x%x)=0x%x.\n", rom_bar_image, *(unsigned short*)rom_bar_image);
    }

    if (!rom_len)
    {
        // if can't get rom bar image, try to get image from pmp
        if (0xaa55 == gf_read16(adapter_info->mmio + GF_PMP_SHADOW_IMG_OFFSET))
        {
            gf_info("get shadow rom image from pmp. \n");
            pbyte = rom_bar_image;
            for (i = 0; i < GF_PMP_SHADOW_IMG_SIZE; i++)
            {
                *pbyte++ = gf_read8(adapter_info->mmio + GF_PMP_SHADOW_IMG_OFFSET + i);
            }

            rom_len = GF_PMP_SHADOW_IMG_SIZE;
        }
        else
        {
            gf_info("get rom image failed! \n");
            rom_len = disp_get_rom_bar_size(disp_info);
        }
    }

    disp_info->rom_image = rom_bar_image;
    return rom_len;
}

int disp_wait_for_vblank(disp_info_t* disp_info, int pipe, int timeout)
{
    unsigned long timeout_j = jiffies + msecs_to_jiffies(timeout) + 1;
    unsigned int ori_vblcnt = 0, curr_vblcnt = 0;
    int ret = 0;
    gf_get_counter_t   get_cnt = {0};

    get_cnt.crtc_index = pipe;
    get_cnt.vblk = &ori_vblcnt;

    disp_cbios_get_counter(disp_info, &get_cnt);

    curr_vblcnt = ori_vblcnt;
    get_cnt.vblk = &curr_vblcnt;

    while(curr_vblcnt == ori_vblcnt)
    {
        if(time_after(jiffies, timeout_j))
        {
            ret = -ETIMEDOUT;
            break;
        }
        if(drm_can_sleep())
        {
            gf_msleep(1);
        }
        else
        {
            gf_udelay(1000);
        }
        disp_cbios_get_counter(disp_info, &get_cnt);
    }

    return  ret;
}

/*CBIOS Initialization sequence:
 * 1) set Call back function
 * 2) Set Mmio Endian Mode
 * 3) CbiosInit
 * 4) CbiosInitHW
 * Before 2, we can't let CBIOS access MMIO as we don't set MMIO mode as we required
 * */
int disp_init_cbios(disp_info_t *disp_info)
{
    CBIOS_PARAM_INIT                 CBParamInit = {0};
    CBIOS_CALLBACK_FUNCTIONS fnCallBack = {0};
    CBIOS_CHIP_ID CBChipId;
    adapter_info_t*  adapter_info = disp_info->adp_info;
    void *pcbe = NULL;
    unsigned int  rom_lenth = 0;
    unsigned int CBiosStatus;
    unsigned int CBiosExtensionSize;
    unsigned int i = 0;

    fnCallBack.Size = sizeof(CBIOS_CALLBACK_FUNCTIONS);

    fnCallBack.pFnDelayMicroSeconds = disp_delay_micro_seconds;
    fnCallBack.pFnReadUchar         = disp_read_uchar;
    fnCallBack.pFnReadUshort        = disp_read_ushort;
    fnCallBack.pFnReadUlong         = disp_read_ulong;
    fnCallBack.pFnWriteUchar        = disp_write_uchar;
    fnCallBack.pFnWriteUshort       = disp_write_ushort;
    fnCallBack.pFnWriteUlong        = disp_write_ulong;
    fnCallBack.pFnQuerySystemTime   = disp_query_sys_time;
    fnCallBack.pFnAllocateNonpagedMemory = disp_alloc_memory;
    fnCallBack.pFnAllocatePagedMemory= disp_alloc_memory;
    fnCallBack.pFnFreePool          = disp_free_pool;
    fnCallBack.pFnAcquireSpinLock   = disp_acquire_spin_lock;
    fnCallBack.pFnReleaseSpinLock   = disp_release_spin_lock;
    fnCallBack.pFnAcquireMutex      = disp_acquire_mutex;
    fnCallBack.pFnReleaseMutex      = disp_release_mutex;
    fnCallBack.pFnReadPortUchar     = disp_read_port_uchar;
    fnCallBack.pFnWritePortUchar    = disp_write_port_uchar;
    fnCallBack.pFnDbgPrintToFile    = disp_write_log_file;
    fnCallBack.pFnGetPlatformConfigU32 = disp_get_platform_config;

    fnCallBack.pFnStrcmp            = gf_strcmp;
    fnCallBack.pFnStrcpy            = gf_strcpy;
    fnCallBack.pFnStrncmp           = gf_strncmp;
    fnCallBack.pFnMemset            = gf_memset;
    fnCallBack.pFnMemcpy            = gf_memcpy;
    fnCallBack.pFnMemcmp            = gf_memcmp;
    fnCallBack.pFnDodiv             = gf_do_div;
    fnCallBack.pFnVsprintf          = gf_vsprintf;
    fnCallBack.pFnVsnprintf         = gf_vsnprintf;
    fnCallBack.pFnVDbgPrint         = gf_cb_vdbgprint;

    if(CBiosSetCallBackFunctions(&fnCallBack) != CBIOS_OK)
    {
        gf_info("CBios set call back func failed.\n");
    }

    rom_lenth = disp_get_rom_bar_image(disp_info);

    gf_memset(&CBChipId, 0, sizeof(CBIOS_CHIP_ID));
    CBChipId.Size = sizeof(CBIOS_CHIP_ID);
    CBChipId.GenericChipID = adapter_info->generic_id;
    CBChipId.ChipID = disp_chipid_to_cbios_chipid(adapter_info->chip_id);

    CBiosStatus = CBiosGetExtensionSize(&CBChipId, &CBiosExtensionSize);

    if(CBiosExtensionSize == 0 || CBiosStatus != CBIOS_OK)
    {
        gf_error("Get cbios extension size failed\n");
    }

    pcbe = gf_calloc(CBiosExtensionSize);

    if(pcbe == NULL)
    {
        gf_error("Alloc memory for cbios pcbe failed\n");
    }

    CBParamInit.pAdapterContext    = disp_info;
    CBParamInit.MAMMPrimaryAdapter = adapter_info->primary;
    CBParamInit.GeneralChipID      = adapter_info->generic_id;
    CBParamInit.ChipID             = disp_chipid_to_cbios_chipid(adapter_info->chip_id);

    CBParamInit.RomImage           = disp_info->rom_image;
    CBParamInit.RomImageLength     = rom_lenth;
    CBParamInit.pSpinLock          = disp_info->cbios_inner_spin_lock;
    CBParamInit.pAuxMutex       = disp_info->cbios_aux_mutex;

    for(i = 0;i < CBIOS_MAX_I2CBUS;i++)
    {
        CBParamInit.pI2CMutex[i] = disp_info->cbios_i2c_mutex[i];
    }

    CBParamInit.Size               = sizeof(CBIOS_PARAM_INIT);

#ifdef GFX_ONLY_FPGA
    CBParamInit.bRunOnQT           = 0x1;
#endif

    if (adapter_info->run_on_qt)
    {
        CBParamInit.bRunOnQT           = 0x1;
    }

    CBiosInit(pcbe, &CBParamInit);

    disp_info->cbios_ext = pcbe;

    return DISP_OK;
}

int disp_cbios_init_hw(disp_info_t *disp_info)
{
    int                    ret        = DISP_OK;
    int                    cb_status  = CBIOS_OK;

    cb_status = CBiosInitHW(disp_info->cbios_ext);

    if(cb_status != CBIOS_OK)
    {
        gf_error("CBiosInitHW failed. cbios status is %x\n",cb_status);
        ret = DISP_FAIL;
    }

    gf_info("disp cbios_init_hw finished.\n");

    return ret;
}

void disp_cbios_get_crtc_resource(disp_info_t *disp_info)
{
    CBIOS_GET_DISP_RESOURCE   cb_disp_res = {0};
    int  i = 0;

    if(CBIOS_OK == CBiosGetDispResource(disp_info->cbios_ext, &cb_disp_res))
    {
        gf_assert(MAX_CORE_CRTCS >=  cb_disp_res.CrtcNum, "crtc num > MAX_CORE_CRTCS");

        disp_info->num_crtc = cb_disp_res.CrtcNum;

        for(i = 0; i < cb_disp_res.CrtcNum; i++)
        {
            disp_info->num_plane[i]= cb_disp_res.PlaneNum[i];
        }

        gf_info("CRTC num: %d.\n", disp_info->num_crtc);
    }
}

void disp_cbios_get_crtc_caps(disp_info_t *disp_info)
{
    CBIOS_DISPLAY_CAPS cb_disp_caps = {0};
    CBIOS_U32      up_plane[MAX_CORE_CRTCS] = {0};
    CBIOS_U32      down_plane[MAX_CORE_CRTCS] = {0};
    int i = 0;

    cb_disp_caps.pUpScalePlaneMask = up_plane;
    cb_disp_caps.pDownScalePlaneMask = down_plane;

    if(CBIOS_OK == CBiosGetDisplayCaps(disp_info->cbios_ext, &cb_disp_caps))
    {
        disp_info->scale_support = cb_disp_caps.SuppCrtcUpScale;   //only use crtc upscaler

        for(i = 0; i < disp_info->num_crtc; i++)
        {
            disp_info->up_scale_plane_mask[i] = up_plane[i];
        }
        for(i = 0; i < disp_info->num_crtc; i++)
        {
            disp_info->down_scale_plane_mask[i] = down_plane[i];
        }
    }
}

int disp_cbios_cleanup(disp_info_t *disp_info)
{
    CBiosUnload(disp_info->cbios_ext);

#if 0
    if(disp_info->cbios_ext)
    {
        gf_free(disp_info->cbios_ext);

        disp_info->cbios_ext = NULL;
    }
#endif

    if(disp_info->rom_image)
    {
        gf_free(disp_info->rom_image);

        disp_info->rom_image = NULL;
    }

    return DISP_OK;
}

void disp_cbios_query_vbeinfo(disp_info_t *disp_info)
{
    adapter_info_t* adapter_info = disp_info->adp_info;
    void* pcbe = disp_info->cbios_ext;
    CBIOS_VBINFO_PARAM vbeinfo = {0};
    int status = CBIOS_OK;

    vbeinfo.Size = sizeof(vbeinfo);
    vbeinfo.BiosVersion = CBIOSVERSION;

    status = CBiosGetVBiosInfo(pcbe, &vbeinfo);
    if(status != CBIOS_OK)
    {
        gf_error("Get cbios vbeinfo failed\n");
    }
    else
    {
        adapter_info->chan_num = vbeinfo.MemChNum;
        adapter_info->avai_mem_size_mb = vbeinfo.AvalMemSize;
        adapter_info->total_mem_size_mb = vbeinfo.TotalMemSize;
        adapter_info->hdaudio_to_local = vbeinfo.HdaudioToLocal;
        adapter_info->non_simul_chip = vbeinfo.NonSimulChip;
        gf_memcpy(disp_info->pmp_version,vbeinfo.PMPVer,sizeof(vbeinfo.PMPVer));

        disp_info->support_output = vbeinfo.SupportDev;
        disp_info->num_output = disp_get_output_num(vbeinfo.SupportDev);

        disp_info->supp_hpd_outputs   = disp_info->support_output & vbeinfo.HPDDevicesMask;
        disp_info->supp_polling_outputs = disp_info->support_output & vbeinfo.PollingDevMask;

        disp_info->vbios_version = vbeinfo.BiosVersion;
        disp_info->firmware_version = vbeinfo.FwVersion;
        gf_memcpy(disp_info->firmware_name, vbeinfo.FwName, sizeof(vbeinfo.FwName));

        gf_info("bios supported device: 0x%x\n", disp_info->support_output);
        gf_info("low_top_address : 0x%x\n", adapter_info->low_top_addr);
    }
}

int disp_cbios_get_modes_size(disp_info_t *disp_info, int output)
{
    void              *pcbe = disp_info->cbios_ext;
    int                 mode_size = 0;

    if(CBIOS_OK == CBiosGetDeviceModeListBufferSize(pcbe, output, &mode_size))
    {
        if(mode_size > 0)
        {
            mode_size += 10 * sizeof(CBiosModeInfoExt); //add some space to avoid overflow in get mode list later
        }
        else
        {
            mode_size = 0;
        }
    }

    return  mode_size;
}

int disp_cbios_get_modes(disp_info_t *disp_info, int output, void* buffer, int buf_size)
{
    void                *pcbe = disp_info->cbios_ext;
    int                 real_size = buf_size;
    int                 real_num = 0;

    if(!buffer || !buf_size)
    {
        return 0;
    }

    CBiosGetDeviceModeList(pcbe, output, (PCBiosModeInfoExt)buffer, &real_size);

    if(real_size > buf_size)
    {
        gf_error("OVERFLOW detected: malloc_size: %d, used size: %x.\n", buf_size, real_size);
    }

    real_num = real_size / sizeof(CBiosModeInfoExt);

    return  real_num;
}

int disp_cbios_get_adapter_modes_size(disp_info_t *disp_info)
{
    void              *pcbe = disp_info->cbios_ext;
    int                 mode_size = 0;

    if(CBIOS_OK == CBiosGetAdapterModeListBufferSize(pcbe, &mode_size))
    {
        if(mode_size > 0)
        {
            mode_size += 10 * sizeof(CBiosModeInfoExt); //add some space to avoid overflow in get mode list later
        }
        else
        {
            mode_size = 0;
        }
    }

    return  mode_size;
}

int disp_cbios_get_adapter_modes(disp_info_t *disp_info, void* buffer, int buf_size)
{
    void                *pcbe = disp_info->cbios_ext;
    int                 real_size = buf_size;
    int                 real_num = 0;

    if(!buffer || !buf_size)
    {
        return 0;
    }

    CBiosGetAdapterModeList(pcbe, (PCBiosModeInfoExt)buffer, &real_size);

    if(real_size > buf_size)
    {
        gf_error("OVERFLOW detected: malloc_size: %d, used size: %x.\n", buf_size, real_size);
    }

    real_num = real_size / sizeof(CBiosModeInfoExt);

    return  real_num;
}

CBiosModeInfoExt* disp_cbios_get_preferred_mode(CBiosModeInfoExt *dev_mode_list, unsigned int mode_num)
{
    CBiosModeInfoExt *pcbios_mode = NULL;
    int i = 0;

    for (i = 0; i < mode_num; i++)
    {
        pcbios_mode = dev_mode_list + i;

        if (pcbios_mode->isPreferredMode)
        {
            break;
        }
    }

    return pcbios_mode;
}

CBiosModeInfoExt* disp_cbios_get_maxium_mode(CBiosModeInfoExt *dev_mode_list)
{
    return &dev_mode_list[0];
}

int disp_cbios_merge_modes(CBiosModeInfoExt* merge_mode_list, CBiosModeInfoExt * adapter_mode_list, unsigned int const adapter_mode_num,
    CBiosModeInfoExt const * dev_mode_list, unsigned int const dev_mode_num)
{
    CBiosModeInfoExt    tmpBuf;
    unsigned int        i = 0;
    unsigned int        adapter_index      = 0;
    unsigned int        dev_index          = 0;
    unsigned int        mode_index         = 0;
    unsigned int        num_mode           = 0;
    unsigned int        *valid_adapter_mode_flag = NULL;


    valid_adapter_mode_flag = gf_calloc(adapter_mode_num * sizeof(unsigned int ));

    // reverse adapter modelist
    for (i = 0; i < adapter_mode_num/2; i++)
    {
        gf_memcpy(&tmpBuf, &adapter_mode_list[i], sizeof(CBiosModeInfoExt));
        gf_memcpy(&adapter_mode_list[i], &adapter_mode_list[adapter_mode_num - i -1], sizeof(CBiosModeInfoExt));
        gf_memcpy(&adapter_mode_list[adapter_mode_num - i -1], &tmpBuf, sizeof(CBiosModeInfoExt));
    }

    //make sure all the adapter modes is smaller than the largest device mode.
    //adapter mode is not strict sorted from small to large .
    //adatper modes : 640x480 800x600 1024x768 1280x720 1280x1024 1680x1050 1920x1080
    //1024x768 is not smaller than 1280x720 strictly.

    // mark adapter mode whether it's bigger than the largest device mode or not
    for (i = 0; i < adapter_mode_num; i++)
    {
        if ((adapter_mode_list[i].XRes <= dev_mode_list[0].XRes) &&
         (adapter_mode_list[i].YRes <= dev_mode_list[0].YRes) &&
         (adapter_mode_list[i].RefreshRate <= dev_mode_list[0].RefreshRate))
        {
            valid_adapter_mode_flag[i] = 1;
        }
        else
        {
            valid_adapter_mode_flag[i] = 0;
        }
    }

    // merge
    dev_index  = 0;
    mode_index = 0;
    num_mode   = 0;
    adapter_index = 0;
    while ((dev_index < dev_mode_num) && (adapter_index < adapter_mode_num))
    {
        if(valid_adapter_mode_flag[adapter_index] == 0)
        {
            adapter_index++;
            continue;
        }
        if ((dev_mode_list[dev_index].XRes > adapter_mode_list[adapter_index].XRes) ||
            ((dev_mode_list[dev_index].XRes == adapter_mode_list[adapter_index].XRes) &&
             (dev_mode_list[dev_index].YRes > adapter_mode_list[adapter_index].YRes)) ||
            ((dev_mode_list[dev_index].XRes == adapter_mode_list[adapter_index].XRes) &&
             (dev_mode_list[dev_index].YRes == adapter_mode_list[adapter_index].YRes) &&
             (dev_mode_list[dev_index].RefreshRate > adapter_mode_list[adapter_index].RefreshRate)))
        {
            gf_memcpy(&merge_mode_list[mode_index], &dev_mode_list[dev_index], sizeof(CBiosModeInfoExt));
            mode_index++;
            dev_index++;
            num_mode++;
        }
        else if ((dev_mode_list[dev_index].XRes == adapter_mode_list[adapter_index].XRes) &&
                 (dev_mode_list[dev_index].YRes == adapter_mode_list[adapter_index].YRes) &&
                 (dev_mode_list[dev_index].RefreshRate == adapter_mode_list[adapter_index].RefreshRate))
        {
            gf_memcpy(&merge_mode_list[mode_index], &dev_mode_list[dev_index], sizeof(CBiosModeInfoExt));
            merge_mode_list[mode_index].InterlaceProgressiveCaps |= adapter_mode_list[adapter_index].InterlaceProgressiveCaps;
            merge_mode_list[mode_index].DeviceFlags |= adapter_mode_list[adapter_index].DeviceFlags;
            merge_mode_list[mode_index].ColorDepthCaps |= adapter_mode_list[adapter_index].ColorDepthCaps;
            merge_mode_list[mode_index].AspectRatioCaps |= adapter_mode_list[adapter_index].AspectRatioCaps;
            mode_index++;
            dev_index++;
            adapter_index++;
            num_mode++;
        }
        else
        {
            gf_memcpy(&merge_mode_list[mode_index], &adapter_mode_list[adapter_index], sizeof(CBiosModeInfoExt));
            mode_index++;
            adapter_index++;
            num_mode++;
        }
    }

    if ((dev_index == dev_mode_num) && (adapter_index < adapter_mode_num))
    {
        while(adapter_index < adapter_mode_num)
        {
            if(valid_adapter_mode_flag[adapter_index] == 1)
            {
                gf_memcpy(&merge_mode_list[mode_index], &adapter_mode_list[adapter_index], sizeof(CBiosModeInfoExt));
                num_mode++;
                mode_index++;
            }
            adapter_index++;
        }
    }

    if ((dev_index < dev_mode_num) && (adapter_index == adapter_mode_num))
    {
        gf_memcpy(&merge_mode_list[mode_index], &dev_mode_list[dev_index],
                   sizeof(CBiosModeInfoExt)*(dev_mode_num - dev_index));
        num_mode += (dev_mode_num - dev_index);
    }

    if(valid_adapter_mode_flag)
    {
        gf_free(valid_adapter_mode_flag);
        valid_adapter_mode_flag = NULL;
    }

    return num_mode;
}

int disp_cbios_cbmode_to_drmmode(disp_info_t *disp_info, int output, void* cbmode, int i, struct drm_display_mode *drm_mode)
{
    void                *pcbe = disp_info->cbios_ext;
    CBIOS_GET_MODE_TIMING_PARAM   get_timing = {0};
    CBIOS_TIMING_ATTRIB    timing_attrib = {0};
    PCBiosModeInfoExt  cbios_mode = (PCBiosModeInfoExt)cbmode + i;

    if(!cbios_mode || !drm_mode)
    {
        return  DISP_FAIL;
    }

    get_timing.DeviceId = output;
    get_timing.pMode = cbios_mode;
    get_timing.pTiming = &timing_attrib;
    if(CBIOS_OK != CBiosGetModeTiming(pcbe, &get_timing))
    {
        return  DISP_FAIL;
    }

    drm_mode->clock = timing_attrib.PixelClock / 10;
    drm_mode->hdisplay = timing_attrib.XRes;
    drm_mode->hsync_start = timing_attrib.HorSyncStart;
    drm_mode->hsync_end = timing_attrib.HorSyncEnd;
    drm_mode->htotal = timing_attrib.HorTotal;
    drm_mode->hskew = 0;

    drm_mode->vdisplay = timing_attrib.YRes;
    drm_mode->vsync_start = timing_attrib.VerSyncStart;
    drm_mode->vsync_end = timing_attrib.VerSyncEnd;
    drm_mode->vtotal = timing_attrib.VerTotal;
    drm_mode->vscan = 0;

#if DRM_VERSION_CODE < KERNEL_VERSION(5,9,0)
    drm_mode->vrefresh = timing_attrib.RefreshRate / 100;
#endif
    drm_mode->type |= DRM_MODE_TYPE_DRIVER;
    if (cbios_mode->isPreferredMode)
    {
        drm_mode->type |= DRM_MODE_TYPE_PREFERRED;
    }

    if (cbios_mode->InterlaceProgressiveCaps & 0x02)
    {
        drm_mode->flags |= DRM_MODE_FLAG_INTERLACE;
    }

    drm_mode->flags |= (timing_attrib.HVPolarity & HOR_NEGATIVE) ?
                DRM_MODE_FLAG_NHSYNC : DRM_MODE_FLAG_PHSYNC;
    drm_mode->flags |= (timing_attrib.HVPolarity & VER_NEGATIVE) ?
                DRM_MODE_FLAG_NVSYNC : DRM_MODE_FLAG_PVSYNC;
    drm_mode->flags |= DRM_MODE_FLAG_3D_NONE;

    return  DISP_OK;
}

int disp_cbios_3dmode_to_drmmode(disp_info_t *disp_info, int output, void* mode, int i, struct drm_display_mode *drm_mode)
{
    void                *pcbe = disp_info->cbios_ext;
    CBIOS_GET_MODE_TIMING_PARAM   get_timing = {0};
    CBIOS_TIMING_ATTRIB    timing_attrib = {0};
    CBiosModeInfoExt  cbios_mode = {0};
    PCBIOS_3D_VIDEO_MODE_LIST  cb_3d_mode = (PCBIOS_3D_VIDEO_MODE_LIST)mode + i;

    if(!cb_3d_mode || !drm_mode)
    {
        return  DISP_FAIL;
    }

    cbios_mode.XRes = cb_3d_mode->XRes;
    cbios_mode.YRes = cb_3d_mode->YRes;
    cbios_mode.RefreshRate = cb_3d_mode->RefreshRate;
    cbios_mode.InterlaceProgressiveCaps = (cb_3d_mode->bIsInterlace)? 2 : 1;

    get_timing.DeviceId = output;
    get_timing.pMode = &cbios_mode;
    get_timing.pTiming = &timing_attrib;

    if(CBIOS_OK != CBiosGetModeTiming(pcbe, &get_timing))
    {
        return  DISP_FAIL;
    }

    drm_mode->clock = timing_attrib.PixelClock / 10;
    drm_mode->hdisplay = timing_attrib.XRes;
    drm_mode->hsync_start = timing_attrib.HorSyncStart;
    drm_mode->hsync_end = timing_attrib.HorSyncEnd;
    drm_mode->htotal = timing_attrib.HorTotal;
    drm_mode->hskew = 0;

    drm_mode->vdisplay = timing_attrib.YRes;
    drm_mode->vsync_start = timing_attrib.VerSyncStart;
    drm_mode->vsync_end = timing_attrib.VerSyncEnd;
    drm_mode->vtotal = timing_attrib.VerTotal;
    drm_mode->vscan = 0;

#if DRM_VERSION_CODE < KERNEL_VERSION(5,9,0)
    drm_mode->vrefresh = timing_attrib.RefreshRate / 100;
#endif

    drm_mode->type |= DRM_MODE_TYPE_DRIVER;

    if (cb_3d_mode->bIsInterlace)
    {
        drm_mode->flags |= DRM_MODE_FLAG_INTERLACE;
    }
    drm_mode->flags |= (timing_attrib.HVPolarity & HOR_NEGATIVE) ?
             DRM_MODE_FLAG_NHSYNC : DRM_MODE_FLAG_PHSYNC;
    drm_mode->flags |= (timing_attrib.HVPolarity & VER_NEGATIVE) ?
             DRM_MODE_FLAG_NVSYNC : DRM_MODE_FLAG_PVSYNC;

    if (cb_3d_mode->SupportStructures.FramePacking)
    {
        drm_mode->flags |= DRM_MODE_FLAG_3D_FRAME_PACKING;
    }
    if (cb_3d_mode->SupportStructures.FieldAlternative)
    {
        drm_mode->flags |= DRM_MODE_FLAG_3D_FIELD_ALTERNATIVE;
    }
    if (cb_3d_mode->SupportStructures.LineAlternative)
    {
        drm_mode->flags |= DRM_MODE_FLAG_3D_LINE_ALTERNATIVE;
    }
    if (cb_3d_mode->SupportStructures.SideBySideFull)
    {
        drm_mode->flags |= DRM_MODE_FLAG_3D_SIDE_BY_SIDE_FULL;
    }
    if (cb_3d_mode->SupportStructures.LDepth)
    {
        drm_mode->flags |= DRM_MODE_FLAG_3D_L_DEPTH;
    }
    if (cb_3d_mode->SupportStructures.LDepthGraphics)
    {
        drm_mode->flags |= DRM_MODE_FLAG_3D_L_DEPTH_GFX_GFX_DEPTH;
    }
    if (cb_3d_mode->SupportStructures.TopAndBottom)
    {
        drm_mode->flags |= DRM_MODE_FLAG_3D_TOP_AND_BOTTOM;
    }
    if (cb_3d_mode->SupportStructures.SideBySideHalf)
    {
        drm_mode->flags |= DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF;
    }

    return  DISP_OK;
}

int disp_cbios_get_3dmode_size(disp_info_t* disp_info, int output)
{
    void                               *pcbe = disp_info->cbios_ext;
    CBIOS_MONITOR_3D_CAPABILITY_PARA   CbiosMonitor3DCapablitityPara    = {0};
    int  buf_size = 0, support = 0;

    CbiosMonitor3DCapablitityPara.DeviceId           = output;
    CbiosMonitor3DCapablitityPara.pMonitor3DModeList = NULL;

    if(CBIOS_OK == CBiosQueryMonitor3DCapability(pcbe, &CbiosMonitor3DCapablitityPara))
    {
        support = CbiosMonitor3DCapablitityPara.bIsSupport3DVideo;
    }

    if(support)
    {
        buf_size = CbiosMonitor3DCapablitityPara.Monitor3DModeNum;
        buf_size *= sizeof(CBIOS_3D_VIDEO_MODE_LIST);
    }

    return  buf_size;
}

int disp_cbios_get_3dmodes(disp_info_t *disp_info, int output, void* buffer, int buf_size)
{
    void                               *pcbe = disp_info->cbios_ext;
    CBIOS_MONITOR_3D_CAPABILITY_PARA   CbiosMonitor3DCapablitityPara    = {0};
    int  real_size = 0, real_num = 0;

    if(!buffer || !buf_size)
    {
        return real_num;
    }

    CbiosMonitor3DCapablitityPara.DeviceId           = output;
    CbiosMonitor3DCapablitityPara.pMonitor3DModeList = (PCBIOS_3D_VIDEO_MODE_LIST)buffer;

    CBiosQueryMonitor3DCapability(pcbe, &CbiosMonitor3DCapablitityPara);

    if(CbiosMonitor3DCapablitityPara.bIsSupport3DVideo)
    {
        real_size = CbiosMonitor3DCapablitityPara.Monitor3DModeNum * sizeof(CBIOS_3D_VIDEO_MODE_LIST);
        if(real_size > buf_size)
        {
            gf_error("OVERFLOW detected: malloc_size: %d, used size: %x.\n", buf_size, real_size);
        }
        real_num = CbiosMonitor3DCapablitityPara.Monitor3DModeNum;
    }

    return  real_num;
}

void* disp_cbios_get_device_modelist(disp_info_t *disp_info, int output_type, int* mode_num)
{
    void *pmode_list = NULL, *mode_buf = NULL, *adapter_buf = NULL, *merge_buf = NULL;
    int dev_mode_size = 0, adapter_mode_size = 0, dev_num = 0, adapter_num = 0, real_num = 0;

    if(!disp_info || !output_type)
    {
        goto END;
    }

    dev_mode_size = disp_cbios_get_modes_size(disp_info, output_type);

    if(!dev_mode_size)
    {
        goto END;
    }

    mode_buf = gf_calloc(dev_mode_size);

    if(!mode_buf)
    {
        goto END;
    }

    dev_num = disp_cbios_get_modes(disp_info, output_type, mode_buf, dev_mode_size);

    pmode_list = mode_buf;
    real_num = dev_num;

    if(disp_info->scale_support)
    {
        adapter_mode_size = disp_cbios_get_adapter_modes_size(disp_info);

        if(adapter_mode_size != 0)
        {
            adapter_buf = gf_calloc(adapter_mode_size);

            if(!adapter_buf)
            {
                goto END;
            }
            adapter_num = disp_cbios_get_adapter_modes(disp_info, adapter_buf, adapter_mode_size);

            merge_buf = gf_calloc((dev_num + adapter_num) * sizeof(CBiosModeInfoExt));

            if(!merge_buf)
            {
                gf_free(adapter_buf);
                goto END;
            }

            real_num = disp_cbios_merge_modes(merge_buf, adapter_buf, adapter_num, mode_buf, dev_num);
            pmode_list = merge_buf;

            gf_free(mode_buf);
            gf_free(adapter_buf);
        }
    }

END:
    if(mode_num)
    {
        *mode_num = real_num;
    }

    return pmode_list;
}

int disp_cbios_get_mode_timing(disp_info_t *disp_info, int output, struct drm_display_mode *drm_mode)
{
    void                *pcbe = disp_info->cbios_ext;
    CBIOS_GET_MODE_TIMING_PARAM   get_timing = {0};
    CBIOS_TIMING_ATTRIB    timing_attrib = {0};
    CBiosModeInfoExt  cbios_mode = {0};
    int temp = 0;

    if(!drm_mode)
    {
        return  DISP_FAIL;
    }

    cbios_mode.XRes = drm_mode->hdisplay;
    cbios_mode.YRes = drm_mode->vdisplay;
    temp = drm_mode->clock * 1000/drm_mode->htotal;
    cbios_mode.RefreshRate = temp * 100/drm_mode->vtotal;
    cbios_mode.InterlaceProgressiveCaps = (drm_mode->flags & DRM_MODE_FLAG_INTERLACE) ? 0x02 : 0x01;

    get_timing.DeviceId = output;
    get_timing.pMode = &cbios_mode;
    get_timing.pTiming = &timing_attrib;
    if(CBIOS_OK != CBiosGetModeTiming(pcbe, &get_timing))
    {
        return  DISP_FAIL;
    }

    drm_mode->crtc_clock = timing_attrib.PixelClock / 10;
    drm_mode->crtc_hdisplay = timing_attrib.XRes;
    drm_mode->crtc_hblank_start = timing_attrib.HorBStart;
    drm_mode->crtc_hblank_end = timing_attrib.HorBEnd;
    drm_mode->crtc_hsync_start = timing_attrib.HorSyncStart;
    drm_mode->crtc_hsync_end = timing_attrib.HorSyncEnd;
    drm_mode->crtc_htotal = timing_attrib.HorTotal;
    drm_mode->crtc_hskew = 0;

    drm_mode->crtc_vdisplay = timing_attrib.YRes;
    drm_mode->crtc_vblank_start = timing_attrib.VerBStart;
    drm_mode->crtc_vblank_end = timing_attrib.VerBEnd;
    drm_mode->crtc_vsync_start = timing_attrib.VerSyncStart;
    drm_mode->crtc_vsync_end = timing_attrib.VerSyncEnd;
    drm_mode->crtc_vtotal = timing_attrib.VerTotal;

    return DISP_OK;
}

void* disp_cbios_read_edid(disp_info_t *disp_info, int drv_dev)
{
    void         *pcbe    = disp_info->cbios_ext;

    CBIOS_PARAM_GET_EDID           cbParamGetEdid = {0};
    unsigned char                  *pEdid = NULL;

    pEdid = gf_calloc(EDID_BUF_SIZE);

    if(!pEdid)
    {
        return  NULL;
    }

    cbParamGetEdid.EdidBuffer    = pEdid;
    cbParamGetEdid.EdidBufferLen = EDID_BUF_SIZE;
    cbParamGetEdid.Size          = sizeof(CBIOS_PARAM_GET_EDID);
    cbParamGetEdid.DeviceId      = drv_dev;

    if(CBiosGetEdid(pcbe, &cbParamGetEdid) != CBIOS_OK)
    {
        gf_debug("device id: 0x%x has no valid EDID.\n", drv_dev);
        gf_free(pEdid);
        pEdid = NULL;
    }

    return pEdid;
}

int disp_cbios_update_output_active(disp_info_t *disp_info, int* outputs)
{
    void                        *pcbe = disp_info->cbios_ext;
    CIP_ACTIVE_DEVICES          activeDevices = {0};

    gf_memcpy(activeDevices.DeviceId, outputs, sizeof(activeDevices.DeviceId));

    return (CBiosSetActiveDevice(pcbe, &activeDevices) == CBIOS_OK)? 0 : -1;
}

int disp_cbios_sync_vbios(disp_info_t *disp_info)
{
    int status = DISP_OK;
    CBIOS_VBIOS_DATA_PARAM DataParam = {0};

    if(CBIOS_OK != CBiosSyncDataWithVbios(disp_info->cbios_ext, &DataParam))
    {
        status = DISP_FAIL;
    }
    return status;
}

int disp_cbios_get_active_devices(disp_info_t *disp_info, int* devices)
{
    int status = DISP_OK;
    CIP_ACTIVE_DEVICES          activeDevices = {0};

    if(CBIOS_OK != CBiosGetActiveDevice(disp_info->cbios_ext, &activeDevices))
    {
        status = DISP_FAIL;
    }
    else
    {
        gf_memcpy(devices, activeDevices.DeviceId, sizeof(activeDevices.DeviceId));
    }
    return status;
}

int disp_cbios_detect_connected_output(disp_info_t *disp_info, int to_detect, int full_detect)
{
    void                        *pcbe = disp_info->cbios_ext;
    CIP_DEVICES_DETECT_PARAM    devicesDetectParam = {0};
    int                         status;
    int                         curr_dev = 0;

    devicesDetectParam.DevicesToDetect = disp_info->support_output & to_detect;
    devicesDetectParam.FullDetect = full_detect;

    status = CBiosDetectAttachedDisplayDevices(pcbe, &devicesDetectParam);

    if(status == CBIOS_OK)
    {
        curr_dev = devicesDetectParam.DetectedDevices;
    }

    return curr_dev;
}

//get monitor type per device id, connected = 0, supported monitor type, =1, current connected monitor type
int  disp_cbios_get_monitor_type(disp_info_t *disp_info, int device_id, int  connected)
{
    int  status = CBIOS_OK;
    int  monitor_type = 0;
    void* pcbe = disp_info->cbios_ext;
    CBIOS_QUERY_MONITOR_TYPE_PER_PORT   query_monitor = {0};

    if(device_id)
    {
        query_monitor.ActiveDevId = GET_LAST_BIT(device_id);
        query_monitor.bConnected = (connected)? CBIOS_TRUE : CBIOS_FALSE;

        status = CBiosGetMonitorTypePerPort(pcbe, &query_monitor);

        if(status == CBIOS_OK)
        {
            monitor_type = (int)query_monitor.MonitorType;
        }
    }
    monitor_type = disp_biosmonitor_to_output(monitor_type);

    return  monitor_type;
}

int disp_cbios_set_hdac_connect_status(disp_info_t *disp_info, int device , int bPresent, int bEldValid)
{
    void                    *pcbe         = disp_info->cbios_ext;
    CBIOS_HDAC_PARA         CbiosHDACPara = {0};
    int                     monitor_type = 0;

    monitor_type = disp_cbios_get_monitor_type(disp_info, device, FALSE);
    if (monitor_type & (UT_OUTPUT_TYPE_HDMI | UT_OUTPUT_TYPE_DP | UT_OUTPUT_TYPE_MHL))
    {
        CbiosHDACPara.Size = sizeof(CBIOS_HDAC_PARA);
        CbiosHDACPara.DeviceId = device;
        CbiosHDACPara.bPresent  = bPresent;
        CbiosHDACPara.bEldValid = bEldValid;

        CBiosSetHDACConnectStatus(pcbe, &CbiosHDACPara);
    }
    return DISP_OK;
}

int disp_cbios_set_mode(disp_info_t *disp_info, int crtc, struct drm_display_mode* mode, struct drm_display_mode* adjusted_mode, int flag)
{
    void*                   pcbe = disp_info->cbios_ext;
    CBIOS_STATUS            cb_status;
    int  temp = 0;

    CBiosSettingModeParams  mode_param = {0};

    temp = adjusted_mode->clock * 1000/adjusted_mode->htotal;
    mode_param.DestModeParams.RefreshRate = temp * 100/adjusted_mode->vtotal;

    mode_param.SourcModeParams.XRes = mode->hdisplay;
    mode_param.SourcModeParams.YRes = mode->vdisplay;

    mode_param.DestModeParams.XRes = adjusted_mode->hdisplay;
    mode_param.DestModeParams.YRes = adjusted_mode->vdisplay;

    mode_param.DestModeParams.InterlaceFlag = (adjusted_mode->flags & DRM_MODE_FLAG_INTERLACE)? 1 : 0;;
    mode_param.DestModeParams.AspectRatioFlag = 0;
    mode_param.DestModeParams.OutputSignal = CBIOS_RGBOUTPUT;

    mode_param.ScalerSizeParams.XRes = adjusted_mode->hdisplay;
    mode_param.ScalerSizeParams.YRes = adjusted_mode->vdisplay;

    mode_param.IGAIndex = crtc;
    mode_param.BitPerComponent = 8;
    mode_param.SkipIgaMode = (flag & UPDATE_CRTC_MODE_FLAG)? 0 : 1;
    mode_param.SkipDeviceMode = (flag & UPDATE_ENCODER_MODE_FLAG)? 0 : 1;

    if(mode_param.SkipIgaMode == 0)
    {
        gf_info("KMS set mode to path(iga_index-->active_device): %d-->0x%x.\n", crtc, disp_info->active_output[crtc]);
    }

    if(adjusted_mode->flags & DRM_MODE_FLAG_3D_MASK)
    {
        mode_param.Is3DVideoMode = 1;
        mode_param.IsSingleBuffer= 0;
        if(mode->flags & DRM_MODE_FLAG_3D_FRAME_PACKING)
        {
            mode_param.Video3DStruct = FRAME_PACKING;
        }
        else if(mode->flags & DRM_MODE_FLAG_3D_FIELD_ALTERNATIVE)
        {
            mode_param.Video3DStruct = FIELD_ALTERNATIVE;
        }
        else if(mode->flags & DRM_MODE_FLAG_3D_LINE_ALTERNATIVE)
        {
            mode_param.Video3DStruct = LINE_ALTERNATIVE;
        }
        else if(mode->flags & DRM_MODE_FLAG_3D_SIDE_BY_SIDE_FULL)
        {
            mode_param.Video3DStruct = SIDE_BY_SIDE_FULL;
        }
        else if(mode->flags & DRM_MODE_FLAG_3D_L_DEPTH)
        {
            mode_param.Video3DStruct = L_DEPTH;
        }
        else if(mode->flags & DRM_MODE_FLAG_3D_L_DEPTH_GFX_GFX_DEPTH)
        {
            mode_param.Video3DStruct = L_DEPTH_GRAPHICS;
        }
        else if(mode->flags & DRM_MODE_FLAG_3D_TOP_AND_BOTTOM)
        {
            mode_param.Video3DStruct = TOP_AND_BOTTOM;
        }
        else if(mode->flags & DRM_MODE_FLAG_3D_SIDE_BY_SIDE_HALF)
        {
            mode_param.Video3DStruct = SIDE_BY_SIDE_HALF;
        }
    }

    cb_status = CBiosSetModeToIGA(pcbe, &mode_param);

    return (cb_status == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}

int disp_cbios_turn_onoff_iga(disp_info_t *disp_info, int iga, int bOn)
{
    void                        *pcbe = disp_info->cbios_ext;
    int                         cb_status;

    cb_status = CBiosSetIgaOnOffState(pcbe, bOn, iga);

    return (cb_status == CBIOS_OK) ? DISP_OK : DISP_FAIL;

}

int disp_cbios_turn_onoff_screen(disp_info_t *disp_info, int iga, int bOn)
{
    void *pcbe = disp_info->cbios_ext;
    int cb_status;

    cb_status = CBiosSetIgaScreenOnOffState(pcbe, bOn, iga);

    return (cb_status == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}

int disp_cbios_set_dpms(disp_info_t *disp_info, int device, int dpms_mode)
{
    int status = DISP_OK;

    if(CBIOS_OK != CBiosSetDisplayDevicePowerState(disp_info->cbios_ext, device, dpms_mode))
    {
        status = DISP_FAIL;
    }

    return status;
}

int disp_cbios_set_gamma(disp_info_t *disp_info, int pipe, void* data)
{
    int  status;
    void  *pcbe = disp_info->cbios_ext;
    CBIOS_GAMMA_PARA gamma_para = {0};

    if(pipe >= disp_info->num_crtc)
    {
        return DISP_FAIL;
    }

    gamma_para.pGammaTable = data;

    gamma_para.IGAIndex = pipe;
    gamma_para.FisrtEntryIndex = 0;
    gamma_para.EntryNum = 256;
    gamma_para.Flags.bConfigGamma = 1;
    gamma_para.Flags.bSetLUT = 1;

    status = CBiosSetGamma(pcbe, &gamma_para);

    return (status == CBIOS_OK)? DISP_OK : DISP_FAIL;
}

/*
int disp_mode_fixup(disp_info_t *disp_info, gf_mode_info_t *mode)
{
    output_t         *output = dispmgri_get_output_by_type(disp_info, mode->output);
    gf_mode_t       *modes  = NULL;

    unsigned int    caps    = 0;
    int             delta_x = 0;
    int             delta_y = 0;
    unsigned int    delta_xy= -1;

    int i, status = DISP_FAIL;

    if(output == NULL)
    {
        return status;
    }

    modes = output->modes;

    if(mode->bpp == 8)
    {
        caps = 0x02;
    }
    else if(mode->bpp == 16)
    {
        caps = 0x04;
    }
    else if(mode->bpp == 32)
    {
        caps = 0x01;
    }

    for(i = 0; i < output->num_modes; i++)
    {
        if((modes[i].xres == mode->width) &&
           (modes[i].yres == mode->height) &&
           (modes[i].color_depth_caps & caps))
        {
            mode->dst_width   = modes[i].xres;
            mode->dst_height  = modes[i].yres;
            mode->dst_refresh = modes[i].refresh_rate;
            break;
        }

        delta_x = modes[i].xres - mode->width;
        delta_y = modes[i].yres - mode->height;

        if((delta_x >= 0) &&
           (delta_y >= 0) &&
           (delta_xy > delta_x + delta_y) &&
           (modes[i].color_depth_caps & caps))
        {
            mode->dst_width   = modes[i].xres;
            mode->dst_height  = modes[i].yres;
            mode->dst_refresh = modes[i].refresh_rate;
            delta_xy = delta_x + delta_y;
        }
    }

    status = mode->dst_refresh ? DISP_OK:DISP_FAIL;

    return status;

}

int disp_pwm_func_ctrl(disp_info_t *disp_info, gf_pwm_func_ctrl_t *pwm)
{
    output_t *output = NULL;
    void     *pvcbe  = disp_info->cbios_ext;

    CBIOS_PWM_FUNCTION_CTRL_PARAMS pwm_param = {0};

    int status = DISP_FAIL;

    switch(pwm->type)
    {
    case GF_SET_FAN:
        pwm_param.PWMFunction = PWM_SET_FAN_CTRL_STATUS;
        pwm_param.PWMFuncSetting.FanCtrlArg.byFanSpeedLevel = pwm->value;

        break;

    case GF_SET_BACKLIGHT:
        pwm_param.PWMFunction = PWM_SET_BACKLIGHT_STATUS;
        pwm_param.PWMFuncSetting.BLctrlArg.byBLLevel = pwm->value;

        output = disp_get_output_by_type(disp_info, pwm->output);

        output->curr_back_light_level = pwm->value;

        break;

    case GF_GET_FAN:
        pwm_param.PWMFunction = PWM_GET_FAN_CTRL_STATUS;

        break;

    case GF_GET_BACKLIGHT:

        pwm_param.PWMFunction = PWM_GET_BACKLIGHT_STATUS;
        break;

    default:
        return DISP_FAIL;
    }

    gf_mutex_lock(disp_mgr->cbios_lock);
    status = CBiosPWMFunctionCtrl(pvcbe, &pwm_param);
    gf_mutex_unlock(disp_mgr->cbios_lock);

    if(status != 0)
    {
        gf_error("pwm setting fail\n");
        status = DISP_FAIL;
    }

    return status;
}

int  disp_cbios_dbg_level_get(disp_info_t *disp_info)
{
    CBIOS_DBG_LEVEL_CTRL  dbg_level_ctl = {0};

    dbg_level_ctl.Size = sizeof(CBIOS_DBG_LEVEL_CTRL);
    dbg_level_ctl.bGetValue = 1;

    CBiosDbgLevelCtl(&dbg_level_ctl);

    return  dbg_level_ctl.DbgLevel;
}

void  disp_cbios_dbg_level_set(disp_info_t *disp_info, int dbg_level)
{
    CBIOS_DBG_LEVEL_CTRL  dbg_level_ctl = {0};
    dbg_level_ctl.Size = sizeof(CBIOS_DBG_LEVEL_CTRL);
    dbg_level_ctl.DbgLevel = dbg_level;

    CBiosDbgLevelCtl(&dbg_level_ctl);
}*/

int disp_cbios_get_connector_attrib(disp_info_t *disp_info, gf_connector_t *gf_connector)
{
    void*  pcbe = disp_info->cbios_ext;
    CBIOS_STATUS   status = CBIOS_OK;
    CBiosMonitorAttribute attrib = {0};

    attrib.Size = sizeof(CBiosMonitorAttribute);
    attrib.ActiveDevId = gf_connector->output_type;

    status = CBiosQueryMonitorAttribute(pcbe, &attrib);

    if (status == CBIOS_OK)
    {
        gf_connector->base_connector.display_info.width_mm = attrib.MonitorHorSize;
        gf_connector->base_connector.display_info.height_mm = attrib.MonitorVerSize;
        gf_connector->monitor_type = disp_biosmonitor_to_output(attrib.MonitorType);
        gf_connector->support_audio = attrib.bSupportHDAudio ? 1 : 0;
    }

    return  (status == CBIOS_OK)? DISP_OK : DISP_FAIL;
}

int  disp_cbios_get_crtc_mask(disp_info_t *disp_info,  int device)
{
    void*  pcbe = disp_info->cbios_ext;
    CBIOS_STATUS   status = CBIOS_OK;
    CBIOS_GET_IGA_MASK   GetIgaMask = {0};

    GetIgaMask.Size = sizeof(CBIOS_GET_IGA_MASK);
    GetIgaMask.DeviceId = device;

    status = CBiosGetIgaMask(pcbe, &GetIgaMask);

    return  (status == CBIOS_OK)?  GetIgaMask.IgaMask : 0;
}

int disp_cbios_get_clock(disp_info_t *disp_info, unsigned int type, unsigned int *output)
{
    adapter_info_t* adapter_info = disp_info->adp_info;
    CBios_GetClock_Params GetClock = {0};

    int   status = CBIOS_OK;

    GetClock.Size      = sizeof(CBios_GetClock_Params);
    GetClock.ClockFreq = output;

    if(type == GF_QUERY_ENGINE_CLOCK || type == GF_QUERY_CORE_CLOCK)
    {
        GetClock.ClockType = CBIOS_ECLKTYPE;
    }
    else if(type == GF_QUERY_I_CLOCK)
    {
        GetClock.ClockType = CBIOS_ICLKTYPE;
    }
    else if (type == GF_QUERY_CPU_FREQUENCE)
    {
       GetClock.ClockType = CBIOS_CPUFRQTYPE;
    }
    else if(type == GF_QUERY_VCLK)
    {
        GetClock.ClockType = CBIOS_VCLKTYPE;
    }
    else if(type == GF_QUERY_MCLK)
    {
        GetClock.ClockType = CBIOS_MCLKTYPE;
    }
    else
    {
        gf_error("unknown get clock type: %d.\n", type);

        gf_assert(0, "Invalid clock type.");
    }

    status = CBiosGetClock(disp_info->cbios_ext, &GetClock);

    if(status != CBIOS_OK)
    {
        gf_error("disp_cbios_get_clock failed: %d.\n", status);
    }

    if (type == GF_QUERY_ENGINE_CLOCK || type == GF_QUERY_CORE_CLOCK)
    {
        *output = (gf_do_div(*output, 10000) + 1) * 10000;

        if((adapter_info->chip_id >= CHIP_ARISE1020) && (adapter_info->chip_id < CHIP_ARISE2030) && (type == GF_QUERY_CORE_CLOCK))
        {
            *output *= 2;
        }
    }

    if ((adapter_info->chip_id >= CHIP_ARISE2030) && (type == GF_QUERY_VCLK))
    {
        *output = (gf_do_div(*output, 10000) + 1) * 10000 * 2;
    }

    return (status == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}

int disp_cbios_set_clock(disp_info_t *disp_info, unsigned int type, unsigned int para)
{
    CBios_SetClock_Params SetClock = {0};

    int   status = CBIOS_OK;

    SetClock.Size = sizeof(CBios_SetClock_Params);

    if (type == GF_SET_I_CLOCK)
    {
        SetClock.ClockType = CBIOS_ICLKTYPE;
        SetClock.ClockFreq = para;
    }
    else if (type == GF_SET_CPU_FREQUENCE)
    {
        SetClock.ClockType = CBIOS_CPUFRQTYPE;
        SetClock.ClockFreq = para;
    }
    else
    {
        gf_error("unknown set clock type: %d.\n", type);

        gf_assert(0, "Invalid clock type.");
    }

    status = CBiosSetClock(disp_info->cbios_ext, &SetClock);

    if(status != CBIOS_OK)
    {
        gf_error("disp_cbios_set_clock failed: %d.\n", status);
    }

    return (status == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}

int disp_cbios_enable_hdcp(disp_info_t *disp_info, unsigned int enable, unsigned int devices)
{
    CBiosContentProtectionOnOffParams hdcp_para = {0};

    int   status = CBIOS_OK;

    hdcp_para.Size = sizeof(CBiosContentProtectionOnOffParams);
    hdcp_para.DevicesId = devices;
    hdcp_para.bHdcpStatus = enable ? CBIOS_TRUE : CBIOS_FALSE;

    status = CBiosContentProtectionOnOff(disp_info->cbios_ext, &hdcp_para);

    if (status != CBIOS_OK)
    {
        gf_info("disp_cbios_enable_hdcp failed\n");
    }

    return (status == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}

int disp_cbios_get_hdcp_status(disp_info_t *disp_info, gf_hdcp_op_t *dhcp_op, unsigned int devices)
{
    CBIOS_HDCP_STATUS_PARA     hdcp_status = {0};

    int status = CBIOS_OK;

    hdcp_status.Size = sizeof(CBIOS_HDCP_STATUS_PARA);
    hdcp_status.DevicesId = devices;

    status = CBiosGetHDCPStatus(disp_info->cbios_ext, &hdcp_status);

    if (status == CBIOS_OK)
    {
        dhcp_op->result = hdcp_status.HdcpStatus == CBIOS_TRUE ? GF_HDCP_ENABLED : GF_HDCP_FAILED;
    }
    else
    {
        gf_error("disp_cbios_get_hdcp_status failed\n");
    }

    return (status == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}

static unsigned int DrmFormat2CBiosFormat(unsigned int drm_format, unsigned long long modifier)
{
    CBIOS_FORMAT cbios_format = CBIOS_FMT_INVALID;
    switch (drm_format)
    {
        case DRM_FORMAT_RGB565:
            cbios_format = CBIOS_FMT_R5G6B5;
            break;
        case DRM_FORMAT_ARGB8888:
            if (modifier == DRM_FORMAT_MOD_GF_RB_ALT)
                cbios_format = CBIOS_FMT_A8B8G8R8;
            else
                cbios_format = CBIOS_FMT_A8R8G8B8;
            break;
        case DRM_FORMAT_ABGR8888:
            cbios_format = CBIOS_FMT_A8B8G8R8;
            break;
        case DRM_FORMAT_XBGR8888:
            cbios_format = CBIOS_FMT_X8B8G8R8;
            break;
        case DRM_FORMAT_XRGB8888:
            if (modifier == DRM_FORMAT_MOD_GF_RB_ALT)
                cbios_format = CBIOS_FMT_X8B8G8R8;
            else
                cbios_format = CBIOS_FMT_X8R8G8B8;
            break;
        case DRM_FORMAT_ARGB2101010:
            cbios_format = CBIOS_FMT_A2R10G10B10;
            break;
        case DRM_FORMAT_YUYV:
        case DRM_FORMAT_YVYU:
            cbios_format = CBIOS_FMT_CRYCBY422_16BIT;
            break;
        case DRM_FORMAT_UYVY:
        case DRM_FORMAT_VYUY:
            cbios_format = CBIOS_FMT_YCRYCB422_16BIT;
            break;
        case DRM_FORMAT_AYUV:
            cbios_format = CBIOS_FMT_YCBCR8888_32BIT;
            break;
        default:
            gf_assert(0, "invalid drm format");
            cbios_format = CBIOS_FMT_A8R8G8B8;
            break;
    }

    return cbios_format;
}

int disp_cbios_crtc_flip(disp_info_t *disp_info, gf_crtc_flip_t *arg)
{
    gf_card_t* gf_card = disp_info->gf_card;
    struct drm_framebuffer *fb = arg->fb;
    struct drm_gf_framebuffer *gfb = fb? to_gfb(arg->fb) : NULL;
    CBIOS_OVERLAY_INFO   overlay = {0};
    CBIOS_STREAM_PARA   input_stream = {0};
    CBIOS_PLANE_PARA      disp_plane = {0};
    CBIOS_UPDATE_FRAME_PARA  update_frame = {0};

    update_frame.Size = sizeof(update_frame);
    update_frame.IGAIndex = arg->crtc;
    update_frame.pPlanePara[0] = &disp_plane;

    disp_plane.PlaneIndex = arg->stream_type;
    disp_plane.StreamType = arg->stream_type;
    disp_plane.pInputStream = &input_stream;

    trace_gfx_crtc_flip(gf_card->index << 16 | arg->crtc, arg, gfb ? gfb->obj : NULL);

    if(fb)
    {
        disp_plane.FlipMode.FlipType = CBIOS_PLANE_FLIP_WITH_ENABLE;
    }
    else
    {
        disp_plane.FlipMode.FlipType = CBIOS_PLANE_FLIP_WITH_DISABLE;
    }

    gf_card->primary_addr[arg->crtc] = gfb ? gfb->obj->info.gpu_virt_addr:0;

    if(fb)
    {
        input_stream.SurfaceAttrib.StartAddr = gfb->obj->info.gpu_virt_addr;
        input_stream.SurfaceAttrib.SurfaceSize = (fb->width) | (fb->height << 16);
#if DRM_VERSION_CODE < KERNEL_VERSION(4, 10, 0)
        input_stream.SurfaceAttrib.SurfaceFmt = DrmFormat2CBiosFormat(fb->pixel_format, fb->modifier[0]);
#elif DRM_VERSION_CODE < KERNEL_VERSION(4, 11, 0)
        input_stream.SurfaceAttrib.SurfaceFmt = DrmFormat2CBiosFormat(fb->pixel_format, fb->modifier);
#else
        input_stream.SurfaceAttrib.SurfaceFmt = DrmFormat2CBiosFormat(fb->format->format, fb->modifier);
#endif
        input_stream.SurfaceAttrib.Pitch = fb->pitches[0];
        input_stream.SurfaceAttrib.bCompress = (gfb->obj->info.compress_format != 0);
        if(input_stream.SurfaceAttrib.bCompress)
        {
            input_stream.SurfaceAttrib.BLIndex = gfb->obj->info.bl_slot_index;
            input_stream.SurfaceAttrib.Range_Type = gfb->obj->info.compress_format;
        }

        input_stream.SrcWindow.Position = arg->src_x | (arg->src_y << 16);
        input_stream.SrcWindow.WinSize = arg->src_w | (arg->src_h << 16);

        input_stream.DispWindow.Position = arg->crtc_x | (arg->crtc_y << 16);
        input_stream.DispWindow.WinSize = arg->crtc_w | (arg->crtc_h << 16);

        if(disp_plane.PlaneIndex == CBIOS_STREAM_PS)
        {
            disp_plane.pOverlayInfo = &overlay;
            overlay.KeyMode = CBIOS_WINDOW_KEY;
            overlay.WindowKey.Ka = 0;
            overlay.WindowKey.Kb = 8;
        }
#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 8, 0)
        else
        {
            disp_plane.pOverlayInfo = &overlay;
            if(arg->blend_mode == DRM_MODE_BLEND_PIXEL_NONE)
            {
                overlay.KeyMode = CBIOS_CONSTANT_ALPHA;
                overlay.ConstantAlphaBlending.ConstantAlpha = (CBIOS_U8)(arg->const_alpha >> 8);
            }
            else if(arg->blend_mode == DRM_MODE_BLEND_COVERAGE)  //coverage with plane alpha
            {
                overlay.KeyMode = CBIOS_ALPHA_BLENDING;
                overlay.AlphaBlending.bUseAAlpha = (arg->blend_alpha_source)? 1 : 0;
                overlay.AlphaBlending.bPremulBlend = 0;
                overlay.AlphaBlending.bUsePlaneAlpha = 1;
                overlay.AlphaBlending.PlaneValue = (CBIOS_U8)(arg->plane_alpha >> 8);
            }
            else  //premultied with plane alpha
            {
                overlay.KeyMode = CBIOS_ALPHA_BLENDING;
                overlay.AlphaBlending.bUseAAlpha = (arg->blend_alpha_source)? 1 : 0;
                overlay.AlphaBlending.bPremulBlend = 1;
                overlay.AlphaBlending.bUsePlaneAlpha = 1;
                overlay.AlphaBlending.PlaneValue = (CBIOS_U8)(arg->plane_alpha >> 8);
            }
        }
#endif

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 12, 0)
        if(arg->async_flip)
        {
            disp_plane.FlipMode.FlipImme = 1;
        }
#endif
    }

    return (CBiosUpdateFrame(disp_info->cbios_ext, &update_frame) == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}


int disp_cbios_update_cursor(disp_info_t *disp_info, gf_cursor_update_t *arg)
{
    CBIOS_CURSOR_PARA  update_cursor = {0};

    //update_cursor.bVsyncOn = arg->vsync_on;
    if (arg->bo)
    {
        update_cursor.bDisable = 0;
        if (arg->width == 64 && arg->height == 64)
        {
            update_cursor.Position.CursorSize = CBIOS_CURSOR_SIZE_64x64;
        }
        else if (arg->width == 128 && arg->height == 128)
        {
            update_cursor.Position.CursorSize = CBIOS_CURSOR_SIZE_128x128;
        }
        update_cursor.CursorAttrib.Type = CBIOS_PREMULT_CURSOR;
        update_cursor.CursorAttrib.CurAddr = arg->bo->info.gpu_virt_addr;
        update_cursor.Position.PositionX = arg->pos_x;
        update_cursor.Position.PositionY = arg->pos_y;
    }
    else
    {
        update_cursor.bDisable = 1;
    }

    update_cursor.Size = sizeof(update_cursor);
    update_cursor.IGAIndex = arg->crtc;

    return  (CBiosSetCursor(disp_info->cbios_ext, &update_cursor) == CBIOS_OK)? DISP_OK : DISP_FAIL;
}

int disp_cbios_get_interrupt_info(disp_info_t *disp_info, unsigned int *interrupt_mask)
{
    int status = CBIOS_OK;
    CBIOS_INTERRUPT_INFO interrupt_info = {0};

    status = CBiosGetInterruptInfo(disp_info->cbios_ext, &interrupt_info);
/*
    if(interrupt_info.InterruptType & CBIOS_VSYNC_1_INT)
    {
        *interrupt_mask |= VSYNC_1_INT;
    }

    if(interrupt_info.InterruptType & CBIOS_VSYNC_2_INT)
    {
        *interrupt_mask |= VSYNC_2_INT;
    }

    if(interrupt_info.InterruptType & CBIOS_VSYNC_3_INT)
    {
        *interrupt_mask |= VSYNC_3_INT;
    }

    if(interrupt_info.InterruptType & CBIOS_VSYNC_4_INT)
    {
        *interrupt_mask |= VSYNC_4_INT;
    }

    if(interrupt_info.InterruptType & CBIOS_DP_1_INT)
    {
        *interrupt_mask |= DP_1_INT;
    }

    if(interrupt_info.InterruptType & CBIOS_DP_2_INT)
    {
        *interrupt_mask |= DP_2_INT;
    }

    if(interrupt_info.InterruptType & CBIOS_DP_3_INT)
    {
        *interrupt_mask |= DP_3_INT;
    }

    if(interrupt_info.InterruptType & CBIOS_DP_4_INT)
    {
        *interrupt_mask |= DP_4_INT;
    }

    if(interrupt_info.InterruptType & CBIOS_HDCP_INT)
    {
        *interrupt_mask |= INT_HDMI_CP;
    }

    if(interrupt_info.InterruptType & CBIOS_CEC_INT)
    {
        *interrupt_mask |= INT_CEC_CONTROL;
    }

    if(interrupt_info.InterruptType & CBIOS_HDA_CODEC_INT)
    {
        *interrupt_mask |= INT_HDCODEC;
    }*/

    if((interrupt_info.AdvancedIntType & CBIOS_FENCE_INT)) /* ||
        (interrupt_info.AdvancedIntType & CBIOS_PAGE_FAULT_INT) ||
        (interrupt_info.AdvancedIntType & CBIOS_MXU_INVALID_ADDR_FAULT_INT))*/  //need refine
    {
        *interrupt_mask |= INT_FENCE;
    }

    return (status == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}

int   disp_cbios_get_dpint_type(disp_info_t *disp_info,unsigned int device)
{
    CBIOS_DP_INT_PARA DpIntPara = {0};
    int hpd = DP_HPD_NONE;

    DpIntPara.Size = sizeof(CBIOS_DP_INT_PARA);
    DpIntPara.DeviceId = device;
    if(CBIOS_OK == CBiosGetDPIntType(disp_info->cbios_ext, &DpIntPara))
    {
        hpd = DpIntPara.IntType;
    }

    return hpd;
}

int  disp_cbios_handle_dp_irq(disp_info_t *disp_info, unsigned int device, int int_type, int* need_detect, int* need_comp_edid)
{
    CBIOS_DP_HANDLE_IRQ_PARA  handle_irq = {0};
    int  status = DISP_FAIL;

    handle_irq.Size = sizeof(CBIOS_DP_HANDLE_IRQ_PARA);
    handle_irq.DeviceId = device;
    handle_irq.IntType = int_type;
    if(CBIOS_OK == CBiosHandleDPIrq(disp_info->cbios_ext, &handle_irq))
    {
        status = DISP_OK;
        if(need_detect)
        {
            *need_detect = handle_irq.bNeedDetect;
        }

        if(need_comp_edid)
        {
            *need_comp_edid = handle_irq.bNeedCompEdid;
        }
    }

    return status;
}


void disp_cbios_dump_registers(disp_info_t *disp_info, int type)
{
    CBIOS_DUMP_PARA para = {0};

    para.Size = sizeof(CBIOS_DUMP_PARA);

    para.DumpType = CBIOS_DUMP_VCP_INFO  |
                    CBIOS_DUMP_MODE_INFO |
                    CBIOS_DUMP_CLOCK_INFO;

    if(type &  DUMP_REGISTER_STREAM)
    {
        para.DumpType |= CBIOS_DUMP_PS1_REG |
                         CBIOS_DUMP_PS2_REG |
                         CBIOS_DUMP_SS1_REG |
                         CBIOS_DUMP_SS2_REG |
                         CBIOS_DUMP_TS1_REG |
                         CBIOS_DUMP_TS2_REG;
    }

    CBiosDumpInfo(disp_info->cbios_ext, &para);
}

int disp_cbios_set_hda_codec(disp_info_t *disp_info, gf_connector_t*  gf_connector)
{
    void                        *pcbe = disp_info->cbios_ext;
    CBIOS_HDAC_PARA             CbiosHDACPara = {0};
    int                         cb_status = CBIOS_OK;

    if((gf_connector->monitor_type == UT_OUTPUT_TYPE_HDMI) ||
       (gf_connector->monitor_type == UT_OUTPUT_TYPE_MHL) ||
       (gf_connector->monitor_type == UT_OUTPUT_TYPE_DP))
    {
        CbiosHDACPara.Size      = sizeof(CBIOS_HDAC_PARA);
        CbiosHDACPara.DeviceId = gf_connector->output_type;

        cb_status = CBiosSetHDACodecPara(pcbe, &CbiosHDACPara);
    }
    return (cb_status == CBIOS_OK) ?  DISP_OK : DISP_FAIL;
}

int disp_cbios_hdcp_isr(disp_info_t *disp_info, gf_connector_t*  gf_connector)
{
    void                        *pcbe = disp_info->cbios_ext;
    CBIOS_HDCP_ISR_PARA         CbiosHdcpIsrPara = {0};
    int                         cb_status = CBIOS_OK;

    if(gf_connector->monitor_type == UT_OUTPUT_TYPE_HDMI || gf_connector->monitor_type == UT_OUTPUT_TYPE_DVI)
    {
        CbiosHdcpIsrPara.Size      = sizeof(CBIOS_HDCP_ISR_PARA);
        CbiosHdcpIsrPara.DevicesId = gf_connector->output_type;

        cb_status = CBiosHDCPIsr(pcbe, &CbiosHdcpIsrPara);
    }
    return (cb_status == CBIOS_OK) ?  DISP_OK : DISP_FAIL;
}


int disp_cbios_get_hdmi_audio_format(disp_info_t *disp_info, unsigned int device_id, gf_hdmi_audio_formats *audio_formats)
{
    void                          *pcbe            = disp_info->cbios_ext;

    unsigned int                  buffer_size      = 0;
    unsigned int                  real_buffer_size = 0;
    unsigned int                  num_formats      = 0;
    unsigned int                  i                = 0;
    CBiosHDMIAudioFormat         *cb_audio_formats = NULL;
    CBIOS_STATUS                  cb_status        = CBIOS_OK;

    cb_status = CBiosGetHDMIAudioFomatListBufferSize(pcbe, device_id, &buffer_size);
    if ((cb_status != CBIOS_OK) || (buffer_size == 0))
    {
        gf_error("can not get the audio format buffer size\n");
        return DISP_FAIL;
    }

    cb_audio_formats = gf_calloc(buffer_size);
    if (cb_audio_formats == NULL)
    {
        return DISP_FAIL;
    }

    num_formats = buffer_size / sizeof(CBiosHDMIAudioFormat);
    for(i = 0; i < num_formats; i++)
    {
       cb_audio_formats[i].Size = sizeof(CBiosHDMIAudioFormat);
    }

    cb_status = CBiosGetHDMIAudioFomatList(pcbe, device_id, cb_audio_formats, &real_buffer_size);

    if ((cb_status != CBIOS_OK) || (real_buffer_size == 0))
    {
        gf_error("can not get the audio format\n");
        gf_free(cb_audio_formats);
        return DISP_FAIL;
    }

    if (num_formats > sizeof(audio_formats->audio_formats)/sizeof(gf_hdmi_audio_format_t))
    {
        gf_error("the num of audio formats exceeds the buffer size, so cut it.");
        num_formats = sizeof(audio_formats->audio_formats)/sizeof(gf_hdmi_audio_format_t);
    }

    audio_formats->num_formats = num_formats;
    for (i = 0; i < num_formats; i++)
    {
        audio_formats->audio_formats[i].format = (GF_HDMI_AUDIO_FORMAT_TYPE)cb_audio_formats[i].Format;
        audio_formats->audio_formats[i].max_channel_num= cb_audio_formats[i].MaxChannelNum;
        audio_formats->audio_formats[i].sample_rate_unit = cb_audio_formats[i].SampleRateUnit;
        audio_formats->audio_formats[i].unit = cb_audio_formats[i].Unit;
    }

    if (cb_audio_formats)
    {
        gf_free(cb_audio_formats);
        cb_audio_formats = NULL;
    }
    return DISP_OK;
}

void disp_cbios_reset_hw_block(disp_info_t *disp_info, gf_hw_block hw_block)
{
    CBIOS_HW_BLOCK  cbios_hw_block = {0};

    if(hw_block == GF_HW_IGA)
    {
        cbios_hw_block = CBIOS_HW_IGA;
    }
    else
    {
        cbios_hw_block = CBIOS_HW_NONE;
    }

    CBiosResetHWBlock(disp_info->cbios_ext, cbios_hw_block);
}

int disp_cbios_get_counter(disp_info_t* disp_info, gf_get_counter_t* get_counter)
{
    CBIOS_GET_HW_COUNTER   CbiosGetHwCnt = {0};
    int  status = DISP_FAIL;

    if(!get_counter || get_counter->crtc_index >= disp_info->num_crtc)
    {
        return  status;
    }

    CbiosGetHwCnt.IgaIndex = get_counter->crtc_index;

    if(get_counter->hpos)
    {
        CbiosGetHwCnt.bGetPixelCnt = 1;
    }

    if(get_counter->vpos)
    {
        CbiosGetHwCnt.bGetLineCnt = 1;
    }

    if(get_counter->vblk)
    {
        CbiosGetHwCnt.bGetFrameCnt = 1;
    }

    if(CBIOS_OK == CBiosGetHwCounter(disp_info->cbios_ext, &CbiosGetHwCnt))
    {
        status = DISP_OK;
    }
    if(get_counter->hpos)
    {
        *get_counter->hpos = (status == DISP_OK)? CbiosGetHwCnt.Value[CBIOS_COUNTER_PIXEL] : 0;
    }
    if(get_counter->vpos)
    {
        *get_counter->vpos = (status == DISP_OK)? CbiosGetHwCnt.Value[CBIOS_COUNTER_LINE] : 0;
    }
    if(get_counter->vblk)
    {
        *get_counter->vblk = (status == DISP_OK)? CbiosGetHwCnt.Value[CBIOS_COUNTER_FRAME] : 0;
    }
    if(get_counter->in_vblk)
    {
        *get_counter->in_vblk = (status == DISP_OK && CbiosGetHwCnt.bInVblank)? 1 : 0;
    }

    return status;
}

int disp_get_vip_capture_fmt(unsigned int fmt)
{
    unsigned int capture_fmt = GF_VIP_FMT_RGB444_24BIT_SDR;

    switch (fmt)
    {
    case CBIOS_VIP_FMT_RGB444_24BIT_SDR:
    {
        capture_fmt = GF_VIP_FMT_RGB444_24BIT_SDR;
    }
    break;

    case CBIOS_VIP_FMT_YCBCR444_24BIT_SDR:
    {
        capture_fmt = GF_VIP_FMT_YCBCR444_24BIT_SDR;
    }
    break;

    case CBIOS_VIP_FMT_YCBCR422_8BIT_SDR_ES:
    {
        capture_fmt = GF_VIP_FMT_YCBCR422_8BIT_SDR_ES;
    }
    break;

    case CBIOS_VIP_FMT_YCBCR422_8BIT_DDR_ES:
    {
        capture_fmt = GF_VIP_FMT_YCBCR422_8BIT_DDR_ES;
    }
    break;


    default:
    {
        gf_error("not supported yet, default to RGB444\n");
    }
    break;

    }

    return capture_fmt;

}

int disp_get_wb_capture_fmt(unsigned int fmt)
{
    unsigned int capture_fmt = GF_WB_FMT_RGB888;

    switch (fmt)
    {
    case CBIOS_RGBOUTPUT:
    {
        capture_fmt = GF_WB_FMT_RGB888;
    }
    break;

    default:
    {
        capture_fmt = GF_WB_FMT_RGB888;
        gf_error("not supported fmt %d, default to RGB\n", fmt);
    }
    break;

    }

    return capture_fmt;

}

int disp_cbios_vip_ctl(disp_info_t *disp_info, gf_vip_set_t *v_set)
{
    void *pcbe = disp_info->cbios_ext;
    CBIOS_VIP_CTRL_DATA  cbios_vip_params = {0};
    int cb_status = CBIOS_OK;
    int i = 0;

    cbios_vip_params.vip = v_set->vip;
    cbios_vip_params.cmd = CBIOS_VIP_NOP;

    switch (v_set->op)
    {
    case GF_VIP_ENABLE:
    case GF_VIP_DISABLE:
    {
        cbios_vip_params.cmd = CBIOS_VIP_ENABLE;
        cbios_vip_params.enable = v_set->op == GF_VIP_ENABLE ? 1 : 0;
    }
    break;

    case GF_VIP_QUERY_CAPS:
    {
        cbios_vip_params.cmd = CBIOS_VIP_QUERY_CAPS;
    }
    break;

    case GF_VIP_SET_MODE:
    {
        cbios_vip_params.cmd = CBIOS_VIP_SET_MODE;
        cbios_vip_params.modeSet.fmt = CBIOS_VIP_FMT_RGB444_24BIT_SDR;
        cbios_vip_params.modeSet.vCard = v_set->mode.chip;
        cbios_vip_params.modeSet.xRes  = v_set->mode.x_res;
        cbios_vip_params.modeSet.yRes  = v_set->mode.y_res;
        cbios_vip_params.modeSet.refs  = v_set->mode.refs;
    }
    break;

    case GF_VIP_SET_BUFFER:
    {
        cbios_vip_params.cmd = CBIOS_VIP_SET_BUFFER;
        cbios_vip_params.fbSet.num  = v_set->fb.fb_num;
        cbios_vip_params.fbSet.idx  = v_set->fb.fb_idx;
        cbios_vip_params.fbSet.addr = v_set->fb.fb_addr;
    }
    break;

    default:
    {
        gf_error("unspported vip set cmd: %d", v_set->op);
    }
    break;

    }

    if (cbios_vip_params.cmd != CBIOS_VIP_NOP)
    {
        cb_status = CBiosVIPCtl(pcbe, &cbios_vip_params);
    }

    if (cbios_vip_params.cmd == CBIOS_VIP_QUERY_CAPS && cb_status == CBIOS_OK)
    {
        v_set->caps.mode_num = cbios_vip_params.caps.supportModeNum;
        if (v_set->caps.mode != NULL)
        {
            for (; i < v_set->caps.mode_num; i++)
            {
                v_set->caps.mode[i].xRes = cbios_vip_params.caps.mode[i].xRes;
                v_set->caps.mode[i].yRes = cbios_vip_params.caps.mode[i].yRes;
                v_set->caps.mode[i].refreshrate = cbios_vip_params.caps.mode[i].refs;
                v_set->caps.mode[i].fmt = disp_get_vip_capture_fmt(cbios_vip_params.caps.mode[i].fmt);
            }
        }
    }

    return (cb_status == CBIOS_OK) ?  DISP_OK : DISP_FAIL;
}

int disp_cbios_wb_ctl(disp_info_t *disp_info, gf_wb_set_t *wb_set)
{
    void *pcbe = disp_info->cbios_ext;
    CBIOS_WB_PARA cbios_wb_params = {0};
    CBiosSettingModeParams cbios_mode = {0};
    int cb_status = CBIOS_OK;

    if (wb_set->op_flags & GF_WB_QUERY_CAPS)
    {
        cbios_mode.IGAIndex = wb_set->iga_idx;
        CBiosGetModeFromIGA(pcbe, &cbios_mode);

        wb_set->input_mode.src_x = cbios_mode.DestModeParams.XRes;
        wb_set->input_mode.src_y = cbios_mode.DestModeParams.YRes;
        wb_set->input_mode.refreshrate = cbios_mode.DestModeParams.RefreshRate;
        wb_set->input_mode.capture_fmt = disp_get_wb_capture_fmt(cbios_mode.DestModeParams.OutputSignal);

        return cb_status;
    }

    cbios_wb_params.IGAIndex = wb_set->iga_idx;

    if (wb_set->op_flags & GF_WB_SET_BYPASS_MODE)
    {
        cbios_wb_params.bByPass   = 1;
    }

    if (wb_set->op_flags & GF_WB_SET_VSYNC_OFF)
    {
        cbios_wb_params.bUpdateImme   = 1;
    }

    if ((wb_set->op_flags & GF_WB_ENABLE) || (wb_set->op_flags & GF_WB_DISABLE))
    {
        cbios_wb_params.bEnableOP = 1;
        cbios_wb_params.bEnable = (wb_set->op_flags & GF_WB_ENABLE) ? 1 : 0;
    }

    if (wb_set->op_flags & GF_WB_SET_MODE)
    {
        cbios_wb_params.bSetModeOP = 1;
        cbios_wb_params.SrcSize = (wb_set->mode.src_x & 0xFFFF) | ((wb_set->mode.src_y & 0xFFFF) << 16);
        //FIXME: to refine the fmt.
        cbios_wb_params.SrcFmt = CSC_FMT_RGB;
        cbios_wb_params.CscOutFmt = CSC_FMT_RGB;

        if (wb_set->op_flags & GF_WB_SET_DOWNSCALER_MODE)
        {
            cbios_wb_params.DSCL.Mode = CBIOS_WB_P2P;
            cbios_wb_params.DSCL.DstSize = (wb_set->mode.dst_x & 0xFFFF) | ((wb_set->mode.dst_x & 0xFFFF) << 16);
            cbios_wb_params.DSCL.bDoubleBuffer = wb_set->mode.double_buf;
        }
    }

    if (wb_set->op_flags & GF_WB_SET_BUFFER)
    {
        cbios_wb_params.bSetAddrOP = 1;
        cbios_wb_params.DstBaseAddr = wb_set->fb.fb_addr;
    }

    cb_status = CBiosSetWriteback(pcbe, &cbios_wb_params);

    return (cb_status == CBIOS_OK) ?  DISP_OK : DISP_FAIL;
}

int disp_wait_idle(void *_disp_info)
{
    disp_info_t *disp_info = _disp_info;
    void *pcbe = NULL;
    unsigned int i = 0;
    int cb_status = CBIOS_OK;

    if (!disp_info)
    {
        gf_info("why disp_info is null, 0x%x  0x%x\n",disp_info,_disp_info);
        return -EINVAL;
    }

    pcbe = disp_info->cbios_ext;

    while (i < disp_info->num_crtc)
    {
        if (disp_info->active_output[i])
        {
            break;
        }
        i++;
    }

    //only wait the first active crtc's vblank
    if (i != disp_info->num_crtc)
    {
        cb_status = CBiosWaitVBlank(pcbe, i);
    }

    return (cb_status == CBIOS_TRUE) ? DISP_OK : DISP_FAIL;
}

static unsigned int cal_bits(unsigned int v)
{
    unsigned int n = 0;

    while(v)
    {
        if(v & 0x1)
            n++;
        v >>= 1;
    }
    return n;
}

static unsigned int validate_chip_slice_mask(unsigned int org)
{
    int aval_efuse_slice = org & 0xFFF;
    unsigned int per_gpc_slice = 0;
    unsigned int gpc_idx = 0;
    unsigned int new_slice = 0;

    while(aval_efuse_slice > 0)
    {
        per_gpc_slice = aval_efuse_slice & 0xF;

        new_slice += per_gpc_slice << (gpc_idx * 4);
        gpc_idx++;
        aval_efuse_slice >>= 4;
    }

    if(new_slice == 0) new_slice = 1;

    return new_slice;
}

int disp_cbios_get_slice_num(disp_info_t *disp_info)
{
    adapter_info_t* adapter_info = disp_info->adp_info;
    void *pcbe = disp_info->cbios_ext;
    signed char  slice_num = 0;
    unsigned int efuse_slice_disable_mask = 0, efuse_slice_enable_mask = 0;
    unsigned int total_usable_slice_num = 0;
    unsigned int final_slice_mask = 0;
    unsigned int gpc_slice_bits = 0, gpc_slice_mask = 0, gpc_idx = 0;

    int status = DISP_OK;

    if(CBIOS_OK != CBiosGetSliceNum(pcbe, &slice_num, &efuse_slice_disable_mask))
    {
        status = DISP_FAIL;
    }

    //efuse use bit1~bit12, bit0 not use.
    efuse_slice_disable_mask &= 0x1FFF;
    efuse_slice_disable_mask >>=1;

    if(efuse_slice_disable_mask >= 0xFFF){
        efuse_slice_disable_mask = 0;
        gf_info("efuse disable all slice, ignore efuse disable bits\n");
    }

    efuse_slice_enable_mask = validate_chip_slice_mask((~efuse_slice_disable_mask) & 0xFFF);

    gf_info("disp_cbios_get_slice_num: get slice_num: %d, efuse enable slice: %x\n", slice_num, efuse_slice_enable_mask);

    total_usable_slice_num = cal_bits(efuse_slice_enable_mask & 0xFFF);

    if(slice_num <= 0 ||slice_num > total_usable_slice_num){
        gf_info("cbios get invalidate slice enable num, force to efuse's slice num: %d\n", total_usable_slice_num);
        slice_num = total_usable_slice_num;
    }

    while(slice_num > 0 && gpc_idx < 3)
    {
        gpc_slice_mask =  efuse_slice_enable_mask & 0xF;
        gpc_slice_bits   = cal_bits(gpc_slice_mask);

        if(slice_num < gpc_slice_bits)
        {
             //if(slice_num == 4), not likely, since nax value for gpc_slice_bits is 4
             if(slice_num == 2 ||slice_num == 3){
                gpc_slice_mask = 0x3;
                gpc_slice_bits = 2;
             }else if(slice_num == 1){
                gpc_slice_mask = 0x1;
                gpc_slice_bits = 1;
             }
        }

        final_slice_mask += (gpc_slice_mask << (gpc_idx*4));

        efuse_slice_enable_mask >>= 4;

        slice_num -=gpc_slice_bits;

        gpc_idx++;
    }

    adapter_info->chip_slice_mask = validate_chip_slice_mask(final_slice_mask);
    gf_info("final slice mask:%x\n", adapter_info->chip_slice_mask);

    return status;
}

int disp_cbios_i2c_ctrl(disp_info_t *disp_info, gf_i2c_param_t *i2c_param)
{
    CBIOS_PARAM_I2C_DATA i2c_data = {0};
    int cb_status = CBIOS_OK;
    i2c_data.bUseDevType = i2c_param->use_dev_type;
    i2c_data.DeviceId = i2c_param->device_id;
    i2c_data.PortNumber = i2c_param->port;
    i2c_data.SlaveAddress = i2c_param->slave_addr;
    i2c_data.OffSet = i2c_param->offset;
    i2c_data.Buffer = i2c_param->buf;
    i2c_data.BufferLen = i2c_param->buf_len;
    i2c_data.bHDCPEnable = i2c_param->use_hdcp;
    switch (i2c_param->request_type)
    {
    case GF_I2C_DDCCI:
        i2c_data.RequestType = CBIOS_I2CDDCCI;
        break;
    case GF_I2C_HDCP:
        i2c_data.RequestType = CBIOS_I2CHDCP;
        break;
    default:
        i2c_data.RequestType = CBIOS_I2CNORMAL;
        break;
    }
    if (i2c_param->op == GF_I2C_WRITE)
        cb_status =  CBiosI2CDataWrite(disp_info->cbios_ext, &i2c_data);
    else
        cb_status =  CBiosI2CDataRead(disp_info->cbios_ext, &i2c_data);
    return (cb_status == CBIOS_OK) ? DISP_OK : DISP_FAIL;
}

