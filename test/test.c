#include <stdio.h>
int hello(int argc, char **argv)
{
	int i;
	printf("there is %d args\n",argc);
	for(i=0;i<argc;i++){
		printf("argv[%d]--%s---\n",i,argv[i]);
	}
	return 0;
}
