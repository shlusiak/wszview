#include "windows.h"
#include "splitter.h"
#include "resource.h"
#include "explorer.h"
#include "commctrl.h"

const SplitterWidth=5;

HWND GetTreeView(HWND wnd)
{
	return GetDlgItem(GetParent(wnd),IDC_TREE);
}

HWND GetListView(HWND wnd)
{
	return GetDlgItem(GetParent(wnd),IDC_LIST);
}

BOOLEAN MouseDown;
POINT MousePos;

LRESULT WINAPI SplitterWndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_CREATE:
		{
			MouseDown=FALSE;
			MousePos.x=-1;
			MousePos.y=-1;
			break;
		}
	case WM_ALIGNSPLITTER:
		{
			RECT r1,r2;
			GetWindowRect(GetTreeView(hWnd),&r1);
			ScreenToClient(GetParent(GetTreeView(hWnd)),&r1);
			GetWindowRect(GetListView(hWnd),&r2);
			ScreenToClient(GetParent(GetListView(hWnd)),&r2);
			SetWindowPos(hWnd,0,r1.right,r1.top,SplitterWidth,r1.bottom-r1.top+1,0);
			InvalidateRect(hWnd,NULL,FALSE);
			break;
		}
	case WM_ALIGNOTHERS:
		{
			RECT r,r2,r3;
			GetWindowRect(hWnd,&r2);
			ScreenToClient(GetParent(hWnd),&r2);
			GetClientRect(GetParent(hWnd),&r3);

			const right=0;
			const bottom=0;

			GetWindowRect(GetListView(hWnd),&r);
			ScreenToClient(GetParent(hWnd),&r);
			SetWindowPos(GetListView(hWnd),0,r2.right,r.top,r.right-r.left+(r.left-r2.right),r.bottom-r.top,SWP_NOACTIVATE);
			
			GetWindowRect(GetTreeView(hWnd),&r);
			ScreenToClient(GetParent(hWnd),&r);
			SetWindowPos(GetTreeView(hWnd),0,r.left,r.top,r2.left-r.left,r.bottom,SWP_NOACTIVATE);

			SendMessage(GetListView(hWnd),LVM_ARRANGE,LVA_DEFAULT,0);

			break;
		}
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC dc=BeginPaint(hWnd,&ps);

			RECT r;
			GetClientRect(hWnd,&r);

			r.top--;
			r.right++;
			r.bottom+=2;

			HBRUSH br;

			br=CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
			FillRect(dc,&r,br);
			DeleteObject(br);
			
			br=CreateSolidBrush(GetSysColor(COLOR_BTNHIGHLIGHT));
			FrameRect(dc,&r,br);
			DeleteObject(br);
			OffsetRect(&r,-1,-2);
			br=CreateSolidBrush(GetSysColor(COLOR_BTNSHADOW));
			FrameRect(dc,&r,br);
			DeleteObject(br);

			EndPaint(hWnd,&ps);
			break;
		}
	case WM_LBUTTONDOWN:
		{
			if (!MouseDown)
			{
				SetCapture(hWnd);
				MouseDown=TRUE;
				MousePos.x=short(LOWORD(lParam));
				MousePos.y=short(HIWORD(lParam));
				SendMessage(hWnd,WM_ALIGNOTHERS,0,0);
			}
			break;
		}
	case WM_MOUSEMOVE:
		{
			POINT m={short(LOWORD(lParam)),short(HIWORD(lParam))};
			if ((MouseDown==TRUE)&&(m.x!=MousePos.x))
			{
				RECT r;
				GetWindowRect(hWnd,&r);
				ScreenToClient(GetParent(hWnd),&r);

				SetWindowPos(hWnd,HWND_TOP,r.left+m.x-MousePos.x,r.top,0,0,SWP_NOSIZE|SWP_NOACTIVATE);
				SendMessage(hWnd,WM_ALIGNOTHERS,0,0);
				InvalidateRect(hWnd,NULL,FALSE);
				InvalidateRect(GetTreeView(hWnd),NULL,FALSE);
				InvalidateRect(GetListView(hWnd),NULL,FALSE);
			}
			break;
		}
	case WM_LBUTTONUP:
		{
			if (MouseDown)
			{
				ReleaseCapture();
				MouseDown=FALSE;
				MousePos.x=MousePos.y=0;
			}
			break;
		}
	default: return DefWindowProc(hWnd,uMsg,wParam,lParam);
	}
	return 0;
}

void RegisterSplitterClass(HINSTANCE hInstance)
{
	WNDCLASS wc;
	ZeroMemory(&wc,sizeof(wc));
	wc.hbrBackground=(HBRUSH)GetStockObject(NULL_BRUSH);
	wc.hCursor=LoadCursor(0,IDC_SIZEWE);
	wc.hInstance=hInstance;
	wc.lpfnWndProc=SplitterWndProc;
	wc.lpszClassName="MYSPLITTER";

	RegisterClass(&wc);
}