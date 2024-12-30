#ifndef _WINDOWS

#ifndef _PS3_DEVICE_MANAGER_SAS_H_
#define _PS3_DEVICE_MANAGER_SAS_H_
#include <linux/version.h>

#include <scsi/scsi_device.h>
#include <scsi/scsi_transport_sas.h>

#include "ps3_htp_def.h"
#include "ps3_htp_dev.h"
#include "ps3_htp_event.h"
#include "ps3_device_manager.h"

#define		PS3_SAS_HBA_MAX_SAS_NUM		3

#define		PS3_SAS_MAX_PHY_NUM		(128)
#define		PS3_SAS_INVALID_ID		(0xFF)
#define		PS3_SAS_INVALID_SAS_ADDR	(0x0)
#define		PS3_SAS_REQ_BUFF_LEN		(4096)

struct ps3_instance;

struct ps3_sas_node {
	struct list_head list;          
	struct device *dev;             
	struct ps3_sas_phy *phys;       
	struct list_head sas_port_list; 
	U64 sas_address;                
	U64 parent_sas_address;         
	U8 phy_count;                   
	U8 encl_id;	                
	U8 parent_encl_id;              
	U8 dev_type;			
	U8 reserved[4];
};

struct ps3_sas_phy {
	struct list_head port_siblings;       	
	struct sas_identify identify;         	
	struct sas_identify remote_identify;  	
	struct sas_phy *phy;                  	
	struct ps3_sas_port *attach_port;      	
	U8 phy_id;                            	
	U8 encl_id;                           	
	U8 slot_id;				
	U8 reserved[5];
};

struct ps3_sas_port {
	struct list_head list;                
	struct sas_identify remote_identify;  
	struct sas_rphy *rphy;                
	struct sas_port *port;                
	struct list_head phy_list;            
	U16 pd_flat_id;                       
	U8 phy_count;                         
	U8 reserved[5];
};

struct ps3_sas_dev_context {
	struct list_head 	ps3_sas_node_list;
	spinlock_t		ps3_sas_node_lock;
	struct ps3_sas_node	ps3_hba_sas;
	U64 ps3_hba_sas_addr[PS3_SAS_HBA_MAX_SAS_NUM]; 
	struct semaphore 	ps3_sas_smp_semaphore; 

	dma_addr_t		ps3_sas_buff_dma_addr;
	dma_addr_t		ps3_sas_phy_buff_dma_addr;
	void			*ps3_sas_buff;
	struct PS3PhyInfo 	*ps3_sas_phy_buff;
	Bool 			is_support_smp;
	U8 			reserved[7];
};

Bool ps3_sas_is_support_smp(struct ps3_instance *instance);

S32 ps3_sas_device_mgr_init(struct ps3_instance *instance);

S32 ps3_sas_device_mgr_exit(struct ps3_instance *instance);

S32 ps3_sas_device_data_init(struct ps3_instance *instance);

S32 ps3_sas_device_data_exit(struct ps3_instance *instance);

S32 ps3_sas_rphy_slot_get(struct ps3_instance *instance, U64 sas_addr, U32 *slot_id);

U64 ps3_sas_rphy_parent_sas_addr_get(struct ps3_instance *instance, U64 sas_addr);

U8 ps3_sas_encl_id_get(struct ps3_instance *instance, U64 sas_addr);

S32 ps3_sas_expander_event_refresh(struct ps3_instance *instance);

S32 ps3_sas_update_detail_proc(struct ps3_instance *instance,
	struct PS3EventDetail *event_detail, U32 event_cnt);

S32 ps3_sas_add_device(struct ps3_instance *instance,
	struct ps3_pd_entry *pd_entry);

S32 ps3_sas_remove_device(struct ps3_instance *instance,
	struct PS3DiskDevPos *diskPos, U8 encl_id, U8 phy_id);

void ps3_sas_expander_node_del(struct ps3_instance *instance,
	struct ps3_sas_node *exp_node);

struct ps3_sas_node* ps3_sas_find_node_by_sas_addr(
	struct ps3_instance *instance, U64 sas_addr);

void ps3_sas_node_phy_update(struct ps3_instance *instance, struct ps3_sas_phy *ps3_phy,
	struct PS3PhyInfo *phy_info);

#if LINUX_VERSION_CODE < KERNEL_VERSION(3,17,0)
	S32 ps3_sas_user_scan(struct Scsi_Host *host, U32 channel, U32 id, U32 lun);
#else
	S32 ps3_sas_user_scan(struct Scsi_Host *host, U32 channel, U32 id, U64 lun);
#endif

#endif
#endif
