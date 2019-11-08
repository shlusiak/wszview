HINSTANCE extern hInstance;
PCHAR extern programpath;

HWND CreateMainWindow(PCHAR path);
void PaintMainWindow(HDC dc,PCHAR path);
void PaintEQWindow(HDC,PCHAR);
void PaintPLWindow(HDC,PCHAR);
BOOLEAN UnZip(PCHAR);
BOOL CALLBACK MyDlgProc(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
BOOL CALLBACK DlgProcUserDefined(HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam);
void DeleteTempPath(HWND wnd,PCHAR p);

void Warte();

const mainwidth=275;
const mainheight=116;
const eqwidth=275;
const eqheight=116;
const eqpicturewidth=275;
const eqpictureheight=315;
const plwidth=275;
const plheight=116;
const plpicturewidth=280;
const plpictureheight=186;
const mbwidth=275;
const mbheight=145;
const mbpicturewidth=242;
const mbpictureheight=119;
const avswidth=275;
const avsheight=200;
const picturewidth=mainwidth;
const pictureheight=mainheight+eqheight+plheight;
const PCHAR pledittxt="pledit.txt";
const PCHAR mainbmp="main.bmp";
const PCHAR text="Text";
const PCHAR WszView="WszView";


typedef struct PCHARREC
{
	HWND w;
	PCHAR temp;
}*PPCHARREC;

void ASSERT(BOOLEAN expr,PCHAR text);
int MessageBox(HWND hWnd,int text,int caption,UINT uType);