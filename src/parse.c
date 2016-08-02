#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "keyword.h"
#include "debug.h"
#include "validate.h"
#include "parse.h"
#include "xmalloc.h"
#include "readjson.h"
#include "loadfunc.h"

struct cmd_node
{
	struct list_head list;
	struct list_head child;
	struct list_head *parent;
	
	char *keyword;//关键字,可以为空
	char *meaning;//含义

	int required;//本节点是否必须
	int single;//本节点与其他同级节点互斥
	int hide;//本节点为隐藏节点，命令行不提示但可输入
	int null_para;//本节点的参数类型为布尔型，无需输入值

	cmd_func_t function;//执行函数
	int need_para;//是否需要参数
	int para_id;//参数的index

	validate_func_t validate_func;//验证参数合法性函数
	prompt_func_t prompt_func;//提示函数
	hook_func_t pre_hook;//命令执行前hook
	hook_func_t post_hook;//命令执行后hook
};

struct parameter_type
{
	char *name;
	validate_func_t validate;
};

#define INVALID_ERRNO -99999999

struct list_head g_root_list = LIST_HEAD_INIT(g_root_list);

struct parameter_type default_types[] = 
{
		{
			PARA_TYPE_NULL,
			NULL
		},
		{
			PARA_TYPE_STRING,
			is_string
		},
		{
			PARA_TYPE_IP,
			is_ip
		},
		{
			PARA_TYPE_INTEGER,
			is_integer
		},
		{
			PARA_TYPE_NUM,
			is_num
		},
		{
			PARA_TYPE_DATETIME,
			is_datetime
		},
		{
			PARA_TYPE_DATE,
			is_date
		},
		{
			PARA_TYPE_TIME,
			is_time
		}
};

int dumb_func()
{
	CFG_ERRORLN("load function error !!!\n");
}

static void init_cmd_node(struct cmd_node *node){
	memset(node,0,sizeof(*node));
	INIT_LIST_HEAD(&node->child);
}

static void free_cmd_node(struct cmd_node *node)
{
	if(node==NULL){
		return;
	}
	if(node->keyword!=NULL){
		xfree(node->keyword);
	}
	if(node->meaning!=NULL){
		xfree(node->meaning);
	}
	xfree(node);
}

static validate_func_t get_default_validate(char *type)
{
	int i;
	for(i=0;i<(sizeof(default_types)/sizeof(default_types[0]));i++)
	{
		if(strcmp(type,default_types[i].name)==0){
			return default_types[i].validate;
		}
	}
	return NULL;
}

static int add_cmd_node(struct cmd_node *node,struct list_head *position)
{
	list_add_tail(&node->list,position);
	return 0;
}

static struct cmd_node *  _parse_cmd_node(pconfig_node node, struct list_head *list){
	struct cmd_node *cmd_node;
	struct cmd_node *cmd_tmp;
	pconfig_attr attr;
	
	pconfig_node subpara = NULL;
	int ret = 0;

	char *node_name;

	char *attr_name;
	char *attr_value;

	char libname[256];
	char funcname[256];

	int line = (int)get_node_line(node);
	
	cmd_node = (struct cmd_node *)xmalloc(sizeof(struct cmd_node));
	if(cmd_node == NULL){
		CFG_ERRORLN("no memory.");
		return NULL;
	}
	init_cmd_node(cmd_node);
	
	attr = get_node_first_attribute(node);
	
	while(attr!=NULL){
		
		attr_name = get_attribute_name(attr);
		attr_value = get_attribute_value(attr);
	
		printf("attr_name:%s\n",attr_name);
		
		if(strcmp(attr_name,CMD_KEYWORD)==0){
			if(*attr_value!=0){
				cmd_node->keyword = (char *)xmalloc(strlen(attr_value) + 3);
				if(cmd_node->keyword == NULL){
					CFG_ERRORLN("no memory.");
					goto error;
				}
				strcpy(cmd_node->keyword, attr_value);
				node_name = cmd_node->keyword;
			}
		}
		else if(strcmp(attr_name,CMD_MEANING)==0){
			cmd_node->meaning= (char *)xmalloc(strlen(attr_value) + 3);
			if(cmd_node->meaning== NULL){
				CFG_ERRORLN("no memory.");
				goto error;
			}
			strcpy(cmd_node->meaning, attr_value);
		}
		else if(strcmp(attr_name,CMD_ATTRIBUTE)==0){
			if(strstr(attr_value,CMD_ATTRIBUTE_SINGLE)!=NULL){
				cmd_node->single = 1;
			}
			if(strstr(attr_value,CMD_ATTRIBUTE_REQUIRED)!=NULL){
				cmd_node->required = 1;
			}
			if(strstr(attr_value,CMD_ATTRIBUTE_HIDE)!=NULL){
				cmd_node->hide = 1;
			}
			//TODO:add new parameter attribute here.
		}
		else if(strcmp(attr_name,CMD_FUNC)==0){
			if(sscanf(attr_value,"%[^:]:%s",libname,funcname)==2){
				cmd_node->function = (cmd_func_t)load_func(libname,funcname);
			}
			if(cmd_node->function==NULL){
				CFG_ERRORLN("LINE %d:load function \"%s\" error of node \"%s\"",line,attr_value,node_name);
				cmd_node->function = dumb_func;
			}
		}
		else if(strcmp(attr_name,CMD_PROMPT_FUNC)==0){
			if(sscanf(attr_value,"%[^:]:%s",libname,funcname)==2){
				cmd_node->prompt_func = (prompt_func_t)load_func(libname,funcname);
			}
			if(cmd_node->prompt_func ==NULL){
				CFG_ERRORLN("LINE %d:load prompt function \"%s\" error of node \"%s\"",line,attr_value,node_name);
			}
		}
		else if(strcmp(attr_name,CMD_PARA)==0){
			cmd_node->need_para = 1;
			cmd_node->validate_func = get_default_validate(attr_value);
			if(strcmp(attr_value,PARA_TYPE_NULL)==0){
				cmd_node->null_para = 1;
			}
		}
		else if(strcmp(attr_name,CMD_PARA_ID)==0){
			ret = sscanf(attr_value,"%d",&cmd_node->para_id);
			if(ret!=1||cmd_node->para_id>128||cmd_node->para_id<0){
				CFG_ERRORLN("LINE %d:para id \"%s\" error of node \"%s\",must be 0-128.",line,attr_value,node_name);
				goto error;
			}
		}
		else if(strcmp(attr_name,CMD_SUBPARA)==0){
			subpara = (pconfig_node)attr_value;
		}
		//TODO:add new attribute here
		else{
			CFG_ERRORLN("LINE %d:unrecognized attribute %s of node %s",line,attr_name,node_name);
		}
		attr = get_next_attribute(attr);
	}
	if(cmd_node->null_para && cmd_node->keyword==NULL){
		CFG_ERRORLN("LINE %d:node has empty para can not has empty keyword",line);
		goto error;
	}
	add_cmd_node(cmd_node,list);

	if(subpara!=NULL){
		cmd_tmp = _parse_cmd_node(subpara,&cmd_node->child);
		if(cmd_tmp!=NULL){
			cmd_tmp->parent = &cmd_node->list;
		}
	}	
	return cmd_node;
error:
	free_cmd_node(cmd_node);
	return NULL;
}

static int print_cmd_tree(struct list_head *root, char *prompt)
{
	struct list_head *tmp;
	struct cmd_node *node;
	char new_prompt[256];
	
	struct list_head *list = root;

	if(list_empty(list)){
		CFG_DEBUG("cmd list empty!!!\n");
		return 0;
	}
	
	list_for_each(tmp,list){
		node = list_entry(tmp,struct cmd_node,list);
		CFG_PRINT("%s%s",prompt,node->keyword);
		if(node->required){
			CFG_PRINT(" [required]");
		}
		if(node->single){
			CFG_PRINT(" [single]");
		}
		if(node->keyword==NULL){
			CFG_PRINT(" [nokeyword]");
		}
		if(node->null_para){
			CFG_PRINT(" [nullpara]");
		}
		if(node->hide){
			CFG_PRINT(" [hide]");
		}
		CFG_PRINTLN("");
		if(!list_empty(&node->child)){
			sprintf(new_prompt,"  %s",prompt);
			print_cmd_tree(&node->child,new_prompt);
		}
	}
	return 0;
}

static void clear_cmd_tree(struct list_head *root)
{
	struct list_head *tmp;
	struct list_head *tmp2;
	struct cmd_node *node;
	
	struct list_head *list = root;

	if(list_empty(list)){
		return;
	}
	
	list_for_each_safe(tmp,tmp2,list){
		node = list_entry(tmp,struct cmd_node,list);
		if(!list_empty(&node->child)){
			clear_cmd_tree(&node->child);
		}
		list_del(tmp);
		free_cmd_node(node);
	}
}

void free_cmd_tree()
{
	clear_cmd_tree(&g_root_list);
}

static void parse_cmd_node(pconfig_node node, struct list_head *list){
	while(node!=NULL){
		_parse_cmd_node(node, list);
		node = get_next_node(node);
	}
}

int load_param(char *config_file){
	pconfig_tree tree = NULL;
	pconfig_node root = NULL;

	pconfig_node children = NULL;
	char *node_name = NULL;

	int line = 0;
	
	tree = load_config_tree(config_file);
	if(tree==NULL){
		return -1;
	}

	root = get_root_node(tree);

	if(root==NULL){
		return -1;
	}
	
	children = get_first_child_node(root);
	parse_cmd_node(children,&g_root_list);

	free_config_tree(tree);
	print_cmd_tree(&g_root_list," --");
	return 0;
}

struct escape_node
{
	char from;
	char to;
};

struct escape_node g_escape_list[] = 
{
	{'a'	,	'\a'},
	{'b'	,	'\b'},
	{'f'	,	'\f'},
	{'n'	,	'\n'},	
	{'r'	,	'\r'},
	{'t'	,	'\t'},
	{'v'	,	'\v'},
	{'0'	,	'\0'},
};

#define SCAN_STATUS_NOMAL 			0
#define SCAN_STATUS_IN_SQUOTE 		1
#define SCAN_STATUS_IN_DQUOTE 		2

#define isoct(x) ((x)>='0'&&(x)<='7')
#define ishex(x) (((x)>='0'&&(x)<='9')||((x)>='a'&&(x)<='f')||((x)>='A'&&(x)<='F'))

/*parse the escape character start with \ ,buffer will be changed */
static int parse_escape_character(char *buff, int len)
{
	char *p = buff;
	char *q = buff;
	char *end = buff + len;
	int digchars = 0;
	int tmp = 0;
	char num[8];
	int i = 0;
	int status = SCAN_STATUS_NOMAL;

	CFG_DEBUGLN("parse len is %d",len);
	while(*p!=0 && p < end){
		CFG_DEBUGLN("now parsing character %c",*p);
		if(*p == '\\' ){
			/* / is the last character */
			if( *(p+1) == 0 || p+1==end){
				*q = '\\';
				continue;
			}
			if(*(p+1)=='x' && p+2 < end){/* eg. \xAB */
				digchars = 0;
				while(digchars<=2 && (p+2+digchars)<end && ishex(p[2+digchars])){
					digchars++;
				}
				memcpy(num, p + 2, digchars);
				num[digchars] = 0;
				CFG_PRINTLN("digchars = %d,%s", digchars, num);
				if(sscanf(num,"%x" ,&tmp) == 1){
					*q = tmp;
					p += 2+digchars;
					q++;
					continue;
				}
			}
			else if(*(p+1)=='0' && p+2 < end){/*isdigit,eg. \007*/
				digchars = 0;
				while(digchars<=3 && (p+2+digchars)<end && isoct(p[2+digchars])){
					digchars++;
				}
				memcpy(num, p + 2, digchars);
				num[digchars] = 0;
				if(sscanf(num,"%o" ,&tmp) == 1){
					*q = tmp;
					p += 2+digchars;
					q++;
					continue;
				}
			}
			*q = *(p+1);
			for(i = 0; i < sizeof(g_escape_list)/sizeof(g_escape_list[0]); i++){//is normal escape ,eg. \t,\n
				CFG_DEBUGLN("from:[%c]p+1:[%c]",g_escape_list[i].from,*(p+1));
				if(g_escape_list[i].from == *(p+1)){
					*q = g_escape_list[i].to;
				}
			}			
			p += 2;
			q++;
			continue;
		}
		else if(*p=='\''){
			if(status==SCAN_STATUS_NOMAL){
				status = SCAN_STATUS_IN_SQUOTE;
				p++;
				continue;
			}
			else if(status==SCAN_STATUS_IN_SQUOTE){
				status = SCAN_STATUS_NOMAL;
				p++;
				continue;
			}
		}
		else if(*p=='\"'){
			if(status==SCAN_STATUS_NOMAL){
				status = SCAN_STATUS_IN_DQUOTE;
				p++;
				continue;
			}
			else if(status==SCAN_STATUS_IN_DQUOTE){
				status = SCAN_STATUS_NOMAL;
				p++;
				continue;
			}
		}
		*q=*p;
		p++;
		q++;
	}

	*q = 0;
	return 0;
}


#define PARSE_STATUS_WORD_START		0
#define PARSE_STATUS_WORD_CONTINUE	1
#define PARSE_STATUS_WORD_END 			2
#define PARSE_STATUS_WORD_SPLIT 		3
#define PARSE_STATUS_SINGLE_QUOTE 		4
#define PARSE_STATUS_DOUBLE_QUOTE 		5
#define PARSE_STATUS_TERMINAL 			6

struct word_node
{
	struct list_head list;
	int start_index;
	int word_len;
	char *word;
};
/*cmd will be changed ,then all the word need not alloc memory*/
static int cmd_to_word_list(char *cmd/*will be changed*/, struct list_head * word_list, int *word_num, int *end_is_space)
{
	char * p = cmd;
	char * wordstart = NULL;
	char * wordend = NULL;
	int wordlen = 0;

	int status = PARSE_STATUS_WORD_START;
	int wordnum = 0;
	int is_escape = 0;
	int ret = 0;
	
	struct word_node * word = NULL;

	struct list_head *tmp;
	struct list_head *tmp2;
	
	INIT_LIST_HEAD(word_list);
	
	CFG_DEBUGLN("cmd is :[%s]",cmd);
	wordstart = p;
	for(;;p++){
		CFG_DEBUGLN("now parse status is %d,current char is %c",status,*p);
		is_escape = 0;
		switch(*p){
			case '\n':
			case 0:
			case ' ':
				if(status == PARSE_STATUS_WORD_CONTINUE){
					if(*p==' '){
						status = PARSE_STATUS_WORD_SPLIT;
					}
					else{
						status = PARSE_STATUS_WORD_END;
					}
					wordend = p;
					wordlen = wordend - wordstart;
					word = (struct word_node *)xmalloc(sizeof(struct word_node) + wordlen + 3);
					if(word==NULL){
						CFG_ERRORLN("no memory.");
						ret = ENOMEM;
						goto error;
					}
					word->word_len = wordlen;
					word->start_index = wordstart - cmd ;
					
					parse_escape_character(wordstart, wordlen);
					word->word = wordstart;
					
					CFG_DEBUGLN("word is %s",word->word);
					list_add_tail(&word->list,word_list);
					wordnum++;
				}
				else{
					if(*p==0){
						goto out;
					}
				}
				if(status == PARSE_STATUS_WORD_END){
					goto out;
				}
				break;
			case '\'':
				if(status == PARSE_STATUS_SINGLE_QUOTE){
					status = PARSE_STATUS_WORD_CONTINUE;
				}
				else if(status == PARSE_STATUS_WORD_CONTINUE){
					status = PARSE_STATUS_SINGLE_QUOTE;
				}
				else if(status==PARSE_STATUS_WORD_SPLIT || status==PARSE_STATUS_WORD_START){
					status = PARSE_STATUS_SINGLE_QUOTE;
					wordstart = p;
				}
				break;
			case '\"':
				if(status == PARSE_STATUS_DOUBLE_QUOTE){
					status = PARSE_STATUS_WORD_CONTINUE;
				}
				else if(status == PARSE_STATUS_WORD_CONTINUE){
					status = PARSE_STATUS_DOUBLE_QUOTE;
				}
				else if(status==PARSE_STATUS_WORD_SPLIT  || status==PARSE_STATUS_WORD_START){
					status = PARSE_STATUS_DOUBLE_QUOTE;
					wordstart = p;
				}
				break;
			case '\\':
				if(*(p+1)=='\'' || *(p+1)=='\"'){
					is_escape = 1;
				}
			default:
				if(status==PARSE_STATUS_WORD_SPLIT  || status==PARSE_STATUS_WORD_START){
					status = PARSE_STATUS_WORD_CONTINUE;
					wordstart = p;
				}
				if(is_escape){
					p++;
				}
				break;
		}
	}
out:
	*word_num = wordnum;
	if(status==PARSE_STATUS_WORD_SPLIT){
		*end_is_space = 1;
	}
	else{
		*end_is_space = 0;
	}
	
	/*print the word list*/
	list_for_each(tmp,word_list){		
		word = list_entry(tmp,struct word_node,list);
		CFG_PRINTLN("[%s]",word->word);
	}
	/*print the word end*/

	return 0;

error:
	list_for_each_safe(tmp,tmp2,word_list)
	{
		word = list_entry(tmp,struct word_node,list);
		list_del(tmp);
		xfree(word);
	}
	return ret;
}

static void clear_word_list(struct list_head * word_list)
{
	struct list_head *tmp;
	struct list_head *tmp2;
	struct word_node *node;
	
	list_for_each_safe(tmp, tmp2, word_list)
	{
		node = list_entry(tmp, struct word_node, list);
		list_del(tmp);
		xfree(node);
	}
}


struct tmp_cmd_node
{
	struct list_head list;
	struct cmd_node *node;
};

#define MATCH_STATUS_NOTMATCH 			0
#define MATCH_STATUS_FULLMATCH 		1
#define MATCH_STATUS_IS_PREFIX 			2
#define MATCH_STATUS_KEYWORD_OMIT		3
#define MATCH_STATUS_MULTI_PREFIX 		4
#define MATCH_STATUS_MULTI_PREFIX_ONE_MATCH 		5

static int word_match(char *pattern, char *string)
{
	char *p = pattern;
	char *pq = string;

	if(p==NULL||*p==0){
		if(pq==NULL||*pq==0){
			return MATCH_STATUS_FULLMATCH;
		}
		else{
			return MATCH_STATUS_IS_PREFIX;
		}
	}
		
	if(pq==NULL||*pq==0){
		return MATCH_STATUS_NOTMATCH;
	}
	
	while(*p!=0){
		if(*p!=*pq)
			return MATCH_STATUS_NOTMATCH;
		p++;
		pq++;
	}
	
	if(*pq==0){
		return MATCH_STATUS_FULLMATCH;
	}
	return MATCH_STATUS_IS_PREFIX;
}

static void clear_tmp_cmd_list(struct list_head *cmd_list)
{
	struct list_head *tmp;
	struct list_head *tmp2;
	struct tmp_cmd_node *node;
	
	list_for_each_safe(tmp, tmp2, cmd_list)
	{
		node = list_entry(tmp, struct tmp_cmd_node, list);
		list_del(tmp);
		xfree(node);
	}
}

static int make_tmp_cmd_list(struct list_head *from, struct list_head *to)
{
	struct list_head *tmp;
	struct tmp_cmd_node *node;
	struct cmd_node *cmd;
	
	INIT_LIST_HEAD(to);

	list_for_each(tmp,from){
		cmd = list_entry(tmp,struct cmd_node,list);
		node =(struct tmp_cmd_node *)xmalloc(sizeof(struct tmp_cmd_node));

		if(node==NULL){
			clear_tmp_cmd_list(to);
			CFG_ERRORLN("no memory.");
			return ENOMEM;
		}
		
		node->node = cmd;
		list_add_tail(&node->list, to);
	}
	
	return 0;
}

struct argv_node
{
	struct list_head list;
	int index;
	char *argv;
};

void clear_argv_list(struct list_head *argv_list)
{
	struct list_head *tmp;
	struct list_head *tmp2;
	struct argv_node *node;
	
	list_for_each_safe(tmp, tmp2, argv_list)
	{
		node = list_entry(tmp, struct argv_node, list);
		list_del(tmp);
		xfree(node);
	}
}

#define CMD_EXEC_OK		 				0
#define CMD_EXEC_KEYWORD_NOT_FOUND	1
#define CMD_EXEC_NEED_PARA				2
#define CMD_EXEC_KEYWORD_CONFUSED	3
#define CMD_EXEC_PARA_CONFLICT			4
#define CMD_EXEC_PARA_INVALID			5

#define CMD_PROMPT_KEYWORD			1
#define CMD_PROMPT_PARA				2
#define CMD_PROMPT_END					3
#define CMD_PROMPT_ERROR				4

struct parse_context
{
	char *cmd;
	int end_is_space;

	int max_para_index;
	int must_end;
	int exec_status;
	int prompt_status;

	struct cmd_node *matched;
	
	char **prompt_word;
	int max_prompt;
	int got_prompt;
	int prompt_word_len;
	
	struct word_node *current_word;
	int is_last_word;
	char *prompt_prefix;
	
	cmd_func_t function;//执行函数	
	hook_func_t pre_hook;//命令执行前hook
	hook_func_t post_hook;//命令执行后hook
	int argc;//参数个数，不计入空参数
	char **argv;//参数列表
}
;
#define MAX_PROMPT_NUM 4096
static void init_parse_context(struct parse_context *ctx)
{
	ctx->cmd = NULL;
	ctx->end_is_space = 0;
	
	ctx->max_para_index = 0;
	ctx->must_end = 0;
	ctx->exec_status = CMD_EXEC_OK;
	ctx->prompt_status = CMD_PROMPT_KEYWORD;
	
	ctx->matched = NULL;

	ctx->current_word = NULL;
	ctx->is_last_word = 0;
	ctx->prompt_prefix  = NULL;
	
	ctx->prompt_word = NULL;
	ctx->max_prompt = MAX_PROMPT_NUM;
	ctx->got_prompt = 0;
	ctx->prompt_word_len = 0;
	
	ctx->function = NULL;
	ctx->pre_hook = NULL;
	ctx->post_hook = NULL;
	ctx->argc = 0;
	ctx->argv = NULL;
}

static int is_prefix(char *a,char*ab){
	char *p = a;
	char *pq = ab;
	if(a==NULL||*a==0)
		return 1;
	if(ab==NULL||*ab==0)
		return 0;
	while(*p!=0){
		if(*p!=*pq)
			return 0;
		p++;
		pq++;
	}
	return 1;
}

static void free_prompt_words_internal(char **prompt_words)
{
	int i = 0;
	if(prompt_words==NULL){
		return;
	}
	for(i=0; prompt_words[i]!=NULL; i++){
		xfree(prompt_words[i]);
		prompt_words[i] = NULL;
	}
}

static int get_keyword_from_prompt_func(prompt_func_t func, struct parse_context *ctx)
{
	CFG_DEBUGLN("get prompt from customed function.");
	if(func==NULL){
		return 0;
	}
	return func(ctx->prompt_prefix, &ctx->prompt_word, &ctx->prompt_word_len);
}

/*all memory alloced in this function will be freed by destroy_parse_context*/
static int get_keyword_from_tmp_list(struct list_head *list_tmp, struct parse_context *ctx)
{
	struct list_head *tmp;
	struct tmp_cmd_node *tmp_cmd;
	struct cmd_node *cmd;
	int ret = 0;
	int keyword_num = 0;
	char **word_list = NULL;
	int is_first = 1;
	int i = 0;

	CFG_DEBUGLN("prefix is [%s]", ctx->prompt_prefix);
	/*first ,count the num of prompt word,and then allocate space*/
	list_for_each(tmp, list_tmp){
		tmp_cmd = list_entry(tmp,struct tmp_cmd_node,list);
		cmd = tmp_cmd->node;
		
		if(is_first && cmd->keyword==NULL){/*next node has no keyword, prompt his para */
			ctx->prompt_status = CMD_PROMPT_PARA;
			return get_keyword_from_prompt_func(cmd->prompt_func, ctx);
		}
		
		if(is_prefix(ctx->prompt_prefix, cmd->keyword)){
			keyword_num++;
		}
		is_first = 0;
	}
	word_list = (char **)xmalloc(sizeof(char *) * (keyword_num + 5));
	if(word_list==NULL){	
		ret = ENOMEM;
		goto error;
	}
	memset(word_list, 0, keyword_num + 5);
	list_for_each(tmp, list_tmp){
		tmp_cmd = list_entry(tmp, struct tmp_cmd_node, list);
		cmd = tmp_cmd->node;
		CFG_DEBUGLN(" [%s]<-->[%s]", ctx->prompt_prefix, cmd->keyword);
		if(cmd->keyword!=NULL && is_prefix(ctx->prompt_prefix, cmd->keyword)){
			word_list[i] = (char *)xmalloc(strlen(cmd->keyword) + 10);
			if(word_list[i]==NULL){
				ret = ENOMEM;
				goto error;
			}
			strcpy(word_list[i], cmd->keyword);
			i++;
		}
		if(i>=keyword_num ||i >= ctx->max_prompt){
			break;
		}
	}
	word_list[i] = NULL;
	ctx->prompt_word = word_list;
	ctx->prompt_word_len = keyword_num;
	return 0;
error:
	free_prompt_words_internal(word_list);
	xfree(word_list);
	word_list = NULL;
	ctx->prompt_word = NULL;
	ctx->prompt_word_len = 0;
	return ret;
}

/*empty keyword command must be sequence.*/
static struct argv_node * get_argv_from_matched_cmd(struct word_node *word, struct cmd_node *cmd_matched,struct parse_context *ctx)
{
	struct argv_node *new_argv = NULL;
	new_argv = (struct argv_node *)xmalloc(sizeof(struct argv_node));
					
	if(new_argv == NULL){
		return NULL;
	}
					
	new_argv->index = cmd_matched->para_id;
	if(cmd_matched->para_id > ctx->max_para_index){
		ctx->max_para_index = cmd_matched->para_id;
	}
	
	if(cmd_matched->null_para){
		new_argv->argv = "true";
	}
	else{
		new_argv->argv = word->word;
	}
	return new_argv;
}

static int is_last_word(struct word_node *node)
{
	return (node->list.next->next == &node->list);
}

static void start_word(struct list_head *list, struct parse_context *ctx)
{
	if(list_empty(list)){
		ctx->current_word = NULL;
	}
	ctx->current_word = list_entry(list->next, struct word_node,list);
	
	if(is_last_word(ctx->current_word)){
		ctx->is_last_word = 1;
		if(!ctx->end_is_space){
			CFG_DEBUGLN("get the prefix [%s].", ctx->current_word->word);
			ctx->prompt_prefix = ctx->current_word->word;
		}
	}
}

static void shift_word(struct parse_context *ctx)
{
	struct word_node *next =  NULL;
	struct word_node *node = ctx->current_word;
	if(node==NULL){
		return;
	}
	
	next = list_entry(node->list.next,struct word_node,list);
	
	list_del(&node->list);
	xfree(node);
	
	ctx->current_word = next;
	if(is_last_word(ctx->current_word)){
		ctx->is_last_word = 1;
		if(!ctx->end_is_space){
			CFG_DEBUGLN("get the prefix [%s].", ctx->current_word->word);
			ctx->prompt_prefix = ctx->current_word->word;
		}
	}
	
}

static void get_cmd_functions(struct cmd_node *cmd, struct parse_context *ctx)
{
	if(cmd->function!=NULL){
		ctx->function = cmd->function;
	}
	if(cmd->pre_hook!=NULL){
		ctx->pre_hook = cmd->pre_hook;
	}
	if(cmd->post_hook!=NULL){
		ctx->post_hook = cmd->post_hook;
	}
}

static int word_to_argv_list(struct list_head *cmd_list, struct list_head *word_list, struct list_head *argv_list, struct parse_context *ctx)
{
	struct list_head list_tmp;

	INIT_LIST_HEAD(&list_tmp);

	struct list_head *tmp = NULL;

	struct tmp_cmd_node *tmp_cmd = NULL;
	struct tmp_cmd_node *matched_tmp_node = NULL;
	
	struct cmd_node *cmd = NULL;
	
	int ret = 0;
	
	int tmp_status = MATCH_STATUS_NOTMATCH;
	int match_status = MATCH_STATUS_NOTMATCH;
	int is_first_node = 0;
	int child_parsed = 0;

	struct cmd_node *cmd_matched = NULL;
	struct argv_node *new_argv = NULL;
	
	if(list_empty(cmd_list)){
		CFG_INFOLN("error:command list is empty!");
		return 0;
	}
	
	start_word(word_list, ctx);

	ret = make_tmp_cmd_list(cmd_list, &list_tmp);
	if(ret != 0){
		return ret;
	}
	
	if(ctx->current_word == NULL){
		CFG_INFOLN("error:word list is empty!");
		goto end;
	}
	
	while(!list_empty(&list_tmp)){		
		
		match_status = MATCH_STATUS_NOTMATCH;
		matched_tmp_node = NULL;
		cmd_matched = NULL;
		is_first_node = 1;
		
		list_for_each(tmp,&list_tmp){
			tmp_cmd = list_entry(tmp,struct tmp_cmd_node,list);
			cmd = tmp_cmd->node;
			
			if(cmd->keyword==NULL && is_first_node){/*none-keyword command must the first node*/
				match_status = MATCH_STATUS_FULLMATCH;
				matched_tmp_node = tmp_cmd;
				break;
			}
			
			CFG_PRINTLN("%s<=>%s",cmd->keyword, ctx->current_word->word);

			tmp_status = word_match(ctx->current_word->word, cmd->keyword);
			if(tmp_status == MATCH_STATUS_FULLMATCH){/*has get the correct node.*/
				match_status =  MATCH_STATUS_FULLMATCH;
				matched_tmp_node = tmp_cmd;
				break;
			}
			else if(tmp_status == MATCH_STATUS_IS_PREFIX){
				if(match_status == MATCH_STATUS_NOTMATCH){
					match_status =  MATCH_STATUS_IS_PREFIX;
					matched_tmp_node = tmp_cmd;
				}
				else if(match_status == MATCH_STATUS_IS_PREFIX){
					match_status =  MATCH_STATUS_MULTI_PREFIX;
					matched_tmp_node = NULL;
				}
			}
			CFG_PRINTLN("now status is %d",match_status);
			is_first_node = 0;
		}

		if(matched_tmp_node!=NULL){
			cmd_matched = matched_tmp_node->node;
			CFG_DEBUGLN("free the cmd node %s.",cmd_matched->keyword);
			if(ctx->prompt_prefix == NULL){/*if this is last word and be used as prefix, do not delete it*/
				list_del(&matched_tmp_node->list);
				xfree(matched_tmp_node);
			}
		}
		
		CFG_DEBUGLN("match status is %d.",match_status);

		switch(match_status){
			case MATCH_STATUS_NOTMATCH:
				CFG_DEBUGLN("no matched keyword.");				
				ctx->exec_status = CMD_EXEC_KEYWORD_NOT_FOUND;
				ctx->prompt_status = CMD_PROMPT_ERROR;
				goto end;
				
			case MATCH_STATUS_MULTI_PREFIX:		
				CFG_DEBUGLN("match multi prefix.");
				ctx->exec_status = CMD_EXEC_KEYWORD_CONFUSED;
				if(ctx->prompt_prefix == NULL){
					ctx->prompt_status = CMD_PROMPT_ERROR;
				}
				goto end;
				
			case MATCH_STATUS_IS_PREFIX:
			case MATCH_STATUS_FULLMATCH:
				CFG_DEBUGLN("match full key word %s.",ctx->current_word->word);
				ctx->matched = cmd_matched;
				get_cmd_functions(cmd_matched, ctx);

				if(cmd_matched->keyword!=NULL){
					if(ctx->is_last_word){
						CFG_DEBUGLN("parse end.");
						if(cmd_matched->need_para && !cmd_matched->null_para){
							ctx->exec_status = CMD_EXEC_NEED_PARA;
							if(ctx->prompt_prefix==NULL){
								CFG_DEBUGLN("prompt para.");
								ctx->prompt_status = CMD_PROMPT_PARA;
							}
						}
						goto end;
					}
					shift_word(ctx);
				}

				if(cmd_matched->need_para){
					CFG_DEBUGLN("need para.");
					
					new_argv = get_argv_from_matched_cmd(ctx->current_word,cmd_matched,ctx);
					if(new_argv == NULL){
						ret = ENOMEM;
						goto end;
					}
					if(cmd_matched->validate_func){/*check the para by validate function*/
						if(cmd_matched->validate_func(new_argv->argv)==0){
							CFG_DEBUGLN("para %s error.",new_argv->argv);
							ctx->exec_status = CMD_EXEC_PARA_INVALID;
							if(ctx->prompt_prefix==NULL){
								ctx->prompt_status = CMD_PROMPT_ERROR;
							}
							goto end;
						}
					}
					list_add_tail(&new_argv->list,argv_list);
					
					CFG_DEBUGLN("get argv [%s].",new_argv->argv);

					if(cmd_matched->need_para && !cmd_matched->null_para){/*need a not empty para, and end is not space, then prompt para*/
						if(ctx->is_last_word){
							if( ctx->prompt_prefix!=NULL){
								CFG_DEBUGLN("prompt para.");
								ctx->prompt_status = CMD_PROMPT_PARA;
							}
							goto end;
						}
						shift_word(ctx);
					}
				}
				
				if(!list_empty(&cmd_matched->child)){//parse the child of this node
					ret = word_to_argv_list(&cmd_matched->child, word_list, argv_list,ctx);
					if(ctx->must_end){
						CFG_DEBUGLN("parse error or end,must end.");
						child_parsed = 1;//the child tree has been looped,need not parse again.
						goto end;
					}
				}
				if(cmd_matched->single){//single node,conflict with other node
					ctx->exec_status = CMD_EXEC_PARA_CONFLICT;
					ctx->prompt_status = CMD_PROMPT_ERROR;
					goto end;
				}

				break;
			default:
				break;
		}
	}
	
	clear_tmp_cmd_list(&list_tmp);
	return 0;
end:
	ctx->must_end = 1;
	if(ctx->got_prompt==0){
		CFG_DEBUGLN("start get the prompt");
		if(ctx->prompt_status==CMD_PROMPT_KEYWORD){
			if(cmd_matched!= NULL){
				if(!list_empty(&cmd_matched->child) && !child_parsed){
					if(ctx->prompt_prefix==NULL){
						clear_tmp_cmd_list(&list_tmp);
						ret = make_tmp_cmd_list(&cmd_matched->child, &list_tmp);
					}
				}
				else if(cmd_matched->single && ctx->prompt_prefix==NULL){/*touch the end, need not prompt*/
					CFG_DEBUGLN("touch the end.");
					clear_tmp_cmd_list(&list_tmp);
					ctx->prompt_status = CMD_PROMPT_END;
				}
			}
			if(!list_empty(&list_tmp)){
				get_keyword_from_tmp_list(&list_tmp, ctx);
				ctx->got_prompt = 1;
			}
		}
		else if(ctx->prompt_status==CMD_PROMPT_PARA){
			get_keyword_from_prompt_func(ctx->matched->prompt_func,ctx);
			ctx->got_prompt = 1;
		}
	}
	CFG_DEBUGLN("parse end,exec status %d,prom status %d",ctx->exec_status,ctx->prompt_status);
	clear_tmp_cmd_list(&list_tmp);	
	return ret;
}
static int argv_list_to_argv(struct list_head *argv_list, int max_index, struct parse_context *ctx)
{
	struct list_head *tmp;
	struct argv_node *node;
	int alloc_len = sizeof(char *) * max_index + 2;
	char **p = (char **)xmalloc(alloc_len);/*will be freed by destroy_parse_context*/
	int i = 0;
	
	if(p==NULL){
		ctx->argv = NULL;
		return ENOMEM;
	}
	memset(p, 0, alloc_len);

	list_for_each(tmp, argv_list){
		node = list_entry(tmp, struct argv_node, list);
		p[node->index] = node->argv;
		CFG_DEBUGLN(" get argv[%d]=%s ", node->index, node->argv);
		i++;
	}
	CFG_DEBUGLN(" get %d args ", i);
	ctx->argv = p;
	ctx->argc = i;
	return 0;
}

static int parse_cmd(char *cmd, struct parse_context *ctx)
{
	struct list_head word_list;
	struct list_head argv_list;

	int ret = 0;
		
	int word_num = 0;

	char *cmd_clone = NULL;
	int cmd_len = 0;
	
	INIT_LIST_HEAD(&word_list);
	INIT_LIST_HEAD(&argv_list);

	/*clone the cmd, and will be freed by destroy_parse_context*/
	if(cmd!=NULL){
		cmd_len = strlen(cmd) + 10;
		cmd_clone = (char *)xmalloc(cmd_len);
		if(cmd_clone==NULL){
			return ENOMEM;
		}
		strcpy(cmd_clone, cmd);
		ctx->cmd = cmd_clone;
	}
	
	ret = cmd_to_word_list(ctx->cmd, &word_list, &word_num, &ctx->end_is_space);
	if(ret==0){
		ret = word_to_argv_list(&g_root_list, &word_list, &argv_list, ctx);
	}
	if(ret==0 && ctx->exec_status==CMD_EXEC_OK){
		ret = argv_list_to_argv(&argv_list, ctx->max_para_index, ctx);
	}
	
	CFG_DEBUGLN("parse argv from cmd ret = %d,parse status=%d,prom status=%d",ret,ctx->exec_status,ctx->prompt_status);

	clear_argv_list(&argv_list);
	clear_word_list(&word_list);
	return ret;
}

void destroy_parse_msg(struct parse_msg *msg)
{
	free_prompt_words_internal(msg->prompt_word);

	xfree(msg->prompt_word);
	msg->prompt_word  = NULL;

	xfree(msg->argv);
	msg->argv = NULL;
	
	xfree(msg->cmd);
	msg->cmd = NULL;
}

int analysis_cmd(char *cmd, struct parse_msg *msg)
{
	struct parse_context ctx;
	int ret = 0;

	init_parse_context(&ctx);
	ret = parse_cmd(cmd, &ctx);

	msg->cmd = ctx.cmd;
	msg->prompt_word = ctx.prompt_word;
	msg->prompt_count = ctx.prompt_word_len;
	msg->error_index = ctx.current_word->start_index;
	msg->argc = ctx.argc;
	msg->argv = ctx.argv;
	msg->function = ctx.function;
	msg->error_code = ret;

	return ret;
}

int exec_cmd(char *cmd)
{
	int ret = 0;
	struct parse_msg msg;
	ret = analysis_cmd(cmd, &msg);

	if(ret==0){
		ret = msg.function(msg.argc, msg.argv, NULL);
	}
	destroy_parse_msg(&msg);
	return ret;
}

int prompt_cmd(char *cmd, struct parse_msg *msg)
{
	int ret = 0;
	ret = analysis_cmd(cmd, msg);
	int i;

	if(ret==0){
		CFG_DEBUGLN("count is [%d]",msg->prompt_count);
		for(i=0;i<msg->prompt_count;i++){
			CFG_DEBUGLN("prompt is [%s]",msg->prompt_word[i]);
		}
	}
	
	destroy_parse_msg(msg);
	return ret;
}

