#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "config.h"

FILE * debug_fp = NULL;
int debug_level = 0;

void debug_init()
{
	FILE * fp = NULL;
	if(global_config.debug_file!=NULL){
		fp = fopen(global_config.debug_file,"a+");
		if(fp!=NULL){
			fclose(debug_fp);
			debug_fp = fp;
		}
	}
	if(debug_fp==NULL){
		debug_fp = stderr;
	}
	debug_level = global_config.debug_level;
}

void debug_set_level(int level){
	debug_level = level;
}

void debug_set_file(char *path){
	FILE * fp = NULL;
	fp = fopen(path,"a+");
	if(fp!=NULL){
		fclose(debug_fp);
		debug_fp = fp;
	}
	
}