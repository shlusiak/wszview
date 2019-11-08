HINSTANCE extern hInstance;
LPITEMIDLIST NextPIDL(LPITEMIDLIST idl);
LPITEMIDLIST CreatePIDL(int size);
int GetPIDLSize(LPITEMIDLIST idl);
LPITEMIDLIST CopyPIDL(LPITEMIDLIST id);
void StripLastID(LPITEMIDLIST idl);
void DisposePIDL(LPITEMIDLIST idl);
LPITEMIDLIST ConcatPIDLs(LPITEMIDLIST idl1,LPITEMIDLIST idl2);
void StrRetToBuf(STRRET s,LPITEMIDLIST idl,PCHAR buf,int buflen);


typedef struct IDLLIST
{
	LPITEMIDLIST idl;
	IDLLIST *Next;
	int count;

	IDLLIST();
	~IDLLIST();
	void Add(LPITEMIDLIST id);
	LPITEMIDLIST Get(int index);
}*PIDLLIST;

PIDLLIST CreatePIDLList(LPITEMIDLIST id);


typedef struct TSHELLFOLDER
{
	IShellFolder *sf;
	TSHELLFOLDER *Parent;
	LPITEMIDLIST absoluteidl,relativeidl;

	void Free();
}*PSHELLFOLDER;

typedef struct TLISTITEM
{
	PSHELLFOLDER Parent;
	LPITEMIDLIST idl;
	BOOLEAN folder,skin;

	void Free();
}*PLISTITEM;

PSHELLFOLDER CreateShellFolder(IShellFolder *vsf,PSHELLFOLDER vParent,LPITEMIDLIST relid);
PLISTITEM CreateListItem(PSHELLFOLDER Parent,LPITEMIDLIST idl,BOOLEAN folder,BOOLEAN skin=FALSE);

typedef struct EXTRACTICONTHREADSTRUCT
{
	HWND owner;
	HIMAGELIST imagelist;
	int imagelistsize;
	IShellFolder *Base;
	struct DATA
	{
		LPITEMIDLIST idl;
		int image;
		DATA *next;
		void Add(LPITEMIDLIST vidl,int vimage);
		void Free();
		LPITEMIDLIST GetIDL(int index);
		int GetImage(int index);
	}Data;
	int count;
}*PEXTRACTICONTHREADSTRUCT;

HICON ExtractIcon(ITEMIDLIST *idl,HWND owner,IShellFolder *Base,int imagesize,BOOLEAN s,BOOLEAN open);
DWORD WINAPI ExtractIconThread(LPVOID param);

