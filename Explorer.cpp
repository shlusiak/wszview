#include "windows.h"
#include "explorer.h"
#include "wszview.h"
#include "skin.h"
#include "resource.h"
#include "commctrl.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "explorerutils.h"
#include "splitter.h"
#include "stdio.h"


const faktor=4;
int viewimagesizex=picturewidth/faktor;
int viewimagesizey=pictureheight/faktor;
const treeimagesizex=GetSystemMetrics(SM_CXSMICON);
const treeimagesizey=GetSystemMetrics(SM_CYSMICON);


const ret_FeedListThread=100;
const ret_AddNodeThread=200;



#define TreeShellImageList
//#define ListShellImageList


PCHAR path;
HIMAGELIST viewimagelist,treeimagelist;
LPITEMIDLIST PDesktopIDL;
IShellFolder *Root;


void ScreenToClient(HWND hWnd,LPRECT r)
{
	POINT p={r->left,r->top};
	ScreenToClient(hWnd,&p);
	r->left=p.x;
	r->top=p.y;
	p.x=r->right;
	p.y=r->bottom;
	ScreenToClient(hWnd,&p);
	r->right=p.x;
	r->bottom=p.y;
}

PSHELLFOLDER GetSelectedShellFolder(HWND owner)
{
	TVITEM ti;
	ZeroMemory(&ti,sizeof(ti));
	ti.hItem=TreeView_GetSelection(GetDlgItem(owner,IDC_TREE));
	ti.mask=TVIF_PARAM;
	TreeView_GetItem(GetDlgItem(owner,IDC_TREE),&ti);
	return (PSHELLFOLDER)ti.lParam;
}

BOOLEAN GetSkinBitmap(PCHAR path,BOOLEAN file,HWND wnd,int imageindex,int itemindex,PLISTITEM li)
{	
#ifdef ListShellImageList
	return FALSE;
#endif
	if (strlen(path)==0)return FALSE;
	if ((!file)&&(path[strlen(path)]!='\\'))
		strcat(path,"\\");
	SKIN skin;
	skin.wnd=0;
	skin.OriginalPath=(PCHAR)malloc(strlen(path)+2);
	strcpy(skin.OriginalPath,path);
	skin.Path=(PCHAR)malloc(2);
	skin.Path[0]='\0';

	HANDLE hMutex=CreateMutex(NULL,FALSE,"WSZMUTEX1");
	WaitForSingleObject(hMutex,INFINITE);
	BOOLEAN b=skin.SetSkin();
	ReleaseMutex(hMutex);

	if (b)
	{
		li->skin=TRUE;
		HDC dc=GetDC(GetDesktopWindow());
		HDC memdc=CreateCompatibleDC(dc),memdc2=CreateCompatibleDC(dc);

		HBITMAP smallbitmap,mask;

		skin.bitmap=CreateCompatibleBitmap(dc,picturewidth,pictureheight);
		smallbitmap=CreateCompatibleBitmap(dc,viewimagesizex,viewimagesizey);
		mask=CreateCompatibleBitmap(dc,viewimagesizex,viewimagesizey);
		ReleaseDC(GetDesktopWindow(),dc);

		SelectObject(memdc,mask);
		PatBlt(memdc,0,0,viewimagesizex,viewimagesizey,BLACKNESS);

		skin.RefreshBitmap();
		SelectObject(memdc,skin.bitmap);
		SelectObject(memdc2,smallbitmap);

		SetStretchBltMode(memdc2,HALFTONE);
		StretchBlt(memdc2,0,0,viewimagesizex,viewimagesizey,memdc,0,0,picturewidth,pictureheight,SRCCOPY);

		RestoreDC(memdc,-1);
		RestoreDC(memdc2,-1);
		DeleteDC(memdc);
		DeleteDC(memdc2);
		DeleteObject(skin.bitmap);

		ImageList_Replace(viewimagelist,imageindex,smallbitmap,mask);
		DeleteObject(smallbitmap);
		DeleteObject(mask);

		SendMessage(wnd,LVM_REDRAWITEMS,itemindex,itemindex);
		WaitForSingleObject(hMutex,INFINITE);
		skin.DeleteTempPath();
		ReleaseMutex(hMutex);
	}
	free(skin.Path);
	free(skin.OriginalPath);
	CloseHandle(hMutex);
	return b;
}

volatile BOOLEAN ExitFeedListThread;

int CALLBACK SortListCB(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort)
{
	PLISTITEM f1=(PLISTITEM)lParam1,f2=(PLISTITEM)lParam2;
	return short(f1->Parent->sf->CompareIDs(0,f1->idl,f2->idl));
}

DWORD WINAPI FeedListThread(LPVOID param) 
{
//	First list all files of the directory and add their items
	ExitFeedListThread=FALSE;
	HWND owner=(HWND)param;
	HWND wnd=GetDlgItem(owner,IDC_LIST);
	BOOLEAN SkinsOnly=GetMenuState(GetSubMenu(GetMenu(owner),1),ID_VIEW_SHOWSKINSONLY,MF_BYCOMMAND)==MF_CHECKED;
	IShellFolder *sf=GetSelectedShellFolder((HWND)param)->sf;

	{
		SendMessage(wnd,LVM_DELETEALLITEMS,0,0);
#ifndef ListShellImageList
		ImageList_RemoveAll(viewimagelist);
#endif

		int image=0;
		LVITEM li;
		CHAR temp[MAX_PATH];
		IEnumIDList *ids;
		STRRET s;
		HICON icon=0;
		LPMALLOC m;
		SHGetMalloc(&m);
		ITEMIDLIST *idl;
		ULONG count;

		if (sf==NULL)return -1;
		if (FAILED(sf->EnumObjects(wnd,SHCONTF_FOLDERS|SHCONTF_NONFOLDERS,&ids)))return -1;
		int nr=0;

		while (ids->Next(1,&idl,&count)==NOERROR)
		{
			s.uType=STRRET_CSTR;
			sf->GetDisplayNameOf(idl,SHGDN_NORMAL,&s);
			StrRetToBuf(s,idl,&temp[0],sizeof(temp));


#ifdef ListShellImageList
			SHFILEINFO sfi;
			LPITEMIDLIST idl2=ConcatPIDLs(GetSelectedShellFolder(owner)->absoluteidl,idl);
			SHGetFileInfo((LPCTSTR)idl2,
				0,
				&sfi,
				sizeof(sfi),
				SHGFI_PIDL|SHGFI_SYSICONINDEX);
			DisposePIDL(idl2);
			image=sfi.iIcon;
#else
			int e=0;
			icon=ExtractIcon(idl,(HWND)param,sf,viewimagesizex,FALSE,FALSE);
			if (icon==0)icon=LoadIcon(0,IDI_APPLICATION);
			e=GetLastError();
			image=ImageList_AddIcon(viewimagelist,icon);
			e=GetLastError();
			DestroyIcon(icon);
			if (image==-1)
			{
				m->Free(idl);

				CHAR temp1[100],temp2[100];
				LoadString(hInstance,IDS_OUTOFMEMORY,&temp2[0],sizeof(temp1));
				sprintf(&temp1[0],&temp2[0],e);
				LoadString(hInstance,IDS_ERROR,&temp2[0],sizeof(temp2));
				MessageBox(wnd,&temp1[0],&temp2[0],MB_OK|MB_ICONHAND);
				goto loadskinicons;
			}
#endif

			BOOLEAN folder=FALSE;

			ZeroMemory(&li,sizeof(li));
			li.mask=LVIF_TEXT|LVIF_IMAGE|LVIF_PARAM; 
			li.iItem=0;
			li.iImage=image;
			li.pszText=&temp[0];
			li.lParam=(LPARAM)CreateListItem(GetSelectedShellFolder((HWND)param),idl,folder,FALSE);
	
			if (SkinsOnly)
			{
				CHAR p[MAX_PATH];
				strcpy(&p[0],path);
				strcat(&p[0],&temp[0]);
				if (GetSkinBitmap(&p[0],BOOLEAN((GetFileAttributes(&p[0])&FILE_ATTRIBUTE_DIRECTORY)==0),wnd,li.iImage,ListView_GetItemCount(wnd),(PLISTITEM)li.lParam))
				{
					SendMessage(wnd,LVM_INSERTITEM,0,LPARAM(&li));
				}else
				{
					((PLISTITEM)(li.lParam))->Free();	
					delete ((PLISTITEM)(li.lParam));
					ImageList_Remove(viewimagelist,image);
					nr--;
				}
			}else SendMessage(wnd,LVM_INSERTITEM,0,LPARAM(&li));
//			DestroyIcon(icon);
			nr++;
			m->Free(idl);
			if (ExitFeedListThread)break;
		}

		ids->Release();
		m->Release();
		SendMessage(wnd,LVM_SORTITEMS,0,LPARAM(&SortListCB));
	}

#ifndef ListShellImageList
	loadskinicons:
#endif
	if (SkinsOnly)goto fertig;

	// Now look for skin images...
#ifndef ListShellImageList
	LVITEM li;
	CHAR temp[MAX_PATH];
	int i;
	for (i=0;i<SendMessage(wnd,LVM_GETITEMCOUNT,0,0);i++)
	{
		ZeroMemory(&li,sizeof(li));
		li.pszText=&temp[0];
		li.cchTextMax=sizeof(temp)-1;
		li.mask=LVIF_IMAGE|LVIF_TEXT|LVIF_PARAM;
		li.iItem=i;
		SendMessage(wnd,LVM_GETITEM,0,LPARAM(&li));

		CHAR p[MAX_PATH];
		strcpy(&p[0],path);
		strcat(&p[0],&temp[0]);

		GetSkinBitmap(&p[0],BOOLEAN((GetFileAttributes(&p[0])&FILE_ATTRIBUTE_DIRECTORY)==0),wnd,li.iImage,i,(PLISTITEM)li.lParam);
		if (ExitFeedListThread)goto fertig;
	}
#endif

fertig:
	// Signalize the thread has finished
	SendMessage(wnd,WM_USER+1,0,0);
	return ret_FeedListThread;
}

HANDLE FeedListThreadHandle=0;

BOOLEAN CloseFeedListThread()
{
	if (FeedListThreadHandle==0)return TRUE;
	ExitFeedListThread=TRUE;
	while (WaitForSingleObject(FeedListThreadHandle,50)==WAIT_TIMEOUT)
	{
		Warte();
	}
	return TRUE;
}

void FeedList(HWND owner)
{
	static BOOLEAN inFeedList=FALSE;
	if (inFeedList)return;
	inFeedList=TRUE;
	if (CloseFeedListThread()==FALSE)return;
	inFeedList=FALSE;
	if (strlen(path)!=0)if (path[strlen(path)-1]!='\\')strcat(path,"\\");

//	Start thread, which lists the content of the directory and loads all icons (with skin previews)
	DWORD id;
	FeedListThreadHandle=CreateThread(NULL,0,FeedListThread,LPVOID(owner),0,&id);
}

void SetListImageSize(HWND list,int vx,int vy)
{
	ImageList_RemoveAll(viewimagelist);
	if (ImageList_SetIconSize(viewimagelist,vx,vy)==0)
	{
		MessageBox(list,"Error setting imagelist size!","Error!",MB_OK|MB_ICONHAND);
		return;
	}
	viewimagesizex=vx;
	viewimagesizey=vy;
	if (ListView_SetImageList(list,viewimagelist,TVSIL_NORMAL)==0)
	{
		MessageBox(list,"Error assigning imagelist!","Error!",MB_OK|MB_ICONHAND);
		ListView_SetImageList(list,0,TVSIL_NORMAL);
	}
	FeedList(GetParent(list));
}

HTREEITEM AddItem(HWND wnd,HTREEITEM parent,PCHAR name,int imageindex,DWORD state,LPVOID param,int children)
{
	TVINSERTSTRUCT ti;
	ZeroMemory(&ti,sizeof(ti));

	ti.hParent=parent;
	ti.hInsertAfter=TVI_LAST;
	ti.item.mask=TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_CHILDREN|TVIF_STATE|TVIF_PARAM;
	ti.item.stateMask=ti.item.state=state;
	ti.item.cChildren=children;
	ti.item.pszText=name;
	ti.item.iSelectedImage=ti.item.iImage=imageindex;
	ti.item.lParam=(LPARAM)param;
	return (HTREEITEM)SendMessage(wnd,TVM_INSERTITEM,0,LPARAM(&ti));
}


typedef struct
{
	HWND owner;
	HTREEITEM node;
	PSHELLFOLDER P;
}*PAddNodeDataRec;

DWORD WINAPI AddNodeThread(LPVOID param)
{
	HANDLE e=CreateEvent(NULL,TRUE,FALSE,"FIRSTNODEADD");
	BOOL First=TRUE;
	PAddNodeDataRec r=(PAddNodeDataRec)param;
	HWND wnd=GetDlgItem(r->owner,IDC_TREE);
	ULONG count;
	IEnumIDList *ids;
	STRRET s;
#ifndef TreeShellImageList
	HICON icon;
#endif
	int image;
	IShellFolder *newsf;
	LPMALLOC m;
	SHGetMalloc(&m);
	ITEMIDLIST *idl;

	if (FAILED(r->P->sf->EnumObjects(r->owner,SHCONTF_FOLDERS,&ids)))
	{
		SetEvent(e);
		CloseHandle(e);
		return -1;
	}
	CHAR temp[MAX_PATH];

	while (ids->Next(1,&idl,&count)==NOERROR)
	{
		s.uType=STRRET_CSTR;
		r->P->sf->GetDisplayNameOf(idl,SHGDN_NORMAL,&s);
		StrRetToBuf(s,idl,&temp[0],sizeof(temp));

		r->P->sf->BindToObject(idl,NULL,IID_IShellFolder,(LPVOID*)&newsf);
		if (newsf==NULL)
		{
			m->Free(idl);
			continue;
		}


#ifdef TreeShellImageList
		SHFILEINFO fi;
		LPITEMIDLIST idl2;
		idl2=ConcatPIDLs(r->P->absoluteidl,idl);
		SHGetFileInfo((PCHAR)idl2,0,&fi,sizeof(fi),SHGFI_PIDL|SHGFI_SYSICONINDEX|SHGFI_OPENICON|SHGFI_SMALLICON);
		DisposePIDL(idl2);
		image=fi.iIcon;
#else
		icon=LoadIcon(0,IDI_APPLICATION);
		image=ImageList_AddIcon(treeimagelist,icon);
#endif

		ULONG attr=SFGAO_CONTENTSMASK;
		r->P->sf->GetAttributesOf(1,(const ITEMIDLIST**)&idl,&attr);
		PSHELLFOLDER sf=CreateShellFolder(newsf,r->P,idl);

		AddItem(wnd,r->node,&temp[0],image,0,sf,(attr&SFGAO_CONTENTSMASK)==SFGAO_HASSUBFOLDER);
		if (First)
		{
			SetEvent(e);
			First=FALSE;
		}
		m->Free(idl);
	}

//	r->P->sf->Release();
	ids->Release();
	m->Release();
	free(r);
	CloseHandle(e);
	return ret_AddNodeThread;
}

int CALLBACK CompareCB(LPARAM lParam1, LPARAM lParam2,LPARAM lParamSort)
{
	PSHELLFOLDER f1=(PSHELLFOLDER)lParam1,f2=(PSHELLFOLDER)lParam2;
	return short(f1->Parent->sf->CompareIDs(0,f1->relativeidl,f2->relativeidl));
}

void AddNode(HWND owner,HTREEITEM node,PSHELLFOLDER P)
{
	PAddNodeDataRec r=(PAddNodeDataRec)malloc(sizeof(*r));
	r->owner=owner;
	r->node=node;
	r->P=P;
	DWORD id;
	HANDLE e=CreateEvent(NULL,TRUE,FALSE,"FIRSTNODEADD"),t;
	t=CreateThread(NULL,0,AddNodeThread,(LPVOID)r,0,&id);
	while (MsgWaitForMultipleObjects(1,&e,FALSE,INFINITE,QS_SENDMESSAGE|QS_POSTMESSAGE|QS_ALLEVENTS)==WAIT_OBJECT_0+1)
		Warte();
	while (MsgWaitForMultipleObjects(1,&t,FALSE,INFINITE,QS_SENDMESSAGE|QS_POSTMESSAGE|QS_ALLEVENTS)==WAIT_OBJECT_0+1)
		Warte();

	TVSORTCB cb;
	cb.hParent=node;
	cb.lParam=0;
	cb.lpfnCompare=&CompareCB;
	TreeView_SortChildrenCB(GetDlgItem(owner,IDC_TREE),&cb,0);
	CloseHandle(t);
	CloseHandle(e);
}

void FreeItem(HWND hWnd,HTREEITEM h);

void DeleteSubNodes(HWND wnd,HTREEITEM node)
{
	FreeItem(wnd,TreeView_GetChild(wnd,node));
	HTREEITEM h;
//	HTREEITEM selected=TreeView_GetSelection(wnd);
	while (h=TreeView_GetChild(wnd,node))
	{
//		if (h==selected)TreeView_SelectItem(wnd,node);
		TreeView_DeleteItem(wnd,h);
	}
/*	TVITEM ti;
	ZeroMemory(&ti,sizeof(ti));
	ti.mask=TVIF_HANDLE|TVIF_STATE;
	ti.hItem=node;
	ti.stateMask=TVIS_EXPANDEDONCE|TVIS_EXPANDED;
	ti.state=0;
	
	SendMessage(wnd,TVM_SETITEM,0,LPARAM(&ti));*/
}

void FeedTree(HWND owner)
{
	HWND wnd=GetDlgItem(owner,IDC_TREE);
	IEnumIDList *ids;
	STRRET s;
	HICON icon=0;
	int image=0;

	Root->EnumObjects(owner,SHCONTF_FOLDERS,&ids);
	CHAR temp[MAX_PATH];

	Root->GetDisplayNameOf(NULL,SHGDN_NORMAL,&s);
	StrRetToBuf(s,PDesktopIDL,&temp[0],MAX_PATH-1);
	
#ifndef TreeShellImageList
	icon=ExtractIcon(NULL,owner,Root,treeimagesizex,TRUE,FALSE);
	image=ImageList_AddIcon(treeimagelist,icon);
#else
	SHFILEINFO sfi;
	SHGetFileInfo(LPSTR(PDesktopIDL),0,&sfi,sizeof(sfi),SHGFI_SYSICONINDEX|SHGFI_PIDL);
	image=sfi.iIcon;
#endif
	AddItem(wnd,TVI_ROOT,&temp[0],image,0,CreateShellFolder(Root,NULL,PDesktopIDL),1);
	DestroyIcon(icon);

	ids->Release();
}

WNDPROC OldListWndProc=NULL,OldTreeWndProc=NULL;
IContextMenu2 *cm2=NULL;

void DisplayListContextMenu(HWND hWnd,int x,int y,PSHELLFOLDER path)
{
	IContextMenu *cm;
	IShellFolder *sf;
	ITEMIDLIST *idl;

	int i=SendMessage(hWnd,LVM_GETNEXTITEM,(WPARAM)-1,MAKELPARAM((UINT)LVNI_SELECTED,0));
	if (i!=-1)
	{
		LVITEM li;
		ZeroMemory(&li,sizeof(li));
		li.iItem=i;
		li.mask=LVIF_PARAM;
		ListView_GetItem(hWnd,&li);
		idl=(LPITEMIDLIST)((PLISTITEM)li.lParam)->idl;
		sf=path->sf;
	}else 
	{
		if (path->Parent!=NULL)sf=path->Parent->sf;else sf=Root;
		if (path->Parent!=NULL)idl=path->relativeidl;else idl=PDesktopIDL;
	}
	if (sf==NULL)return;
	if (idl==NULL)return;
	if (sf->GetUIObjectOf(hWnd,1,(LPCITEMIDLIST*)&(idl),IID_IContextMenu,NULL,(LPVOID*)(&cm))!=NOERROR)return;

	HMENU menu=CreatePopupMenu();
	cm->QueryInterface(IID_IContextMenu2,(LPVOID*)(&cm2));
    cm->QueryContextMenu(menu,0,1,0x7FFF,CMF_EXPLORE);

	BOOL bcmd=TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_RETURNCMD|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,x,y,0,hWnd,NULL);
	cm2->Release();
	cm2=NULL;

	if (bcmd)
	{
		UINT cmd=UINT(bcmd)-1;
		CMINVOKECOMMANDINFO ICI;
		ZeroMemory(&ICI,sizeof(ICI));
                
        ICI.cbSize=sizeof(ICI);
        ICI.hwnd=hWnd;
        ICI.lpVerb=MAKEINTRESOURCE(cmd);
        ICI.nShow=SW_SHOWNORMAL;
        cm->InvokeCommand(&ICI);
	}


	cm->Release();
	DestroyMenu(menu);
}

void DisplayTreeContextMenu(HWND hWnd,int x,int y,PSHELLFOLDER sf)
{
	HMENU menu=CreatePopupMenu();
	IContextMenu *cm;
	IShellFolder *isf;

	if (sf->Parent!=NULL)isf=sf->Parent->sf;else isf=sf->sf;

	isf->GetUIObjectOf(hWnd,1,(LPCITEMIDLIST*)&(sf->relativeidl),IID_IContextMenu,NULL,(LPVOID*)(&cm));

	cm->QueryInterface(IID_IContextMenu2,(LPVOID*)(&cm2));
    cm->QueryContextMenu(menu,0,1,0x7FFF,CMF_EXPLORE);

	BOOL bcmd=TrackPopupMenu(menu,TPM_LEFTALIGN|TPM_RETURNCMD|TPM_LEFTBUTTON|TPM_RIGHTBUTTON,x,y,0,hWnd,NULL);
	cm2->Release();
	cm2=NULL;

	if (bcmd)
	{
		UINT cmd=UINT(bcmd)-1;
		CMINVOKECOMMANDINFO ICI;
		ZeroMemory(&ICI,sizeof(ICI));
                
        ICI.cbSize=sizeof(ICI);
        ICI.hwnd=hWnd;
        ICI.lpVerb=MAKEINTRESOURCE(cmd);
        ICI.nShow=SW_SHOWNORMAL;
        cm->InvokeCommand(&ICI);
	}

	DestroyMenu(menu);

	cm->Release();
}

LRESULT CALLBACK ListWndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
	case WM_USER+1:
		SendMessage(GetParent(hWnd),uMsg,wParam,lParam);
		break;
	case WM_CONTEXTMENU:
		{
			TVITEM t;
			ZeroMemory(&t,sizeof(t));
			t.hItem=TreeView_GetSelection(GetDlgItem(GetParent(hWnd),IDC_TREE));
			t.mask=TVIF_PARAM;
			TreeView_GetItem(GetDlgItem(GetParent(hWnd),IDC_TREE),&t);
			DisplayListContextMenu(hWnd,LOWORD(lParam),HIWORD(lParam),PSHELLFOLDER(t.lParam));
		}
		break;
	case WM_INITMENUPOPUP:
	case WM_DRAWITEM:
	case WM_MENUCHAR:
    case WM_MEASUREITEM:
		if (cm2!=NULL)
		{
			cm2->HandleMenuMsg(uMsg,wParam,lParam);
			return 0;
		}
		break;
	case LVM_DELETEALLITEMS:
		{
			LVITEM li;
			ZeroMemory(&li,sizeof(li));
			for (int i=0;i<SendMessage(hWnd,LVM_GETITEMCOUNT,0,0);i++)
			{
				li.mask=LVIF_PARAM;
				li.iItem=i;
				ListView_GetItem(hWnd,&li);
				(PLISTITEM(li.lParam))->Free();
				delete (PLISTITEM)li.lParam;
			}
			CallWindowProc(OldListWndProc,hWnd,uMsg,wParam,lParam);
			break;
		}
	default: return CallWindowProc(OldListWndProc,hWnd,uMsg,wParam,lParam);
	}
	return 0;
}

void FreeItem(HWND hWnd,HTREEITEM h)
{
	TVITEMEX it;
	ZeroMemory(&it,sizeof(it));
	it.mask=LVIF_PARAM|LVIF_TEXT;
	CHAR temp[MAX_PATH];
	it.pszText=&temp[0];
	it.cchTextMax=MAX_PATH;
	do
	{
		if (TreeView_GetChild(hWnd,h))
			FreeItem(hWnd,TreeView_GetChild(hWnd,h));
		it.hItem=h;
		TreeView_GetItem(hWnd,&it);
		if (it.lParam)
		{
			(PSHELLFOLDER(it.lParam))->Free();
			delete (PSHELLFOLDER)it.lParam;
		}
		HTREEITEM h2=h;
		h=TreeView_GetNextItem(hWnd,h,TVGN_NEXT);
//		TreeView_DeleteItem(hWnd,h2);
	}while (h);
}

LRESULT CALLBACK TreeWndProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_COMMAND:
	case WM_USER+1:
		SendMessage(GetParent(hWnd),uMsg,wParam,lParam);
		break;
	case WM_RBUTTONDOWN:
		{
			TVHITTESTINFO hti;
			hti.pt.x=short(LOWORD(lParam));
			hti.pt.y=short(HIWORD(lParam));
			HTREEITEM ti=TreeView_HitTest(hWnd,&hti);
			if (ti==0)break;
			TreeView_SelectItem(hWnd,ti);
		}
		break;
	case WM_INITMENUPOPUP:
	case WM_DRAWITEM:
	case WM_MENUCHAR:
    case WM_MEASUREITEM:
		if (cm2!=NULL)cm2->HandleMenuMsg(uMsg,wParam,lParam);
		break;
	default: return CallWindowProc(OldTreeWndProc,hWnd,uMsg,wParam,lParam);
	}
	return 0;
}

int NodeLevel(HWND wnd,HTREEITEM node)
{
	int level=1;
	while (node=TreeView_GetParent(wnd,node))level++;
	return level;
}

HTREEITEM TreeView_GetNext(HWND wnd,HTREEITEM itemid)
{
	if (itemid==0)return 0;
	HTREEITEM node,parent;
	node=TreeView_GetChild(wnd,itemid);
	if (node==0)node=TreeView_GetNextSibling(wnd,itemid);
	parent=itemid;
	while ((node==0)&&(parent!=0))
	{
		parent=TreeView_GetParent(wnd,parent);
		node=TreeView_GetNextSibling(wnd,parent);
	}
	return node;
}

HTREEITEM FolderExists(HWND wnd,LPITEMIDLIST id,HTREEITEM node)
{
	int alevel=NodeLevel(wnd,node);
	do
	{
		LPITEMIDLIST absid;
		TVITEM item;
		ZeroMemory(&item,sizeof(item));
		item.mask=TVIF_PARAM|TVIF_TEXT;
		item.hItem=node;
		CHAR temp[MAX_PATH];
		item.pszText=&temp[0];
		item.cchTextMax=100;
		TreeView_GetItem(wnd,&item);

		absid=((PSHELLFOLDER)(item.lParam))->absoluteidl;
   		if (Root->CompareIDs(0,id,absid)==0)
			return node;
		node=TreeView_GetNext(wnd,node);
	}while ((node!=0)/*&&(NodeLevel(wnd,node)<=alevel)*/);
	return 0;
}

void SetPathIDList(HWND owner,LPITEMIDLIST idl)
{
	TreeView_SelectItem(owner,0);
	if (idl==NULL)
	{
		TreeView_Expand(owner,TreeView_GetRoot(owner),TVE_EXPAND);
		return;
	}
	int i;

	static BOOLEAN updating=FALSE;

	if (updating)return;
	updating=TRUE;
	PIDLLIST pidls=CreatePIDLList(idl);

	HTREEITEM node=TreeView_GetRoot(owner),temp;
	for (i=pidls->count-1;i>=0;i--)
	{
		temp=FolderExists(owner,pidls->Get(i),node);
		if (temp!=0)
		{
			node=temp;
			TreeView_Expand(owner,node,TVE_EXPAND);
			TreeView_EnsureVisible(owner,node);
		}
	}
    node=FolderExists(owner,idl,node);
	TreeView_SelectItem(owner,node);

	delete pidls;
	updating=FALSE;
}

void SetPath(HWND owner,PCHAR path)
{
	OLECHAR c[MAX_PATH*2];
	ITEMIDLIST *idl;
	ULONG chars=strlen(path);
	MultiByteToWideChar(CP_ACP,0,path,-1,&c[0],sizeof(c));

	Root->ParseDisplayName(0,NULL,&c[0],&chars,&idl,0);

	SetPathIDList(owner,idl);
}

DWORD WINAPI SetPathThread(LPVOID param)
{
	PPCHARREC r=(PPCHARREC)param;
	SetPath(r->w,r->temp);
	free(r);
	return 0;
}

BOOLEAN FindSkinPath(PCHAR p)
{
	CHAR temp1[MAX_PATH],temp2[MAX_PATH];
	strcpy(&temp2[0],p);
	strcat(&temp2[0],"Winamp.ini");
	if (GetPrivateProfileString("Winamp","Skindir","",&temp1[0],MAX_PATH,&temp2[0])==0)return FALSE;
	strcpy(p,&temp1[0]);
	return TRUE;
}

BOOLEAN FindWinamp(PCHAR p)
{
	const PCHAR item="UninstallString";
	const PCHAR defaultpath1="c:\\program files\\winamp\\";
	const PCHAR defaultpath2="c:\\programme\\winamp\\";
	CHAR temp[1024];
	ULONG c;
	HKEY hKey=0;
	int e;

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\Winamp",0,KEY_READ,&hKey)==ERROR_SUCCESS)
	{
		e=RegCloseKey(HKEY_LOCAL_MACHINE);
		c=1023;
		e=RegQueryValueEx(hKey,item,NULL,NULL,(UCHAR*)(&temp[0]),&c);
		RegCloseKey(hKey);
		if (e==ERROR_SUCCESS)
		{
			while (temp[strlen(&temp[0])-1]!='\\')temp[strlen(&temp[0])-1]='\0';
			if (temp[0]=='"')strcpy(&temp[0],&temp[1]);
			if (FindSkinPath(&temp[0]))
			{
				strcpy(p,&temp[0]);
				return TRUE;
			}
		}
	}

	if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,"SOFTWARE\\Microsoft\\Windows\\CurrentVersion",0,KEY_READ,&hKey)==ERROR_SUCCESS)
	{
		RegCloseKey(HKEY_LOCAL_MACHINE);
		e=RegQueryValueEx(hKey,"ProgramFilesDir",NULL,NULL,(UCHAR*)(&temp[0]),&c);
		RegCloseKey(hKey);
		if ((e==ERROR_SUCCESS)&&(strlen(&temp[0])>=2))
		{
			if (temp[strlen(&temp[0])-1]!='\\')strcat(&temp[0],"\\");
			strcat(&temp[0],"Winamp\\");
			if (FindSkinPath(&temp[0]))
			{
				strcpy(p,&temp[0]);
				return TRUE;
			}
		}

	}
	strcpy(&temp[0],defaultpath1);
	if (FindSkinPath(&temp[0]))
	{
		strcpy(p,&temp[0]);
		return TRUE;
	}
	strcpy(&temp[0],defaultpath2);
	if (FindSkinPath(&temp[0]))
	{
		strcpy(p,&temp[0]);
		return TRUE;
	}
//	Nothing else to do. Winamp not found on machine... =(
	return FALSE;
}

void CheckImageSize(HWND hWnd,int nr)
{
	HMENU gmenu=GetMenu(hWnd);
	HMENU menu=GetSubMenu(GetSubMenu(gmenu,1),0);

	for (int i=0;i<7;i++)CheckMenuItem(menu,i,MF_BYPOSITION|MF_UNCHECKED);

	CheckMenuItem(menu,nr,MF_BYPOSITION|MF_CHECKED);

	SetMenu(hWnd,gmenu);
}

void SetListColor(HWND hWnd,int nr)
{
	SendMessage(hWnd,LVM_DELETEALLITEMS,0,0);
	SendMessage(hWnd,LVM_SETIMAGELIST,TVSIL_NORMAL,0);
	CloseFeedListThread();
	CloseHandle(FeedListThreadHandle);

	ImageList_RemoveAll(viewimagelist);
	ImageList_Destroy(viewimagelist);
	viewimagelist=ImageList_Create(viewimagesizex,viewimagesizey,
		ILC_MASK|((nr==0)?ILC_COLOR4:(nr==1)?ILC_COLOR8:(nr==2)?ILC_COLOR16:(nr==3)?ILC_COLOR24:(nr==4)?ILC_COLOR32:ILC_COLOR8)
		,1,2);
	SendMessage(hWnd,LVM_SETIMAGELIST,LVSIL_NORMAL,LPARAM(viewimagelist));
	FeedList(GetParent(hWnd));
}

void CheckListColor(HWND hWnd,int nr)
{
	HMENU gmenu=GetMenu(hWnd);
	HMENU menu=GetSubMenu(GetSubMenu(gmenu,1),1);
	for (int i=0;i<5;i++)CheckMenuItem(menu,i,MF_BYPOSITION|MF_UNCHECKED);
	CheckMenuItem(menu,nr,MF_BYPOSITION|MF_CHECKED);
	SetMenu(hWnd,gmenu);
}

BOOL CALLBACK DialogProc(HWND hWnd,UINT uMsg,WPARAM wParam,LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
		{
			path=(PCHAR)malloc(MAX_PATH);
			if (!FindWinamp(path))
				strcpy(path,"c:\\");

			SHGetDesktopFolder(&Root);
			SHGetSpecialFolderLocation(0, CSIDL_DESKTOP, &PDesktopIDL);

			OldListWndProc=(WNDPROC)GetWindowLong(GetDlgItem(hWnd,IDC_LIST),GWL_WNDPROC);
			SetWindowLong(GetDlgItem(hWnd,IDC_LIST),GWL_WNDPROC,UINT(&ListWndProc));

			OldTreeWndProc=(WNDPROC)GetWindowLong(GetDlgItem(hWnd,IDC_TREE),GWL_WNDPROC);
			SetWindowLong(GetDlgItem(hWnd,IDC_TREE),GWL_WNDPROC,UINT(&TreeWndProc));

#ifdef ListShellImageList
			SHFILEINFO fi;
			viewimagelist=(HIMAGELIST)SHGetFileInfo("",0,&fi,sizeof(fi),SHGFI_PIDL|SHGFI_ICON|SHGFI_SYSICONINDEX);
#else
			viewimagelist=ImageList_Create(viewimagesizex,viewimagesizey,
				ILC_COLOR16|ILC_MASK,
				1,2);
			CheckListColor(hWnd,2);
			ImageList_SetBkColor(viewimagelist,GetSysColor(COLOR_WINDOW));
#endif
			SendDlgItemMessage(hWnd,IDC_LIST,LVM_SETIMAGELIST,LVSIL_NORMAL,LPARAM(viewimagelist));


#ifndef TreeShellImageList
			treeimagelist=ImageList_Create(treeimagesizex,treeimagesizey,ILC_COLORDDB|ILC_MASK,1,0);
			ImageList_SetBkColor(treeimagelist,GetSysColor(COLOR_WINDOW));
#else
			SHFILEINFO fi2;
			treeimagelist=(HIMAGELIST)SHGetFileInfo("",0,&fi2,sizeof(fi2),SHGFI_SMALLICON|SHGFI_SYSICONINDEX);
#endif
			SendDlgItemMessage(hWnd,IDC_TREE,TVM_SETIMAGELIST,TVSIL_NORMAL,LPARAM(treeimagelist));

			TreeView_SelectItem(GetDlgItem(hWnd,IDC_TREE),0);
			FeedTree(hWnd);
//			FeedList(hWnd);

			PPCHARREC r=(PPCHARREC)malloc(sizeof(PCHARREC));
			r->temp=path;
			r->w=GetDlgItem(hWnd,IDC_TREE);

			DWORD id;
			CloseHandle(CreateThread(NULL,0,SetPathThread,r,0,&id));

			SendDlgItemMessage(hWnd,IDC_SPLITTER,WM_ALIGNSPLITTER,0,0);
			SendDlgItemMessage(hWnd,IDC_SPLITTER,WM_ALIGNOTHERS,0,0);

			break;
		}
	case WM_DESTROY:
		{
			DeleteSubNodes(GetDlgItem(hWnd,IDC_TREE),TVI_ROOT);
			SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_DELETEALLITEMS,0,0);
			SendDlgItemMessage(hWnd,IDC_TREE,TVM_SETIMAGELIST,TVSIL_NORMAL,0);
			SendDlgItemMessage(hWnd,IDC_LIST,LVM_SETIMAGELIST,TVSIL_NORMAL,0);
			CloseFeedListThread();
			CloseHandle(FeedListThreadHandle);
			Root->Release();
#ifndef ListShellImageList
			ImageList_Destroy(viewimagelist);
#endif
#ifndef TreeShellImageList
			ImageList_Destroy(treeimagelist);
#endif

			LPMALLOC m;
			SHGetMalloc(&m);
			m->Free(PDesktopIDL);
			m->Release();

			PostQuitMessage(0);
			break;
		}
	case WM_COMMAND:
		{
			switch(LOWORD(wParam))
			{
			case IDCANCEL:	// Close Dialog 
				DestroyWindow(hWnd);
				break;
			case 101:	// Refresh (F5)
				SetCursor(LoadCursor(0,IDC_WAIT));
				FeedList(hWnd);
				SetCursor(LoadCursor(0,IDC_ARROW));
				break;
			case 102:	// Info
				DialogBox(hInstance,MAKEINTRESOURCE(101),hWnd,MyDlgProc);
				break;
			case ID_VIEW_IMAGESIZE_100:
				CheckImageSize(hWnd,0);
				SetListImageSize(GetDlgItem(hWnd,IDC_LIST),picturewidth/1,pictureheight/1);
				break;
			case ID_VIEW_IMAGESIZE_50:
				CheckImageSize(hWnd,1);
				SetListImageSize(GetDlgItem(hWnd,IDC_LIST),picturewidth/2,pictureheight/2);
				break;
			case ID_VIEW_IMAGESIZE_25:
				CheckImageSize(hWnd,2);
				SetListImageSize(GetDlgItem(hWnd,IDC_LIST),picturewidth/4,pictureheight/4);
				break;
			case ID_VIEW_IMAGESIZE_20:
				CheckImageSize(hWnd,3);
				SetListImageSize(GetDlgItem(hWnd,IDC_LIST),picturewidth/5,pictureheight/5);
				break;
			case ID_VIEW_IMAGESIZE_10:
				CheckImageSize(hWnd,4);
				SetListImageSize(GetDlgItem(hWnd,IDC_LIST),picturewidth/10,pictureheight/10);
				break;
			case ID_VIEW_COLORS_4:
				CheckListColor(hWnd,0);
				SetListColor(GetDlgItem(hWnd,IDC_LIST),0);
				break;
			case ID_VIEW_COLORS_8:
				CheckListColor(hWnd,1);
				SetListColor(GetDlgItem(hWnd,IDC_LIST),1);
				break;
			case ID_VIEW_COLORS_16:
				CheckListColor(hWnd,2);
				SetListColor(GetDlgItem(hWnd,IDC_LIST),2);
				break;
			case ID_VIEW_COLORS_24:
				CheckListColor(hWnd,3);
				SetListColor(GetDlgItem(hWnd,IDC_LIST),3);
				break;
			case ID_VIEW_COLORS_32:
				CheckListColor(hWnd,4);
				SetListColor(GetDlgItem(hWnd,IDC_LIST),4);
				break;
			case ID_VIEW_SHOWSKINSONLY:
				{
					HMENU gmenu=GetMenu(hWnd);
					HMENU submenu=GetSubMenu(gmenu,1);

					CheckMenuItem(submenu,ID_VIEW_SHOWSKINSONLY,MF_BYCOMMAND|((GetMenuState(submenu,ID_VIEW_SHOWSKINSONLY,MF_BYCOMMAND==MF_CHECKED)?MF_UNCHECKED:MF_CHECKED)));
					SetMenu(hWnd,gmenu);
					FeedList(hWnd);
				}
				break;
			case ID_VIEW_SKINSIZE_USERDEFINED:
				{
					EnableWindow(hWnd,FALSE);
					DWORD cmd=DialogBoxParam(hInstance,MAKEINTRESOURCE(IDD_USERSIZE),hWnd,DlgProcUserDefined,MAKELONG(viewimagesizex,viewimagesizey));
					if (cmd!=0)
					{
						CheckImageSize(hWnd,6);
						SetListImageSize(GetDlgItem(hWnd,IDC_LIST),HIWORD(cmd),LOWORD(cmd));
					}
					EnableWindow(hWnd,TRUE);
				}
				break;
			}
			break;
		}
	case WM_USER+1:
			CloseHandle(FeedListThreadHandle);
			FeedListThreadHandle=0;
			break;
	case WM_SIZE:
		{
			RECT r2;
			RECT r;
			GetClientRect(hWnd,&r2);

			const right=0;
			const bottom=0;

			GetWindowRect(GetDlgItem(hWnd,IDC_LIST),&r);
			ScreenToClient(hWnd,&r);
			SetWindowPos(GetDlgItem(hWnd,IDC_LIST),0,r.left,r.top,r2.right-r.left-right,r2.bottom-r.top-bottom,SWP_NOACTIVATE);
			
			GetWindowRect(GetDlgItem(hWnd,IDC_TREE),&r);
			ScreenToClient(hWnd,&r);
			SetWindowPos(GetDlgItem(hWnd,IDC_TREE),0,r.left,r.top,r.right-r.left,r2.bottom-r.top-bottom,SWP_NOACTIVATE);

			SendMessage(GetDlgItem(hWnd,IDC_LIST),LVM_ARRANGE,LVA_DEFAULT,0);
			SendMessage(GetDlgItem(hWnd,IDC_SPLITTER),WM_ALIGNSPLITTER,0,0);
			break;
		}
	case WM_GETMINMAXINFO:
		{
			LPMINMAXINFO mm=LPMINMAXINFO(lParam);
			mm->ptMinTrackSize.x=200;
			mm->ptMinTrackSize.y=100;
			break;
		}
	case WM_NOTIFY:
		{
			NMHDR *notify=(NMHDR*)lParam;
			if (notify->idFrom==IDC_TREE)
			{
				switch(notify->code)
				{
				case TVN_ITEMEXPANDING:
					{
						NMTREEVIEW *tv=(NMTREEVIEW*)notify;
						if ((tv->action==TVE_EXPAND)&((tv->itemNew.state&TVIS_EXPANDEDONCE)==0))
						{
//							Node is getting expanded -> fill with needed information
							HCURSOR old=SetCursor(LoadCursor(0,IDC_WAIT));
							AddNode(hWnd,tv->itemNew.hItem,(PSHELLFOLDER)tv->itemNew.lParam);
							SetCursor(old);
						}
						if (tv->action==TVE_COLLAPSERESET)
						{
//							Delete subnodes and their lparam belonging to the collapsed one
							DeleteSubNodes(notify->hwndFrom,tv->itemNew.hItem);
							return FALSE;
						}
						break;
					}
				case TVN_SELCHANGED:
					{
						if (!IsWindowVisible(hWnd))break;
						NMTREEVIEW *tv=(NMTREEVIEW*)notify;
						TVITEM ti;
						ZeroMemory(&ti,sizeof(ti));
						ti.mask=TVIF_IMAGE;
#ifndef TreeShellImageList
						if (tv->itemOld.hItem!=0)
						{
							ti.hItem=tv->itemOld.hItem;
							TreeView_GetItem(tv->hdr.hwndFrom,&ti);
							ImageList_ReplaceIcon(treeimagelist,ti.iImage,
								ExtractIcon(((PSHELLFOLDER)tv->itemOld.lParam)->relativeidl,tv->hdr.hwndFrom,((PSHELLFOLDER)tv->itemOld.lParam)->sf,treeimagesizex,TRUE,FALSE));
						}
#endif

						if (tv->itemNew.hItem!=0)
						{
#ifndef TreeShellImageList
							ti.hItem=tv->itemNew.hItem;
							TreeView_GetItem(tv->hdr.hwndFrom,&ti);
							ImageList_ReplaceIcon(treeimagelist,ti.iImage,
								ExtractIcon(((PSHELLFOLDER)tv->itemNew.lParam)->relativeidl,tv->hdr.hwndFrom,((PSHELLFOLDER)tv->itemNew.lParam)->sf,treeimagesizex,TRUE,TRUE));
#endif
							CHAR temp[MAX_PATH];

							LPITEMIDLIST idl=((PSHELLFOLDER)tv->itemNew.lParam)->absoluteidl;
							SHGetPathFromIDList(idl,&temp[0]);
//							if (strcmp(&temp[0],"")!=0)
							{
								strcpy(path,&temp[0]);
								FeedList(hWnd);
							}
						}
						break;
					}
				}
			}
			if (notify->idFrom==IDC_LIST)
			{
				switch(notify->code)
				{
				case NM_DBLCLK:
					{
						int i=ListView_GetNextItem(GetDlgItem(hWnd,IDC_LIST),-1,LVNI_SELECTED);	
						if (i==-1)break;
						CHAR name[MAX_PATH];
						LVITEM lvi;
						ZeroMemory(&lvi,sizeof(lvi));
						lvi.pszText=&name[0];
						lvi.cchTextMax=sizeof(name)-1;
						lvi.iItem=i;
						lvi.mask=LVIF_PARAM|LVIF_TEXT;
						ListView_GetItem(GetDlgItem(hWnd,IDC_LIST),&lvi);
						//	Now execute the standard function with the item
						PLISTITEM li=(PLISTITEM)lvi.lParam;
						if (li->skin==TRUE)
						{
							SKIN skin;
							skin.OriginalPath=(PCHAR)malloc(strlen(path)+strlen(&name[0])+3);
							strcpy(skin.OriginalPath,path);
							strcat(skin.OriginalPath,&name[0]);
							skin.wnd=hWnd;
							skin.ParseSkinPath(skin.OriginalPath);
							skin.SelectSkin();
							free(skin.OriginalPath);
//							if (SetWinampSkin(&temp[0]))ShowWinamp();
						}
						break;
					}
				}
			}
			break;
		}
	case WM_CONTEXTMENU:
		{
			if ((HWND)wParam==GetDlgItem(hWnd,IDC_TREE))
			{
				POINT p={(short)LOWORD(lParam),(short)HIWORD(lParam)};
				if ((p.x==-1)||(p.y==-1))
				{
					RECT r;
					TreeView_GetItemRect((HWND)wParam,TreeView_GetSelection((HWND)wParam),&r,TRUE);

					p.x=r.left;
					p.y=r.bottom;
					ClientToScreen((HWND)wParam,&p);
				}
				DisplayTreeContextMenu((HWND)wParam,p.x,p.y,GetSelectedShellFolder(hWnd));
			}else return FALSE;
		}
		break;

	default: return FALSE;
	}
	return TRUE;
}


void CreateExplorerDialog()
{
	RegisterSplitterClass(hInstance);
	HWND wnd;
	wnd=CreateDialog(hInstance,MAKEINTRESOURCE(IDD_EXPLORER),0,DialogProc);

	SendMessage(wnd,WM_SETICON,ICON_BIG,(LPARAM)LoadIcon(hInstance,MAKEINTRESOURCE(100)));

	UpdateWindow(wnd);
	ShowWindow(wnd,SW_SHOW);
}