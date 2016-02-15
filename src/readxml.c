#include "debug.h"
#include "xmalloc.h"
#include "readxml.h"

pconfig_tree load_config_tree(char *path)
{
	pconfig_tree root = NULL;
	xmlNodePtr xml_root = NULL;
  	xmlDocPtr doc = xmlReadFile(path,"gb2312",0);
	if(doc==NULL){
		CFG_ERRORLN("read xml file %s error",path);
		return NULL;
	}
	CFG_INFOLN("load config file %s OK.",path);
	xml_root = doc->children;
	if(xml_root==NULL){
		CFG_ERRORLN("empty xml file %s",path);
		return NULL;
	}
	root = (pconfig_tree)xmalloc(sizeof(struct config_tree));
	if(root == NULL){
		CFG_ERRORLN("no memory can be alloced.");
		xmlFreeDoc(doc);
		return NULL;
	}
	snprintf(root->file_path,sizeof(root->file_path),"%s",path);  
	root->xml_doc = doc;
	CFG_INFOLN("init config tree OK.");
	return root;
}

pconfig_node get_root_node(pconfig_tree tree)
{
	if(tree==NULL||tree->xml_doc==NULL||tree->xml_doc->children==NULL){
		CFG_ERROR("empty config tree.");
		return NULL;
	}
	return tree->xml_doc->children;
}
/*返回子节点，不包括text类型子节点*/
pconfig_node  get_first_child_node(pconfig_node root)
{
	pconfig_node tmp;
	if(root==NULL){
		return NULL;
	}
	if(root->children==NULL){
		return NULL;
	}
	tmp = root->children;
	while(tmp!=NULL&&tmp->type!=XML_ELEMENT_NODE){
		tmp=tmp->next;
	}
	return tmp;
}
/*返回下一个节点，不包括text类型子节点*/
pconfig_node  get_next_node(pconfig_node current_node)
{
	pconfig_node tmp;
	if(current_node==NULL){
		return NULL;
	}
	if(current_node->next==NULL){
		CFG_INFOLN("end of node .");
		return NULL;
	}
	tmp = current_node->next;
	while(tmp!=NULL&&tmp->type!=XML_ELEMENT_NODE){
		tmp=tmp->next;
	}
	return tmp;
}
char *get_node_name(pconfig_node node)
{
	return (char *)node->name;
}
pconfig_attr get_node_first_attribute(pconfig_node node)
{
	return node->properties;
}

pconfig_attr get_next_attribute(pconfig_attr current_attr)
{
	return current_attr->next;
}

char * get_attribute_name(pconfig_attr attr)
{
	return (char *)attr->name;
}

char * get_attribute_value(pconfig_attr attr)
{
	return (char *)attr->children->content;
}

void free_config_tree(pconfig_tree root)
{
	root->file_path[0] = 0;
	if(root->xml_doc!=NULL){
		xmlFreeDoc(root->xml_doc);
		root->xml_doc = NULL;
	}
	xfree(root);
}
int get_node_line(pconfig_node node){
	return node->line;
}

