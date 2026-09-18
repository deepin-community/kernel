/*****************************************************************************
 * 版权所有 (C)2001-2005, 深圳市中兴通讯股份有限公司。
 * 
 * 文件名称：   Rtm_ListStack.h
 * 文件标识：    
 * 内容摘要：   链表栈管理头文件
 * 其它说明：   释放的元素放到队头
 * 当前版本：    ZXR10 V2.6
 * 作    者：    郑纪伟
 * 完成日期：    2006-9-27 10:34
 * 当前责任人-1：
 * 当前责任人-2：
 *
 * 修改记录1：   
 *    修改日期：2006-9-27 10:34
 *    版 本 号：ZXR10 V2.6
 *    修 改 人：郑纪伟
 *    修改内容：创建 
 * 
 *修改记录2：
 *    修改文件名称：ftmcomm_liststack.h
 *    修改日期：2008-10-16 15:14
 *    版 本 号：ZXR10 V2.6
 *    修 改 人：HuangHe 170389
 *    修改内容：移植到T8000项目使用 
 *修改记录3：
 *    修改文件名称：zxic_liststack.h
 *    修改日期：2012-03-15 15:14
 *    版 本 号：ZXR10 V2.6
 *    修 改 人：JiangWenming 12010401
 *    修改内容：移植到NSE项目使用 
 * 
 *****************************************************************************/
#ifndef __ZXIC_COMM_LIST_STACK_H__
#define __ZXIC_COMM_LIST_STACK_H__


/**************************************************************************
 *                            宏定义                                      *
 **************************************************************************/

#define LISTSTACK_MAX_ELEMENT  ((ZXIC_UINT32)(0x0ffffffe))
#define LISTSTACK_INVALID_INDEX     (0)
#define ALLOC_NUMBER                (0x3)
/**************************************************************************
 *                        liststack api                                   *
 **************************************************************************/

typedef struct _s_freelink
{
    ZXIC_UINT32 index;
    ZXIC_UINT32 next;
}ZXIC_COMM_FREELINK;


typedef struct _s_List_Stack_Manager
{
  ZXIC_COMM_FREELINK  *p_array;

  ZXIC_UINT32 capacity;

  ZXIC_UINT32 p_head;

  ZXIC_UINT32 free_num;
  ZXIC_UINT32 used_num;

}ZXIC_LISTSTACK_MANGER;

/*
**zxic_comm_liststack_creat:
*/

ZXIC_RTN32 zxic_comm_liststack_creat  (ZXIC_UINT32                  element_num, 
                                 ZXIC_LISTSTACK_MANGER  **p_list);

/*
**NOTE:index allocated from 0:
*/
ZXIC_RTN32 zxic_comm_liststack_alloc  (ZXIC_LISTSTACK_MANGER   *p_list,
                                 ZXIC_UINT32                 *index);
ZXIC_RTN32 zxic_comm_liststack_free   (ZXIC_LISTSTACK_MANGER*   p_list,
                                 ZXIC_UINT32                  index);
ZXIC_RTN32 zxic_comm_liststack_destroy(ZXIC_LISTSTACK_MANGER*   p_list);
ZXIC_RTN32 zxic_comm_liststack_alloc_spec_index(ZXIC_LISTSTACK_MANGER* p_list, ZXIC_UINT32 index);

ZXIC_RTN32 zxic_comm_liststack_show_used(ZXIC_LISTSTACK_MANGER* p_list, ZXIC_UINT32 line_number );
ZXIC_RTN32 zxic_comm_liststack_show_free (ZXIC_LISTSTACK_MANGER* p_list, ZXIC_UINT32 line_number );



#endif /* end "_FTMCOMM_LIST_STACK_H" */




