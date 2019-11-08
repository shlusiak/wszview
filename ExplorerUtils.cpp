#include "windows.h"
#include "commctrl.h"
#include "shlobj.h"
#include "shlwapi.h"
#include "explorerutils.h"
#include "Options.h"


// GLobal Options

BOOLEAN KeepRelation=TRUE;

// ---



LPITEMIDLIST NextPIDL(LPITEMIDLIST idl)
{
	return (LPITEMIDLIST)(PCHAR(idl)+idl->mkid.cb);
}

LPITEMIDLIST CreatePIDL(int size)
{
	LPITEMIDLIST idl;
	IMalloc *m;
	SHGetMalloc(&m);
	idl=(LPITEMIDLIST)m->Alloc(size);
	ZeroMemory(idl,sizeof(idl));
	m->Release();
	return idl;
}

int GetPIDLSize(LPITEMIDLIST idl)
{
	int s=sizeof(idl->mkid.cb);
	while (idl->mkid.cb!=0)
	{
		s+=idl->mkid.cb;
		idl=NextPIDL(idl);
	}
	return s;
}

LPITEMIDLIST CopyPIDL(LPITEMIDLIST id)
{
	int size=GetPIDLSize(id);
	LPITEMIDLIST r=CreatePIDL(size);
	CopyMemory(r,id,size);
	return r;
}

void StripLastID(LPITEMIDLIST idl)
{
	LPITEMIDLIST Marker=idl;
	while (idl->mkid.cb!=0)
	{
		Marker=idl;
		idl=NextPIDL(idl);
	}
	Marker->mkid.cb=0;
}

void DisposePIDL(LPITEMIDLIST idl)
{
	IMalloc *m;
	SHGetMalloc(&m);
	m->Free(idl);
	m->Release();
}



IDLLIST::IDLLIST()
{
	idl=NULL;
	Next=NULL;
	count=0;
}

IDLLIST::~IDLLIST()
{
	if (Next!=NULL)delete Next;
	DisposePIDL(idl);
}

void IDLLIST::Add(LPITEMIDLIST id)
{
	if (Next)Next->Add(id);
	else 
	{
		idl=id;
		Next=new IDLLIST;
	}
	count++;
}

LPITEMIDLIST IDLLIST::Get(int index)
{
	if (index==0)return idl;
		else return Next->Get(index-1);
}

PIDLLIST CreatePIDLList(LPITEMIDLIST id)
{
	if (id==NULL)return NULL;
	LPITEMIDLIST temp=id;
	PIDLLIST idl=new IDLLIST;
	while (temp->mkid.cb!=0)
	{
		temp=CopyPIDL(temp);
		idl->Add(temp);
		StripLastID(temp);
	}
	return idl;
}

LPITEMIDLIST ConcatPIDLs(LPITEMIDLIST idl1,LPITEMIDLIST idl2)
{
	int cb1,cb2;

	if (idl1!=NULL)cb1=GetPIDLSize(idl1)-sizeof(idl1->mkid.cb);
	else cb1=0;

	cb2=GetPIDLSize(idl2);

	LPITEMIDLIST r=CreatePIDL(cb1+cb2);
	CopyMemory(r,idl1,cb1);
	CopyMemory((PCHAR(r)+cb1),idl2,cb2);
	return r;
}

void TSHELLFOLDER::Free()
{
	if (sf!=NULL)sf->Release();
	sf=NULL;
	if (Parent)
	{
		DisposePIDL(absoluteidl);
		DisposePIDL(relativeidl);
	}
	absoluteidl=NULL;
	relativeidl=NULL;
	Parent=NULL;
}

void TLISTITEM::Free()
{
	Parent=NULL;
	DisposePIDL(idl);
}

PSHELLFOLDER CreateShellFolder(IShellFolder *vsf,PSHELLFOLDER vParent,LPITEMIDLIST relid)
{
	PSHELLFOLDER sf=new TSHELLFOLDER;
	sf->Parent=vParent;
	sf->sf=vsf;
	if (vParent!=NULL)sf->absoluteidl=ConcatPIDLs(vParent->absoluteidl,relid);
		else sf->absoluteidl=CopyPIDL(relid);
	sf->relativeidl=CopyPIDL(relid);

	return sf;
}

PLISTITEM CreateListItem(PSHELLFOLDER Parent,LPITEMIDLIST idl,BOOLEAN folder,BOOLEAN skin)
{
	PLISTITEM li=new TLISTITEM;
	li->Parent=Parent;
	li->idl=CopyPIDL(idl);
	li->folder=folder;
	li->skin=skin;
	return li;
}



void StrRetToBuf(STRRET s,LPITEMIDLIST idl,PCHAR buf,int buflen)
{
	if (s.uType==STRRET_CSTR)
	{
		strcpy(buf,&s.cStr[0]);
		return;
	}
	if (s.uType==STRRET_WSTR)
	{
		wcstombs(buf,(unsigned short*)s.pOleStr,buflen);
		IMalloc *m;
		SHGetMalloc(&m);
		m->Free(s.pOleStr);
		m->Release();
		return;
	}
	if (s.uType==STRRET_OFFSET)
	{
		LPVOID P=&(idl->mkid.abID[s.uOffset-sizeof(idl->mkid.cb)]);
		strncpy(buf,PCHAR(P),idl->mkid.cb-s.uOffset);
		return;
	}
}

HICON ExtractIcon(ITEMIDLIST *idl,HWND owner,IShellFolder *Base,int imagesize,BOOLEAN s,BOOLEAN open)
{
	IExtractIcon *ei;
 	UINT uFlags;
	INT index;
	CHAR ifile[MAX_PATH];
	HICON hicon,hicon2;
    
	HRESULT r=Base->GetUIObjectOf(owner,1,(LPCITEMIDLIST*)&idl,IID_IExtractIcon,NULL,(LPVOID*)&ei);
	if (ei==NULL)return 0;
	ei->GetIconLocation(GIL_FORSHELL|(open?GIL_OPENICON:0), &ifile[0], MAX_PATH-1, &index, &uFlags);
	if (ei->Extract(&ifile[0],index,&hicon2,&hicon,MAKELONG(imagesize,imagesize))!=NOERROR)
	{
		ei->Release();
		return 0;
	}
	if (s)DestroyIcon(hicon2);else DestroyIcon(hicon);
	
	ei->Release();

	if (s) return hicon;else return hicon2;
}


void EXTRACTICONTHREADSTRUCT::DATA::Add(LPITEMIDLIST vidl,int vimage)
{
	if (next==NULL)
	{
		idl=vidl;
		image=vimage;
		next=new EXTRACTICONTHREADSTRUCT::DATA;
		next->next=NULL;
		next->idl=NULL;
	}else next->Add(vidl,vimage);
}

LPITEMIDLIST EXTRACTICONTHREADSTRUCT::DATA::GetIDL(int index)
{
	if (index==0)return idl; else return next->GetIDL(index-1);
}

int EXTRACTICONTHREADSTRUCT::DATA::GetImage(int index)
{
	if (index==0)return image; else return next->GetImage(index-1);
}

void EXTRACTICONTHREADSTRUCT::DATA::Free()
{
	if (next!=NULL)
	{
		next->Free();
		delete next;
	}
	if (idl!=NULL)DisposePIDL(idl);
}

DWORD WINAPI ExtractIconThread(LPVOID param)
{
	PEXTRACTICONTHREADSTRUCT s=(PEXTRACTICONTHREADSTRUCT)param;

	for (int i=0;i<s->count;i++)
	{
		HICON ic=ExtractIcon(s->Data.GetIDL(i),s->owner,s->Base,s->imagelistsize,TRUE,FALSE);
		if (ic==0)
		{
			CHAR temp[MAX_PATH];
			WORD nr;
			SHGetPathFromIDList(s->Data.GetIDL(i),&temp[0]);
			ic=ExtractAssociatedIcon(hInstance,&temp[0],&nr);
			if (ic==0)ic=LoadIcon(0,IDI_ERROR);
		}
		
		ImageList_ReplaceIcon(s->imagelist,s->Data.GetImage(i),ic);

		DestroyIcon(ic);
	}
	s->Data.Free();
	s->Base->Release();
	InvalidateRect(s->owner,NULL,FALSE);
	free(s);
	return 5000;
}

