#ifndef __PARSE_H__
#define  __PARSE_H__

struct context;
typedef int (*cmd_func_t)(int argc, char **argv, struct context *ctx);
typedef int (*prompt_func_t)(char *prefix, char ***keywords, int *len);
typedef int (*validate_func_t)(char *parameter);
typedef int (*hook_func_t)(int argc, char **argv, struct context *ctx);

struct parse_msg
{
	char *cmd;
	int argc;
	char **argv;
	cmd_func_t function;
	char **prompt_word;
	int prompt_count;
	int error_index;
	int error_code;
	char *msg;
};

void free_cmd_tree();
void free_errno_list();
int load_param(char *config_file);
int prompt_cmd(char *cmd, struct parse_msg *msg);
int exec_cmd(char *cmd);
void destroy_parse_msg(struct parse_msg * msg);
#endif /* __PARSE_H__*/