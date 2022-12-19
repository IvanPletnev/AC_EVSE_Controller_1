/*
**************************************************************************************************************
*	@file	debug.c
*	@author Ysheng
*	@version 
*	@date    
*	@brief	debug
***************************************************************************************************************
*/

#ifndef __debug_H
#define __debug_H
#ifdef __cplusplus
 extern "C" {
#endif
#include <stdio.h>

#define DEBUG__						1
#define DEBUG_LEVEL	  				2					//���Եȼ������DEBUG���Ժ���Ƶ��������Χ,���ڸõȼ��ĵ��Բ����	 
	 
#define _DEBUG(level, fmt, arg...)  if(level <= DEBUG_LEVEL)	printf(fmt,##arg);


#ifdef __cplusplus
}
#endif
#endif /*__ pinoutConfig_H */
