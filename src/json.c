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
	char *type; /*string|number|object|array|boolean|null|integer*/
	char *keyword;
	union{
		struct JSON *object;
		char *string;
		int boolean;
		double number;
		long long int integer;
	}data;	
	struct JSON * next;
};

typedef int (*parse_func_t)(char *start, struct JSON *node, int *offset);

struct JSON * new_node(){
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

int skip_white_char(char *start)
{
	char * str = start;
	while(isspace(*(str++)));
	return str - start - 1;	
}

int match_syntax(char * syntax, char * next)
{
	return (strchr(syntax, *next)!=NULL);
}

int get_next_string(char * start, char ** string, int *offset)
{
	char *json = start;
	char *str = NULL;
	int len = 0;

	if(*json!='\"'){
		printf("error at %s:%d:-->%s\n",__FUNCTION__,__LINE__,json);
		return -1;
	}

	json++;
	while(*json!='\"' && *json!='\0'){
		json++;
		if(*json=='\\' && *(json+1)!='\0'){
			json++;
		}
	}

	if(*json=='\0'){
		printf("error at %s:%d:-->%s\n",__FUNCTION__,__LINE__,json);
		return -1;
	}
	else{
		len = json - start - 1;
		str = (char *)malloc(len + 2);
		memcpy(str, start+1, len);
		str[len] = '\0';
		*string = str;
	}
	json++;
	*offset =  json - start;
	return 0;
}

int json_parse_subnode(char *start, struct JSON *node, int *offset, char endstr, parse_func_t func)
{
	int offs = 0;
	char *json = start;
	int ret = -1;
	struct JSON *next = NULL;
	struct JSON **newnode = &node->data.object;

	json ++;
	json += skip_white_char(json);
	if(*json==endstr){
		json ++;
		ret = 0;
		goto end;
	}

	do{
		next = new_node();	
		*newnode = next;
		json += skip_white_char(json);
		ret = func(json,next,&offs);	
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
		printf("error at %s:%d:-->%s(%c)\n",__FUNCTION__,__LINE__,json,endstr);
		ret = -1;
	}	

end:
	*offset = json - start;
	return ret;
}

int json_parse_all(char *start, struct JSON *node, int *offset);
int json_parse_key_value(char *start, struct JSON *node, int *offset)
{
	int offs = 0;
	char *json = start;
	int ret = -1;

	json += skip_white_char(json);
	ret = get_next_string(json, &node->keyword, &offs);
	json += offs;
	if(ret<0){
		goto end;
	}
	json += skip_white_char(json);

	if(*json!=':'){
		printf("error at %s:%d:-->%s\n",__FUNCTION__,__LINE__,json);
		goto end;
	}
	json ++;
	json += skip_white_char(json);

	ret = json_parse_all(json, node, &offs);
	json += offs;
	if(ret<0){
		goto end;
	}
end:
	*offset = json - start;
	return ret;
}

int json_parse_string(char *start, struct JSON *node, int *offset)
{
	return get_next_string(start, &node->data.string, offset);
}

int json_parse_object(char *start, struct JSON *node, int *offset)
{
	return json_parse_subnode(start, node, offset, '}', json_parse_key_value);
}

int json_parse_array(char *start, struct JSON *node, int *offset)
{
	return json_parse_subnode(start, node, offset, ']', json_parse_all);
}

int json_parse_boolean(char *start, struct JSON *json, int *offset)
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
	printf("error at %s:%d:-->%s\n",__FUNCTION__,__LINE__,start);
	return -1;
}

int json_parse_null(char *start, struct JSON *json, int *offset)
{
	if(strncmp(start,"null", 4)==0){
		*offset = 4;
		return 0;
	}
	printf("error at %s:%d:-->%s\n",__FUNCTION__,__LINE__,start);
	return -1;
}

int json_parse_number(char * start, struct JSON *node, int *offset)
{
	char *json;
	char *json2;
	long long int integer;
	
	node->data.number = strtod(start,&json);
	integer = strtoll(start, &json2, 0); /*extend json type: integer */
	if(json2==json){
		node->type = "integer";	
		node->data.integer = integer;
	}
	if(json==start){
		printf("error at %s:%d:-->%s\n",__FUNCTION__,__LINE__,json);
		return -1;
	}
	*offset =  json - start;
	return 0;
}

struct json_parse_syntax json_syntax[] = 
{
	{"{", json_parse_object, "object"},
	{"[", json_parse_array, "array"},
	{"-0123456789", json_parse_number, "number"},
	{"tf", json_parse_boolean, "boolean"},
	{"n", json_parse_null, "null"},
	{"\"", json_parse_string, "string"}
};

int json_parse_all(char *start, struct JSON *node, int *offset)
{
	int ret = 0;
	char *json = start;
	int offs = 0;

	int len = sizeof(json_syntax)/sizeof(json_syntax[0]);
	int i;

	json += skip_white_char(json);

	if(*json=='\0'){
		goto end;
	}
	
	for(i = 0;i < len;i++){
		if(match_syntax(json_syntax[i].start, json)){
			node->type = json_syntax[i].type;
			ret = json_syntax[i].func(json, node, &offs);
			json += offs;
			goto end;
		}
	}

	printf("%s:error at:%s",__FUNCTION__,json);
	ret = -1;
end:
	*offset = json - start;
	return ret;
}

struct JSON *json_parse(char *string)
{
	struct JSON *root;
	int ret = 0;
	int offset = 0; 

	root = new_node();
	ret = json_parse_all(string, root, &offset);
	if(ret==0){
		return root;
	}
	else{
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

	if(stat(path,&st)!=0||st.st_size==0){
		return NULL;
	}

	fp = fopen(path, "r");
	if(fp==NULL){
		return NULL;
	}

	buff = (char *)malloc(st.st_size + 10);
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

void print_json_prefix(struct JSON *node,char * prefix)
{
	char new_prefix[256];

	printf("%s",prefix);
	if(node->keyword!=NULL){
		printf("\"%s\": ", node->keyword);
		if((strcmp(node->type,"object")==0
			|| strcmp(node->type, "array")==0) 
			&& node->data.object!=NULL){
			/*printf("\n%s",prefix);*/
		}
	}
	if(strcmp(node->type,"string")==0){
		printf("\"%s\"",node->data.string);
	}
	else if(strcmp(node->type,"number")==0){
		printf("%f",node->data.number);
	}
	else if(strcmp(node->type,"integer")==0){
		printf("%lld",node->data.integer);
	}
	else if(strcmp(node->type,"boolean")==0){
		printf("%s",node->data.boolean?"true":"false");
	}
	else if(strcmp(node->type,"null")==0){
		printf("null");
	}
	else if(strcmp(node->type,"object")==0){
		snprintf(new_prefix, sizeof(new_prefix), "%s  ", prefix);
		if(node->data.object!=NULL){
			printf("{\n");
			print_json_prefix(node->data.object, new_prefix);
			printf("%s}",prefix);
		}
		else {
			printf("{}");
		}
	}
	else if(strcmp(node->type,"array")==0){
		if(node->data.object!=NULL){
			snprintf(new_prefix, sizeof(new_prefix), "%s  ", prefix);
			printf("[\n");
			print_json_prefix(node->data.object, new_prefix);
			printf("%s]",prefix);
		}
		else {
			printf("[]");
		}
        }
	if(node->next){
		printf(",\n");
		print_json_prefix(node->next,prefix);
	}
	else{
		printf("\n");
	}
}

void print_json(struct JSON *root){
	print_json_prefix(root, "");
}
/*
int main(int argc,char **argv)
{
	struct JSON * json = json_parse_file(argv[1]);
	if(json!=NULL){
		print_json(json);
		json_release(json);
	}
	return 0;
}
*/
