#include <string.h>
#include <dlfcn.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
 #include <stdlib.h>

#include "debug.h"
void * load_func(char *libpath, char *func_name)
{
	void *handle;
	void * function;
	char fullpath[1024];
	
	if(libpath==NULL||func_name==NULL){
		CFG_ERRORLN("load function:library path or function name is NULL.");
		return NULL;
	}
	
	handle = dlopen(libpath,RTLD_LAZY);
	if(handle==NULL && libpath[0]!='/'){
		snprintf(fullpath, sizeof(fullpath), "%s/%s", getenv("PWD") ,libpath);
		handle = dlopen(fullpath,RTLD_LAZY);
	}
	if(handle!=NULL){
		function = (void *)dlsym(handle,func_name);
		if(function==NULL){
			CFG_ERRORLN("dlsym:%s",dlerror());
		}
		return function;
	}
	else{
		CFG_ERRORLN("dlopen:%s",dlerror());
		return NULL;
	}
	return NULL;
}
