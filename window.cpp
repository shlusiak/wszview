#include "windows.h"
#include "wszview.h"
#include "stdio.h"
#include "dibapi.h"
#include "skin.h"
#include "resource.h"


const PCHAR ClassName="WszView";

SKIN skin;
BYTE FirstShow;


void ShowLoadSkinDialog(HWND owner,PCHAR result)
{
	PCHAR b=(PCHAR)malloc(MAX_PATH);
	OPENFILENAME ofs;

	b[0]='\0';
	ZeroMemory(&ofs,sizeof(ofs));
	ofs.lStructSize=sizeof(ofs);
	ofs.hInstance=hInstance;
	ofs.hwndOwner=owner;
	ofs.lpstrFilter="Skins (*.wsz;*.zip;main.bmp)\0*.wsz;*.zip;main.bmp\0"
		"Wsz archives (*.wsz;*.zip)\0*.wsz;*.zip\0"
		"Mainbitmap (main.bmp)\0main.bmp\0"
		"All files (*.*)\0*.*\0\0";
	ofs.lpstrFile=b;
	ofs.nMaxFile=1024-1;
	ofs.nMaxCustFilter=3;
	ofs.lpstrTitle="Select wsz-file";
	ofs.lpstrDefExt="wsz";
	ofs.Flags=OFN_PATHMUSTEXIST|OFN_HIDEREADONLY;
	if (GetOpenFileName(&ofs)==TRUE) 
	    strcpy(result,b);
	else result[0]='\0';
	free(b);
}

void DeleteTempPath(HWND wnd,PCHAR p)
{		// Deletes the temporary path, created during unzipping the file
	CHAR temp[MAX_PATH];
	WIN32_FIND_DATA ffd;
	SetCurrentDirectory("\\");
	strcpy(&temp[0],p);
	strcat(&temp[0],"*.*");
	HANDLE search=FindFirstFile(&temp[0],&ffd);
	if (search!=INVALID_HANDLE_VALUE) do
	{
		strcpy(&temp[0],p);
		strcat(&temp[0],ffd.cFileName);
		DeleteFile(&temp[0]);
	}while (FindNextFile(search,&ffd)==TRUE);
	FindClose(search);
	if (RemoveDirectory(p)==FALSE)
	{
		LoadString(hInstance,IDS_CANTREMOVEDIR,&temp[0],sizeof(temp));
		sprintf(&temp[0],&temp[0],p);
		MessageBox(wnd,&temp[0],WszView,MB_OK|MB_ICONEXCLAMATION);
	}
}

void SaveSkinBitmap(HWND owner)
{
	PCHAR pc=(PCHAR)malloc(MAX_PATH);
	GetWindowText(owner,pc,MAX_PATH);
	int i=strlen(pc);
	do i--; while ((i>=0)&&(pc[i]!='.'));
	if (i>=0)
	{
		strcpy(&pc[i+1],"bmp");
	}else strcat(pc,".bmp");


	OPENFILENAME ofs;
	ZeroMemory(&ofs,sizeof(ofs));
	ofs.lStructSize=sizeof(ofs);
	ofs.hInstance=hInstance;
	ofs.hwndOwner=owner;
	ofs.lpstrFilter=
		"Bitmap files (*.bmp)\0*.bmp\0"
		"All files (*.*)\0*.*\0\0";
	ofs.lpstrFile=pc;
	ofs.nMaxFile=MAX_PATH-1;
	ofs.nMaxCustFilter=1;
	ofs.lpstrTitle="Enter filename";
	ofs.lpstrDefExt="bmp";
	ofs.Flags=OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
	if (GetSaveFileName(&ofs)) 
	{
		HDIB dib=BitmapToDIB(skin.bitmap,GetSystemPalette());
		SaveDIB(dib,pc);
		DestroyDIB(dib);
	}
	free(pc);
}

DWORD WINAPI ReloadSkinThread(LPVOID param)
{
	PSKIN s=(PSKIN)param;
	s->ReloadSkin();
	InvalidateRect(s->wnd,NULL,FALSE);
	return 1;
}

LRESULT CALLBACK MyWndProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch (Msg)
	{
	case WM_CREATE:
		{
			HDC dc=GetDC(GetDesktopWindow());
			skin.bitmap=CreateCompatibleBitmap(dc,picturewidth,pictureheight);
			ReleaseDC(GetDesktopWindow(),dc);
			DragAcceptFiles(hWnd,TRUE);
		}
		break;
	case WM_DESTROY:
		if (skin.zipped)DeleteTempPath(hWnd,skin.Path);
		DeleteObject(skin.bitmap);
		free(skin.Path);
		free(skin.OriginalPath);
		PostQuitMessage(0);
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc=BeginPaint(hWnd,&ps);				

			if (FirstShow==2)
			{
				RECT r;
				GetClientRect(hWnd,&r);
				PatBlt(dc,0,0,r.right,r.bottom,WHITENESS);
				EndPaint(hWnd,&ps);
				return 0;
			}
			if (FirstShow==1)
			{
				FirstShow=0;
				DWORD id;
				CreateThread(NULL,0,ReloadSkinThread,LPVOID(&skin),0,&id);
			}else
			{
				HDC memdc=CreateCompatibleDC(dc);
				SelectObject(memdc,skin.bitmap);
				BitBlt(dc,0,0,picturewidth,pictureheight,memdc,0,0,SRCCOPY);
			
				RestoreDC(memdc,-1);
				DeleteDC(memdc);
			}
			EndPaint(hWnd,&ps);
		}
		break;
	case WM_RBUTTONDOWN:
		{
			HMENU m=LoadMenu(hInstance,MAKEINTRESOURCE(IDR_POPUPMENU));
			if (m==0)return 1;
			POINT p;
			p.x=LOWORD(lParam);
			p.y=HIWORD(lParam);
			ClientToScreen(hWnd,&p);
			TrackPopupMenu(GetSubMenu(m,0),TPM_RIGHTBUTTON,p.x,p.y,0,hWnd,NULL);
			DestroyMenu(m);
  		}
		break;
	case WM_DROPFILES:
		{
			CHAR c[MAX_PATH];
			int num=DragQueryFile(HDROP(wParam),-1,c,MAX_PATH);

			for (int i=0;i<num;i++)
			{
				DragQueryFile(HDROP(wParam),i,c,MAX_PATH);
				if (i==0)
				{
					if (((GetFileAttributes(&c[0])&FILE_ATTRIBUTE_DIRECTORY)!=0)&&(c[strlen(&c[0])-1]!='\\'))
						strcat(&c[0],"\\");
					free(skin.Path);
					skin.Path=(PCHAR)malloc(strlen(&c[0])+1);
					strcpy(skin.Path,&c[0]);
					skin.SetSkin();
					skin.ReloadSkin();
					InvalidateRect(hWnd,NULL,FALSE);
				}else
				{
					CHAR temp_[MAX_PATH];
					PCHAR temp=&temp_[0];
					strcpy(temp,programpath);
					strcat(temp," \"");
					strcat(temp,&c[0]);
					strcat(temp,"\"");
					WinExec(temp,SW_SHOW);
				}
			}
		}
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case 100:DestroyWindow(hWnd);	// Quit
			break;
		case 101:	// Reload
			{
				SetCursor(LoadCursor(0,IDC_WAIT));
				skin.ReloadSkin();
				InvalidateRect(hWnd,NULL,FALSE);
				SetCursor(LoadCursor(0,IDC_ARROW));
			}
			break;
		case 102:	// Load new
			{
				CHAR temp[MAX_PATH],old1[MAX_PATH],old2[MAX_PATH];
				ShowLoadSkinDialog(hWnd,temp);
				if (temp[0]=='\0')break;
				strcpy(&old1[0],skin.Path);
				strcpy(&old2[0],skin.OriginalPath);
				free(skin.OriginalPath);
				skin.OriginalPath=(PCHAR)malloc(strlen(&temp[0])+2);
				strcpy(skin.OriginalPath,&temp[0]);
				BOOLEAN oz=skin.zipped;
				if (skin.SetSkin())
				{
					SetCurrentDirectory("..");
					if (oz)DeleteTempPath(hWnd,&old1[0]);
					skin.ReloadSkin();
					InvalidateRect(hWnd,NULL,FALSE);
				}else 
				{
					free(skin.OriginalPath);
					free(skin.Path);
					skin.Path=(PCHAR)malloc(strlen(&old1[0])+2);
					strcpy(skin.Path,&old1[0]);
					skin.OriginalPath=(PCHAR)malloc(strlen(&old2[0])+2);
					strcpy(skin.OriginalPath,&old2[0]);
					skin.zipped=oz;
				}
			}
			break;
		case 103:SaveSkinBitmap(hWnd);	// Save as bitmap
			break;
		case 104:	
			// Info
			DialogBox(hInstance,MAKEINTRESOURCE(101),hWnd,MyDlgProc);
			break;
		case 105:
			// Select skin in Winamp
			skin.SelectSkin();
			break;
		case 106:
			{	// Delete skin
				CHAR c[MAX_PATH];
				strcpy(&c[0],skin.OriginalPath);
				if (c[strlen(&c[0])-1]=='\\')c[strlen(&c[0])-1]='\0';

				if (c[strlen(&c[0])+1]!='\0')c[strlen(&c[0])+1]='\0';

				SHFILEOPSTRUCT shf;
				ZeroMemory(&shf,sizeof(shf));
				shf.hwnd=hWnd;
				shf.wFunc=FO_DELETE;
				shf.fFlags=GetAsyncKeyState(VK_SHIFT)?0:FOF_ALLOWUNDO;
				shf.pFrom=c;
				
				SHFileOperation(&shf);
				skin.ReloadSkin();
				InvalidateRect(hWnd,NULL,FALSE);
			}
		}
 		break;
	default:return DefWindowProc(hWnd,Msg,wParam,lParam);
	}
	return 0;
}

DWORD WINAPI SetSkinThread(LPVOID param)
{
	PSKIN s=(PSKIN)param;
	if (s->SetSkin()==FALSE)
	{
		FirstShow=0;
	}else{
		FirstShow=1;
		InvalidateRect(s->wnd,NULL,FALSE);
	}
	return 1;
}

HWND CreateMainWindow(PCHAR mypath)
{
	WNDCLASS wc;
	HWND wnd;
	RECT r;

	ZeroMemory(&wc,sizeof(wc));

	wc.lpfnWndProc=&MyWndProc;
	wc.hInstance=hInstance;
	wc.hbrBackground=(HBRUSH)GetStockObject(NULL_BRUSH);
	wc.lpszClassName=ClassName;
	wc.hCursor=LoadCursor(0,IDC_ARROW);
	wc.hIcon=LoadIcon(hInstance,MAKEINTRESOURCE(100));
	
	RegisterClass(&wc);

	r.left=0;
	r.top=0;
	r.right=picturewidth;
	r.bottom=pictureheight;
	AdjustWindowRect(&r,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,FALSE);

	FirstShow=2;
	wnd=CreateWindowEx(0,ClassName,WszView,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
		CW_USEDEFAULT,0,r.right-r.left,r.bottom-r.top,0,0,hInstance,NULL);

	if (wnd==NULL)
	{
		PostQuitMessage(0);
		return 0;
	}
	UpdateWindow(wnd);
	ShowWindow(wnd,SW_SHOW);

	PCHAR temp=(PCHAR)malloc(MAX_PATH);
	if (mypath!=NULL)
	{
		skin.Path=(PCHAR)malloc(2);
		strcpy(temp,mypath);
		if (((GetFileAttributes(temp)&FILE_ATTRIBUTE_DIRECTORY)!=0)&&(temp[strlen(temp)-1]!='\\'))
			strcat(temp,"\\");
	}else
	{
		skin.Path=(PCHAR)malloc(2);
		ShowLoadSkinDialog(wnd,temp);
		if (temp[0]=='\0')
		{
			DestroyWindow(wnd);
			PostQuitMessage(0);
			return 0;
		}
	}

	skin.OriginalPath=(PCHAR)malloc(strlen(&temp[0])+2);
	strcpy(skin.OriginalPath,&temp[0]);
	skin.wnd=wnd;

	DWORD id;
	CreateThread(NULL,0,SetSkinThread,&skin,0,&id);

	return wnd;
}