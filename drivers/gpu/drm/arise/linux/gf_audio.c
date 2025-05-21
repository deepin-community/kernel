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
#include "gf_cbios.h"
#include "gf_driver.h"
#include "gf_audio.h"
#if  DRM_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)
#include <sound/hdaudio.h>
#endif

#if  DRM_VERSION_CODE >= KERNEL_VERSION(6, 14, 0)
static int gf_audio_match(struct device *dev, const void *data)
#else
static int gf_audio_match(struct device *dev, void *data)
#endif
{
    const int                *addr = data;

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
    int                pm_ret = 0, pm_req = 0;

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
                if(!(gf_card->flags & GF_SHUT_DOWN))
                {
                    pm_req = 1;
                }
                if (pm_req)
                {
                    pm_ret = pm_runtime_get_sync(dev); // set codec device to RPM_ACTIVE
                }
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
                if (pm_req)
                {
                    if (pm_ret < 0)
                    {
                        pm_runtime_put_noidle(dev);
                    }
                    else
                    {
                        pm_runtime_mark_last_busy(dev);
                        pm_runtime_put_autosuspend(dev);
                    }
                }
                put_device(dev);
            }

            break;
        }
    }
}


