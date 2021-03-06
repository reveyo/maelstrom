/*
 * =====================================================================================
 *
 *       Filename:  lstrstr.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  10/22/2008 12:27:45 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Zhenghua Dai (Zhenghua Dai), djx.zhenghua@gamil.com
 *        Company:  dzh
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include "report.h"
char* lstrchr(const char *str,char c);
static const unsigned int magic_bits = 0x7efefeffL;

static inline int haszerobyte(unsigned int a)
{
register unsigned long himagic = 0x80808080L;
register unsigned long lomagic = 0x01010101L;
	//return ((((a+ magic_bits) ^ ~a) & ~magic_bits));
	return ((a- lomagic) & himagic);
}

static inline long long haszerobytel(unsigned long long a)
{
register unsigned long long himagic = 0x8080808080808080LL;
register unsigned long long lomagic = 0x0101010101010101LL;
	//return ((((a+ magic_bits) ^ ~a) & ~magic_bits));
	return ((a- lomagic) & himagic);
}

inline static int strcmpInline(const char* str1, const char* str2)
{
	while(*str1 == * str2) {
		if(*str2 == 0) return 0;
		str1++;
		str2++;
	}
	return 1;
}

//#define REPORT(i) return i 
#ifndef REPORT
#define REPORT(i) {if( report_function(text, i-text, pattern)== SEARCH_STOP) return (char*)i;};
#endif
char* lstrstr(const char* text, const char* pattern)
{
	unsigned int * intPtr = (unsigned int *) text;
	unsigned char * chPtrAligned = (unsigned char*)text;
	unsigned int intWord  ;//= *intPtr ;
	char chara = pattern[0];
	char charb = pattern[1];
	int byte4a0;
	int byte4b0;
	register int byte4a;
	register int byte4b;
	char* bytePtr = (char*) &byte4a0;
	if(text==NULL) return NULL;
	if(text[0] == 0) {
		return pattern[0]?NULL: (char*)text;
	}
	if(pattern ==NULL) return NULL;
	if(pattern[0] == 0) return (char*)text;
	if(pattern[1] == 0) return lstrchr(text,pattern[0]); 
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=pattern[0];
	bytePtr = (char*) &byte4b0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=pattern[1];
	byte4a = byte4a0;
	byte4b = byte4b0;
#if 1
	//! the pre-byte that is not aligned.
	{
		int preBytes = 4 - (((size_t) text) & 3);
		preBytes &= 3;
		if (preBytes == 0) goto alignStart;
		chPtrAligned = (unsigned char*)text + preBytes;
		switch(preBytes){
			case 1:
				if((text[0])&&(text[0] == chara)){
					if(strcmpInline(text+1,pattern +1) == 0) REPORT(text); //return text + 0;
				}
				break;
			case 2:
				if((text[0])&&(text[0] == chara)){
					if(strcmpInline(text+1,pattern +1) == 0) REPORT(text);//return text + 0;
				}
				if((text[1])&&(text[1] == chara)){
					if(strcmpInline(text+2,pattern +1) == 0) REPORT(text+1);//return text + 1;
				}
				break;
			case 3:
				{
					intWord = *(unsigned int*) text;
					unsigned int reta = haszerobyte(intWord ^ byte4a);
					unsigned int retb = haszerobyte(intWord ^ byte4b);
					if(((reta | retb)&((reta| retb)>>8)/*ab|ba|aa|bb*/ )){
						if((text[0])&&(text[0] == chara)){
							if(strcmpInline(text+1,pattern +1) == 0) REPORT(text);//return text + 0;
						}
						if((text[1])&&(text[1] == chara)){
							if(strcmpInline(text+2,pattern +2) == 0) REPORT(text+1);//return text + 1;
						}
						if((text[2])&&(text[2] == chara)){
							if(strcmpInline(text+3,pattern +3) == 0) REPORT(text+1);//return text + 2;
						}
					}
				}
				break;
			default:
				printf("err\n");
				break;

		}
		intPtr = (unsigned int *) chPtrAligned;
	}


#endif
alignStart:
	intWord = *intPtr;
	while( haszerobyte(intWord) ==0) 
	{
		unsigned int reta ;
//searcha:
		reta = haszerobyte(intWord ^ byte4a);
		if(reta!=0 ) {
			unsigned int retb ;
//findouta:		
			retb = haszerobyte(intWord ^ byte4b) ;
findoutb:
			//if(((reta | retb)&((reta| retb)>>8)/*ab|ba|bb|aa*/ )){
			if(((reta ) & (retb >>8))){
				// have ab|ba|aa|bb
				int i=1;
				char * bytePtr0 = (char*) ( intPtr );
				bytePtr = (char*) ( intPtr );
				if(bytePtr0[0] == chara){
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) REPORT(bytePtr);//return bytePtr;
				}
				if(bytePtr0[1] == chara){
					i =1;
					bytePtr = bytePtr0 + 1;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) REPORT(bytePtr);//return bytePtr;
				}
				if(bytePtr0[2] == chara){
					i =1;
					bytePtr = bytePtr0 + 2;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) REPORT(bytePtr);//return bytePtr;
				}
			}
			// search b
			intPtr++;
			intWord = *intPtr;

			while(haszerobyte(intWord) ==0) {
				retb = haszerobyte(intWord ^ byte4b) ;
				if(retb !=0){
					// findout b
					if((*((char*) intPtr)) == charb){
						//b000
						int i=1;
						char * bytePtr0 = ((char*) ( intPtr )) -1;
						if(bytePtr0[0] == chara){
							while((pattern[i] )&&(bytePtr0[i] == pattern[i])) i++;
							if(pattern[i] == 0) REPORT(bytePtr0);//return bytePtr0;
							if(bytePtr0[i] == 0) return NULL;
						}

					}
					reta = haszerobyte(intWord ^ byte4a);
					if(reta !=0) 
						goto findoutb;
					else{
						goto nextWord;					
					}
				}
				intPtr++;
				intWord = *intPtr;
			}
			bytePtr = ((char*)intPtr )-1;
			goto prePareForEnd;
		}
nextWord:
		intPtr++;
		intWord = *intPtr;
		}
		bytePtr = (char*)intPtr;
prePareForEnd:
		while(*bytePtr){
			if(*bytePtr == chara) {
				int i=1;
				while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
				if(pattern[i] == 0) REPORT(bytePtr);//return bytePtr;
				if(bytePtr[i] == 0) return NULL;

			}
			bytePtr++;
		}

		return NULL;
}

char* lstrchr(const char *text,char c) 
{

	const char *char_ptr=text;
	const unsigned int *longword_ptr;
	register unsigned int longword;
	register unsigned int byte4c;
	unsigned int byte4c0;
	char* p =(char*) &byte4c0;
	char pattern[2] ={c,0};
	p[0] =p[1] = p[2]=p[3] = c;
	byte4c = byte4c0;

	for (char_ptr = text; ((size_t)char_ptr 
				& (sizeof(unsigned int) - 1)) != 0;
			++char_ptr) {
		if (*char_ptr == '\0')return NULL;
		if (*char_ptr == c)
			REPORT(char_ptr);
	}

	longword_ptr = (unsigned int*)char_ptr;


	longword = *longword_ptr;
	while ((haszerobyte(longword) ==0)) {


		if (haszerobytel(longword^byte4c)  != 0) {

			char *cp = (char*)(longword_ptr );

			if (cp[0] == c)
				REPORT(cp) ;
			if (cp[1] == c)
				REPORT(cp + 1);
			if (cp[2] == c)
				REPORT(cp + 2);
			if (cp[3] == c)
				REPORT(cp + 3);
		}
		longword_ptr++;
		longword = *longword_ptr;
	}
	return NULL;
}
#if 0
size_t strlen_d(const char *str) 
{

	const char *char_ptr=str;
	const unsigned int *longword_ptr;
	register unsigned int longword, himagic, lomagic;

	for (char_ptr = str; ((unsigned int)char_ptr 
				& (sizeof(unsigned int) - 1)) != 0;
			++char_ptr) {
		if (*char_ptr == '\0')
			return char_ptr - str;
	}

	longword_ptr = (unsigned int*)char_ptr;

	himagic = 0x80808080L;
	lomagic = 0x01010101L;

	while (1) {

		longword = *longword_ptr++;

		if (((longword - lomagic) & himagic) != 0) {

			const char *cp = (const char*)(longword_ptr - 1);

			if (cp[0] == 0)
				return cp - str;
			if (cp[1] == 0)
				return cp - str + 1;
			if (cp[2] == 0)
				return cp - str + 2;
			if (cp[3] == 0)
				return cp - str + 3;
		}
	}
}

size_t strlen_l(const char *str) 
{

	const char *char_ptr=str;
	const unsigned long long *longword_ptr;
	register unsigned long long longword, himagic, lomagic;

	for (char_ptr = str; ((unsigned long long)char_ptr 
				& (sizeof(unsigned long long) - 1)) != 0;
			++char_ptr) {
		if (*char_ptr == '\0')
			return char_ptr - str;
	}

	longword_ptr = (unsigned long long*)char_ptr;

	//himagic = 0x80808080L;
	//lomagic = 0x01010101L;

	while (1) {

		longword = *longword_ptr++;

		if (haszerobytel(longword) != 0) {

			const char *cp = (const char*)(longword_ptr - 1);

			if (cp[0] == 0)
				return cp - str;
			if (cp[1] == 0)
				return cp - str + 1;
			if (cp[2] == 0)
				return cp - str + 2;
			if (cp[3] == 0)
				return cp - str + 3;
			if (cp[4] == 0)
				return cp - str + 4;
			if (cp[5] == 0)
				return cp - str + 5;
			if (cp[6] == 0)
				return cp - str + 6;
			if (cp[7] == 0)
				return cp - str + 7;
		}
	}
}
#endif
#if 0 
/* test version*/
char* lstrstr(char* text, char* pattern)
{
	unsigned int * intPtr = (unsigned int *) text;
	unsigned int intWord = *intPtr;
	char chara = pattern[0];
	int byte4a0;
	int byte4b0;
	register int byte4a;
	register int byte4b;
	char* bytePtr = (char*) &byte4a0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=pattern[0];
	bytePtr = (char*) &byte4b0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=pattern[4];
	byte4a = byte4a0;
	byte4b = byte4b0;
	while( haszerobyte(intWord) ==0) 
	{
		if(haszerobyte(intWord ^ byte4a) !=0 ) {
			unsigned int nextWord = *(intPtr+1);

			if(	haszerobyte(nextWord ^ byte4b) != 0){
				// check it ;	
				int i=1;
				char * bytePtr0 = (char*) ( intPtr );
				bytePtr = (char*) ( intPtr );
				if(bytePtr0[0] == chara){
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
				if(bytePtr0[1] == chara){
					i =1;
					bytePtr = bytePtr0 + 1;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
				if(bytePtr0[2] == chara){
					i =1;
					bytePtr = bytePtr0 + 2;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
				if(bytePtr0[3] == chara){
					i =1;
					bytePtr = bytePtr0 + 3;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
			}
			intPtr++;
			intWord = nextWord;
			continue;
		}

		intPtr++;
		intWord = *intPtr;
	}
	return NULL;
}

#define checkComplete() \
	i=1;\
while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;\
if(pattern[i] == 0) return bytePtr;

#if 0
char* lstrstr2(char* text, char* pattern)
{
	unsigned int * intPtr = (unsigned int *) text;
	unsigned int intWord = *intPtr;
	char chara = pattern[0];
	int byte4a0;
	int byte4b0;
	register int byte4a;
	register int byte4b;
	char* bytePtr = (char*) &byte4a0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=pattern[0];
	bytePtr = (char*) &byte4b0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=pattern[4];
	byte4a = byte4a0;
	byte4b = byte4b0;
	while( haszerobyte(intWord) ==0) 
	{
		if(haszerobyte(intWord ^ byte4a) !=0 ) {
			unsigned int nextWord = *(intPtr+1);
			if(haszerobyte(intWord ^ byte4b) !=0 ){
				//check	
				//(a*b)
			} else if( haszerobyte(nextWord ^ byte4b) !=0 ){
				if(){
					// 000a *b
					check
				} 

			} else {
				if(haszerobyte(nextWord ^ byte4a) !=0 ) {

				} else {
					a [^b^a]
				}
			}

			if(	haszerobyte(nextWord ^ byte4b) != 0){
				int i=1;
				char * bytePtr0 = (char*) ( intPtr );
				bytePtr = (char*) ( intPtr );
				if(bytePtr0[0] == chara){
					checkComplete();
				}
				if(bytePtr0[1] == chara){
					bytePtr = bytePtr0 + 1;
					checkComplete();
				}
				if(bytePtr0[2] == chara){
					bytePtr = bytePtr0 + 2;
					checkComplete();
				}
				if(bytePtr0[3] == chara){
					bytePtr = bytePtr0 + 3;
					checkComplete();
				}
			}
			intPtr++;
			intWord = nextWord;
			continue;
		}

		intPtr++;
		intWord = *intPtr;
	}
	return NULL;
}
#endif 
char* lstrstr3(char* text, char* pattern)
{
	unsigned int * intPtr = (unsigned int *) text;
	unsigned int intWord = *intPtr;
	char chara = pattern[0];
	int byte4a0;
	int byte4b0;
	register int byte4a;
	register int byte4b;
	char* bytePtr = (char*) &byte4a0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=pattern[0];
	bytePtr = (char*) &byte4b0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=pattern[1];
	byte4a = byte4a0;
	byte4b = byte4b0;
	while( haszerobyte(intWord) ==0) 
	{
finda:
		unsigned int reta = haszerobyte(intWord ^ byte4a);
		if(reta!=0 ) {
			//unsigned int nextWord = *(intPtr+1);
			unsigned int retb = haszerobyte(intWord ^ byte4b) ;

			if(((reta | retb)&((reta| retb)>>8)/*ab|ba*/ )){
				// have ab|ba
				int i=1;
				char * bytePtr0 = (char*) ( intPtr );
				bytePtr = (char*) ( intPtr );
				if(bytePtr0[0] == chara){
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
				if(bytePtr0[1] == chara){
					i =1;
					bytePtr = bytePtr0 + 1;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
				if(bytePtr0[2] == chara){
					i =1;
					bytePtr = bytePtr0 + 2;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
			}
			// check if 000a****
			if(((char*)intPtr)[3] == chara){
				int i =1;
				char* bytePtr = ((char*)intPtr) + 3;
				while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
				if(pattern[i] == 0) return bytePtr;
			}
		}
		intPtr++;
		intWord = *intPtr;
	}
	return NULL;
}

char* lstrstr4(char* text, char* pattern)
{
	unsigned long long * intPtr = (unsigned long long *) text;
	unsigned long long  intWord = *intPtr;
	char chara = pattern[0];
	unsigned long long byte4a0;
	unsigned long long byte4b0;
	register unsigned long long byte4a;
	register unsigned long long byte4b;
	char* bytePtr = (char*) &byte4a0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=bytePtr[4]=bytePtr[5]=bytePtr[6]=bytePtr[7]=pattern[0];
	bytePtr = (char*) &byte4b0;
	bytePtr[0]=bytePtr[1]=bytePtr[2]=bytePtr[3]=bytePtr[4]=bytePtr[5]=bytePtr[6]=bytePtr[7]=pattern[1];
	byte4a = byte4a0;
	byte4b = byte4b0;
	while( haszerobytel(intWord) ==0) 
	{
finda:
		unsigned long long reta = haszerobyte(intWord ^ byte4a);
		if(reta!=0 ) {
			//unsigned int nextWord = *(intPtr+1);
			unsigned long long retb = haszerobyte(intWord ^ byte4b) ;

			if(((reta | retb)&((reta| retb)>>8)/*ab|ba*/ )){
				// have ab|ba
				int i=1;
				char * bytePtr0 = (char*) ( intPtr );
				bytePtr = (char*) ( intPtr );
				if(bytePtr0[0] == chara){
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
				if(bytePtr0[1] == chara){
					i =1;
					bytePtr = bytePtr0 + 1;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
				if(bytePtr0[2] == chara){
					i =1;
					bytePtr = bytePtr0 + 2;
					while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
					if(pattern[i] == 0) return bytePtr;
				}
				if(bytePtr0[3] == chara){
					bytePtr = bytePtr0 + 3;
					checkComplete();
				}
				if(bytePtr0[4] == chara){
					bytePtr = bytePtr0 + 4;
					checkComplete();
				}
				if(bytePtr0[5] == chara){
					bytePtr = bytePtr0 + 5;
					checkComplete();
				}
				if(bytePtr0[6] == chara){
					bytePtr = bytePtr0 + 6;
					checkComplete();
				}
			}
			// check if 000a****
			//if(((char*)intPtr)[7] == chara){
			//	int i =1;
			//	char* bytePtr = ((char*)intPtr) + 7;
			//	while((pattern[i] )&&(bytePtr[i] == pattern[i])) i++;
			//	if(pattern[i] == 0) return bytePtr;
			//}
		}
		intPtr++;
		intWord = *intPtr;
	}
	return NULL;
}

#endif
