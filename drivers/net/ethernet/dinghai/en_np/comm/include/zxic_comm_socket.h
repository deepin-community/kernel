/**************************************************************
* 版权所有 (C)2013-2015, 深圳市中兴通讯股份有限公司
* 文件名称 : zxic_comm_socket.h
* 文件标识 : 
* 内容摘要 : 
* 其它说明 : 
* 当前版本 : 
* 完成日期 : 2014/02/08
* DEPARTMENT: ASIC_FPGA_R&D_Dept 
* MANUAL_PERCENT: 100%   
 
* 修改记录1: 
* 修改日期:  
* 版 本 号:  
* 修 改 人:  
* 修改内容:  
***************************************************************/

#ifndef _ZXIC_COMM_SOCKET_H_
#define _ZXIC_COMM_SOCKET_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef ZXIC_OS_WIN
#include <winsock.h>
typedef SOCKET                 ZXIC_SOCKET;

#else
#include <linux/socket.h>
#include <linux/in.h>
#include <linux/inet.h>
#include <linux/tcp.h>
typedef ZXIC_SINT32            ZXIC_SOCKET;
#endif

typedef struct sockaddr        SOCKADDR;
typedef struct sockaddr_in     SOCKADDR_IN;


#define ZXIC_SOCK_VALID             (0)
#define ZXIC_SOCK_INVALID           (-1)
#define ZXIC_SOCK_NUM_MAX           (16)

#define ZXIC_SOCK_INADDR_ANY        (0x00000000)
#define ZXIC_SOCK_INADDR_LOOPBACK   (0x7f000001)
#define ZXIC_SOCK_INADDR_BROADCAST  (0xffffffff)
#define ZXIC_SOCK_INADDR_NONE       (0xffffffff)

/* socket domain */
#define ZXIC_SOCK_AF_INET           AF_INET     /* internetwork: UDP, TCP, etc. */
#define ZXIC_SOCK_AF_INET6          AF_INET6    /* Internetwork Version 6 */

/* socket type */
#define ZXIC_SOCK_STREAM            SOCK_STREAM     /* stream socket */
#define ZXIC_SOCK_DGRAM             SOCK_DGRAM      /* datagram socket */
#define ZXIC_SOCK_RAW               SOCK_RAW        /* raw-protocol interface */
#define ZXIC_SOCK_RDM               SOCK_RDM        /* reliably-delivered message */
#define ZXIC_SOCK_SEQPACKET         SOCK_SEQPACKET  /* sequenced packet stream */

/* socket protocol */
#define ZXIC_SOCK_IPPROTO_IP        IPPROTO_IP   /* dummy for IP */
#define ZXIC_SOCK_IPPROTO_TCP       IPPROTO_TCP  /* tcp */
#define ZXIC_SOCK_IPPROTO_UDP       IPPROTO_UDP  /* user datagram protocol */

/* socket level */
#define ZXIC_SOCK_SOL_SOCKET        SOL_SOCKET   /* options for socket level */

/* socket OptName */
#define ZXIC_SOCK_SO_DEBUG          SO_DEBUG          /* turn on debugging info recording */
#define ZXIC_SOCK_SO_ACCEPTCONN     SO_ACCEPTCONN     /* socket has had listen() */
#define ZXIC_SOCK_SO_REUSEADDR      SO_REUSEADDR      /* allow local address reuse */
#define ZXIC_SOCK_SO_KEEPALIVE      SO_KEEPALIVE      /* keep connections alive */
#define ZXIC_SOCK_SO_DONTROUTE      SO_DONTROUTE      /* just use interface addresses */
#define ZXIC_SOCK_SO_BROADCAST      SO_BROADCAST      /* permit sending of broadcast msgs */
#define ZXIC_SOCK_SO_USELOOPBACK    SO_USELOOPBACK    /* bypass hardware when possible */
#define ZXIC_SOCK_SO_LINGER         SO_LINGER         /* linger on close if data present */
#define ZXIC_SOCK_SO_OOBINLINE      SO_OOBINLINE      /* leave received OOB data in line */
#define ZXIC_SOCK_SO_SNDBUF         SO_SNDBUF         /* send buffer size */
#define ZXIC_SOCK_SO_RCVBUF         SO_RCVBUF         /* receive buffer size */
#define ZXIC_SOCK_SO_SNDLOWAT       SO_SNDLOWAT       /* send low-water mark */
#define ZXIC_SOCK_SO_RCVLOWAT       SO_RCVLOWAT       /* receive low-water mark */
#define ZXIC_SOCK_SO_SNDTIMEO       SO_SNDTIMEO       /* send timeout */
#define ZXIC_SOCK_SO_RCVTIMEO       SO_RCVTIMEO       /* receive timeout */
#define ZXIC_SOCK_SO_ERROR          SO_ERROR          /* get error status and clear */
#define ZXIC_SOCK_SO_TYPE           SO_TYPE           /* get socket type */


#define ZXIC_TCP_OP_NODELAY         TCP_NODELAY

typedef struct zxic_comm_sock_addr_t
{
    ZXIC_UINT32  family;
    ZXIC_UINT32  port;
    ZXIC_UINT32   addr;
}ZXIC_SOCK_ADDR_T;

typedef struct zxic_comm_sock_mgr_t
{
    ZXIC_UINT32     is_init;
    ZXIC_UINT32     count;
    ZXIC_SOCKET     socks[ZXIC_SOCK_NUM_MAX];
    ZXIC_UINT8      sock_vld[ZXIC_SOCK_NUM_MAX];
    ZXIC_MUTEX_T    mutex;
}ZXIC_SOCK_MGR_T;

/* API */
ZXIC_RTN32 zxic_comm_sock_init(ZXIC_VOID);
ZXIC_RTN32 zxic_comm_sock_service_start(ZXIC_VOID);
ZXIC_RTN32 zxic_comm_sock_service_close(ZXIC_VOID);
ZXIC_RTN32 zxic_comm_sock_create(ZXIC_SOCKET  *p_socket, 
                            ZXIC_SINT32     domain, 
                            ZXIC_SINT32     type, 
                            ZXIC_SINT32     protocol);

ZXIC_RTN32 zxic_comm_sock_set_opt(ZXIC_SOCKET  sock, 
                             ZXIC_SINT32     level,
                             ZXIC_SINT32     opt_name,
                             ZXIC_VOID        *p_opt_val,
                             ZXIC_UINT32     opt_len);

ZXIC_RTN32 zxic_comm_sock_get_opt(ZXIC_SOCKET  sock, 
                             ZXIC_SINT32     level,
                             ZXIC_SINT32     opt_name,
                             ZXIC_VOID        *p_opt_val,
                             ZXIC_UINT32     *p_opt_len);

ZXIC_RTN32 zxic_comm_sock_bind_listen(ZXIC_SOCKET       sock,
                                 ZXIC_SOCK_ADDR_T  *p_sock_addr);

ZXIC_RTN32 zxic_comm_sock_accpet(ZXIC_SOCKET       listen_sock,
                            ZXIC_SOCKET       *p_cnnt_sock,
                            ZXIC_SOCK_ADDR_T  *p_sock_addr);

ZXIC_RTN32 zxic_comm_sock_connect(ZXIC_SOCKET       sock,
                             ZXIC_SOCK_ADDR_T  *p_sock_addr);

ZXIC_SINT32 zxic_comm_sock_send(ZXIC_SOCKET  sock,
                           ZXIC_CHAR        *p_buf,
                           ZXIC_SINT32     len,
                           ZXIC_SINT32     flag);

ZXIC_SINT32 zxic_comm_sock_recv(ZXIC_SOCKET  sock,
                           ZXIC_CHAR        *p_buf,
                           ZXIC_SINT32     len,
                           ZXIC_SINT32     flag);

ZXIC_RTN32 zxic_comm_sock_close(ZXIC_SOCKET sock);


#ifdef __cplusplus
}
#endif

#endif
