
// QtMFCDraw.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CQtMFCDrawApp:
// �йش����ʵ�֣������ QtMFCDraw.cpp
//

class CQtMFCDrawApp : public CWinAppEx
{
public:
	CQtMFCDrawApp();

// ��д
	public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CQtMFCDrawApp theApp;