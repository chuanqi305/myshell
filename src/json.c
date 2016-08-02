#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>

struct JSON
{
	char *type; /*string|number|object|array|boolean|null*/
	char *keyword;
	union{
		struct JSON *object;
		char *string;
		int boolean;
		double number;
	}data;	

	struct JSON * next;
	struct JSON * parent;

	int is_integer; /*only for type is number, not a json specs*/
	long long int integer; /*only for number type is integer*/
	char * position;/*for debug or trace*/
};

char *json_get_type(struct JSON *json)
{
	return json?json->type:"undefined";
}

char *json_get_keyword(struct JSON *json)
{
	return json?json->keyword:NULL;
}

char *json_get_data_string(struct JSON *json)
{
	return json?json->data.string:NULL;
}

struct JSON *json_get_data_object(struct JSON *json)
{
	return json?json->data.object:NULL;
}

double json_get_data_number(struct JSON *json)
{
	return json?json->data.number:0;
}

int json_get_data_boolean(struct JSON *json)
{
	return json?json->data.boolean:0;
}

struct JSON * json_get_next(struct JSON *json)
{
	return json?json->next:NULL;
}

struct JSON * json_get_item_by_keyword(struct JSON *json,char *keyword)
{
	if(strcmp(json->type,"object")){
		return NULL;
	}
	json = json->data.object;
	while(json!=NULL){
		if(strcmp(json->keyword,keyword)==0){
			return json;	
		}
		json = json->next;
	}
	return NULL;
}

struct JSON * json_get_parent(struct JSON *json)
{
	return json->parent;
}

static void offset_to_line(char *string, int offset, int *line, int *ch);
int json_get_line(struct JSON *json)
{
	int line = 0, c = 0, offset = 0;
	struct JSON *root = json;
	while(root->parent!=NULL){
		root = root->parent;
	}
	offset = (json->position) - (root->position);
	offset_to_line(root->position, offset, &line, &c);
	return line;
}

typedef int (*parse_func_t)(char *start, struct JSON *node, int *offset, char **msg);

static struct JSON * alloc_node(){
	struct JSON *json = (struct JSON *)malloc(sizeof(struct JSON));
	memset(json, 0, sizeof(*json));
	json->type = "undefined";
	return json;
}

struct json_parse_syntax{
	char * start;
	parse_func_t func;
	char * type;
};	

void json_release(struct JSON *json)
{
	struct JSON *tmp;
	if(json==NULL){
		return;
	}
	else{
		if(json->parent==NULL){
			free(json->position);
			json->position = NULL;
		}
		while(json!=NULL){
			tmp = json->next;
			if(json->keyword!=NULL){
				free(json->keyword);
			}
			if(strcmp(json->type,"object")==0||strcmp(json->type,"array")==0){
				json_release(json->data.object);
			}
			else if(strcmp(json->type,"string")==0){
				free(json->data.string);
			}
			free(json);
			json = tmp;
		}
	}
}

static int skip_white_char(char *start)
{
	char * str = start;
	while(isspace(*(str++)));
	return str - start - 1;	
}

static int match_syntax(char * syntax, char * next)
{
	return (strchr(syntax, *next)!=NULL);
}

static int get_next_string(char * start, char ** string, int *offset, char **msg)
{
	char *json = start;
	char *str = NULL;
	int len = 0;

	if(*json!='\"'){
		*msg = "keyword must be started with \" character.";
		goto end;
	}

	json++;
	while(*json!='\"' && *json!='\0'){
		json++;
		if(*json=='\\' && *(json+1)!='\0'){
			json++;
		}
	}

	if(*json=='\0'){
		*msg = "missing terminating \" character.";
		goto end;
	}
	else{
		len = json - start - 1;
		str = (char *)malloc(len + 2);
		if(str==NULL){
			*msg = "no memory.";
			goto end;
		}
		memcpy(str, start+1, len);
		str[len] = '\0';
		*string = str;
	}
	json++;
end:
	*offset =  json - start;
	return 0;
}

static int json_parse_all(char *start, struct JSON *node, int *offset, char **msg);

static int json_parse_key_value(char *start, struct JSON *node, int *offset, char **msg)
{
	int offs = 0;
	char *json = start;
	int ret = 0;

	json += skip_white_char(json);
	ret = get_next_string(json, &node->keyword, &offs, msg);
	json += offs;
	if(ret<0){
		goto end;
	}
	json += skip_white_char(json);

	if(*json!=':'){
		ret = -1;
		*msg = "need : after keyword.";
		goto end;
	}
	json ++;
	json += skip_white_char(json);

	ret = json_parse_all(json, node, &offs, msg);
	json += offs;

end:
	*offset = json - start;
	return ret;
}

static int json_parse_subnode(char *start, struct JSON *node, int *offset, char endstr, parse_func_t func, char **msg)
{
	int offs = 0;
	char *json = start;
	int ret = 0;
	struct JSON *next = NULL;
	struct JSON **newnode = &node->data.object;

	json ++;
	json += skip_white_char(json);
	if(*json==endstr){
		json ++;
		goto end;
	}

	do{
		next = alloc_node();	
		if(next==NULL){
			*msg = "no memory.";
			ret = -1;
			goto end;
		}
		next->parent = node;
		*newnode = next;
		json += skip_white_char(json);
		ret = func(json, next, &offs, msg);	
		json += offs;	
		json += skip_white_char(json);
		if(ret<0){
			goto end;
		}	
		newnode = &(next->next);
	}
	while(*json==',' && json++);

	if(*json==endstr){
		json++;
	}	
	else {
		ret = -1;
		if(func==json_parse_all){
			*msg = "missing terminating ] character";
		}
		else if(func==json_parse_key_value){
			*msg = "missing terminating } character";
		}
	}	

end:
	*offset = json - start;
	return ret;
}

static int json_parse_string(char *start, struct JSON *node, int *offset, char **msg)
{
	return get_next_string(start, &node->data.string, offset, msg);
}

static int json_parse_object(char *start, struct JSON *node, int *offset, char **msg)
{
	return json_parse_subnode(start, node, offset, '}', json_parse_key_value, msg);
}

static int json_parse_array(char *start, struct JSON *node, int *offset, char **msg)
{
	return json_parse_subnode(start, node, offset, ']', json_parse_all, msg);
}

static int json_parse_boolean(char *start, struct JSON *json, int *offset, char **msg)
{
	if(strncmp(start,"true", 4)==0){
		json->data.boolean = 1;
		*offset = 4;
		return 0;
	}
	else if(strncmp(start,"false", 5)==0){
		json->data.boolean = 0;
		*offset = 5;
		return 0;
	}
	*msg = "illegal string, it may be 'true' or 'false' here.";
	return -1;
}

static int json_parse_null(char *start, struct JSON *json, int *offset, char **msg)
{
	if(strncmp(start,"null", 4)==0){
		*offset = 4;
		return 0;
	}
	*msg = "illegal string,it may be 'null' here.";
	return -1;
}

static int json_parse_number(char * start, struct JSON *node, int *offset, char **msg)
{
	char *json;
	char *json2;
	int ret = 0;
	long long int integer;

	node->data.number = strtod(start,&json);
	if(json==start || isalpha(*json)){
		*msg = "illegal number formate.";
		ret = -1;
		goto end;
	}

	integer = strtoll(start, &json2, 0); /*extend json type: integer */
	if(json2==json){
		node->is_integer = 1;	
		node->integer = integer;
	}
end:
	*offset =  json - start;
	return ret;
}

static struct json_parse_syntax json_syntax[] = 
{
	{"{", json_parse_object, "object"},
	{"[", json_parse_array, "array"},
	{"-0123456789", json_parse_number, "number"},
	{"tf", json_parse_boolean, "boolean"},
	{"n", json_parse_null, "null"},
	{"\"", json_parse_string, "string"}
};

static int json_parse_all(char *start, struct JSON *node, int *offset, char **msg)
{
	int ret = 0;
	char *json = start;
	int offs = 0;

	int len = sizeof(json_syntax)/sizeof(json_syntax[0]);
	int i;

	node->position = start;
	json += skip_white_char(json);

	if(*json=='\0'){
		goto end;
	}

	for(i = 0;i < len;i++){
		if(match_syntax(json_syntax[i].start, json)){
			node->type = json_syntax[i].type;
			ret = json_syntax[i].func(json, node, &offs, msg);
			json += offs;
			goto end;
		}
	}

	ret = -1;
	*msg = "illegal character.";
end:
	*offset = json - start;
	return ret;
}

static void offset_to_line(char *string, int offset, int *line, int *ch)
{
	int index;
	int lindex = 1,cindex = 1;
	while(*string!='\0' && index<offset){
		if(*string=='\n'){
			lindex++;
			cindex = 1;
		}
		else{
			cindex++;
		}
		index++;
		string++;
	}
	*line = lindex;
	*ch = cindex;
}

struct JSON *json_parse(char *string)
{
	struct JSON *root;
	int ret = 0;
	int offset = 0; 
	char *msg = "no error.";
	char *tmp;
	int line, c;
	int len = strlen(string);
	char *str = NULL;

	if(string==NULL){
		return NULL;
	}
	str = (char *)malloc(len + 2);
	if(str==NULL){
		return NULL;
	}
	memcpy(str, string, len+1);

	root = alloc_node();
	if(root==NULL){
		return NULL;
	}
	ret = json_parse_all(str, root, &offset, &msg);
	if(ret==0){
		return root;
	}
	else{
		offset_to_line(str, offset, &line, &c);
		printf("error at line:%d character:%d, %s\n", line, c, msg);
		tmp = str + offset;
		while(*tmp!='\0' && *tmp!='\n'){
			tmp++;
		}	
		*tmp = '\0';
		printf("%s\n", str + offset - (c - 1));
		printf("%*c\n", c, '^');
		json_release(root);
		return NULL;
	}
}

struct JSON * json_parse_file(char *path)
{
	struct stat st;
	char * buff = NULL;
	FILE * fp = NULL;
	struct JSON *json = NULL;

	if(stat(path,&st)!=0 || st.st_size==0){
		return NULL;
	}

	fp = fopen(path, "r");
	if(fp==NULL){
		return NULL;
	}

	buff = (char *)malloc(st.st_size + 10);
	if(buff==NULL){
		goto error;	
	}
	if(fread(buff, 1, st.st_size, fp) < 0){
		goto error;
	}

	buff[st.st_size] = '\0';

	json = json_parse(buff);

error:
	fclose(fp);	
	free(buff);
	return json;
}

static void print_json_prefix(struct JSON *node, char * prefix)
{
	char new_prefix[256];
	char *keyword, *type;
	struct JSON *child, *next;

	keyword = json_get_keyword(node);
	type = json_get_type(node);
	child = json_get_data_object(node);
	next = json_get_next(node);

	printf("%s",prefix);
	if(keyword!=NULL){
		printf("\"%s\": ", keyword);
		if((strcmp(type,"object")==0 || strcmp(type,"array")==0) && child!=NULL){
			/*printf("\n%s",prefix);*/
		}
	}
	else{
		printf("<null>");
	}
	if(strcmp(type,"string")==0){
		printf("\"%s\"",json_get_data_string(node));
	}
	else if(strcmp(type,"number")==0){
		if(node->is_integer){
			printf("%lld",node->integer);
		}
		else{
			printf("%f",json_get_data_number(node));
		}
	}
	else if(strcmp(type,"boolean")==0){
		printf("%s",json_get_data_boolean(node)?"true":"false");
	}
	else if(strcmp(type,"null")==0){
		printf("null");
	}
	else if(strcmp(type,"object")==0){
		snprintf(new_prefix, sizeof(new_prefix), "%s  ", prefix);
		if(child!=NULL){
			printf("{\n");
			print_json_prefix(child, new_prefix);
			printf("%s}",prefix);
		}
		else {
			printf("{}");
		}
	}
	else if(strcmp(node->type,"array")==0){
		if(child!=NULL){
			snprintf(new_prefix, sizeof(new_prefix), "%s  ", prefix);
			printf("[\n");
			print_json_prefix(child, new_prefix);
			printf("%s]",prefix);
		}
		else {
			printf("[]");
		}
	}
	if(next){
		printf(",\n");
		print_json_prefix(next,prefix);
	}
	else{
		printf("\n");
	}
}

void print_json(struct JSON *root)
{
	print_json_prefix(root, "");
}

