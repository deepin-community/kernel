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
#include "gf.h"
#include "gf_disp.h"
#include "gf_cbios.h"
#include "gf_driver.h"
#include "gf_version.h"

/*
    DEVICE_ATTR() will do PERMISSIONS Check
    Kernel Code VERIFY_OCTAL_PERMISSIONS() not allowed to Give "Other User" Write permissions
*/

#define GF_DEVICE_ATTR_RO(name) \
    static DEVICE_ATTR(name,0444,gf_##name##_show,NULL)

#define GF_DEVICE_ATTR_RW(name) \
    static DEVICE_ATTR(name,0664,gf_##name##_show,gf_##name##_store)

#define GF_DEVICE_ATTR_WO(name) \
    static DEVICE_ATTR(name,0220,NULL,gf_##name##_store)

/*
    "gf_string_show" and "gf_string_store" is demo shows
    how to add sys read write file with GF_DEVICE_ATTR_RW()
*/

static char string_buffer [64] = {0};

static ssize_t gf_string_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    return sprintf(buf, "Store String:%s\n",string_buffer);
}

static ssize_t gf_string_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t len)
{
    if(len>=64)
    {
        len = 63;
    }
    memcpy(string_buffer,buf,len);
    string_buffer[len] = '\0';
    return len;
}

extern int hwq_get_hwq_info(void *adp,gf_hwq_info *hwq_info);
extern int hwq_get_video_info(void *adp, gf_video_info *video_info);
static ssize_t gf_enable_usage_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t len)
{
    int ret = 0;
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    unsigned int power_state;
    unsigned long value = 0;

    ret = gf_core_interface->get_power_state(gf_card->adapter, &power_state);
    if (ret == 0)
        return -EPERM;

    ret = kstrtoul(buf, 0, &value);
    if (ret < 0)
        return ret;

    value = (value == 0) ? 0 : 1;
    gf_core_interface->ctl_flags_set(gf_card->adapter, 1, (1UL << 16), (value << 16)); //First Int Bit16 hwq_event_enable
    if (value)
        gf_core_interface->ctl_flags_set(gf_card->adapter, 1, (1UL << 9), (value << 9)); //First Int Bit11, perf_event_enable

    return len;
}

static ssize_t gf_enable_perf_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t len)
{
    int ret = 0;
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    unsigned int power_state;
    unsigned long value = 0;

    ret = gf_core_interface->get_power_state(gf_card->adapter, &power_state);
    if (ret == 0)
        return -EPERM;

    ret = kstrtoul(buf, 0, &value);
    if (ret < 0)
        return ret;

    value = (value == 0) ? 0 : 1;
    gf_core_interface->ctl_flags_set(gf_card->adapter, 1, (1UL << 9), (value << 9)); //First Int Bit11 perf_event_enable
    if (value == 0)
        gf_core_interface->ctl_flags_set(gf_card->adapter, 1, (1UL << 16), (value << 16)); //First Int Bit16 hwq_event_enable

    return len;
}

static ssize_t gf_voltage_core_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    int voltage = 0;
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    adapter_info_t* adapter_info    = &gf_card->adapter_info;

    voltage = gf_read32(adapter_info->mmio + 0xd3d8);
    if (voltage)
        return sprintf(buf, "Voltage: %d mV\n", voltage);

    return sprintf(buf, "Voltage: read fail\n");
}

static ssize_t gf_engine_3d_usage_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    int ret = 0;
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;

    gf_hwq_info hwq_info;
    ret = hwq_get_hwq_info(gf_card->adapter,&hwq_info);

    if(ret == 0)
    {
        return sprintf(buf, "%d\n",hwq_info.Usage_3D);
    }
    return sprintf(buf, "Not Enable!\n");
}

static ssize_t gf_engine_vcp_usage_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    int ret = 0;
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;

    gf_hwq_info hwq_info;
    ret = hwq_get_hwq_info(gf_card->adapter,&hwq_info);

    if(ret == 0)
    {
        return sprintf(buf, "%d\n",hwq_info.Usage_VCP);
    }
    return sprintf(buf, "Not Enable!\n");

}

static ssize_t gf_engine_vcp_info_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    int ret = 0, i;
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    int buf_size = 0, size_offset = 0;
    int pid = 0;

    gf_sscanf(string_buffer, "%d", &pid);

    for (i = 0; i < VIDEO_INFO_NUM; i++)
    {
        gf_video_info* video_info = gf_calloc(sizeof(gf_video_info));
        gf_memset(video_info, 0, sizeof(gf_video_info));
        video_info->index = i;

        ret = hwq_get_video_info(gf_card->adapter, video_info);
        if (ret)
        {
            gf_free(video_info);
            return sprintf(buf, "Not Enable!\n");
        }

        if (pid == 0) { //get all info
            if (video_info->pid[video_info->index]) {
                //PID
                size_offset = sprintf(buf + buf_size, "pid:%d\n", video_info->pid[video_info->index]);
                buf_size += size_offset;
                //clip width
                size_offset = sprintf(buf + buf_size, "width:%d\n", video_info->width[video_info->index]);
                buf_size += size_offset;
                //clip height
                size_offset = sprintf(buf + buf_size, "height:%d\n", video_info->height[video_info->index]);
                buf_size += size_offset;
                //clip codec
                size_offset = sprintf(buf + buf_size, "codec:%s\n", video_info->codec[video_info->index]);
                buf_size += size_offset;
                 //decodeapi
                size_offset = sprintf(buf + buf_size, "decode_api:%s\n", video_info->decodeapi[video_info->index]);
                buf_size += size_offset;
                //renderapi
                size_offset = sprintf(buf + buf_size, "render_api:%s\n", video_info->presentapi[video_info->index]);
                buf_size += size_offset;
                //total decode frame
                size_offset = sprintf(buf + buf_size, "total_decodeframe:%d\n", video_info->TotalDecodeFrameNum[video_info->index]);
                buf_size += size_offset;
                //total render frame
                size_offset = sprintf(buf + buf_size, "total_renderframe:%d\n", video_info->TotalRenderFrameNum[video_info->index]);
                buf_size += size_offset;
                //presentspeed
                size_offset = sprintf(buf + buf_size, "average_presentspeed:%dFPS\n", video_info->presentspeed[video_info->index]);
                buf_size += size_offset;
                //bit rate
                size_offset = sprintf(buf + buf_size, "average_bitrate:%dKbPS\n", video_info->bitrate[video_info->index]);
                buf_size += size_offset;
                //gpu deocdespeed
                size_offset = sprintf(buf + buf_size, "average_decodespeed:%dFPS\n", video_info->decodespeed[video_info->index]);
                buf_size += size_offset;
                //others
            }
        } else {  //get pid info
            if (video_info->pid[video_info->index] == pid) {
                //PID
                size_offset = sprintf(buf + buf_size, "pid:%d\n", video_info->pid[video_info->index]);
                buf_size += size_offset;
                //clip width
                size_offset = sprintf(buf + buf_size, "width:%d\n", video_info->width[video_info->index]);
                buf_size += size_offset;
                //clip height
                size_offset = sprintf(buf + buf_size, "height:%d\n", video_info->height[video_info->index]);
                buf_size += size_offset;
                //clip codec
                size_offset = sprintf(buf + buf_size, "codec:%s\n", video_info->codec[video_info->index]);
                buf_size += size_offset;
                 //decodeapi
                size_offset = sprintf(buf + buf_size, "decode_api:%s\n", video_info->decodeapi[video_info->index]);
                buf_size += size_offset;
                //renderapi
                size_offset = sprintf(buf + buf_size, "render_api:%s\n", video_info->presentapi[video_info->index]);
                buf_size += size_offset;
                //total decode frame
                size_offset = sprintf(buf + buf_size, "total_decodeframe:%d\n", video_info->TotalDecodeFrameNum[video_info->index]);
                buf_size += size_offset;
                //total render frame
                size_offset = sprintf(buf + buf_size, "total_renderframe:%d\n", video_info->TotalRenderFrameNum[video_info->index]);
                buf_size += size_offset;
                //presentspeed
                size_offset = sprintf(buf + buf_size, "average_presentspeed:%dFPS\n", video_info->presentspeed[video_info->index]);
                buf_size += size_offset;
                //bit rate
                size_offset = sprintf(buf + buf_size, "average_bitrate:%dKbPS\n", video_info->bitrate[video_info->index]);
                buf_size += size_offset;
                //gpu deocdespeed
                size_offset = sprintf(buf + buf_size, "average_decodespeed:%dFPS\n", video_info->decodespeed[video_info->index]);
                buf_size += size_offset;
                //others
            }
        }
        gf_free(video_info);
    }

    //gf_info("buffer size %d, pid %d\n", buf_size, pid);
    return buf_size;
}

static ssize_t gf_engine_vpp_usage_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    int ret = 0;
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;

    gf_hwq_info hwq_info;
    ret = hwq_get_hwq_info(gf_card->adapter,&hwq_info);

    if(ret == 0)
    {
        return sprintf(buf, "%d\n",hwq_info.Usage_VPP);
    }
    return sprintf(buf, "Not Enable!\n");
}

static ssize_t gf_mclk_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    disp_info_t*       disp_info    = (disp_info_t*)gf_card->disp_info;

    unsigned int value = 0;
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_MCLK, &value))
    {
        value = gf_calc_double_standard_mclk(value);
        return sprintf(buf,"%dMHz\n", value / 2);
    }
    return sprintf(buf, "Mclk Read Error\n");
}

static ssize_t gf_fps_count_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    unsigned long long flip_timestamp;
    gf_get_nsecs(&flip_timestamp);
    if(gf_card->fps_count)
    {
        return sprintf(buf,"%u %lld\n", gf_card->fps_count,flip_timestamp);
    }
    return sprintf(buf, "fps Read Error\n");
}

static ssize_t gf_rxa_blt_scn_cnt_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t len)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    if(len>=16)
    {
        len = 16;
    }

    sscanf(buf, "%d", &gf_card->rxa_blt_scn_cnt);
    //gf_info("gf_rxa_blt_scn_cnt_store set data %d",  gf_card->rxa_blt_scn_cnt);

    return len;
}


static ssize_t gf_rxa_blt_scn_cnt_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;

    //gf_info("gf_rxa_blt_scn_cnt_show get data %d",  gf_card->rxa_blt_scn_cnt);

    if(gf_card->rxa_blt_scn_cnt > 0)
    {
        return sprintf(buf,"%d\n", gf_card->rxa_blt_scn_cnt);
    }
    return sprintf(buf, "rxa_blt_scn_cnt Read Error\n");
}



static ssize_t gf_misc_ctl_flag_store(struct device *dev,struct device_attribute *attr, const char *buf,size_t len)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    if(len>=16)
    {
        len = 16;
    }

    sscanf(buf, "%d", &gf_card->misc_control_flag);
    //gf_info("gf_misc_ctl_flag_store set data %d, len %d",  gf_card->misc_control_flag, len);

    return len;
}


static ssize_t gf_misc_ctl_flag_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;

    //gf_info("gf_misc_ctl_flag_show get data %d",  gf_card->rxa_blt_scn_cnt);
    return sprintf(buf,"%d\n", gf_card->misc_control_flag);
}




static ssize_t gf_vclk_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    disp_info_t*       disp_info    = (disp_info_t*)gf_card->disp_info;

    unsigned int value = 0;
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_VCLK, &value))
    {
        return sprintf(buf,"%dMHz\n", (value + 500)/1000);
    }

    return sprintf(buf, "Vclk Read Error\n");
}

static ssize_t gf_coreclk_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct pci_dev *   pdev      = container_of(dev, struct pci_dev, dev);
    struct drm_device *drm_dev   = pci_get_drvdata(pdev);
    gf_card_t *        gf_card   = drm_dev->dev_private;
    disp_info_t *      disp_info = (disp_info_t *)gf_card->disp_info;

    unsigned int value = 0;
    if (DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_CORE_CLOCK, &value))
    {
        return sprintf(buf, "%dMHz\n", value / 1000);
    }

    return sprintf(buf, "Coreclk Read Error\n");
}

static ssize_t gf_eclk_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    disp_info_t*       disp_info    = (disp_info_t*)gf_card->disp_info;

    unsigned int value = 0;
    if(DISP_OK == disp_cbios_get_clock(disp_info, GF_QUERY_ENGINE_CLOCK, &value))
    {
        return sprintf(buf, "%dMHz\n", value / 1000);
    }

    return sprintf(buf, "Eclk Read Error\n");

}

static ssize_t gf_free_fb_mem_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;

    gf_query_info_t  query_info;
    int mem_total = 0;
    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 1;
    gf_core_interface->query_info(gf_card->adapter,&query_info);
    mem_total += query_info.signed_value;

    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 4;
    gf_core_interface->query_info(gf_card->adapter,&query_info);
    mem_total += query_info.signed_value;

    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 5;
    gf_core_interface->query_info(gf_card->adapter,&query_info);
    mem_total += query_info.signed_value;

    return sprintf(buf,"%d\n",mem_total);
}

static ssize_t gf_fb_size_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    adapter_info_t* adapter_info    = &gf_card->adapter_info;

    return sprintf(buf,"%d M\n",adapter_info->total_mem_size_mb);
}

static ssize_t gf_mem_usage_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    adapter_info_t* adapter_info    = &gf_card->adapter_info;

    gf_query_info_t  query_info;
    int mem_free_total = 0, mem_usage;
    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 1;
    gf_core_interface->query_info(gf_card->adapter, &query_info);
    mem_free_total += query_info.signed_value;

    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 4;
    gf_core_interface->query_info(gf_card->adapter, &query_info);
    mem_free_total += query_info.signed_value;

    query_info.type = GF_QUERY_SEGMENT_FREE_SIZE;
    query_info.argu = 5;
    gf_core_interface->query_info(gf_card->adapter, &query_info);
    mem_free_total += query_info.signed_value;

    mem_free_total /= 1024;

    mem_usage = ((adapter_info->total_mem_size_mb - mem_free_total) * 100) / adapter_info->total_mem_size_mb;

    return sprintf(buf,"%d%%\n", mem_usage);
}

static ssize_t gf_segment_info_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    gf_segment_mem_t seg_mem_data;
    gf_query_info_t  query_info;
    int segmenti = 0;
    int ret = 0;
    int print_offset = 0;

    for(segmenti = 0; segmenti < 6; segmenti ++)
    {
        gf_memset(&seg_mem_data, 0, sizeof(gf_segment_mem_t));
        query_info.type = GF_QUERY_SEGMENT_MEM_INFO;
        query_info.argu = (segmenti | 0x80000000);
        query_info.buf = &seg_mem_data;

        ret = gf_core_interface->query_info(gf_card->adapter,&query_info);
        if(ret < 0)
        {
            break;
        }
        else
        {
            print_offset += sprintf(buf + print_offset,"segment_id:%d,total_size:%d,used_size:%d    \n",
                seg_mem_data.segment_id,
                seg_mem_data.segment_total_size,
                seg_mem_data.segment_used_size);
        }
    }

    return print_offset;
}

static ssize_t gf_slice_mask_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    adapter_info_t* adapter_info    = &gf_card->adapter_info;

    return sprintf(buf,"0x%x\n",adapter_info->chip_slice_mask);
}

static ssize_t gf_miu_channel_num_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    adapter_info_t* adapter_info    = &gf_card->adapter_info;

    return sprintf(buf,"%d\n",adapter_info->chan_num);

}

static ssize_t gf_driver_version_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    return sprintf(buf,"%02x.%02x.%02x%s\n",DRIVER_MAJOR,DRIVER_MINOR,DRIVER_PATCHLEVEL,DRIVER_CLASS);
}

static ssize_t gf_release_date_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    return sprintf(buf,"%s\n",DRIVER_DATE);
}

static ssize_t gf_pmp_version_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    disp_info_t*       disp_info    = (disp_info_t*)gf_card->disp_info;

    unsigned char* pmpversion       = disp_info->pmp_version;
    int  pmpdatelen                 = 0;
    int  pmptimelen                 = 0;

    if(*pmpversion)
    {
        // pmpVersion:str1 -> pmp version,str2 -> pmp build date,str3 -> pmp build time
        pmpdatelen = gf_strlen(pmpversion) + 1;
        pmptimelen = gf_strlen(pmpversion + pmpdatelen) + 1;
        return sprintf(buf, "%s %s %s\n",pmpversion,(pmpversion + pmpdatelen),(pmpversion  + pmpdatelen + pmptimelen) );
    }
    return sprintf(buf,"get error\n");

}

static ssize_t gf_vbios_version_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    disp_info_t*       disp_info    = (disp_info_t*)gf_card->disp_info;
    int  vbiosVer                   = disp_info->vbios_version;
    return sprintf(buf, "%02x.%02x.%02x.%02x\n", (vbiosVer>>24)&0xff,(vbiosVer>>16)&0xff,(vbiosVer>>8)&0xff,vbiosVer&0xff);
}

static ssize_t gf_task_timeout_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    unsigned long long timeout;
    int ret;

    gf_core_interface->task_timeout_update(gf_card->adapter, &timeout, 0);

    return sprintf(buf, "%llums\n", timeout);
}

static ssize_t gf_task_timeout_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    unsigned long long timeout;
    int ret;

    ret = sscanf(buf, "%llu", &timeout);
    if (ret <= 0)
        return -EINVAL;

    gf_core_interface->task_timeout_update(gf_card->adapter, &timeout, 1);

    return len;
}

static ssize_t gf_power_state_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    unsigned int level = 0;
    int ret;

    ret = gf_core_interface->get_power_state(gf_card->adapter, &level);
    if (ret)
        return -EPERM;

    return sprintf(buf, "E%u\n", level);
}

static ssize_t gf_power_state_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t len)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    unsigned int level = 0;
    int ret;

    ret = sscanf(buf, "%u", &level);
    if (ret <= 0)
        return -EINVAL;

    ret = gf_core_interface->set_power_state(gf_card->adapter, level, 300, TRUE, TRUE, TRUE);
    if (ret)
        return -EPERM;

    gf_debug("trying to fix power state.\n");

    return len;
}

static ssize_t gf_firmware_version_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    disp_info_t*       disp_info    = (disp_info_t*)gf_card->disp_info;
    int  fw_version                 = disp_info->firmware_version;

    return sprintf(buf, "%02x.%02x.%02x.%02x\n", (fw_version >> 24) & 0xff, (fw_version >> 16) & 0xff, (fw_version >> 8) & 0xff, fw_version & 0xff);
}

GF_DEVICE_ATTR_RW(string);
GF_DEVICE_ATTR_WO(enable_usage);
GF_DEVICE_ATTR_WO(enable_perf);

GF_DEVICE_ATTR_RO(voltage_core);
GF_DEVICE_ATTR_RO(engine_3d_usage);
GF_DEVICE_ATTR_RO(engine_vcp_usage);
GF_DEVICE_ATTR_RO(engine_vcp_info);
GF_DEVICE_ATTR_RO(engine_vpp_usage);
GF_DEVICE_ATTR_RO(vbios_version);
GF_DEVICE_ATTR_RO(pmp_version);
GF_DEVICE_ATTR_RO(release_date);
GF_DEVICE_ATTR_RO(driver_version);
GF_DEVICE_ATTR_RO(miu_channel_num);
GF_DEVICE_ATTR_RO(slice_mask);
GF_DEVICE_ATTR_RO(fb_size);
GF_DEVICE_ATTR_RO(free_fb_mem);
GF_DEVICE_ATTR_RO(mem_usage);
GF_DEVICE_ATTR_RO(coreclk);
GF_DEVICE_ATTR_RO(eclk);
GF_DEVICE_ATTR_RO(vclk);
GF_DEVICE_ATTR_RO(mclk);
GF_DEVICE_ATTR_RO(fps_count);
GF_DEVICE_ATTR_RW(rxa_blt_scn_cnt);
GF_DEVICE_ATTR_RW(misc_ctl_flag);
GF_DEVICE_ATTR_RO(segment_info);
GF_DEVICE_ATTR_RW(task_timeout);
GF_DEVICE_ATTR_RO(firmware_version);
GF_DEVICE_ATTR_RW(power_state);

static struct attribute *gf_info_attributes[] = {
    &dev_attr_string.attr,
    &dev_attr_enable_usage.attr,
    &dev_attr_enable_perf.attr,

    &dev_attr_voltage_core.attr,
    &dev_attr_engine_3d_usage.attr,
    &dev_attr_engine_vpp_usage.attr,
    &dev_attr_engine_vcp_usage.attr,
    &dev_attr_engine_vcp_info.attr,
    &dev_attr_vbios_version.attr,
    &dev_attr_pmp_version.attr,
    &dev_attr_release_date.attr,
    &dev_attr_driver_version.attr,
    &dev_attr_miu_channel_num.attr,
    &dev_attr_slice_mask.attr,
    &dev_attr_fb_size.attr,
    &dev_attr_free_fb_mem.attr,
    &dev_attr_mem_usage.attr,
    &dev_attr_coreclk.attr,
    &dev_attr_eclk.attr,
    &dev_attr_vclk.attr,
    &dev_attr_mclk.attr,
    &dev_attr_fps_count.attr,
    &dev_attr_rxa_blt_scn_cnt.attr,
    &dev_attr_misc_ctl_flag.attr,
    &dev_attr_segment_info.attr,
    &dev_attr_task_timeout.attr,
    &dev_attr_firmware_version.attr,
    &dev_attr_power_state.attr,
    NULL
};

const struct attribute_group gf_sysfs_group = {
    .attrs = gf_info_attributes,
    .name  = "gf_info"
};


/**
 * Format such as:
 * VRAM total size:0x80000000
*/
static ssize_t gf_gpu_info_show(struct device *dev, struct device_attribute *attr,char *buf)
{
    struct pci_dev*     pdev        = container_of(dev,struct pci_dev,dev);
    struct drm_device* drm_dev      = pci_get_drvdata(pdev);
    gf_card_t*         gf_card      = drm_dev->dev_private;
    adapter_info_t* adapter_info    = &gf_card->adapter_info;
    unsigned long long total_mem_size = adapter_info->total_mem_size_mb;
    total_mem_size                  = total_mem_size *1024*1024;

    return sprintf(buf,"VRAM total size:0x%llx\n",total_mem_size);
}

static struct device_attribute dev_attr_gpu_info = __ATTR(gpu-info,0444,gf_gpu_info_show,NULL);

const struct attribute *gf_os_gpu_info[] = {
    &dev_attr_gpu_info.attr,
    NULL
};

static ssize_t gf_sysfs_trace_read(struct file *filp, struct kobject *kobj, struct bin_attribute *bin_attr, char *buf, loff_t pos, size_t size)
{
    struct pci_dev*    pdev    = to_pci_dev(container_of(kobj,struct device,kobj));
    struct drm_device* drm_dev = pci_get_drvdata(pdev);
    gf_card_t*         gf_card = drm_dev->dev_private;
    ssize_t            ret     = 0;

    if (gf_card->trace_buffer_vma && gf_card->trace_buffer_vma->virt_addr)
    {
        char val_buf[32];
        unsigned int len;

        len = sprintf(val_buf, "%llu\n", *((uint64_t *)gf_card->trace_buffer_vma->virt_addr));
        ret = memory_read_from_buffer(buf, size, &pos, val_buf, len);
    }

    return ret;
}

static ssize_t gf_sysfs_trace_write(struct file *filp, struct kobject *kobj, struct bin_attribute *bin_attr, char *buf, loff_t pos, size_t size)
{
    struct pci_dev*    pdev    = to_pci_dev(container_of(kobj,struct device,kobj));
    struct drm_device* drm_dev = pci_get_drvdata(pdev);
    gf_card_t*         gf_card = drm_dev->dev_private;
    unsigned long val;
    int ret;

    if (!gf_card->trace_buffer_vma || !gf_card->trace_buffer_vma->virt_addr)
        return 0;

    ret = kstrtoul(buf, 0, &val);
    if (ret)
        return ret;

    *((uint64_t *)gf_card->trace_buffer_vma->virt_addr) = val;

    return size;
}

// for gdb (ptrace) to access this memory by access_process_vm
static int gf_sysfs_trace_access(struct vm_area_struct *vma, unsigned long addr, void *buf, int len, int write)
{
    gf_card_t* gf_card = vma->vm_private_data;
    unsigned long offset = addr - vma->vm_start;

    // gf_info("gf_sysfs_trace_access: vma=%p, addr=%lx, vma->vm_start=%llx\n", vma, addr, vma->vm_start);

    if (offset + len > gf_card->trace_buffer->size)
        return -EINVAL;

    if (!gf_card->trace_buffer_vma || !gf_card->trace_buffer_vma->virt_addr)
        return 0;

    if (write)
        gf_memcpy(gf_card->trace_buffer_vma->virt_addr + offset, buf, len);
    else
        gf_memcpy(buf, gf_card->trace_buffer_vma->virt_addr + offset, len);

    return len;
}

static const struct vm_operations_struct gf_sysfs_trace_vmops = {
    .access = gf_sysfs_trace_access,
};

extern unsigned char gf_validate_page_cache(struct os_pages_memory *memory, int start_page, int end_page, unsigned char request_cache_type);

#if DRM_VERSION_CODE < KERNEL_VERSION(6, 13, 0)
static int gf_sysfs_trace_mmap(struct file *filp, struct kobject *kobj,
                                   struct bin_attribute *attr,
                                   struct vm_area_struct *vma)
#else
static int gf_sysfs_trace_mmap(struct file *filp, struct kobject *kobj,
                                   const struct bin_attribute *attr,
                                   struct vm_area_struct *vma)
#endif
{
    struct pci_dev*    pdev    = to_pci_dev(container_of(kobj,struct device,kobj));
    struct drm_device* drm_dev = pci_get_drvdata(pdev);
    gf_card_t*         gf_card = drm_dev->dev_private;
    unsigned int       cache_type;

    gf_map_argu_t      map_argu = {0};
    int                start_page, end_page;

    // gf_info("gf_sysfs_trace_mmap: vma=%p, vma->vm_start=%llx\n", vma, vma->vm_start);

    vma->vm_ops = &gf_sysfs_trace_vmops;
    vma->vm_private_data = gf_card;

    map_argu.memory = gf_card->trace_buffer;
    map_argu.flags.mem_space = GF_MEM_USER;
    map_argu.flags.read_only = true;
    map_argu.flags.mem_type = GF_SYSTEM_RAM;
    map_argu.size = gf_card->trace_buffer->size;
    map_argu.offset = 0;
    start_page = _ALIGN_DOWN(map_argu.offset, PAGE_SIZE)/PAGE_SIZE;
    end_page = start_page + ALIGN(map_argu.size, PAGE_SIZE) / PAGE_SIZE;
    map_argu.flags.cache_type = gf_validate_page_cache(map_argu.memory, start_page, end_page, GF_MEM_WRITE_BACK);

    return gf_map_system_ram(vma, &map_argu);
}

struct bin_attribute gf_sysfs_trace_attr = {
    .attr = {.name = "trace", .mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH}, // 0644
    .size = PAGE_SIZE,
    .read = gf_sysfs_trace_read,
    .write = gf_sysfs_trace_write,
    .mmap = gf_sysfs_trace_mmap,
};
