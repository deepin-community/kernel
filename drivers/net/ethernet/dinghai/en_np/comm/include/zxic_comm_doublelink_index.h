/*****************************************************************************
 * 版权所有 (C)2008-2010, 深圳市中兴通讯股份有限公司。
 * 
 * 文件名称：zxic_doublelink_index.h
 * 文件标识：
 * 内容摘要：zxic_doublelink_index.c 的头文件， 这里定义了.c文件需要用到的数据结构，
             并且申明了提供外部模块调用的接口函数
 * 其它说明： 其它内容的说明
 * 当前版本：
 * 作    者：HuangHe(170389)   Z T E 中兴
 * 完成日期：2009-02-16
 * 当前责任人-1：HuangHe   Z T E 中兴
 * 当前责任人-2： 
 * 历史责任人-3: 
 * 
 * 修改记录： 
 * 修改日期：   版 本 号   修 改 人     修改内容 
 1 yyyymmdd       V*.*     姓名工号 
 2  
  *****************************************************************************/
#ifndef __ZXIC_COMM_DOUBLELINK_INDEX_H__
#define __ZXIC_COMM_DOUBLELINK_INDEX_H__

/**************************  include head files  *****************************/

/**************************  type define  *****************************/
#define DOUBLELINK_CHECKSUM                   ((ZXIC_UINT32)(0xAABBBAAB))   


/**************************  const variables  **************************/
#define DOUBLELINK_INVALID_PREVIOUS                     (0x0)
#define DOUBLELINK_INVALID_NEXT                         (0x0)
#define DOUBLELINK_USED_FLAG                            ((ZXIC_UINT32)(0xffffffff))
#define DOUBLELINK_LASTEST_ELEMENT                      ((ZXIC_UINT32)(0x0ffffffe))

/**************************************************************************
 *                        double_link_index api                           *
 **************************************************************************/

/**
 * NAME: DLINK_NODE
 *
 * DESCRIPTION: Structure Node the information of the doublelink.
 **/
typedef struct
{
    ZXIC_UINT32 dw_next_node; /*后一个节点*/
    ZXIC_UINT32 dw_pre_node;  /*前一个节点*/
    ZXIC_UINT32 dw_self_node; /*当前一个节点*/
    
}DLINK_NODE;

/**
 * NAME: FTMCOMM_DOUBLELINK_MANGER
 *
 * DESCRIPTION: Structure containing the information required by the
 * implementation of the doublelink.
**/
 
typedef struct _FtmComm_DoubleLink_Manager
{
    /*
     * p_array is a pointer to the array of elements used to track which
     * indexes have been allocated.
     */
    DLINK_NODE *p_array;

    /*
     * numElements is the number of indexes managed by this instance of the
     * index_pool.
     */
    ZXIC_UINT32   capacity;

    /*
     * currFreeElement stores a free element for where to alloc next free element.
     * This helps prevent looping over a large sections of the array each time
     * a new index is allocated.
     */
    ZXIC_UINT32   free_num;

    ZXIC_UINT32   used_num;

    ZXIC_UINT32   first_used;

    ZXIC_UINT32   last_used;

    ZXIC_UINT32   first_free;

    ZXIC_UINT32   last_free;
    /*
     * offset is an adjustment value, allowing the caller to prevent certain
     * indexes from being allocated.  This value is only meaningful to the
     * client, and does not affect how the indexes are managed within the
     * doublelink.
     */
    ZXIC_UINT32   offset;

    ZXIC_UINT32   check_sum; /*用来检查传入的地址是否是双链表管理结构地址*/

}ZXIC_DOUBLELINK_MANGER;


ZXIC_RTN32 zxic_comm_dlink_manage_create(ZXIC_UINT32                     dw_element_num,       
                                   ZXIC_UINT32                     dw_offset, 
                                   ZXIC_DOUBLELINK_MANGER **p_dlink);


ZXIC_RTN32 zxic_comm_dlink_alloc        (ZXIC_DOUBLELINK_MANGER  *p_dlink, 
                                   ZXIC_UINT32                     *index);


ZXIC_RTN32 zxic_comm_dlink_free         (ZXIC_DOUBLELINK_MANGER *p_dlink,
                                   ZXIC_UINT32                     index);


ZXIC_RTN32 zxic_comm_dlink_get_next     (ZXIC_DOUBLELINK_MANGER *p_dlink,
                                   ZXIC_UINT32                     dw_index,
                                   ZXIC_UINT32                    *p_next_index);


ZXIC_RTN32 zxic_comm_dlink_manage_clear (ZXIC_DOUBLELINK_MANGER *p_dlink);


ZXIC_RTN32 zxic_comm_dlink_get_previous (ZXIC_DOUBLELINK_MANGER *p_dlink,
                                   ZXIC_UINT32                     dw_index,
                                   ZXIC_UINT32                    *p_pre_index);

ZXIC_RTN32 zxic_comm_dlink_is_used      (ZXIC_DOUBLELINK_MANGER *p_dlink,
                                   ZXIC_UINT32                     dw_index, 
                                   ZXIC_UINT8                      *p_is_used);


ZXIC_RTN32 zxic_comm_dlink_first_free   (ZXIC_DOUBLELINK_MANGER *p_dlink,
                                   ZXIC_UINT32                    *p_index);


ZXIC_RTN32 zxic_comm_dlink_first_used   (ZXIC_DOUBLELINK_MANGER *p_dlink, 
                                   ZXIC_UINT32                    *p_index);



ZXIC_RTN32 zxic_comm_dlink_manage_reset        (ZXIC_DOUBLELINK_MANGER *p_dlink);

ZXIC_RTN32 zxic_comm_dlink_get_curr_info      (ZXIC_DOUBLELINK_MANGER  *p_dlink, 
                                         ZXIC_UINT32                     *p_free_num, 
                                         ZXIC_UINT32                     *p_curr_free_index);

ZXIC_RTN32 zxic_comm_dlink_last_used          (ZXIC_DOUBLELINK_MANGER  *p_dlink, 
                                         ZXIC_UINT32                     *p_index);

ZXIC_RTN32 zxic_comm_dlink_used_num           (ZXIC_DOUBLELINK_MANGER  *p_dlink,
                                         ZXIC_UINT32                     *p_num);

ZXIC_RTN32 zxic_comm_dlink_show_node_info     (ZXIC_DOUBLELINK_MANGER  *p_dlink,
                                         ZXIC_UINT32                      dw_node_index);

ZXIC_RTN32 zxic_comm_dlink_show_current_status(ZXIC_DOUBLELINK_MANGER  *p_dlink);


ZXIC_RTN32 zxic_comm_dlink_self_test(ZXIC_VOID);



#endif


