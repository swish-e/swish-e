/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * class FilePath 
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
class FilePath {
public:
	FilePath& append(const char *extra);

	unsigned int getSize() {return m_size;};
	char * c_str() {return m_str;};
	
	bool Reallocate( unsigned int NewSize );
	FilePath * copy();
	FilePath &operator+( FilePath &extrapath );
	FilePath &operator+( const char *extrapath );


	FilePath(const char *path);
	FilePath(const char *path, unsigned int storage);
	FilePath();

	~FilePath();
	
private:
	char * m_str;
	unsigned int m_size;
	void Initialise(const char *path, unsigned int storage);
};

void FilePath::Initialise(const char *path, unsigned int storage)
{
	m_str = NULL;
	m_size = 0;

	if (storage) {
		bool result = Reallocate (storage);
		
		if (result && path) {
			strcpy (m_str, path);
		}
	}
}

FilePath::FilePath()
{
	Initialise (NULL, 0);
}


FilePath::FilePath(const char *path, unsigned int storage)
{
	Initialise( path, storage );
}

FilePath::FilePath(const char *path)
{
	if ( ! path ) {
		Initialise( NULL, 0 );
	} else {
		Initialise( path, strlen(path) + 1 );
	}
}


FilePath& FilePath::append(const char *extra)
{
	if ( m_str ) {

		FilePath *temp = new FilePath();

		if ( !extra || !temp || (*extra == '\0')) return  *temp; 

		int thislen = strlen( m_str );
		int extralen = strlen( extra );
		int totallen = thislen + extralen;
		bool appendbackslash = false;

		if ( m_str[thislen-1] != '\\' && m_str[thislen-1] != '/' ) {
			appendbackslash = true;
			if (!temp->Reallocate( thislen + extralen + 2 ) ) {
				return *temp;
			}
			strcpy( temp->c_str(), this->m_str );
			strcat(  temp->c_str(), "\\" );
			strcat (temp->c_str(), extra); 
			
		} else {
			temp->Reallocate( thislen + extralen + 1 );
			strcpy( temp->c_str(), this->m_str );
			strcat (temp->c_str(), extra); 
		}
		return *temp;		
	} else {                 // this.m_str is null 
		// return a copy of the extra path...
		return *(new FilePath(extra));
	}
}


FilePath &FilePath::operator+( const char * extra )
{
	return append(extra);
}



FilePath &FilePath::operator+( FilePath &extrapath )
{
	char *extra = extrapath.c_str();
	return *this + extra;
}


FilePath::~FilePath()
{
	delete[] m_str;
}

bool FilePath::Reallocate(unsigned int NewSize)
{
	// 
	m_size = 0;
	delete[] m_str;
	m_str = new char[NewSize];

	if (! m_str ) return false;
	m_str[0] = '\0';
	m_size = NewSize;
	return true;
}


FilePath * FilePath::copy()
{
	FilePath *temp = new FilePath();
	if ( temp->Reallocate( this->getSize() ) ) {
		strcpy( temp->c_str(), this->c_str() );
	}
	return temp;
}

