// paths.h: interface for the CDirectory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PATHS_H__AE9DC3AE_4C19_4DEB_9074_053EF38B7400__INCLUDED_)
#define AFX_PATHS_H__AE9DC3AE_4C19_4DEB_9074_053EF38B7400__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



class CBuffer  
{
public:
	virtual ~CBuffer ();
	CBuffer (unsigned int maxsize);
	CBuffer (unsigned int maxsize, CBuffer &source);
	CBuffer (unsigned int maxsize, char *initialvalue);
	void operator= (CBuffer &source);
	void operator= (char *initialvalue);

	virtual unsigned int getSize() {return buffersize;};
	virtual char * getBuffer(){return buffer;};
protected:
	virtual void init( char *initialvalue, unsigned int initialsize );
	char * buffer;
	unsigned int buffersize;
	unsigned int buffmax;
};

class CPath ;
class CFilename ;
class CDirectory;

class CPath : public CBuffer
{
public:
	CDirectory& getDirectory();
	CFilename& getFilename();
	CPath () : CBuffer( _MAX_PATH ) {};
	CPath (CPath &source) : CBuffer( _MAX_PATH, source ) {};
	CPath (char *pathname) : CBuffer( _MAX_PATH, pathname ) {};
	void operator= (CPath &source);
	void operator= (char *pathname);

	virtual unsigned int getSize() {return buffersize;};
	virtual char * getBuffer(){return buffer;};
};

class CFilename : public CBuffer 
{
public:
	CFilename () : CBuffer( _MAX_FNAME ) {};
	CFilename (CFilename &source) : CBuffer( _MAX_FNAME, source ) {};
	CFilename (char *filename) : CBuffer( _MAX_FNAME, filename ) {};
	void operator= (CFilename &source);
	void operator= (char *filename);

	virtual unsigned int getSize() {return buffersize;};
	virtual char * getBuffer(){return buffer;};
};

class CDirectory  : public CBuffer
{
public:
	CPath& Append( char * extra);
	CPath& operator+ (CFilename& filename);
	CPath& operator+ (char *extra);
	CDirectory () : CBuffer( _MAX_DIR ) {};
	CDirectory (CDirectory &source) : CBuffer( _MAX_DIR, source ) {};
	CDirectory (char *dirname) : CBuffer( _MAX_DIR, dirname ) {};
	CDirectory& operator= (CDirectory &source);
	CDirectory& operator= (char *dirname);

	virtual unsigned int getSize() {return buffersize;};
	virtual char * getBuffer(){return buffer;};
};


void TestPathsModule(HINSTANCE apphandle);


class CAppDirectory : public CDirectory  
{
public:
	CAppDirectory( HINSTANCE apphandle);
protected:
	CAppDirectory() {}; // prevent use of no parameter constructor
private:
};

class CWinDirectory : public CDirectory  
{
public:
	CWinDirectory();
};

class CStartProgramsDirectory : public CDirectory  
{
public:
	CDirectory & operator+( char * childname );
	bool DeleteChild( char * childname);
	CDirectory &CreateChild( char * childname );
	void NotifyChanged();
	CStartProgramsDirectory();
};


#endif // !defined(AFX_PATHS_H__AE9DC3AE_4C19_4DEB_9074_053EF38B7400__INCLUDED_)
