#include "windows.h"
#include "skin.h"
#include "stdio.h"
#include "wszview.h"
#include "resource.h"


const MaxZipSize=1024*768;



BOOLEAN SKIN::UnzipSkin()
{	// Just unzippes the filename and saves the temporary path in .path
	CHAR _temp[MAX_PATH],_newpath[MAX_PATH];
	PCHAR temp=&_temp[0],newpath=&_newpath[0];
	GetTempPath(MAX_PATH,&temp[0]);
	if (temp[strlen(temp)-1]!='\\')strcat(temp,"\\\0");

repeat:
	int i=0;
	DWORD attr;
	do
	{
		i++;
		sprintf(newpath,"%swszview%d.tmp",temp,i);
		attr=GetFileAttributes(newpath);
	}while (attr!=DWORD(-1));
	sprintf(newpath,"%swszview%d.tmp\\",temp,i);

	if (!CreateDirectory(newpath,NULL))goto repeat;
	SetCurrentDirectory(newpath);
	if (UnZip(OriginalPath)==TRUE)
	{
		free(Path);
		Path=(PCHAR)malloc(strlen(newpath)+2);
		strcpy(Path,newpath);
		return TRUE;
	}
	::DeleteTempPath(wnd,newpath);
	return FALSE;
}

void SKIN::DeleteTempPath()
{
	if (!zipped)return;
	::DeleteTempPath(wnd,Path);
}

BOOLEAN ParseSkinPath(PCHAR p)
{
	DWORD attr=GetFileAttributes(p);
	HBITMAP b=0;
	if (attr==DWORD(-1))return FALSE;


	if ((attr&FILE_ATTRIBUTE_DIRECTORY)!=0)return TRUE;

	b=(HBITMAP)LoadImage(0,p,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
	if (b!=0)
	{	// File is a Bitmap, extract path and use it!
		DeleteObject(b);
		CHAR path_[MAX_PATH],*path=&path_[0];
		strcpy(path,p);

		while (path[strlen(path)-1]!='\\')path[strlen(path)-1]='\0';

		CHAR temp[MAX_PATH];
		strcpy(&temp[0],path);
		strcat(&temp[0],mainbmp);
		if (GetFileAttributes(&temp[0])==DWORD(-1))return FALSE;

		strcpy(p,path);

		return TRUE;
	}
	return FALSE;
}

BOOLEAN SKIN::ParseSkinPath(PCHAR path)
{
	return ::ParseSkinPath(path);
}

BOOLEAN SKIN::SetSkin()
{	// Extracts the zip file, if skin is a zip file and sets the title of the window
	DWORD attr=GetFileAttributes(OriginalPath);
	HBITMAP b=0;
	zipped=FALSE;
	if (attr==DWORD(-1))
	{
		CHAR temp[MAX_PATH];
		sprintf(&temp[0],"Path not found!\n(%s)",OriginalPath);
		if (wnd)MessageBox(wnd,&temp[0],WszView,MB_OK|MB_ICONHAND);
		return FALSE;
	}

	if ((attr&FILE_ATTRIBUTE_DIRECTORY)!=0)
	{	// Skin is a Directory, so use it!
		free(Path);
		Path=(PCHAR)malloc(strlen(OriginalPath)+2);
		strcpy(Path,OriginalPath);

		CHAR temp[MAX_PATH];
		strcpy(&temp[0],OriginalPath);
		if (temp[strlen(&temp[0])-1]!='\\')strcat(&temp[0],"\\");
		strcat(&temp[0],mainbmp);		
		if (GetFileAttributes(&temp[0])==DWORD(-1))
		{
			sprintf(&temp[0],"Directory doesn't contain a skin!\n(%s)",OriginalPath);
			if (wnd)MessageBox(wnd,&temp[0],WszView,MB_OK|MB_ICONHAND);
			return FALSE;
		}

		strcpy(&temp[0],OriginalPath);
		if (temp[strlen(&temp[0])-1]=='\\')temp[strlen(&temp[0])-1]='\0';

		int i=strlen(&temp[0])-1;
		while (temp[i]!='\\')i--;
		i++;
		strcpy(&temp[0],&temp[i]);

		if (wnd)SetWindowText(wnd,&temp[0]);
		return TRUE;
	}

	b=(HBITMAP)LoadImage(0,OriginalPath,IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
	if (b!=0)
	{	// File is a Bitmap, extract path and use it!
		DeleteObject(b);
		CHAR p_[MAX_PATH],*p=&p_[0];
		strcpy(p,OriginalPath);

		while (p[strlen(p)-1]!='\\')p[strlen(p)-1]='\0';

		CHAR temp[MAX_PATH];
		strcpy(&temp[0],p);
		strcat(&temp[0],mainbmp);
		if (GetFileAttributes(&temp[0])==DWORD(-1))
		{
			sprintf(&temp[0],"File not found!\n(%s)",p);
			if (wnd)MessageBox(wnd,&temp[0],WszView,MB_OK|MB_ICONHAND);
			return FALSE;
		}

		free(OriginalPath);
		OriginalPath=(PCHAR)malloc(strlen(p)+3);
		strcpy(OriginalPath,p);

		strcpy(&temp[0],p);
		if (temp[strlen(&temp[0])-1]=='\\')temp[strlen(&temp[0])-1]='\0';

		int i=strlen(&temp[0])-1;
		while (temp[i]!='\\')i--;
		i++;
		strcpy(&temp[0],&temp[i]);

		free(Path);
		Path=(PCHAR)malloc(strlen(p)+2);
		strcpy(Path,p);
		
		if (wnd)SetWindowText(wnd,&temp[0]);
		return TRUE;
	}
//	Must be a zip file... Try to unzip it
	CHAR temp[MAX_PATH];
//	Unzip file only if filesize is small enough: Speed up at very huge zip files

	{
		HANDLE file=CreateFile(OriginalPath,GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);
		if (file!=INVALID_HANDLE_VALUE)
		{
			DWORD s=GetFileSize(file,NULL);
			CloseHandle(file);
			if (s==DWORD(-1))return FALSE;
			if (s>MaxZipSize)
			{
				sprintf(&temp[0],"File is to big to be a valid skin zip file!\n(%s)",OriginalPath);
				if (wnd)MessageBox(wnd,&temp[0],WszView,MB_OK|MB_ICONHAND);
				return FALSE;
			}
		}
	}

	if (UnzipSkin()==TRUE)
	{
		zipped=TRUE;
		strcpy(&temp[0],Path);
		strcat(&temp[0],"main.bmp");
//		File is a skin zip only if the file main.bmp exists in the unzipped folder
		if (GetFileAttributes(&temp[0])==DWORD(-1))
		{
			DeleteTempPath();
			return FALSE;
		}
		strcpy(&temp[0],OriginalPath);
		if (temp[strlen(&temp[0])-1]=='\\')temp[strlen(&temp[0])-1]='\0';

		int i=strlen(&temp[0])-1;
		while (temp[i]!='\\')i--;
		i++;
		strcpy(&temp[0],&temp[i]);
		
		if (wnd)SetWindowText(wnd,&temp[0]);
		return TRUE;
	}
	
	sprintf(&temp[0],"Not a valid skin zip file!\n(%s)",OriginalPath);
	if (wnd)MessageBox(wnd,&temp[0],WszView,MB_OK|MB_ICONHAND);
	return FALSE;
}

void SKIN::RefreshBitmap()
{
	HDC memdc=CreateCompatibleDC(0);
	SelectObject(memdc,bitmap);

	PaintMainWindow(memdc,Path);
	PaintEQWindow(memdc,Path);
	PaintPLWindow(memdc,Path);

	RestoreDC(memdc,-1);
	DeleteDC(memdc);
}

DWORD WINAPI ReloadThreadProc(LPVOID p)
{
	PSKIN skin=(PSKIN)p;
	if (skin->zipped)
	{	// Neu entpacken?
		skin->DeleteTempPath();
		skin->UnzipSkin();
	}
	skin->RefreshBitmap();
	return 1;
}

void SKIN::ReloadSkin()
{	// Reloads the bitmap files and draws them to a bitmap
	DWORD id;
	HANDLE th;
	th=CreateThread(NULL,0,ReloadThreadProc,LPVOID(this),0,&id);
	WaitForSingleObject(th,INFINITE);
	CloseHandle(th);
}

int SetWinampSkin(PCHAR path)
// -1: Not supportet
// 0 : Winamp not found
// 1 : OK
{ 
	const bufsize=1024;
	LPVOID p;
	HANDLE handle;
	DWORD process,written;
	HWND wnd=FindWindow("Winamp v1.x",NULL);

/*	Lässt Winamp den Skin laden, der in path angegeben wurde. 
	Dazu wird Speicher im Fremd-Process reserviert und path
	hineinkopiert. Mit SendMessage lädt Winamp den Skin neu */

//	Klappt leider nur unter Windows 2000... =(

	typedef BOOL (WINAPI *WRITEPROCESSMEMORY)(HANDLE,LPVOID,LPVOID,DWORD,LPDWORD);
	typedef LPVOID (WINAPI *VIRTUALALLOCEX)(HANDLE,LPVOID,SIZE_T,DWORD,DWORD);
	typedef BOOL (WINAPI *VIRTUALFREEEX)(HANDLE,LPVOID,SIZE_T,DWORD);


	// Dll laden
	HINSTANCE hDll=LoadLibrary("KERNEL32.DLL");
	if (hDll<(HINSTANCE)HINSTANCE_ERROR)return -1;

	// Funktionen suchen 
	WRITEPROCESSMEMORY MyWriteProcessMemory;
	VIRTUALALLOCEX MyVirtualAllocEx;
	VIRTUALFREEEX MyVirtualFreeEx;

	MyWriteProcessMemory=(WRITEPROCESSMEMORY)GetProcAddress(hDll,"WriteProcessMemory");
	MyVirtualAllocEx=(VIRTUALALLOCEX)GetProcAddress(hDll,"VirtualAllocEx");
	MyVirtualFreeEx=(VIRTUALFREEEX)GetProcAddress(hDll,"VirtualFreeEx");

	// Nur, wenn alle Funktionen gefunden, dann weitermachen
	if ((MyWriteProcessMemory==NULL)||(MyVirtualAllocEx==NULL)||(MyVirtualFreeEx==NULL))
	{
		FreeLibrary(hDll);
		return -1;
	}
	// Wenn Winamp nicht läuft, 0 zurück
	if (wnd==0)
	{
		FreeLibrary(hDll);
		return 0;
	}

	{

		GetWindowThreadProcessId(wnd,&process);
		handle=OpenProcess(PROCESS_ALL_ACCESS,TRUE,process);

		p=MyVirtualAllocEx(handle,NULL,bufsize,MEM_COMMIT,PAGE_READWRITE);
		if (p==NULL)
		{
			CloseHandle(handle);
			FreeLibrary(hDll);
			return -1;
		}

		// Wenn Speicherblock kopiert, Winamp zeigen, wo's lang geht.
		if (MyWriteProcessMemory(handle,p,path,strlen(path)+2,&written)==TRUE)
			SendMessage(wnd,WM_USER,(unsigned int)(p),200);

		MyVirtualFreeEx(handle,p,bufsize,MEM_RELEASE);
		CloseHandle(handle);
	}
	FreeLibrary(hDll);
	return 1;
}

BOOLEAN ShowWinamp()
{
	HWND winamp=FindWindow("Winamp v1.x",NULL);
	if (winamp==0)return FALSE;
    SendMessage(winamp,WM_USER+1,0,0x202);
	ShowWindow(winamp,SW_RESTORE);
	SetForegroundWindow(winamp);
	SetActiveWindow(winamp);
	return TRUE;
}

BOOLEAN SKIN::SelectSkin()
{
	CHAR temp[MAX_PATH];
	strcpy(temp,OriginalPath);
	int i=strlen(OriginalPath);
	while (temp[i]!='.')i--;
	strcpy(&temp[0],&temp[i+1]);
	strupr(&temp[0]);

	i=SetWinampSkin(OriginalPath);
	if (i==1)
	{
		ShowWinamp();
		return TRUE;
	}
	if (i==0)
	{
		if (strcmp(&temp[0],"WSZ")!=0)
		{
			MessageBox(wnd,IDS_WANOTRUNNING,IDS_WANOTRUNNING_CAPTION,MB_OK);
			return FALSE;
		}
	}
	if (strcmp(&temp[0],"WSZ")==0)
	{
		HINSTANCE e=ShellExecute(0,"install",OriginalPath,NULL,NULL,SW_SHOWNORMAL);
		if (int(e)<=32)
		{
			MessageBox(wnd,IDS_ERRORWSZ,IDS_CANTEXECUTEFILE,MB_OK|MB_ICONINFORMATION);
		}
	}
	else
	{
		MessageBox(wnd,IDS_NOWSZSKIN,IDS_UNFAVOURABLE,MB_OK|MB_ICONINFORMATION);
		return FALSE;
	}
	return TRUE;
}

