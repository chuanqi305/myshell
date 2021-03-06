int encode_base64(char *in, int len, char *out, int *outlen)
{
	int i = 0, j = 0, s = 0, tmp;
	while (i<len || (i==len && s!=0)){
		tmp = (((i>0?in[i-1]:0)<<((4-s)<<1)>>2)|(in[i]>>((s+1)<<1)))&(0xFF>>2);
		out[j++] = tmp<26?tmp+65:(tmp<52?tmp+71:(tmp<62?tmp-4:(tmp==62?'+':'/')));
		s = (s+1)&0x03;
		if (s != 0) i++;
	}
	if (s >= 2) out[j++] = '=';
 	if (s == 2) out[j++] = '=';
	*outlen = j;
	out[j++] = '\0';
	return 0;
}
int decode_base64(char *in, int len, char *out, int *outlen)
{
	int i = 0, j = 0, s = 0, tmp, hbyte, lbyte, last;
	*outlen = 0;
	while (in[i]!=0 && in[i]!='='){
		tmp = in[i];
		hbyte = last;
		lbyte = tmp=='/'?63:(tmp=='+'?62:((tmp>='a'&&tmp<='z')?tmp-'a'+26:((tmp>='A'&&tmp<='Z')?tmp-'A':((tmp>='0'&&tmp<='9')?tmp-'0'+52:-1))));
		if (lbyte < 0) return -1;
		last = lbyte;
		if(s!=0) out[j++] = (hbyte<<(s<<1))|(lbyte>>((3-s)<<1));
		s = (s+1)&0x03;
		i++;
	}
	*outlen = j;
	return 0;
}
