// paths.cpp: implementation of the CDirectory class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "paths.h"
#include <string>

using namespace std;

void CBuffer::init(char *initialvalue, unsigned int initialsize)
{
	delete[] buffer; // 
	buffersize = initialsize;
	if ( !buffersize ) {
		buffer = NULL;
		return;
	}
	
	if ( !initialvalue ) {
		buffer = new char[buffersize];
		buffer[0] = '\0';
	} else {
		unsigned int namelength = strlen( initialvalue );
		// use a larger buffer if necessary
		if ( namelength > buffersize ) {
			buffersize = namelength + 1;
		}
		buffer = new char[buffersize];
		strcpy( buffer, initialvalue );
	}
}

CBuffer::CBuffer(unsigned int maxsize) 
{
	buffer = NULL;
	buffmax = maxsize;
	init( NULL, buffmax );
}

CBuffer::CBuffer(unsigned int maxsize, CBuffer &source) 
{
	buffmax = maxsize;
	buffer = NULL;
	init( source.getBuffer(), source.getSize());
}

CBuffer::CBuffer (unsigned int maxsize, char *initialvalue) 
{
	buffmax = maxsize;
	buffer = NULL;
	init( initialvalue, buffmax);
}

void CBuffer::operator =(CBuffer &source)
{
	init( source.getBuffer(), source.getSize());
}

void CBuffer::operator =(char *initialvalue)
{
	init( initialvalue, buffmax);
}

CBuffer::~CBuffer()
{
	delete[] buffer;
}


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDirectory& CDirectory::operator =(CDirectory &source)
{
	// should I delete "this" first?
	if (this == &source) return *this;	
	return *(new CDirectory(source));
}

CDirectory& CDirectory::operator =(char *dirname)
{
	// should I delete "this" first?
	return *(new CDirectory(dirname));
}

CPath& CDirectory::Append(char *extra)
{
	if ( buffer ) {
		CPath *temp = new CPath(buffer);

		int thislen = strlen (buffer);
		char finalchar = buffer[thislen-1];

		if ( finalchar != '\\' &&  finalchar != '/' ) {
			strcat( temp->getBuffer(), "\\");
		}
		
		strcat(  temp->getBuffer(), extra);
		return *temp;		
	} else {                 // this.m_str is null 
		// return a copy of the filename...
		return *(new CPath(extra));
	}

}

CPath& CDirectory::operator +(char *extra)
{
	return Append(extra);
}


CPath& CDirectory::operator +(CFilename &filename)
{
	return Append( filename.getBuffer());
}

void CPath::operator =(CPath &source)
{
	init( source.getBuffer(), source.getSize());
}

void CPath::operator =(char *pathname)
{
	init( pathname, _MAX_PATH);
}



CFilename& CPath::getFilename()
{
	static char fname[_MAX_FNAME];
	static char extname[_MAX_EXT];
	CFilename *temp = new CFilename();

	// split the FileNameAndExtension parameter
   _splitpath (buffer, NULL, NULL, fname, extname);
	// assemble full path
   _makepath (temp->getBuffer(), NULL, NULL, fname, extname);

   return *temp;
}


CDirectory& CPath::getDirectory() 
{
	static char drvname[_MAX_FNAME];
	static char dirname[_MAX_EXT];
	CDirectory *temp = new CDirectory();

	// split the FileNameAndExtension parameter
   _splitpath (buffer, drvname, dirname, NULL, NULL);
	// assemble full path
   _makepath (temp->getBuffer(), drvname, dirname, NULL, NULL);

   return *temp;
}



//////////////////////////////////////////////////////////////////////
// CFilename Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CFilename::operator =(CFilename &source)
{
	init( source.getBuffer(), source.getSize());
}

void CFilename::operator =(char *filename)
{
	init( filename, buffmax);
}

//////////////////////////////////////////////////////////////////////
// CAppDirectory Class
//////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAppDirectory::CAppDirectory(HINSTANCE apphandle) 
{
	CPath temppath;

	DWORD copied = GetModuleFileName( apphandle,
		temppath.getBuffer(), temppath.getSize());
	CDirectory tempdir = temppath.getDirectory();
	strcpy(getBuffer(), tempdir.getBuffer());
	
}

CWinDirectory::CWinDirectory() 
{
	UINT buffsize = GetWindowsDirectory (getBuffer(), 0);
	if ( buffsize > getSize() ) {
		init( NULL, buffsize );
	} 
	GetWindowsDirectory (getBuffer(), buffsize); // result should be < buffsize
}

CStartProgramsDirectory::CStartProgramsDirectory() 
{
	LPITEMIDLIST   pidlPrograms;

	//get the pidl for Start|Programs
	SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidlPrograms);

	BOOL result = SHGetPathFromIDList( pidlPrograms, getBuffer() ); 

}

void CStartProgramsDirectory::NotifyChanged()
{
	SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, getBuffer(), 0);
}

CDirectory & CStartProgramsDirectory::operator+(char *childname)
{
	CDirectory *fullpath = new CDirectory(*this);
	
	unsigned int pathlength = strlen(fullpath->getBuffer());

	char finalchar = (fullpath->getBuffer())[pathlength-1];

	if ( finalchar != '\\' &&  finalchar != '/' ) {
		strcat( fullpath->getBuffer(), "\\");
	}
	strcat( fullpath->getBuffer(), childname);	

	return *fullpath;
}

CDirectory &CStartProgramsDirectory::CreateChild(char *childname)
{
	CDirectory *fullpath = new CDirectory(*this);
	
	unsigned int pathlength = strlen(fullpath->getBuffer());

	char finalchar = (fullpath->getBuffer())[pathlength-1];

	if ( finalchar != '\\' &&  finalchar != '/' ) {
		strcat( fullpath->getBuffer(), "\\");
	}
	strcat( fullpath->getBuffer(), childname);	

	if (! CreateDirectory(fullpath->getBuffer(), NULL) ) {
		// what todo: ???
	}

	return *fullpath;
}

bool CStartProgramsDirectory::DeleteChild(char *childname)
{
	CDirectory fullpath = *this + childname;
	return RemoveDirectory(fullpath.getBuffer()) != 0;
}


void TestPathsModule(HINSTANCE apphandle)
{
	CDirectory dir1( "C:\\Program Files\\" );
	CDirectory dircopy( dir1 );
	CFilename fn("yaah.txt" );
	CPath &pathname = dir1 + fn;
	CAppDirectory app(apphandle);

	CWinDirectory dir2;
	CPath path3 = dir2 + fn;
	CFilename fn2 = pathname.getFilename();

	try {
		char *dir1buff = dir1.getBuffer();
		unsigned int dir1size = dir1.getSize();


		unsigned int dir2size = dircopy.getSize(); // should throw an exception
		char *dir2buff = dircopy.getBuffer();

	} catch (...) {
		int i = 1;
	}
}


