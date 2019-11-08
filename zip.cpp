// zip.cpp : Definiert den Einsprungpunkt für die Konsolenanwendung.
//

#include "windows.h"
#include "stdio.h"
#include "unzip.h"

const WRITEBUFFERSIZE=8192;


BOOLEAN ExtractCurrentFile(unzFile uf)
{
	char filename_inzip[256];
	char* filename_withoutpath;
	char* p;
    int err=UNZ_OK;
    FILE *fout=NULL;
    void* buf;
    uInt size_buf;
	
	unz_file_info file_info;
	uLong ratio=0;
	err = unzGetCurrentFileInfo(uf,&file_info,filename_inzip,sizeof(filename_inzip),NULL,0,NULL,0);

	if (err!=UNZ_OK)
	{
		return err;
	}

    size_buf = WRITEBUFFERSIZE;
    buf = (void*)malloc(size_buf);
    if (buf==NULL)
    {
        return UNZ_INTERNALERROR;
    }

	p = filename_withoutpath = filename_inzip;
	while ((*p) != '\0')
	{
		if (((*p)=='/') || ((*p)=='\\'))
			filename_withoutpath = p+1;
		p++;
	}

	if ((*filename_withoutpath)=='\0')
	{
	}
	else
	{
		const char* write_filename;
		int skip=0;

		write_filename = filename_withoutpath;

		err = unzOpenCurrentFile(uf);
		if (err!=UNZ_OK)
		{
		}

		if ((skip==0) && (err==UNZ_OK))
		{
			fout=fopen(write_filename,"wb");

            /* some zipfile don't contain directory alone before file */
			if (fout==NULL)
			{
			}
		}

		if (fout!=NULL)
		{

			do
			{
				err = unzReadCurrentFile(uf,buf,size_buf);
				if (err<0)	
				{
					break;
				}
				if (err>0)
					if (fwrite(buf,err,1,fout)!=1)
					{
                        err=UNZ_ERRNO;
						break;
					}
			}
			while (err>0);
			fclose(fout);
		}

        if (err==UNZ_OK)
        {
		    err = unzCloseCurrentFile (uf);
		    if (err!=UNZ_OK)
		    {
		    }
        }
        else
            unzCloseCurrentFile(uf); /* don't lose the error */       
	}

    free(buf);    
    return err;

}

BOOLEAN UnZip(PCHAR file)
{
	unzFile f;
	f=unzOpen(file);

	uLong i;
	unz_global_info gi;
	int err;
	FILE* fout=NULL;	

	err = unzGetGlobalInfo (f,&gi);
	if (err!=UNZ_OK)return FALSE;

	for (i=0;i<gi.number_entry;i++)
	{
        if (ExtractCurrentFile(f)!=UNZ_OK)
		{
			unzClose(f);
			return FALSE;
		}

		if ((i+1)<gi.number_entry)
		{
			err = unzGoToNextFile(f);
			if (err!=UNZ_OK) 
			{
				unzClose(f);
				return FALSE;
			}
		}
	}

	unzClose(f);
	return TRUE;
}

