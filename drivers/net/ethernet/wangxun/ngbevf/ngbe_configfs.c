/*******************************************************************************

  WangXun(R) GbE PCI Express Virtual Function Linux Network Driver
  Copyright(c) 2015 - 2017 Beijing WangXun Technology Co., Ltd.

  This program is free software; you can redistribute it and/or modify it
  under the terms and conditions of the GNU General Public License,
  version 2, as published by the Free Software Foundation.

  This program is distributed in the hope it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  The full GNU General Public License is included in this distribution in
  the file called "COPYING".

  Contact Information:
  Software Team <linux.nic@trustnetic.com>
  WangXun Technology, HuaXing Times Square A507, Hangzhou, China.

*******************************************************************************/


#include <linux/configfs.h>
#include "ngbe.h"

#if IS_ENABLED(CONFIG_CONFIGFS_FS) && (LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0))

/**
 * configfs structure for ngbe
 *
 * This file adds code for configfs support for the ngbe driver.  This sets
 * up a filesystem under /sys/kernel/config in which configuration changes
 * can be made for the driver's netdevs.
 *
 * The initialization in this code creates the "ngbe" entry in the configfs
 * system.  After that, the user needs to use mkdir to create configurations
 * for specific netdev ports; for example "mkdir eth3".  This code will verify
 * that such a netdev exists and that it is owned by ngbe.
 *
 **/

struct ngbe_cfgfs_vsi {
	struct config_item item;
	struct ngbe_vsi *vsi;
};

static inline struct ngbe_cfgfs_vsi *to_ngbe_cfgfs_vsi(struct config_item *item)
{
	return item ? container_of(item, struct ngbe_cfgfs_vsi, item) : NULL;
}

static struct configfs_attribute ngbe_cfgfs_vsi_attr_min_bw = {
	.ca_owner = THIS_MODULE,
	.ca_name = "min_bw",
	.ca_mode = S_IRUGO | S_IWUSR,
};

static struct configfs_attribute ngbe_cfgfs_vsi_attr_max_bw = {
	.ca_owner = THIS_MODULE,
	.ca_name = "max_bw",
	.ca_mode = S_IRUGO | S_IWUSR,
};

static struct configfs_attribute ngbe_cfgfs_vsi_attr_commit = {
	.ca_owner = THIS_MODULE,
	.ca_name = "commit",
	.ca_mode = S_IRUGO | S_IWUSR,
};

static struct configfs_attribute ngbe_cfgfs_vsi_attr_port_count = {
	.ca_owner = THIS_MODULE,
	.ca_name = "ports",
	.ca_mode = S_IRUGO | S_IWUSR,
};

static struct configfs_attribute ngbe_cfgfs_vsi_attr_part_count = {
	.ca_owner = THIS_MODULE,
	.ca_name = "partitions",
	.ca_mode = S_IRUGO | S_IWUSR,
};

static struct configfs_attribute *ngbe_cfgfs_vsi_attrs[] = {
	&ngbe_cfgfs_vsi_attr_min_bw,
	&ngbe_cfgfs_vsi_attr_max_bw,
	&ngbe_cfgfs_vsi_attr_commit,
	&ngbe_cfgfs_vsi_attr_port_count,
	&ngbe_cfgfs_vsi_attr_part_count,
	NULL,
};

/**
 * ngbe_cfgfs_vsi_attr_show - Show a VSI's NPAR BW partition info
 * @item: A pointer back to the configfs item created on driver load
 * @attr: A pointer to this item's configuration attribute
 * @page: A pointer to the output buffer
 **/
static ssize_t ngbe_cfgfs_vsi_attr_show(struct config_item *item,
				      struct configfs_attribute *attr,
				      char *page)
{
	struct ngbe_cfgfs_vsi *ngbe_cfgfs_vsi = to_ngbe_cfgfs_vsi(item);
	struct ngbe_pf *pf = ngbe_cfgfs_vsi->vsi->back;
	ssize_t count;

	if (ngbe_cfgfs_vsi->vsi != pf->vsi[pf->lan_vsi])
		return 0;

	if (strncmp(attr->ca_name, "min_bw", 6) == 0)
		count = sprintf(page, "%s %s %d%%\n",
				ngbe_cfgfs_vsi->vsi->netdev->name,
				(pf->min_bw & NGBE_ALT_BW_RELATIVE_MASK) ?
				"Relative Min BW" : "Absolute Min BW",
				pf->min_bw & NGBE_ALT_BW_VALUE_MASK);
	else if (strncmp(attr->ca_name, "max_bw", 6) == 0)
		count = sprintf(page, "%s %s %d%%\n",
				ngbe_cfgfs_vsi->vsi->netdev->name,
				(pf->max_bw & NGBE_ALT_BW_RELATIVE_MASK) ?
				"Relative Max BW" : "Absolute Max BW",
				pf->max_bw & NGBE_ALT_BW_VALUE_MASK);
	else if (strncmp(attr->ca_name, "ports", 5) == 0)
		count = sprintf(page, "%d\n",
				pf->hw.num_ports);
	else if (strncmp(attr->ca_name, "partitions", 10) == 0)
		count = sprintf(page, "%d\n",
				pf->hw.num_partitions);
	else
		return 0;

	return count;
}

/**
 * ngbe_cfgfs_vsi_attr_store - Show a VSI's NPAR BW partition info
 * @item: A pointer back to the configfs item created on driver load
 * @attr: A pointer to this item's configuration attribute
 * @page: A pointer to the user input buffer holding the user input values
 **/
static ssize_t ngbe_cfgfs_vsi_attr_store(struct config_item *item,
				       struct configfs_attribute *attr,
				       const char *page, size_t count)
{
	struct ngbe_cfgfs_vsi *ngbe_cfgfs_vsi = to_ngbe_cfgfs_vsi(item);
	struct ngbe_pf *pf = ngbe_cfgfs_vsi->vsi->back;
	char *p = (char *) page;
	int rc;
	unsigned long tmp;

	if (ngbe_cfgfs_vsi->vsi != pf->vsi[pf->lan_vsi])
		return 0;

	if (!p || (*p && (*p == '\n')))
		return -EINVAL;

	rc = kstrtoul(p, 10, &tmp);
	if (rc)
		return rc;
	if (tmp > 100)
		return -ERANGE;

	if (strncmp(attr->ca_name, "min_bw", 6) == 0) {
		if (tmp > (pf->max_bw & NGBE_ALT_BW_VALUE_MASK))
			return -ERANGE;
		/* Preserve the valid and relative BW bits - the rest is
		 * don't care.
		 */
		pf->min_bw &= (NGBE_ALT_BW_RELATIVE_MASK |
				    NGBE_ALT_BW_VALID_MASK);
		pf->min_bw |= (tmp & NGBE_ALT_BW_VALUE_MASK);
		ngbe_set_partition_bw_setting(pf);
	} else if (strncmp(attr->ca_name, "max_bw", 6) == 0) {
		if (tmp < 1 ||
		    tmp < (pf->min_bw & NGBE_ALT_BW_VALUE_MASK))
			return -ERANGE;
		/* Preserve the valid and relative BW bits - the rest is
		 * don't care.
		 */
		pf->max_bw &= (NGBE_ALT_BW_RELATIVE_MASK |
				    NGBE_ALT_BW_VALID_MASK);
		pf->max_bw |= (tmp & NGBE_ALT_BW_VALUE_MASK);
		ngbe_set_partition_bw_setting(pf);
	} else if (strncmp(attr->ca_name, "commit", 6) == 0 && tmp == 1) {
		if (ngbe_commit_partition_bw_setting(pf))
			return -EIO;
	}

	return count;
}

/**
 * ngbe_cfgfs_vsi_release - Free up the configuration item memory
 * @item: A pointer back to the configfs item created on driver load
 **/
static void ngbe_cfgfs_vsi_release(struct config_item *item)
{
	kfree(to_ngbe_cfgfs_vsi(item));
}

static struct configfs_item_operations ngbe_cfgfs_vsi_item_ops = {
	.release		= ngbe_cfgfs_vsi_release,
	.show_attribute		= ngbe_cfgfs_vsi_attr_show,
	.store_attribute	= ngbe_cfgfs_vsi_attr_store,
};

static struct config_item_type ngbe_cfgfs_vsi_type = {
	.ct_item_ops	= &ngbe_cfgfs_vsi_item_ops,
	.ct_attrs	= ngbe_cfgfs_vsi_attrs,
	.ct_owner	= THIS_MODULE,
};

struct ngbe_cfgfs_group {
	struct config_group group;
};

/**
 * to_ngbe_cfgfs_group - Get the group pointer from the config item
 * @item: A pointer back to the configfs item created on driver load
 **/
static inline struct ngbe_cfgfs_group *
to_ngbe_cfgfs_group(struct config_item *item)
{
	return item ? container_of(to_config_group(item),
				   struct ngbe_cfgfs_group, group) : NULL;
}

/**
 * ngbe_cfgfs_group_make_item - Create the configfs item with group container
 * @group: A pointer to our configfs group
 * @name: A pointer to the nume of the device we're looking for
 **/
static struct config_item *
ngbe_cfgfs_group_make_item(struct config_group *group, const char *name)
{
	struct ngbe_cfgfs_vsi *ngbe_cfgfs_vsi;
	struct net_device *netdev;
	struct ngbe_netdev_priv *np;

	read_lock(&dev_base_lock);
	netdev = first_net_device(&init_net);
	while (netdev) {
		if (strncmp(netdev->name, name, sizeof(netdev->name)) == 0)
			break;
		netdev = next_net_device(netdev);
	}
	read_unlock(&dev_base_lock);

	if (!netdev)
		return ERR_PTR(-ENODEV);

	/* is this netdev owned by ngbe? */
	if (netdev->netdev_ops->ndo_open != ngbe_open)
		return ERR_PTR(-EACCES);

	ngbe_cfgfs_vsi = kzalloc(sizeof(struct ngbe_cfgfs_vsi), GFP_KERNEL);
	if (!ngbe_cfgfs_vsi)
		return ERR_PTR(-ENOMEM);

	np = netdev_priv(netdev);
	ngbe_cfgfs_vsi->vsi = np->vsi;
	config_item_init_type_name(&ngbe_cfgfs_vsi->item, name,
				   &ngbe_cfgfs_vsi_type);

	return &ngbe_cfgfs_vsi->item;
}

static struct configfs_attribute ngbe_cfgfs_group_attr_description = {
	.ca_owner = THIS_MODULE,
	.ca_name = "description",
	.ca_mode = S_IRUGO,
};

static struct configfs_attribute *ngbe_cfgfs_group_attrs[] = {
	&ngbe_cfgfs_group_attr_description,
	NULL,
};

static ssize_t ngbe_cfgfs_group_attr_show(struct config_item *item,
					 struct configfs_attribute *attr,
					 char *page)
{
	return sprintf(page,
"ngbe\n"
"\n"
"This subsystem allows the modification of network port configurations.\n"
"To start, use the name of the network port to be configured in a 'mkdir'\n"
"command, e.g. 'mkdir eth3'.\n");
}

static void ngbe_cfgfs_group_release(struct config_item *item)
{
	kfree(to_ngbe_cfgfs_group(item));
}

static struct configfs_item_operations ngbe_cfgfs_group_item_ops = {
	.release	= ngbe_cfgfs_group_release,
	.show_attribute	= ngbe_cfgfs_group_attr_show,
};

/*
 * Note that, since no extra work is required on ->drop_item(),
 * no ->drop_item() is provided.
 */
static struct configfs_group_operations ngbe_cfgfs_group_ops = {
	.make_item	= ngbe_cfgfs_group_make_item,
};

static struct config_item_type ngbe_cfgfs_group_type = {
	.ct_item_ops	= &ngbe_cfgfs_group_item_ops,
	.ct_group_ops	= &ngbe_cfgfs_group_ops,
	.ct_attrs	= ngbe_cfgfs_group_attrs,
	.ct_owner	= THIS_MODULE,
};

static struct configfs_subsystem ngbe_cfgfs_group_subsys = {
	.su_group = {
		.cg_item = {
			.ci_namebuf = "ngbe",
			.ci_type = &ngbe_cfgfs_group_type,
		},
	},
};

/**
 * ngbe_configfs_init - Initialize configfs support for our driver
 **/
int ngbe_configfs_init(void)
{
	int ret;
	struct configfs_subsystem *subsys;

	subsys = &ngbe_cfgfs_group_subsys;

	config_group_init(&subsys->su_group);
	mutex_init(&subsys->su_mutex);
	ret = configfs_register_subsystem(subsys);
	if (ret) {
		pr_err("Error %d while registering configfs subsystem %s\n",
		       ret, subsys->su_group.cg_item.ci_namebuf);
		return ret;
	}

	return 0;
}

/**
 * ngbe_configfs_init - Bail out - unregister configfs subsystem and release
 **/
void ngbe_configfs_exit(void)
{
	configfs_unregister_subsystem(&ngbe_cfgfs_group_subsys);
}

#else /* CONFIG_CONFIGFS_FS */
/**
 * ngbe_configfs_init - Initialize configfs support for our driver
 **/
int ngbe_configfs_init(void)
{
	return 0;
}

/**
 * ngbe_configfs_init - Bail out - unregister configfs subsystem and release
 **/
void ngbe_configfs_exit(void)
{
}

#endif /* CONFIG_CONFIGFS_FS */
