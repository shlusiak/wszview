typedef struct
{
	HWND wnd;			// Owner window of the skin
	CHAR* OriginalPath;	// Path to the directory or the zip-file
	CHAR* Path;			// Path to the bitmap files of the skin (temp)
	BOOLEAN zipped;		// If zipped, OriginalPath contains the original zip-file
	HBITMAP bitmap;		// The bitmap containing the whole skin

	BOOLEAN UnzipSkin();
	void DeleteTempPath();
	BOOLEAN SetSkin();
	void RefreshBitmap();
	void ReloadSkin();
	BOOLEAN SelectSkin();
	BOOLEAN ParseSkinPath(PCHAR p);
}*PSKIN,SKIN;

BOOLEAN ShowWinamp();
int SetWinampSkin(PCHAR path);
