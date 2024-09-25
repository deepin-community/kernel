
#ifndef _PS3_INJECT_H_
#define _PS3_INJECT_H_

typedef struct PS3Inject {
	ps3_mutex lock;
	struct list_head scsi_rw_list;
	struct list_head scsi_task_list;
	struct list_head mgr_list;
} PS3Inject_s;

typedef struct PS3HitCmd {
	struct list_head scsi_rw_list;
	struct list_head scsi_task_list;
	struct list_head mgr_list;
} PS3HitCmd_s;

typedef struct ps3_scsi_sense{
	U8 resp_code; 
	U8 sense_key;
	U8 asc;
	U8 ascq;
} ps3_scsi_sense_s;


typedef struct PS3ScsiErrorReply {
    S32 result;         
	ps3_scsi_sense_s sshdr; 
} PS3ScsiErrorReply_s;


typedef struct PS3ScsiForceReply {
    U32 step;         
} PS3ScsiForceReply_s;

union PS3CmdInjectDeal {
    PS3ScsiErrorReply_s  errReply;     
    PS3ScsiForceReply_s forceReply; 
};

struct inject_scsi_cmds_t {
	U32 host_no;
	U32 id;
	U32	channel;
	struct scsi_device *device;
	U64 lba;
	U32 len;
	U32 dealType;
	union PS3CmdInjectDeal cmdDeal;
	U32 inject_count;
};

struct inject_scsi_task_cmds_t {
	U32 host_no;
	U32 id;
	U32	channel;
	struct scsi_device *device;
	U8 cmd_type;
	U8 cmd_sub_type;
	U32 dealType;
	union PS3CmdInjectDeal cmdDeal;
	U32 inject_count;
};

struct inject_mgr_cmds_t {
	U32 host_no;
	U8 cmd_type;
	U8 cmd_sub_type;
	U32 dealType;
	U32 errType;
	U32 inject_count;
};

union PS3CmdInject {
	struct inject_scsi_cmds_t scsi_cmd;
	struct inject_scsi_task_cmds_t scsi_task_cmd;
	struct inject_mgr_cmds_t mgr_cmd;
};
struct inject_cmds_t {
	struct list_head list;
	union PS3CmdInject item;
};

struct inject_hit_cmds_t {
	struct list_head list;
	struct ps3_cmd *cmd;
	struct inject_cmds_t *pitem;
};

enum {
	PS3_SCSI_CMD_TIMEOUT_FORCE_REPLY = 1,
	PS3_SCSI_CMD_ERROR = 2,
	PS3_SCSI_TASK_CMD_NORMAL = 3,
	PS3_SCSI_TASK_CMD_TIMEOUT = 4,
	PS3_SCSI_TASK_CMD_ERROE = 5,
	PS3_MGR_CMD_NORMAL = 6,
	PS3_MGR_CMD_TIMEOUT = 7,
	PS3_MGR_CMD_ERROE = 8,
} ;
PS3Inject_s * get_inject(void);

#ifndef PS3_UT
S32 ps3_scsi_rw_cmd_filter_handle(struct scsi_cmnd *scmd);
S32 ps3_scsi_task_cmd_filter_handle(struct ps3_cmd *cmd);
S32 ps3_mgr_cmd_filter_handle(struct ps3_cmd *cmd);
#endif

Bool ps3_add_cmd_filter(struct ps3_instance *instance, struct PS3CmdWord *cmd_word);

void ps3_delete_scsi_rw_inject(struct inject_cmds_t *this_pitem);
void ps3_delete_scsi_task_inject(struct inject_cmds_t *this_pitem);
void ps3_delete_mgr_inject(struct inject_cmds_t *this_pitem);

PS3HitCmd_s * get_hit_inject(void);

void ps3_inject_init(void);

void ps3_inject_exit(void);

#endif
