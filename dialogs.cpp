#include "windows.h"
#include "wszview.h"
#include "stdio.h"
#include "resource.h"
#include "Options.h"


BOOL CALLBACK MyDlgProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	switch (Msg)
	{
	case WM_COMMAND:
		{
			if (LOWORD(wParam)==IDCANCEL)
			{
				EnableWindow(GetParent(hWnd),TRUE);
				DestroyWindow(hWnd);
			}
		}
	default: return FALSE;
	}
	return TRUE;
}

void SetRelation(HWND hWnd,int C1)
{
	double ratio=1.0f;
	int C2;
	switch(C1)
	{
	case IDC_EDITX:
		ratio=double(pictureheight)/double(picturewidth);
		C2=IDC_EDITY;
		break;
	case IDC_EDITPX:
		C2=IDC_EDITPY;
		break;
	case IDC_EDITY:
		ratio=double(picturewidth)/double(pictureheight);
		C2=IDC_EDITX;
		break;
	case IDC_EDITPY:
		C2=IDC_EDITPX;
		break;
	}
	CHAR temp[10];
	SendDlgItemMessage(hWnd,C1,WM_GETTEXT,9,LPARAM(&temp[0]));
	int v=atoi(&temp[0]);
	v=int(v*ratio);
	sprintf(&temp[0],"%d",v);
	SendDlgItemMessage(hWnd,C2,WM_SETTEXT,0,LPARAM(&temp[0]));
}

void SetPercent(HWND wnd,int c1,int c2,int hundertp)
{
	CHAR temp[10];
	SendDlgItemMessage(wnd,c1,WM_GETTEXT,9,LPARAM(&temp[0]));
	int v=atoi(&temp[0]);
	v=int(double(v)/double(hundertp)*100);
	sprintf(&temp[0],"%d",v);
	SendDlgItemMessage(wnd,c2,WM_SETTEXT,0,LPARAM(&temp[0]));
}

void SetPixels(HWND wnd,int c1,int c2,int hundertp)
{
	CHAR temp[10];
	SendDlgItemMessage(wnd,c1,WM_GETTEXT,9,LPARAM(&temp[0]));
	int v=atoi(&temp[0]);
	v=int(double(v)*double(hundertp)/100.0f);
	sprintf(&temp[0],"%d",v);
	SendDlgItemMessage(wnd,c2,WM_SETTEXT,0,LPARAM(&temp[0]));
}

BOOL CALLBACK DlgProcUserDefined(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam)
{
	static BOOLEAN inupdate=FALSE;
	switch (Msg)
	{
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDCANCEL: 
				EndDialog(hWnd,0);
				break;
			case IDOK:
				{
					CHAR temp[10];
					SendDlgItemMessage(hWnd,IDC_EDITX,WM_GETTEXT,9,LPARAM(&temp[0]));
					int x=atoi(&temp[0]);
					SendDlgItemMessage(hWnd,IDC_EDITY,WM_GETTEXT,9,LPARAM(&temp[0]));
					int y=atoi(&temp[0]);
					KeepRelation=(SendDlgItemMessage(hWnd,IDC_RELATION,BM_GETCHECK,0,0)==BST_CHECKED);
					EndDialog(hWnd,MAKELONG(y,x));
					break;
				}
			case IDC_RELATION:
				{
					if (SendDlgItemMessage(hWnd,IDC_RELATION,BM_GETCHECK,0,0)==BST_CHECKED)
					{
						inupdate=TRUE;
						SetRelation(hWnd,IDC_EDITX);
						SetPercent(hWnd,IDC_EDITY,IDC_EDITPY,pictureheight);
						inupdate=FALSE;
					}
					break;
				}
			case IDC_EDITX:
			case IDC_EDITY:
			case IDC_EDITPX:
			case IDC_EDITPY:
				{
					if (HIWORD(wParam)==EN_CHANGE)
					{
						if (inupdate)break;
						inupdate=TRUE;
						if (SendDlgItemMessage(hWnd,IDC_RELATION,BM_GETCHECK,0,0)==BST_CHECKED)
						{
							SetRelation(hWnd,LOWORD(wParam));
						}
						if ((LOWORD(wParam)==IDC_EDITPX)||(LOWORD(wParam)==IDC_EDITPY))
						{
							SetPixels(hWnd,IDC_EDITPX,IDC_EDITX,picturewidth);
							SetPixels(hWnd,IDC_EDITPY,IDC_EDITY,pictureheight);
						}else{
							SetPercent(hWnd,IDC_EDITX,IDC_EDITPX,picturewidth);
							SetPercent(hWnd,IDC_EDITY,IDC_EDITPY,pictureheight);
						}
						inupdate=FALSE;
					}
				}
			}
			break;
		}
	case WM_INITDIALOG:
		{
			CHAR temp[10];
			sprintf(&temp[0],"%d",LOWORD(lParam));
			SendDlgItemMessage(hWnd,IDC_EDITX,WM_SETTEXT,0,LPARAM(&temp[0]));
			sprintf(&temp[0],"%d",HIWORD(lParam));
			SendDlgItemMessage(hWnd,IDC_EDITY,WM_SETTEXT,0,LPARAM(&temp[0]));
			SetPercent(hWnd,IDC_EDITX,IDC_EDITPX,picturewidth);
			SetPercent(hWnd,IDC_EDITY,IDC_EDITPY,pictureheight);
			SendDlgItemMessage(hWnd,IDC_RELATION,BM_SETCHECK,KeepRelation,0);
		}
	default: return FALSE;
	}
	return TRUE;
}
