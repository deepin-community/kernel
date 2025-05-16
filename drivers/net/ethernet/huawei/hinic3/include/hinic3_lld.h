/* SPDX-License-Identifier: GPL-2.0 */
/* Copyright(c) 2021 Huawei Technologies Co., Ltd */

#ifndef HINIC3_LLD_H
#define HINIC3_LLD_H

#include "hinic3_crm.h"

#define WAIT_TIME        1

#ifdef HIUDK_SDK

int hwsdk_set_vf_load_state(struct hinic3_lld_dev *lld_dev, bool vf_load_state);

int hwsdk_set_vf_service_load(struct hinic3_lld_dev *lld_dev, u16 service,
			      bool vf_srv_load);

int hwsdk_set_vf_service_state(struct hinic3_lld_dev *lld_dev, u16 vf_func_id,
			       u16 service, bool en);
#else
struct hinic3_lld_dev {
	struct pci_dev *pdev;
	void *hwdev;
};

struct hinic3_uld_info {
	/* When the function does not need to initialize the corresponding uld,
	 * @probe needs to return 0 and uld_dev is set to NULL;
	 * if uld_dev is NULL, @remove will not be called when uninstalling
	 */
	int (*probe)(struct hinic3_lld_dev *lld_dev, void **uld_dev, char *uld_dev_name);
	void (*remove)(struct hinic3_lld_dev *lld_dev, void *uld_dev);
	int (*suspend)(struct hinic3_lld_dev *lld_dev, void *uld_dev, pm_message_t state);
	int (*resume)(struct hinic3_lld_dev *lld_dev, void *uld_dev);
	void (*event)(struct hinic3_lld_dev *lld_dev, void *uld_dev,
		      struct hinic3_event_info *event);
	int (*ioctl)(void *uld_dev, u32 cmd, const void *buf_in, u32 in_size,
		     void *buf_out, u32 *out_size);
};
#endif

#ifndef HIUDK_ULD
/* hinic3_register_uld - register an upper-layer driver
 * @type: uld service type
 * @uld_info: uld callback
 *
 * Registers an upper-layer driver.
 * Traverse existing devices and call @probe to initialize the uld device.
 */
int hinic3_register_uld(enum hinic3_service_type type, struct hinic3_uld_info *uld_info);

/**
 * hinic3_unregister_uld - unregister an upper-layer driver
 * @type: uld service type
 *
 * Traverse existing devices and call @remove to uninstall the uld device.
 * Unregisters an existing upper-layer driver.
 */
void hinic3_unregister_uld(enum hinic3_service_type type);

void lld_hold(void);
void lld_put(void);

/**
 * @brief hinic3_get_lld_dev_by_chip_name - get lld device by chip name
 * @param chip_name: chip name
 *
 * The value of lld_dev reference increases when lld_dev is obtained. The caller needs
 * to release the reference by calling hinic3_lld_dev_put.
 **/
struct hinic3_lld_dev *hinic3_get_lld_dev_by_chip_name(const char *chip_name);

/**
 * @brief hinic3_lld_dev_hold - get reference to lld_dev
 * @param dev: lld device
 *
 * Hold reference to device to keep it from being freed
 **/
void hinic3_lld_dev_hold(struct hinic3_lld_dev *dev);

/**
 * @brief hinic3_lld_dev_put - release reference to lld_dev
 * @param dev: lld device
 *
 * Release reference to device to allow it to be freed
 **/
void hinic3_lld_dev_put(struct hinic3_lld_dev *dev);

/**
 * @brief hinic3_get_lld_dev_by_dev_name - get lld device by uld device name
 * @param dev_name: uld device name
 * @param type: uld service type, When the type is SERVICE_T_MAX, try to match
 *	all ULD names to get uld_dev
 *
 * The value of lld_dev reference increases when lld_dev is obtained. The caller needs
 * to release the reference by calling hinic3_lld_dev_put.
 **/
struct hinic3_lld_dev *hinic3_get_lld_dev_by_dev_name(const char *dev_name,
						      enum hinic3_service_type type);

/**
 * @brief hinic3_get_lld_dev_by_dev_name_unsafe - get lld device by uld device name
 * @param dev_name: uld device name
 * @param type: uld service type, When the type is SERVICE_T_MAX, try to match
 *	all ULD names to get uld_dev
 *
 * hinic3_get_lld_dev_by_dev_name_unsafe() is completely analogous to
 * hinic3_get_lld_dev_by_dev_name(), The only difference is that the reference
 * of lld_dev is not increased when lld_dev is obtained.
 *
 * The caller must ensure that lld_dev will not be freed during the remove process
 * when using lld_dev.
 **/
struct hinic3_lld_dev *hinic3_get_lld_dev_by_dev_name_unsafe(const char *dev_name,
							     enum hinic3_service_type type);

/**
 * @brief hinic3_get_lld_dev_by_chip_and_port - get lld device by chip name and port id
 * @param chip_name: chip name
 * @param port_id: port id
 **/
struct hinic3_lld_dev *hinic3_get_lld_dev_by_chip_and_port(const char *chip_name, u8 port_id);

/**
 * @brief hinic3_get_ppf_dev - get ppf device without depend on input parameter
 **/
void *hinic3_get_ppf_dev(void);

/**
 * @brief hinic3_get_ppf_lld_dev - get ppf lld device by current function's lld device
 * @param lld_dev: current function's lld device
 *
 * The value of lld_dev reference increases when lld_dev is obtained. The caller needs
 * to release the reference by calling hinic3_lld_dev_put.
 **/
struct hinic3_lld_dev *hinic3_get_ppf_lld_dev(struct hinic3_lld_dev *lld_dev);

/**
 * @brief hinic3_get_ppf_lld_dev_unsafe - get ppf lld device by current function's lld device
 * @param lld_dev: current function's lld device
 *
 * hinic3_get_ppf_lld_dev_unsafe() is completely analogous to hinic3_get_ppf_lld_dev(),
 * The only difference is that the reference of lld_dev is not increased when lld_dev is obtained.
 *
 * The caller must ensure that ppf's lld_dev will not be freed during the remove process
 * when using ppf lld_dev.
 **/
struct hinic3_lld_dev *hinic3_get_ppf_lld_dev_unsafe(struct hinic3_lld_dev *lld_dev);

/**
 * @brief uld_dev_hold - get reference to uld_dev
 * @param lld_dev: lld device
 * @param type: uld service type
 *
 * Hold reference to uld device to keep it from being freed
 **/
void uld_dev_hold(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type);

/**
 * @brief uld_dev_put - release reference to lld_dev
 * @param dev: lld device
 * @param type: uld service type
 *
 * Release reference to uld device to allow it to be freed
 **/
void uld_dev_put(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type);

/**
 * @brief hinic3_get_uld_dev - get uld device by lld device
 * @param lld_dev: lld device
 * @param type: uld service type
 *
 * The value of uld_dev reference increases when uld_dev is obtained. The caller needs
 * to release the reference by calling uld_dev_put.
 **/
void *hinic3_get_uld_dev(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type);

/**
 * @brief hinic3_get_uld_dev_unsafe - get uld device by lld device
 * @param lld_dev: lld device
 * @param type: uld service type
 *
 * hinic3_get_uld_dev_unsafe() is completely analogous to hinic3_get_uld_dev(),
 * The only difference is that the reference of uld_dev is not increased when uld_dev is obtained.
 *
 * The caller must ensure that uld_dev will not be freed during the remove process
 * when using uld_dev.
 **/
void *hinic3_get_uld_dev_unsafe(struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type);

/**
 * @brief hinic3_get_chip_name - get chip name by lld device
 * @param lld_dev: lld device
 * @param chip_name: String for storing the chip name
 * @param max_len: Maximum number of characters to be copied for chip_name
 **/
int hinic3_get_chip_name(struct hinic3_lld_dev *lld_dev, char *chip_name, u16 max_len);

struct card_node *hinic3_get_chip_node_by_lld(struct hinic3_lld_dev *lld_dev);

struct hinic3_hwdev *hinic3_get_sdk_hwdev_by_lld(struct hinic3_lld_dev *lld_dev);

bool hinic3_get_vf_service_load(struct pci_dev *pdev, u16 service);

int hinic3_set_vf_service_load(struct pci_dev *pdev, u16 service,
			       bool vf_srv_load);

int hinic3_set_vf_service_state(struct pci_dev *pdev, u16 vf_func_id,
				u16 service, bool en);

bool hinic3_get_vf_load_state(struct pci_dev *pdev);

int hinic3_set_vf_load_state(struct pci_dev *pdev, bool vf_load_state);

int hinic3_attach_nic(struct hinic3_lld_dev *lld_dev);

void hinic3_detach_nic(const struct hinic3_lld_dev *lld_dev);

int hinic3_attach_service(const struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type);
void hinic3_detach_service(const struct hinic3_lld_dev *lld_dev, enum hinic3_service_type type);
const char **hinic3_get_uld_names(void);
int hinic3_lld_init(void);
void hinic3_lld_exit(void);
#endif
#endif
