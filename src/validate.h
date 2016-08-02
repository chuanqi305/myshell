#include <stdio.h>
#include <string.h>
int is_string(char *str){
	return 1;
}
int is_ip(char * str)
{
	int a,b,c,d;
	char end;
	if(sscanf(str,"%3u.%3u.%3u.%3u%c",&a,&b,&c,&d,&end)!=4)
		return 0;
	if(a>255||b>255||c>255||d>255)
		return 0;
	return 1;
}
int is_integer(char * str){
	char *p = str;
	if(p==NULL)
		return 0;
	while(*p!=0){
		if(*p > '9'||*p < '0'){
			return 0;
		}
		p++;
	}
	return 1;
}
int is_num(char * str)
{
	char *p = str;
	char hasdot = 0;
	if(p==NULL)
		return 0;
	while(*p!=0){
		if(*p > '9'||*p < '0'){
			if((*p == '-' && p != str)||(*p='.' && hasdot++)){
				return 0;
			}
		}
		p++;
	}
	return 1;
}

int is_date(char *str)
{
	int yyyy;
	int mm;
	int dd;
	char end;
	if(str==NULL){
			return 0;
	}
	if(sscanf(str,"%4d-%2d-%2d%c",&yyyy,&mm,&dd,&end)!=3){
			return 0;
	}
	if(yyyy>=9999||yyyy<1901||mm>12||mm<1||dd>31||dd<1){
			return 0;
	}
	return 1;
}
int is_time(char *str)
{
	int hh; 
	int mm; 
	int ss; 
	char end;
	if(str==NULL){
			return 0;
	}		
	if(sscanf(str,"%2d:%2d:%2d%c",&hh,&mm,&ss,&end)!=3){
			return 0;
	}		
	if(hh>=24||hh<0||mm>=60||mm<0||ss>=60||ss<0){
			return 0;
	}		
	return 1;
}
int is_datetime(char *str)
{
	char time[32] = "time";
	char date[32] = "date";
	char end;
	if(str==NULL){
		return 0;
	}
	if(sscanf(str,"%10[0-9-] %8[0-9:]%c",date,time,&end)!=2){
			return 0;
	}
	if(!is_time(time)){
			return 0;
	}
	if(!is_date(date)){
			return 0;
	}
	return 1;
}

