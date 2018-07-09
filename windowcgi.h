/* -------------------------------------------------------------------------
//	文件名		：	windowcgi.h
//	创建者		：	magictong
//	创建时间	：	2016/11/16 21:24:44
//	功能描述	：	
//
//	$Id: $
// -----------------------------------------------------------------------*/
#ifndef __WINDOWCGI_H__
#define __WINDOWCGI_H__

#include <windows.h>
// -------------------------------------------------------------------------
class CWinCGI
{
public:
	CWinCGI();
	~CWinCGI();

	BOOL Exec(const char* path, const char* query_string);
	BOOL Write(PBYTE in_buffer, DWORD dwSize);
	BOOL Read(PBYTE out_buffer, DWORD dwSize);

	DWORD Wait();

private:
	void __Reset();

	HANDLE  hStdInRead;
	HANDLE  hStdInWrite;

	HANDLE  hStdOutRead;
	HANDLE  hStdOutWrite;

// 	HANDLE  hStdErrRead;
// 	HANDLE  hStdErrWrite;

	STARTUPINFO siStartInfo;
	PROCESS_INFORMATION piProcInfo;
};
// -------------------------------------------------------------------------
// $Log: $

#endif /* __WINDOWCGI_H__ */