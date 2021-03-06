/*
Copyright (c) Docoocoo

Module Name:

AutoPrinterBinary.cpp
Abstract:
自动将ShellCode代码数据以特定的格式排列，最终写入结果文件。
*/
#include "stdafx.h"
#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>

int main()
{
	LoadLibrary("kernel32.dll");

	int ShellCodeSize=0; //用于保存ShellCode代码长度
	char * ShellCodeAddr; //指向ShellCode代码在内存中的起始地址
	__asm
	{
		//-------------------------------------------------------------------------­--------
		//获取ShellCode的内存起始地址和代码大小，以便打印输出
		PUSHAD
			JMP _L1
_L2:
		POP ESI
			MOV ShellCodeAddr,ESI //;获得ShellCode起址
			LEA ECX,ShellCodeEnd //;计算ShellCode代码长度
			LEA EDX,ShellCodeBegin
			SUB ECX,EDX
			MOV ShellCodeSize,ECX

			POPAD
			JMP ShellCodeEnd

_L1: CALL _L2
	//-------------------------------------------------------------------------­--------
	//ShellCode代码
ShellCodeBegin:
		push ebp
			call Deleta 
Deleta:
		pop ebp
			sub ebp,offset Deleta

			jmp $+0x0d
fun1:
		_emit 0x02
			_emit 0x07
			_emit 0xd5
			_emit 0x77
fun2:
		_emit 0xa0
			_emit 0xad
			_emit 0x80
			_emit 0x7c
			push 0x00000040	
			call L1
			_emit 'h'
			_emit 'e'
			_emit 'l'
			_emit 'l'
			_emit 'o'
			_emit 0
			_emit 0
			_emit 0
L1:
		call L2
			_emit 'C'
			_emit 'o'
			_emit 'm'
			_emit 'b'
			_emit 'o'
			_emit 'j'
			_emit 'i'
			_emit 'a'
			_emit 'n'
			_emit 'g'
			_emit 0
			_emit 0
L2:
		push 0
			lea eax,[ebp + fun1]
		call [eax]
		lea eax,[ebp + fun2]
		pop ebp
			jmp DWORD ptr[eax]
		ret
ShellCodeEnd:
		NOP
			NOP
	}

	//-------------------------------------------------------------------------­--------
	//将ShellCode代码打印输出

	LPTSTR OutPutFile=(LPTSTR)malloc(200);
	int ch=0x5C; //"\"

	GetModuleFileName(NULL,OutPutFile,200);

	char *pdest=strrchr(OutPutFile,ch); //为结果文件构造路径
	memset(pdest+1,0x00,50);
	StrCat(OutPutFile,"OutPut.txt");
	HANDLE HOutPut=CreateFile( //创建输出文件
		OutPutFile,
		FILE_WRITE_DATA|GENERIC_WRITE,
		FILE_SHARE_WRITE,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,NULL
		);

	int WriteNumber=0;
	char * WriteBuff=(char
		*)malloc(10000); //具体长度不太好确定，需要加上"\x"等字符，可能是ShellCode代码长度的好几倍
	memset(WriteBuff,0x00,10000);

	DWORD Written;

	//ShellCode数据做统一格式处理
	WriteNumber=sprintf(WriteBuff,"The ShellCode Data:\r\n\"");
	for (int i=0;i<ShellCodeSize;i++)
	{
		if ((i!=0)&&(i%10==0))
		{
			WriteNumber+=sprintf(WriteBuff+WriteNumber,"\"\r\n\"");
		}
		if ((BYTE)ShellCodeAddr[i]>0x10)
		{
			WriteNumber+=sprintf(WriteBuff+WriteNumber,"\\x%02x",(BYTE)ShellCodeAddr[i]);
		}

		else
			WriteNumber+=sprintf(WriteBuff+WriteNumber,"\\x%02x",(BYTE)ShellCodeAddr[i]);

	}

	WriteNumber+=sprintf(WriteBuff+WriteNumber,"\";");

	SetFilePointer(HOutPut,0,NULL,FILE_BEGIN); //写ShellCode数据至结果文件
	WriteFile(HOutPut,WriteBuff,WriteNumber,&Written,NULL);

	CloseHandle(HOutPut);
	free(OutPutFile);
	free(WriteBuff);

	return 1;
}



//     另外:san的那个模板。

//	   hume的模板可用