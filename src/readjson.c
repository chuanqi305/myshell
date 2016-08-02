#include "debug.h"
#include "xmalloc.h"
#include "readjson.h"

pconfig_tree load_config_tree(char *path)
{
	pconfig_tree root = NULL;
	struct JSON *json = NULL;
  	json = json_parse_file(path);
	if(json==NULL){
		CFG_ERRORLN("read json file %s error",path);
		return NULL;
	}
	CFG_INFOLN("load config file %s OK.",path);
	root = (pconfig_tree)xmalloc(sizeof(struct config_tree));
	if(root == NULL){
		CFG_ERRORLN("no memory can be alloced.");
		json_release(json);
		return NULL;
	}
	snprintf(root->file_path,sizeof(root->file_path),"%s",path);  
	root->json = json;
	CFG_INFOLN("init config tree OK.");
	return root;
}

pconfig_node get_root_node(pconfig_tree tree)
{
	if(tree->json==NULL){
		CFG_ERROR("empty config tree.");
	}
	return tree->json;
}

pconfig_node  get_first_child_node(pconfig_node root)
{
	return json_get_data_object(root);
}

pconfig_node  get_next_node(pconfig_node node)
{
	return json_get_next(node);
}

pconfig_attr get_node_first_attribute(pconfig_node node)
{
	return json_get_data_object(node);
}

pconfig_attr get_next_attribute(pconfig_attr attr)
{
	return json_get_next(attr);
}

char * get_attribute_name(pconfig_attr attr)
{
	return json_get_keyword(attr);
}

char * get_attribute_value(pconfig_attr attr)
{
	return json_get_data_string(attr);
}

void free_config_tree(pconfig_tree root)
{
	root->file_path[0] = 0;
	if(root->json!=NULL){
		json_release(root->json);
		root->json = NULL;
	}
	xfree(root);
}

int get_node_line(pconfig_node node){
	return json_get_line(node);
}

