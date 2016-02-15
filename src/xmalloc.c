#include <malloc.h>
#include "debug.h"

int malloc_num = 0;
void * xmalloc(size_t size)
{
	malloc_num++;
	//CFG_DEBUGLN("malloc num is %d",malloc_num);
	return malloc(size);
}

void xfree(void *ptr)
{
	if(ptr!=NULL){
		malloc_num--;
	}
	//CFG_DEBUGLN("malloc num is %d",malloc_num);
	free(ptr);
}

