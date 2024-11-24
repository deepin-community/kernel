//*****************************************************************************
//  Copyright (c) 2024 Glenfly Tech Co., Ltd..
//  All Rights Reserved.
//
//  This is UNPUBLISHED PROPRIETARY SOURCE CODE of Glenfly Tech Co., Ltd..;
//  the contents of this file may not be disclosed to third parties, copied or
//  duplicated in any form, in whole or in part, without the prior written
//  permission of Glenfly Tech Co., Ltd..
//
//  The copyright of the source code is protected by the copyright laws of the People's
//  Republic of China and the related laws promulgated by the People's Republic of China
//  and the international covenant(s) ratified by the People's Republic of China.
//*****************************************************************************


#include "gf_disp.h"
#include "gf_cbios.h"
#include "gf_driver.h"
#include "gf_audio.h"
#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
#include <sound/hdaudio.h>
#endif

static int gf_audio_match(struct device *dev, void *data)
{
    int                *addr = data;

#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
    struct hdac_device *hdev = dev_to_hdac_dev(dev);

    if ((hdev) && ((hdev->vendor_id >> 16) == 0x6766) && (hdev->addr == *addr))
    {
        return 1;
    }
    else
    {
        return 0;
    }
#else
    return 0;
#endif
}

struct device *gf_audio_find_device(gf_card_t *gf_card, int addr)
{
    struct pci_dev     *pdev = gf_card->pdev;
    struct pci_dev     *pdev_audio = NULL;

    pdev_audio = pci_get_slot(pdev->bus, PCI_DEVFN(PCI_SLOT(pdev->devfn), 1));
    if (!pdev_audio)
    {
        gf_info("Can't find the pci device of hdaudio controller.\n");
        return NULL;
    }

    return device_find_child(&(pdev_audio->dev), &addr, gf_audio_match);
}

void gf_audio_set_connect(gf_connector_t *gf_connector, int enable)
{
    struct drm_device  *drm_dev;
    gf_card_t          *gf_card;
    int                addr = 0;
    struct device      *dev = NULL;

    if (!gf_connector)
    {
        return;
    }

    drm_dev = gf_connector->base_connector.dev;
    gf_card = drm_dev->dev_private;

    for (addr = 0; addr < GF_DEFAULT_CODECS; addr++)
    {
        if (gf_connector->hda_codec_index & (1 << addr))
        {

            dev = gf_audio_find_device(gf_card, addr+1);  // Our codec addr in hdaudio driver is 1-based.

            if (dev)
            {
                pm_runtime_get_sync(dev); // set codec device to RPM_ACTIVE
            }

            if (enable && gf_connector->support_audio)
            {
                disp_cbios_set_hdac_connect_status((disp_info_t *)gf_card->disp_info, gf_connector->output_type, TRUE, TRUE);
            }
            else
            {
                disp_cbios_set_hdac_connect_status((disp_info_t *)gf_card->disp_info, gf_connector->output_type, FALSE, FALSE);
            }
            gf_msleep(1);

            if (dev)
            {
                pm_runtime_mark_last_busy(dev);
                pm_runtime_put_autosuspend(dev);
                put_device(dev);
            }

            break;
        }
    }

}


