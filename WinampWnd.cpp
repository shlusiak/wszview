#include "windows.h"
#include "wszview.h"
#include "stdio.h"


BYTE ReadValue(HANDLE f)
{
	CHAR s[1024];
	CHAR c='-',oc;
	ULONG read;

	do
	{
		oc=c;
		ReadFile(f,&c,1,&read,NULL);
		if (read<=0) return 0;
		if ((oc=='/')&&(c=='/'))
		{
			do 
			{
				ReadFile(f,&c,1,&read,NULL);
			}while (c!='\n');
			c='-';
		}
	}while (!((c>='0')&&(c<='9')));
	strcpy(&s[0],"");

	do
	{
		s[strlen(&s[0])+1]='\0';
		s[strlen(&s[0])]=c;
		ReadFile(f,&c,1,&read,NULL);
		if (read<=0)return 0;
	}while ((c>='0')&&(c<='9'));

	if (s[0]!=0)
	{
		return (BYTE)strtol(&s[0],NULL,10);
	}else return 0;
}

void ReadRGB(HANDLE f,COLORREF &c)
{
	BYTE r,g,b;
	r=ReadValue(f);
	g=ReadValue(f);
	b=ReadValue(f);
	c=RGB(r,g,b);
}

void DrawVis(HDC dc,HDC memdc,PCHAR path)
{
	COLORREF spec[16];
	const BYTE heights[]={0,2,6,0,4,8,9,7,16,6,14,15,9,4,3,4,2,3,0};
	int i;
	HANDLE f;

	f=CreateFile(strcat(path,"viscolor.txt"),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,0);
	if (f==0)return;

	COLORREF c;
	HBRUSH b;
	ReadRGB(f,c);
	{
		b=CreateSolidBrush(c);
		SelectObject(dc,GetStockObject(NULL_PEN));
		SelectObject(dc,b);
		Rectangle(dc,24,43,24+76,43+16);
		RestoreDC(dc,-1);
		DeleteObject(b);
	}

	ReadRGB(f,c);
	for (i=0;i<16;i++)ReadRGB(f,spec[i]);

	for (i=1;i<19;i++)
	{
		int j;
		HPEN pen,oldpen;
		for (j=1;j<=heights[i];j++)
		{
			pen=(HPEN)CreatePen(PS_SOLID,1,spec[16-j]);
			oldpen=(HPEN)SelectObject(dc,pen);
			MoveToEx(dc,24+(i-1)*4,59-j,NULL);
			LineTo(dc,24+(i-1)*4+3,59-j);
			SelectObject(dc,oldpen);
			DeleteObject(pen);
		}
	}
	
	CloseHandle(f);
}


#define Load(file) {\
	strcpy(&path[0],orgpath);\
	bitmap=(HBITMAP)LoadImage(0,strcat(path,file),IMAGE_BITMAP,0,0,LR_LOADFROMFILE); \
	if (bitmap!=0)SelectObject(memdc,bitmap);}

#define Free {\
	RestoreDC(memdc,-1); \
	DeleteObject(bitmap);}


void PaintMainWindow(HDC dc,PCHAR orgpath)
{
	HBITMAP bitmap;
	HDC memdc;
	CHAR path[MAX_PATH];
	strcpy(&path[0],orgpath);

//	Main.bmp
	bitmap=(HBITMAP)LoadImage(0,strcat(path,mainbmp),IMAGE_BITMAP,mainwidth,mainheight,LR_LOADFROMFILE);
	if (bitmap==0)
	{
		PatBlt(dc,0,0,mainwidth,mainheight,WHITENESS);
		return;
	}

	memdc=CreateCompatibleDC(dc);
	SelectObject(memdc,bitmap);
	BitBlt(dc,0,0,mainwidth,mainheight,memdc,0,0,SRCCOPY);
	Free;

//	Volume.bmp
	Load("volume.bmp");
	BitBlt(dc,107,57,68,13,memdc,0,330,SRCCOPY);
	BitBlt(dc,146,58,14,11,memdc,15,422,SRCCOPY);
	Free;

//	Balance.bmp
	Load("balance.bmp");
	BitBlt(dc,177,57,38,13,memdc,9,0,SRCCOPY);
	BitBlt(dc,189,58,14,11,memdc,15,422,SRCCOPY);
	Free;

//	CButtons.bmp
	Load("cbuttons.bmp");
	BitBlt(dc,16,88,114,18,memdc,0,0,SRCCOPY);
	BitBlt(dc,137,89,22,16,memdc,114,0,SRCCOPY);
	Free;

//	Shufrep.bmp
	Load("shufrep.bmp");
	BitBlt(dc,164,89,47,15,memdc,28,30,SRCCOPY);
	BitBlt(dc,211,89,28,15,memdc,0,30,SRCCOPY);
	BitBlt(dc,219,58,46,12,memdc,0,73,SRCCOPY);
	Free;

//	monoster.bmp
	Load("monoster.bmp");
	BitBlt(dc,239-29,41,29,12,memdc,29,12,SRCCOPY);
	BitBlt(dc,239,41,29,12,memdc,0,0,SRCCOPY);
	Free;

//	posbar.bmp
	Load("posbar.bmp");
	BitBlt(dc,16,72,248,10,memdc,0,0,SRCCOPY);
	BitBlt(dc,35,72,29,10,memdc,248,0,SRCCOPY);
	Free;

//	titlebar.bmp
	Load("titlebar.bmp");
	BitBlt(dc,0,0,275,14,memdc,27,0,SRCCOPY);
	BitBlt(dc,10,22,8,43,memdc,312,44,SRCCOPY);
	Free;

//	playpaus.bmp
	Load("playpaus.bmp");
	BitBlt(dc,24,28,3,9,memdc,36,0,SRCCOPY);
	BitBlt(dc,27,28,8,9,memdc,1,0,SRCCOPY);
	Free;

//	Numbers.bmp  
	Load("numbers.bmp");
	if (bitmap==0) Load("nums_ex.bmp");
	if (bitmap!=0)
	{
		BitBlt(dc,48,26,9,13,memdc,9*0,0,SRCCOPY);
		BitBlt(dc,60,26,9,13,memdc,9*0,0,SRCCOPY);
		BitBlt(dc,78,26,9,13,memdc,9*4,0,SRCCOPY);
		BitBlt(dc,90,26,9,13,memdc,9*6,0,SRCCOPY);
		Free;
	}

//	songtitle (text.bmp)  
	{
		HFONT f;
		COLORREF textcolor,bk;
		int i,j;
		CHAR c[100];
		HBRUSH b;

		Load("text.bmp");
		bk=GetPixel(memdc,50,15);
		textcolor=RGB(0,0,0);
		for (i=0;i<50;i++)for (j=0;j<50;j++)if (GetPixel(memdc,i,j)!=bk)
		{
			textcolor=GetPixel(memdc,i,j);
			goto fertig;
		}
fertig:
		Free;

		b=CreateSolidBrush(bk);
		SelectObject(dc,b);
		SelectObject(dc,GetStockObject(NULL_PEN));
		Rectangle(dc,111,24,111+154,24+12);

		strcpy(&path[0],orgpath);
		strcat(&path[0],"pledit.txt");
		GetPrivateProfileString(text,"Font","Arial",c,100,&path[0]);
		SetBkMode(dc,TRANSPARENT);
		SetTextColor(dc,textcolor);

		f=CreateFont(-10,0,0,0,0,0,0,0,ANSI_CHARSET,OUT_CHARACTER_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,c);
		SelectObject(dc,f);

		TextOut(dc,112,23,"2. Active (1:59)",16);

		RestoreDC(dc,-1);
		DeleteObject(b);
		DeleteObject(f);
	}

//	viscolor.txt
	strcpy(&path[0],orgpath);
	DrawVis(dc,memdc,&path[0]);

//	Clean up
	DeleteDC(memdc);
}


void DrawBar(HDC dc,HDC memdc,int x,int y,int value)
{
	int vx=((value-1)%14)*15+13,vy=164;
	if ((value>=15)&&(value<=28))vy=229;
	BitBlt(dc,x,y,14,63,memdc,vx,vy,SRCCOPY);
	BitBlt(dc,x+1,y+57-int((double(57/28)*double(value))),11,11,memdc,0,164,SRCCOPY);
}


void PaintEQWindow(HDC dc,PCHAR orgpath)
{
	CHAR path[MAX_PATH];
	HBITMAP bitmap;
	HDC memdc;

	strcpy(&path[0],orgpath);
	bitmap=(HBITMAP)LoadImage(0,strcat(&path[0],"eqmain.bmp"),IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
	if (bitmap==0)
	{
		PatBlt(dc,0,mainheight,eqwidth,mainheight+eqheight,WHITENESS);
		return;
	}
	memdc=CreateCompatibleDC(dc);
	SelectObject(memdc,bitmap);

//	Main
	BitBlt(dc,0,mainheight,eqwidth,eqheight,memdc,0,0,SRCCOPY);  
//	On
	BitBlt(dc,14,mainheight+18,25,12,memdc,69,119,SRCCOPY);      
//	Auto
	BitBlt(dc,39,mainheight+18,33,12,memdc,35,119,SRCCOPY);     
	
//	Presets
	BitBlt(dc,217,mainheight+18,44,12,memdc,224,164,SRCCOPY);    
//	General Bar (1)
	DrawBar(dc,memdc,21,mainheight+38,15);                                
//	Other bars (2-11)
	for (int i=1;i<=10;i++) DrawBar(dc,memdc,78+((i-1)*18),mainheight+38,(int(double(i)*2.8)));  
//	Field
	BitBlt(dc,86,mainheight+17,113,19,memdc,0,294,SRCCOPY);
//	Line
	BitBlt(dc,86,mainheight+17+9,113,1,memdc,0,314,SRCCOPY);
//	Bezier
	{
		BITMAP bobj;
		GetObject(bitmap,sizeof(bobj),&bobj);

		if (bobj.bmHeight>297)
		{
			HDC memdc2;
			HBITMAP bmp;
			COLORREF c;

			memdc2=CreateCompatibleDC(dc);
			bmp=(HBITMAP)CreateCompatibleBitmap(dc,119,19);
			SelectObject(memdc2,bmp);
			SelectObject(memdc2,GetStockObject(BLACK_PEN));
	
			PatBlt(memdc2,0,0,119,19,WHITENESS);

			MoveToEx(memdc2,0,17,NULL);
			LineTo(memdc2,108,0);

			for (int i=0;i<=107;i++)for (int j=0;j<=18;j++)
			{
				c=GetPixel(memdc2,i,j);
				if (c!=RGB(255,255,255))
					SetPixel(dc,86+i+2,mainheight+17+j,GetPixel(memdc,115,294+j));
			}
			RestoreDC(memdc,-1);
			DeleteDC(memdc2);
			DeleteObject(bmp);
		}
	}
	
	RestoreDC(memdc,-1);
	DeleteObject(bitmap);
	DeleteDC(memdc);
}

BYTE GetC(PCHAR s)
{
	CHAR c[3];
	strncpy(&c[0],s,2);
	return (BYTE)strtol(&c[0],NULL,16);
}

COLORREF MakeColor(PCHAR s2)
{
	CHAR s[10];
	strcpy(&s[0],s2);
	while (strlen(&s[0])>6)
	{
		strncpy(&s[0],&s[1],6);
		s[strlen(&s[0])-1]='\0';
	}

	return RGB(GetC(&s[0]),GetC(&s[2]),GetC(&s[4]));
}

void Text(HDC dc,int nr,int y,int ty,PCHAR title,PCHAR time)
{
	CHAR s[10];
	sprintf(&s[0],"%d.",nr);
	SIZE si;
	GetTextExtentPoint32(dc,&s[0],strlen(&s[0]),&si);
	TextOut(dc,13,y+ty,&s[0],strlen(&s[0]));
	TextOut(dc,16+si.cx,y+ty,title,strlen(title));
	GetTextExtentPoint32(dc,time,strlen(time),&si);
	TextOut(dc,250-si.cx,y+ty,time,strlen(time));
}

void PaintPLWindow(HDC dc,PCHAR orgpath)
{
	CHAR path[MAX_PATH];
	HBITMAP bitmap;
	HDC memdc;
	int i,y=mainheight+eqheight;
	CHAR a[MAX_PATH];
	HBRUSH b;
	COLORREF c1,c2,c3;
	HFONT f;

	strcpy(&path[0],orgpath);
	
	bitmap=(HBITMAP)LoadImage(0,strcat(path,"pledit.bmp"),IMAGE_BITMAP,0,0,LR_LOADFROMFILE);
	if (bitmap==0)
	{
		PatBlt(dc,0,mainheight+eqheight,plwidth,plheight,WHITENESS);
		return;
	}
	memdc=CreateCompatibleDC(dc);
	SelectObject(memdc,bitmap);

//	Titlebar
	BitBlt(dc,0,y,25,20,memdc,0,21,SRCCOPY);
	BitBlt(dc,250,y,25,20,memdc,153,21,SRCCOPY);
	for (i=1;i<=9;i++) if (((i<4)||(i>6)))BitBlt(dc,i*25,y,25,20,memdc,127,21,SRCCOPY);
	BitBlt(dc,87,y,100,20,memdc,26,21,SRCCOPY);

//	Bottom
	BitBlt(dc,0,y+plheight-38,125,38,memdc,0,72,SRCCOPY);
	BitBlt(dc,275-150,y+plheight-38,150,38,memdc,126,72,SRCCOPY);

//	Sides
	for (i=1;i<=2;i++)
	{
	    BitBlt(dc,0,y+20+(i-1)*29,25,29,memdc,0,42,SRCCOPY);
	    BitBlt(dc,250,y+20+(i-1)*29,25,29,memdc,26,42,SRCCOPY);
	}
	BitBlt(dc,260,y+20,8,18,memdc,52,53,SRCCOPY);

//	Middle
	strcpy(&path[0],orgpath);
	strcat(&path[0],pledittxt);
	GetPrivateProfileString(text,"NormalBG","#000000",a,MAX_PATH,&path[0]);
	if (strlen(&a[0])>6)strcpy(&a[0],&a[1]);

	b=CreateSolidBrush(MakeColor(&a[0]));
	SelectObject(dc,b);
	SelectObject(dc,GetStockObject(NULL_PEN));
	Rectangle(dc,12,y+20,plwidth-19,y+79);

	SelectObject(dc,GetStockObject(WHITE_BRUSH));
	DeleteObject(b);
  
//	Text in Playlist
	SetBkMode(dc,TRANSPARENT);
	GetPrivateProfileString(text,"Normal","#00D800",a,MAX_PATH,&path[0]);
	c1=MakeColor(&a[0]);
	GetPrivateProfileString(text,"Current","#FFFFFF",a,MAX_PATH,&path[0]);
	c2=MakeColor(&a[0]);
	GetPrivateProfileString(text,"SelectedBG","#0000D8",a,MAX_PATH,&path[0]);
	c3=MakeColor(&a[0]);

	GetPrivateProfileString(text,"Font","Arial",a,MAX_PATH,&path[0]);
	
	SetTextColor(dc,c1);

	f=CreateFont(-10,0,0,0,0,0,0,0,ANSI_CHARSET,OUT_CHARACTER_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,a);
	SelectObject(dc,f);

	Text(dc,1,y,22,"Normal","3:14");
	SetTextColor(dc,c2);
	Text(dc,2,y,35,"Active","1:59");
	SetTextColor(dc,c1);

	b=CreateSolidBrush(c3);
	SelectObject(dc,b);
	Rectangle(dc,12,y+48,256,y+62);
	SelectObject(dc,GetStockObject(WHITE_BRUSH));
	DeleteObject(b);

	Text(dc,3,y,48,"Selected","2:65");
	Text(dc,4,y,61,"Normal","3:58");

	RestoreDC(dc,-1);
	DeleteObject(b);
	DeleteObject(f);

	RestoreDC(memdc,-1);
	DeleteDC(memdc);
	DeleteObject(bitmap);
}


