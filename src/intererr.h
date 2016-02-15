struct internal_errno
{
	int num ;
	char *meaning;
};


#define PARSE_ERROR_UNTERMINAL_QTUOTE 1001
#define PARSE_ERROR_UNKNOWN_KEYWORD 1002
#define PARSE_ERROR_NEED_PARAMETER 1003

struct internal_errno g_internal_error = 
{
	{
		PARSE_ERROR_UNTERMINAL_QTUOTE,
		"quota not terminaled."
	},
	{
		PARSE_ERROR_UNKNOWN_KEYWORD,
		"unknown keyword."
	},
	{
		PARSE_ERROR_NEED_PARAMETER,
		"need parameter,but not given."
	}
}

