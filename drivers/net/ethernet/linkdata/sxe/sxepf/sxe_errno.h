
#ifndef __SXE_ERRNO_H__
#define __SXE_ERRNO_H__

#define SXE_ERR_MODULE_STANDARD			0
#define SXE_ERR_MODULE_PF				1
#define SXE_ERR_MODULE_VF				2
#define SXE_ERR_MODULE_HDC				3

#define SXE_ERR_MODULE_OFFSET	16
#define SXE_ERR_MODULE(module, errcode)		\
	((module << SXE_ERR_MODULE_OFFSET) | errcode)
#define SXE_ERR_PF(errcode)		SXE_ERR_MODULE(SXE_ERR_MODULE_PF, errcode)
#define SXE_ERR_VF(errcode)		SXE_ERR_MODULE(SXE_ERR_MODULE_VF, errcode)
#define SXE_ERR_HDC(errcode)	SXE_ERR_MODULE(SXE_ERR_MODULE_HDC, errcode)

#define SXE_ERR_CONFIG                        EINVAL
#define SXE_ERR_PARAM                         EINVAL
#define SXE_ERR_RESET_FAILED                  EPERM
#define SXE_ERR_NO_SPACE                      ENOSPC
#define SXE_ERR_FNAV_CMD_INCOMPLETE           EBUSY
#define SXE_ERR_MBX_LOCK_FAIL                 EBUSY
#define SXE_ERR_OPRATION_NOT_PERM             EPERM
#define SXE_ERR_LINK_STATUS_INVALID           EINVAL
#define SXE_ERR_LINK_SPEED_INVALID            EINVAL
#define SXE_ERR_DEVICE_NOT_SUPPORTED          EOPNOTSUPP
#define SXE_ERR_HDC_LOCK_BUSY                 EBUSY
#define SXE_ERR_HDC_FW_OV_TIMEOUT             ETIMEDOUT
#define SXE_ERR_MDIO_CMD_TIMEOUT              ETIMEDOUT
#define SXE_ERR_INVALID_LINK_SETTINGS         EINVAL
#define SXE_ERR_FNAV_REINIT_FAILED            EIO
#define SXE_ERR_CLI_FAILED                    EIO
#define SXE_ERR_MASTER_REQUESTS_PENDING       SXE_ERR_PF(1)
#define SXE_ERR_SFP_NO_INIT_SEQ_PRESENT       SXE_ERR_PF(2)
#define SXE_ERR_ENABLE_SRIOV_FAIL             SXE_ERR_PF(3)
#define SXE_ERR_IPSEC_SA_STATE_NOT_EXSIT      SXE_ERR_PF(4)
#define SXE_ERR_SFP_NOT_PERSENT               SXE_ERR_PF(5)
#define SXE_ERR_PHY_NOT_PERSENT               SXE_ERR_PF(6)
#define SXE_ERR_PHY_RESET_FAIL                SXE_ERR_PF(7)
#define SXE_ERR_FC_NOT_NEGOTIATED             SXE_ERR_PF(8)
#define SXE_ERR_SFF_NOT_SUPPORTED             SXE_ERR_PF(9)

#define SXEVF_ERR_MAC_ADDR_INVALID              EINVAL
#define SXEVF_ERR_RESET_FAILED                  EIO
#define SXEVF_ERR_ARGUMENT_INVALID              EINVAL
#define SXEVF_ERR_NOT_READY                     EBUSY
#define SXEVF_ERR_POLL_ACK_FAIL                 EIO
#define SXEVF_ERR_POLL_MSG_FAIL                 EIO
#define SXEVF_ERR_MBX_LOCK_FAIL                 EBUSY
#define SXEVF_ERR_REPLY_INVALID                 EINVAL
#define SXEVF_ERR_IRQ_NUM_INVALID               EINVAL
#define SXEVF_ERR_PARAM                         EINVAL
#define SXEVF_ERR_MAILBOX_FAIL                  SXE_ERR_VF(1)
#define SXEVF_ERR_MSG_HANDLE_ERR                SXE_ERR_VF(2)
#define SXEVF_ERR_DEVICE_NOT_SUPPORTED          SXE_ERR_VF(3)
#define SXEVF_ERR_IPSEC_SA_STATE_NOT_EXSIT      SXE_ERR_VF(4)

#endif
