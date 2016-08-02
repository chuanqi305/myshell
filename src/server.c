#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <unistd.h>

#include "parse.h"
#include "debug.h"
#include "list.h"
int main(int argc,char *argv[])
{
	char workdir[1024];
	struct parse_msg msg;
	
	snprintf(workdir,sizeof(workdir),"%s",dirname(argv[0]));
	chdir(workdir);
	debug_init();
	load_param(argv[1]);

	fgets(workdir,1024,stdin);

	exec_cmd(workdir);
	prompt_cmd(workdir, &msg);
	
	free_cmd_tree();
	return 0;
}
