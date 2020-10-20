/******************************************************************************
* Copyright (c) 2007-2016, ZeroTech Co., Ltd.
* All rights reserved.
*******************************************************************************
* File name     : osp_common.c
* Description   : common function
* Version       : v1.0
* Create Time   : 2016/4/13
* Author   		: wangbo
* Modify history:
*******************************************************************************
* Modify Time   Modify person  Modification
* ------------------------------------------------------------------------------
*
*******************************************************************************/

/******************************* include *********************************/
#include "stdio.h"
#include "osp_syslog.h"
#include "osp_common.h"
#include <sys/un.h>
#include <sys/time.h>
#include <time.h>
#include "osp_syslog.h"

#define IP_ADDR_LEN                    4
#define LOAD_FAIL                      0
#define LOAD_SUCCESS                   1
#define OSP_INVALID_TIMERID            0xFFFFFFFF

ipinfo ip_pool[IP_POOL] ;
//ipinfo port_pool[IP_POOL] = {0} ;



/*******************************************************************************
* function name	: osp_print_buffer
* description	: print timer node information
* param[in] 	:
* param[out] 	: none
* return 		: 0:success ;-1:failed
*******************************************************************************/
void osp_print_buffer (u8 logid, u8 loglevel, u8 * detail, u8 * buffer, s32 len)
{
	u8 print[MAX_PRINT_BUFSIZE] = { 0 };
	s32 index = 0;

	if (len > 0){
		len = len > MAX_MSG_BUFSIZE ? MAX_MSG_BUFSIZE : len;

		for (index = 0; index < len; index++){
			sprintf ((char *)&print[3 * index], "%02x ", *buffer++);
		}

		//SYSLOG(logid, loglevel, "%s%s\n", detailStr, u8PrintStr);
		DEBUG(LOG_TRACE,"%s %d %s\n", detail, len, print);
	}
}
/*******************************************************************************
* function name	: osp_free
* description	: free data address
* param[in] 	:
* param[out] 	: none
* return 		: none
*******************************************************************************/
void osp_free (void *data)
{
	if (data != NULL){
		free (data);
	}
}
