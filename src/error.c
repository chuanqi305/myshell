#include <stdio.h>
#include <string.h>
#include <errno.h>

#include "list.h"
#include "keyword.h"
#include "debug.h"
#include "parse.h"
#include "xmalloc.h"
#include "readxml.h"

struct errno_node
{
	struct list_head list;
	int num;
	char *meaning;
};

struct list_head g_errno_list = LIST_HEAD_INIT(g_errno_list);

void free_errno_list()
{
	struct list_head *tmp;
	struct list_head *tmp2;
	struct errno_node *node;
	
	struct list_head *list = &g_errno_list;

	if(list_empty(list)){
		return;
	}
	
	list_for_each_safe(tmp,tmp2,list){
		node = list_entry(tmp,struct errno_node,list);
		list_del(tmp);
		xfree(node->meaning);
		xfree(node);
	}
}

static int print_errno_list(struct list_head *root)
{
	struct list_head *tmp;
	struct errno_node *node;
	
	struct list_head *list = root;

	if(list_empty(list)){
		CFG_PRINT("errno list empty!!!\n");
		return 0;
	}
	
	list_for_each(tmp,list){
		node = list_entry(tmp,struct errno_node,list);
		CFG_PRINT("%d===>%s\n",node->num,node->meaning);
	}
	return 0;
}

static void parse_errno_node(pconfig_node conf_node, struct list_head *list){
	pconfig_attr attr;
	pconfig_node next;
	struct errno_node *node;
	char *attr_name;
	char *attr_value;
	int line = 0;
	
	next = conf_node;
	while(next!=NULL){
		line = (int)get_node_line(next);
		node = (struct errno_node *)xmalloc(sizeof(struct errno_node));
		if(node==NULL){
			CFG_ERRORLN("no memory!");
			exit(-1);
		}
		node->num = INVALID_ERRNO;
		node->meaning = NULL;
		
		attr = get_node_first_attribute(next);
		
		while(attr!=NULL){
			attr_name = get_attribute_name(attr);
			attr_value = get_attribute_value(attr);
			if(strcmp(attr_name,ERRNO_MEANING)==0){
				node->meaning = (char *)xmalloc(strlen(attr_value)+5);
				if(node->meaning==NULL){
					CFG_ERRORLN("no memory!");
					exit(-1);
				}
				sprintf(node->meaning,"%s",attr_value);
			}
			else if(strcmp(attr_name,ERRNO_NUM)==0){
				if(sscanf(attr_value,"%d",&node->num)!=1){
					CFG_ERRORLN("LINE %d:errno '%s' is not a num!",line,attr_value);
				}
			}
			//TODO:add new errno attribute here.
			else{
				CFG_ERRORLN("LINE %d:unrecognized errno attribute %s.",line,attr_name);
			}
			attr = get_next_attribute(attr);
		}
		
		if(node->num == INVALID_ERRNO){
			CFG_ERRORLN("LINE %d:errno node has no errno.",line);
		}
		list_add_tail(&node->list,&g_errno_list);
		next = get_next_node(next);
	}
}

