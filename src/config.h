struct cmd_config
{
	int listen_port;
	int max_process;//���̳���������
	char *debug_file;
	int debug_level; 
	//TODO:add new configuration
};
struct cmd_config global_config = 
{
	.listen_port = 9000,
	.max_process = 16,
	.debug_file = NULL,
	.debug_level = 7
};
