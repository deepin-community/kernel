#ifndef  _HDMI_H_
#define  _HDMI_H_


//-----------------------------------------------------------------------------
// Type Definition
//-----------------------------------------------------------------------------

typedef enum _HDMI_PNP_STATUS_
{
    HDMI_INTR_HOT_PLUG, 
    HDMI_INTR_OTHERS = 0xFF, 
}HDMI_PNP_STATUS;


/*
 *  Function:
 *      HDMI_Set_Mode
 *
 *  Input:
 *      pLogicalMode
 *
 *  Output:
 *      None
 *
 *  Return:
 *      0 - Success
 *      -1 - Error 
 *
 */
int EP952_HDMI_Set_Mode (logicalMode_t *pLogicalMode);

/*
 *  Function:
 *      HDMI_Read_Edid.
 *		 
 *  Input:
 *      pEDIDBuffer - EDID buffer
 *      bufferSize - EDID buffer size (usually 128-bytes or 256 bytes)
 *  Output:
 *      -1 - Error
 *      0 - exist block0 EDID (128 Bytes)
 *      1 - exist block0 & block1 EDID (256 Bytes)
 */
int EP952_HDMI_Read_Edid(char *pEDIDBuffer, unsigned long bufferSize);

/*
 *  Function:
 *      HDMI_hotplug_check
 *		 
 *  Input:
 *      None
 * 
 *  Output:
 *      0 - unplugged
 *      1 - plugged
 * 
 */

void hdmiHandler(void);

#endif  /* _HDMI_H_ */
