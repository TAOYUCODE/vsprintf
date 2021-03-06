/**linux/kernel/vsprintf.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/* vsprintf.c -- Lars Wirzenius & Linus Torvalds. */
/*
 * Wirzenius wrote this portably, Torvalds fucked it up :-)
 */

//描述 ：将可变参数列表的格式化数据写入字符串

#include <stdarg.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/ctype.h>

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base) // 功能：将一个字符串转换成unsigend long long型数据
{
	unsigned long result = 0,value;

	if (!base) { //如果base为0，由字符串转换进制
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((*cp == 'x') && isxdigit(cp[1])) {//判断是否为16进制
				cp++;
				base = 16;
			}
		}
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp) // 判断小写
	    ? toupper(*cp) : *cp)-'A'+10) < base) { // 将小写转成大写
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result; // 返回字符串的值
}

/* we use this so that we can do without the ctype library */
#define is_digit(c)	((c) >= '0' && (c) <= '9')

static int skip_atoi(const char **s) // 将连续的字符串转成整数
{
	int i=0;

	while (is_digit(**s)) // 判断是否为数字
		i = i*10 + *((*s)++) - '0';// 转数字
	return i;
}

#define ZEROPAD	1		/* pad with zero */
#define SIGN	2		/* unsigned/signed long */
#define PLUS	4		/* show plus */
#define SPACE	8		/* space if plus */
#define LEFT	16		/* left justified */
#define SPECIAL	32		/* 0x */
#define SMALL	64		/* use 'abcdef' instead of 'ABCDEF' */

#define do_div(n,base) ({ \
int __res; \
__asm__("divl %4":"=a" (n),"=d" (__res):"0" (n),"1" (0),"r" (base)); \
__res; })

static char * number(char * str, int num, int base, int size, int precision
	,int type) // 转换字符串
{
	char c,sign,tmp[36];
	const char *digits="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
	int i;

	if (type&SMALL) digits="0123456789abcdefghijklmnopqrstuvwxyz"; // 用小写字母
	if (type&LEFT) type &= ~ZEROPAD;//按位取反
	if (base<2 || base>36)
		return 0;
	c = (type & ZEROPAD) ? '0' : ' ' ;//补零
	if (type&SIGN && num<0) {//判断负数
		sign='-';
		num = -num;
	} else
		sign=(type&PLUS) ? '+' : ((type&SPACE) ? ' ' : 0);//是否有‘+’ 或 ‘ ’
	if (sign) size--;
	if (type&SPECIAL) // 判断特殊字符
		if (base==16) size -= 2;
		else if (base==8) size--;
	i=0;
	if (num==0)
		tmp[i++]='0';
	else while (num!=0)
		tmp[i++]=digits[do_div(num,base)];//把数存到数组中
	if (i>precision) precision=i;
	size -= precision;
	if (!(type&(ZEROPAD+LEFT))) // 是否补零或左对齐
		while(size-->0)
			*str++ = ' ';
	if (sign)
		*str++ = sign;
	if (type&SPECIAL)//判断特殊字符
		if (base==8)
			*str++ = '0';
		else if (base==16) {
			*str++ = '0';
			*str++ = digits[33];
		}
	if (!(type&LEFT)) // 左对齐
		while(size-->0)
			*str++ = c;
	while(i<precision--) // 补零
		*str++ = '0';
	while(i-->0)
		*str++ = tmp[i];
	while(size-->0)
		*str++ = ' ';
	return str;//返回字符串
}

int vsprintf(char *buf, const char *fmt, va_list args)
{
	int len;
	int i;
	char * str;
	char *s;
	int *ip;

	int flags;		/* flags to number() */

	int field_width;	/* width of output field */
	int precision;		/* min. # of digits for integers; max
				   number of chars for from string */
	int qualifier;		/* 'h', 'l', or 'L' for integer fields */

	for (str=buf ; *fmt ; ++fmt) {//遍历字符串
		if (*fmt != '%') {
			*str++ = *fmt;
			continue;
		}
			
		/* process flags */
		flags = 0;
		repeat:
			++fmt;		/* this also skips first '%' */
			switch (*fmt) {
				case '-': flags |= LEFT; goto repeat;
				case '+': flags |= PLUS; goto repeat;
				case ' ': flags |= SPACE; goto repeat;
				case '#': flags |= SPECIAL; goto repeat;
				case '0': flags |= ZEROPAD; goto repeat;
				}
		
		/* get field width */
		field_width = -1;
		if (is_digit(*fmt)) // 判断数字字符
			field_width = skip_atoi(&fmt);
		else if (*fmt == '*') {
			/* it's the next argument */
			field_width = va_arg(args, int);//获取参数值
			if (field_width < 0) {
				field_width = -field_width;
				flags |= LEFT;//左对齐
			}
		}

		/* get the precision */
		precision = -1;
		if (*fmt == '.') {
			++fmt;	
			if (is_digit(*fmt))//判断数字字符
				precision = skip_atoi(&fmt);
			else if (*fmt == '*') {
				/* it's the next argument */
				precision = va_arg(args, int);
			}
			if (precision < 0)
				precision = 0;
		}

		/* get the conversion qualifier */
		qualifier = -1;
		if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L') {
			qualifier = *fmt;
			++fmt;
		}

		switch (*fmt) { //判断数据类型
		case 'c': // 字符型
			if (!(flags & LEFT))
				while (--field_width > 0)
					*str++ = ' ';
			*str++ = (unsigned char) va_arg(args, int);
			while (--field_width > 0)
				*str++ = ' ';
			break;

		case 's': // 字符串
			s = va_arg(args, char *);
			if (!s)
				s = "<NULL>";
			len = strlen(s);
			if (precision < 0)
				precision = len;
			else if (len > precision)
				len = precision;

			if (!(flags & LEFT))
				while (len < field_width--)
					*str++ = ' ';
			for (i = 0; i < len; ++i)
				*str++ = *s++;
			while (len < field_width--)
				*str++ = ' ';
			break;

		case 'o': // 八进制输出
			str = number(str, va_arg(args, unsigned long), 8,
				field_width, precision, flags);
			break;

		case 'p': //指针
			if (field_width == -1) {
				field_width = 8;
				flags |= ZEROPAD;
			}
			str = number(str,
				(unsigned long) va_arg(args, void *), 16,
				field_width, precision, flags);
			break;

		case 'x': //十六进制
			flags |= SMALL;
		case 'X': //字母大写的十六进制 
			str = number(str, va_arg(args, unsigned long), 16,
				field_width, precision, flags);
			break;

		case 'd': //十进制整形
		case 'i':
			flags |= SIGN;
		case 'u': //无符号整形
			str = number(str, va_arg(args, unsigned long), 10,
		    field_width, precision, flags);
			break;

		case 'n': //字符个数
			ip = va_arg(args, int *);
			*ip = (str - buf);
			break;

		default:
			if (*fmt != '%')
				*str++ = '%';
			if (*fmt)
				*str++ = *fmt;
			else
				--fmt;
			break;
		}
	}
	*str = '\0';//结束
	return str-buf;//返回字符个数
}

int sprintf(char * buf, const char *fmt, ...) // 第一个参数是写入一个字符串， 第二个是写入格式， 后面的...是参数占位符
{
	va_list args;
	int i; //记录函数的返回值

	va_start(args, fmt); // 初始化va_list, 得到第一个参数的地址
	i=vsprintf(buf,fmt,args); // 调用vsprintf()  
	va_end(args); // 将args置空
	return i;//返回值
}
