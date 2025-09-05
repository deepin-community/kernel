/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * pm_domain.h - Definitions and headers related to device power domains.
 *
 * Copyright (C) 2011 Rafael J. Wysocki <rjw@sisk.pl>, Renesas Electronics Corp.
 */

#include <linux/pm_domain.h>

#define GENPD_FLAG_OPP_TABLE_FW	 (1U << 7)
#define genpd_is_opp_table_fw(genpd)	(genpd->flags & GENPD_FLAG_OPP_TABLE_FW)

typedef struct generic_pm_domain *(*fwnode_genpd_xlate_t)(struct fwnode_reference_args *args,
						   void *data);

struct fwnode_genpd_onecell_data {
	struct generic_pm_domain **domains;
	unsigned int num_domains;
	fwnode_genpd_xlate_t fwnode_xlate;
};

#ifdef CONFIG_PM_GENERIC_DOMAINS
int fwnode_pm_genpd_init(struct generic_pm_domain *genpd,
		  struct dev_power_governor *gov, bool is_off);
int fwnode_genpd_add_provider_onecell(struct fwnode_handle *np,
				  struct fwnode_genpd_onecell_data *data);
void fwnode_genpd_del_provider(struct fwnode_handle *fwnode);
struct device *fwnode_genpd_dev_pm_attach_by_id(struct device *dev,
					 unsigned int index);
struct device *fwnode_genpd_dev_pm_attach_by_name(struct device *dev, const char *name);
int scmi_device_set_freq(struct device *dev, unsigned long freq);
unsigned long scmi_device_get_freq(struct device *dev);
int scmi_device_opp_table_parse(struct device *dev);
#else
static inline int fwnode_pm_genpd_init(struct generic_pm_domain *genpd,
				struct dev_power_governor *gov, bool is_off)
{
	return -EOPNOTSUPP;
}

static inline int fwnode_genpd_add_provider_onecell(struct fwnode_handle *np,
				  struct fwnode_genpd_onecell_data *data)
{
	return -EOPNOTSUPP;
}

static inline void fwnode_genpd_del_provider(struct fwnode_handle *fwnode)
{

}

static inline struct device *fwnode_genpd_dev_pm_attach_by_id(struct device *dev,
					 unsigned int index)
{
	return -EOPNOTSUPP;
}

static inline struct device
*fwnode_genpd_dev_pm_attach_by_name(struct device *dev, const char *name)
{
	return -EOPNOTSUPP;
}

static inline int scmi_device_set_freq(struct device *dev, unsigned long freq)
{
	return -EOPNOTSUPP;
}

static inline unsigned long scmi_device_get_freq(struct device *dev)
{
	return -EOPNOTSUPP;
}

static inline int scmi_device_opp_table_parse(struct device *dev)
{
	return -EOPNOTSUPP;
}
#endif

