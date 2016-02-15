#ifndef __COMMON_DEBUG_H__
#define __COMMON_DEBUG_H__

#include <stdio.h>

extern FILE * debug_fp;
extern int debug_level;
extern void debug_init();

#define EMERG    0  /* system is unusable               */
#define ALERT    1  /* action must be taken immediately */
#define CRIT     2  /* critical conditions              */
#define ERR      3  /* error conditions                 */
#define WARNING  4  /* warning conditions               */
#define NOTICE   5  /* normal but significant condition */
#define INFO     6  /* informational                    */
#define DEBUG    7  /* debug-level messages             */

#define __CFG_DEBUG(level,formate,args...) do {\
		if(level<=debug_level){\
			fprintf(debug_fp,"%s:%s(%d):"formate,__FILE__,__FUNCTION__,__LINE__,##args);\
		}\
	}\
	while(0)
		
#define __CFG_DEBUGLN(level,formate,args...) do {\
		if(level<=debug_level){\
			fprintf(debug_fp,"%s:%s(%d):"formate"\n",__FILE__,__FUNCTION__,__LINE__,##args);\
		}\
	}\
	while(0)
		
#define CFG_PRINT(formate,args...)	 do {\
		if(DEBUG<=debug_level){\
			fprintf(debug_fp,formate,##args);\
		}\
	}\
	while(0)
		
#define CFG_PRINTLN(formate,args...)	 do {\
		if(DEBUG<=debug_level){\
			fprintf(debug_fp,formate"\n",##args);\
		}\
	}\
	while(0)
		
#define CFG_EMERG(formate,args...)  __CFG_DEBUG(EMERG,formate,##args)
#define CFG_ALERT(formate,args...)  __CFG_DEBUG(ALERT,formate,##args)
#define CFG_CRITICAL(formate,args...)  __CFG_DEBUG(CRIT,formate,##args)
#define CFG_ERROR(formate,args...)  __CFG_DEBUG(ERR,formate,##args)
#define CFG_WARNING(formate,args...)  __CFG_DEBUG(WARNING,formate,##args)
#define CFG_NOTICE(formate,args...)  __CFG_DEBUG(NOTICE,formate,##args)
#define CFG_INFO(formate,args...)  __CFG_DEBUG(INFO,formate,##args)
#define CFG_DEBUG(formate,args...)  __CFG_DEBUG(DEBUG,formate,##args)

#define CFG_EMERGLN(formate,args...)  __CFG_DEBUGLN(EMERG,formate,##args)
#define CFG_ALERTLN(formate,args...)  __CFG_DEBUGLN(ALERT,formate,##args)
#define CFG_CRITICALLN(formate,args...)  __CFG_DEBUGLN(CRIT,formate,##args)
#define CFG_ERRORLN(formate,args...)  __CFG_DEBUGLN(ERR,formate,##args)
#define CFG_WARNINGLN(formate,args...)  __CFG_DEBUGLN(WARNING,formate,##args)
#define CFG_NOTICELN(formate,args...)  __CFG_DEBUGLN(NOTICE,formate,##args)
#define CFG_INFOLN(formate,args...)  __CFG_DEBUGLN(INFO,formate,##args)
#define CFG_DEBUGLN(formate,args...)  __CFG_DEBUGLN(DEBUG,formate,##args)



/*fast debug for temporary add*/
#define D \
		do \
		fprintf(stderr,"%s:%s(%d)\n",__FILE__,__FUNCTION__,__LINE__);\
		while(0)

#endif /*__COMMON_DEBUG_H__*/
