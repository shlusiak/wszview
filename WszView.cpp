// Main

#include "windows.h"
#include "wszview.h"
#include "explorer.h"


HINSTANCE hInstance;
PCHAR programpath;

void Warte()
{
	MSG msg;
	while (PeekMessage(&msg,0,0,0,PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

void ASSERT(BOOLEAN expr,PCHAR text)
{
#ifdef _DEBUG
	if (expr==FALSE)
	{
		MessageBox(0,text,"Assertion failed!",MB_OK|MB_ICONHAND);
	}
#endif
}

int MessageBox(HWND hWnd,int text,int caption,UINT uType)
{
	CHAR t[MAX_PATH],c[MAX_PATH];
	LoadString(hInstance,text,&t[0],sizeof(t));
	LoadString(hInstance,caption,&c[0],sizeof(c));
	return MessageBox(hWnd,&t[0],&c[0],uType);
}

int APIENTRY WinMain(HINSTANCE _hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow )
{
	hInstance=_hInstance;
	CoInitialize(NULL);

	LPWSTR *cmd;
	int num;
	cmd=CommandLineToArgvW(GetCommandLineW(),&num);
	if (num<=1)
	{	// Show openfile dialog
//		CreateMainWindow(NULL);
		CreateExplorerDialog();
	}else
	{	// parse commandline and run multiple instances...
		CHAR p[MAX_PATH];
		int pos=0,pos2=0,parameter=0;

		while ((((*cmd)[pos])!=0)&&(((*cmd)[pos])!=0))
		{
			p[pos2]=CHAR((*cmd)[pos]);
			pos++;
			pos2++;

			if ((*cmd)[pos]==0)
			{
				p[pos2]=0;
				if (parameter!=0) 
				{
					if (parameter==1)CreateMainWindow(&p[0]);
						else 
						{
							CHAR temp[MAX_PATH];
							strcpy(&temp[0],programpath);
							strcat(&temp[0]," ");
							strcat(&temp[0],&p[0]);
							WinExec(&temp[0],SW_SHOW);
						}
				}
					else 
					{
						programpath=(PCHAR)malloc(strlen(&p[0])+1);
						strcpy(programpath,&p[0]);
					}
				pos2=0;
				pos++;
				parameter++;
			}
		}
	}
	GlobalFree(cmd);


	MSG msg;
	HACCEL hacc=LoadAccelerators(hInstance,MAKEINTRESOURCE(100));

	while (GetMessage(&msg,0,0,0)) 
	{
		if (TranslateAccelerator(msg.hwnd,hacc,&msg)==0)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	CoUninitialize();
	return msg.wParam;
}



