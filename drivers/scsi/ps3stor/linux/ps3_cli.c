#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/errno.h>
#include <linux/miscdevice.h>   
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/version.h>
#include "ps3_cli.h"

#if (LINUX_VERSION_CODE < KERNEL_VERSION(6, 6, 0))
#define PS3_CLI_MINOR 26
#else
#define PS3_CLI_MINOR MISC_DYNAMIC_MINOR
#endif

#define PS3_CLI_HASH_LEN	256		

struct ps3_cli_cmd_s {
	struct ps3_cli_cmd_s *next; 	
	char cmd[PS3_CLI_CMD_MAXLEN]; 	
	char help[PS3_CLI_HELP_LEN]; 	
	ps3_cli_func_t func; 			
};

static int misc_registered;			
static int dev_opened;				
static int cmd_ready; 				
static char ps3_cli_input[PS3_CLI_INPUT_LEN]; 		
static char ps3_cli_output[PS3_CLI_OUTLINE_LEN]; 	
static char __user *read_buf; 						
static int  read_buf_len; 							
static int  read_buf_ptr; 							

static struct ps3_cli_cmd_s *ps3_cli_cmd_head[PS3_CLI_HASH_LEN]; 	
static struct mutex ps3_cli_mutex; 									

#if 0
#define __pl()	printk("func = %s, line = %d\n", __FUNCTION__, __LINE__)
#else
#define __pl()
#endif

static ssize_t ps3_cli_write(struct file *fp, const char __user *buffer,
                  size_t nbytes, loff_t *ppos)
{
	__pl();
	(void)fp;
	(void)ppos;
	if (nbytes>PS3_CLI_INPUT_LEN-1)
		return -EINVAL;

	if (copy_from_user(ps3_cli_input, buffer, nbytes))
		return -EFAULT;
	ps3_cli_input[nbytes] = '\0';
	cmd_ready = 1;
	__pl();
	return (ssize_t)nbytes;
}

static u32 str_hash(const char *name)
{
	u32 hash, hash0 = 0x12a3fe2d, hash1 = 0x37abe8f9;
	const signed char *scp = (const signed char*)name;

	while (*scp) {
		hash = hash1 + (hash0 ^ (((int) *scp++) * 7152373));

		if (hash & 0x80000000)
			hash -= 0x7fffffff;
		hash1 = hash0;
		hash0 = hash;
	}
	return hash0 << 1;
}

static struct ps3_cli_cmd_s *ps3_cli_find_cmd(const char *cmd)
{
	u32 idx = str_hash(cmd) & (PS3_CLI_HASH_LEN-1);
	struct ps3_cli_cmd_s *p;

	for (p=ps3_cli_cmd_head[idx]; p; p=p->next)
		if (!strcmp(p->cmd, cmd))
			return p;
	return NULL;
}

int ps3stor_cli_printf(const char *fmt, ...)
{
	va_list args;
	int len, n, ret;

	__pl();

	va_start(args, fmt);
	len = vsnprintf(ps3_cli_output, PS3_CLI_OUTLINE_LEN, fmt, args);
	va_end(args);

	if (read_buf_ptr>=read_buf_len) {
		return read_buf_ptr;
	}

	n = read_buf_len-read_buf_ptr;
	if (n>len)
		n = len;

	ret = copy_to_user(read_buf+read_buf_ptr, ps3_cli_output, n);
	if (ret<0) {
		pr_err("copy_to_user err=%d\n", ret);
		return -1;
	}

	read_buf_ptr += n;

	__pl();

	return len;
}
EXPORT_SYMBOL(ps3stor_cli_printf);

int ps3stor_cli_register(ps3_cli_func_t func, const char *cmd_str, const char *help)
{
	u32 idx = str_hash(cmd_str) & (PS3_CLI_HASH_LEN-1);
	struct ps3_cli_cmd_s *cmd;
	int ret;

	__pl();

	ret = mutex_lock_killable(&ps3_cli_mutex);
	if (ret!=0) {
		pr_err("ps3stor_cli_register(): mutex_lock_killable return err = %d\n", ret);
		return ret;
	}
	cmd = ps3_cli_find_cmd(cmd_str);
	if (cmd) {
		pr_err("cmd=['%s'] has already been registered\n", cmd_str);
		mutex_unlock(&ps3_cli_mutex);
		return -EEXIST;
	}

	cmd = (struct ps3_cli_cmd_s *)kmalloc(sizeof(struct ps3_cli_cmd_s), GFP_KERNEL);
	if (cmd==NULL) {
		pr_err("out of memory\n");
		mutex_unlock(&ps3_cli_mutex);
		return -ENOMEM;
	}

	memset(cmd, 0, sizeof(struct ps3_cli_cmd_s));

	strncpy(cmd->cmd, cmd_str, PS3_CLI_CMD_MAXLEN - 1);
	strncpy(cmd->help, help, PS3_CLI_HELP_LEN - 1);

	cmd->func = func;

	cmd->next = ps3_cli_cmd_head[idx];
	ps3_cli_cmd_head[idx] = cmd;

	mutex_unlock(&ps3_cli_mutex);
	__pl();
	return 0;
}
EXPORT_SYMBOL(ps3stor_cli_register);

static void ps3_parse_cmd(char *cmdline, int *argc, char *argv[])
{
	char *p = cmdline;
	int i=0, spc=1;
	while(*p) {
		if (spc) {
			if (*p!=' ') {
				spc = 0;
				argv[i] = p;
				i++;
			} else {
				*p = '\0';
			}
		} else {
			if (*p==' ') {
				spc = 1;
				*p = '\0';
			} else {
			}
		}
		p++;
	}
	*argc = i;
}

static ssize_t ps3_cli_read(struct file *fp, char __user *buf,
                 size_t nbytes, loff_t *ppos)
{
	struct ps3_cli_cmd_s *cmd;
	int argc;
	char *argv[PS3_MAX_ARGV];
	int ret;

	(void)fp;
	(void)ppos;
	__pl();

	if (!cmd_ready) {
		pr_err("ps3_cli_write() must be called before ps_cli_read()\n");
		return -EINVAL;
	}
	read_buf = buf;
	read_buf_len = (int)nbytes;
	read_buf_ptr = 0;

	ps3_parse_cmd(ps3_cli_input, &argc, argv);

	ret = mutex_lock_killable(&ps3_cli_mutex);
	if (ret!=0) {
		pr_err("ps3stor_cli_register(): mutex_lock_killable return err = %d\n", ret);
		return ret;
	}
	cmd = ps3_cli_find_cmd((const char*)argv[0]);
	if (cmd!=NULL) {
		cmd->func(argc, argv);
	}
	mutex_unlock(&ps3_cli_mutex);

	__pl();
	return read_buf_ptr;
}

static int ps3_cli_open(struct inode *ip, struct file *fp)
{
	(void)ip;
	(void)fp;
	if (!__sync_bool_compare_and_swap(&dev_opened, 0, 1)) {
		pr_err("/dev/ps3stor_cli has already been opened\n");
		return -EBUSY;
	}
	cmd_ready = 0;
	return 0;
}

static int ps3_cli_release(struct inode *ip, struct file *fp)
{
	(void)ip;
	(void)fp;
	dev_opened = 0;
	return 0;
}

static const struct file_operations ps3_cli_fops = {
	.owner = NULL,
	.unlocked_ioctl = NULL,
	.open = ps3_cli_open,
	.release = ps3_cli_release,
	.llseek = NULL,
	.read = ps3_cli_read,
	.write = ps3_cli_write,
    .fasync = NULL
};

static struct miscdevice ps3_cli_device = {
    .minor = PS3_CLI_MINOR,
    .name = "ps3stor_cli",
    .fops = &ps3_cli_fops,
};

static void ps3_cli_help(int argc, char *argv[])
{
	int i;
	struct ps3_cli_cmd_s *cmd;
	(void)argc;
	(void)argv;
	__pl();
	for (i=0; i<PS3_CLI_HASH_LEN; i++) {
		for (cmd=ps3_cli_cmd_head[i]; cmd!=NULL; cmd=cmd->next) {
			ps3stor_cli_printf("%20s -- %s\n", cmd->cmd, (const char*)cmd->help);
		}
	}
	__pl();
}

static void ps3_free_cmds(void)
{
	int i;
	struct ps3_cli_cmd_s *cmd;
	for (i=0; i<PS3_CLI_HASH_LEN; i++) {
		while (ps3_cli_cmd_head[i]) {
			cmd = ps3_cli_cmd_head[i];
			ps3_cli_cmd_head[i] = cmd->next;
			kfree(cmd);
		}
	}
}

int ps3cmd_init(void)
{
	int err;

	if (misc_registered != 0)
		return -EBUSY;

	mutex_init(&ps3_cli_mutex);

	err = misc_register(&ps3_cli_device);
	if (err) {
		pr_warn("Couldn't initialize miscdevice /dev/ps3stor_cli.\n");
		return err;
	}
	misc_registered = 1;
	ps3stor_cli_register(ps3_cli_help, "help", "show this help information");
	return 0;
}

void ps3cmd_exit(void)
{
	if (!misc_registered)
		return;
	misc_deregister(&ps3_cli_device);
	ps3_free_cmds();
	misc_registered = 0;
	mutex_destroy(&ps3_cli_mutex);
}

