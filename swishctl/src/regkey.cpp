/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RegKey Starts
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
#include "stdafx.h"
#include <string>
using namespace std;

#include "regkey.h"

RegKey::RegKey(HKEY startkey)
{
	hkey=NULL; 
	this->startkey = startkey;
}

bool RegKey::CreateKey(const char *keyname)
{
	if ( ! keyname ) return false;
	if (hkey) RegCloseKey(hkey);

	LONG res = RegCreateKeyEx(startkey,  keyname, 
			0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, 0, &hkey, 0 ) ;

	return res == ERROR_SUCCESS;
}

bool RegKey::OpenKey(const char *keyname, REGSAM samdesired )
{
	if ( ! keyname ) return false;
	if (hkey) RegCloseKey(hkey);
#if 0
	LONG res = RegOpenKeyEx (startkey, 
							(const char *)keyname, 
							0, 
							samdesired, 
							&hkey);
#endif
	DWORD disposition;
	LONG res = RegCreateKeyEx(
	  startkey,                // handle to an open key
	  (const char *)keyname,         // address of subkey name
	  0,           // reserved
	  NULL,           // address of class string
	  REG_OPTION_NON_VOLATILE,          // special options flag
	  samdesired,        // desired security access
	  NULL,
								// address of key security structure
	  &hkey,          // address of buffer for opened handle
	  &disposition   // address of disposition value buffer
	);
 
	return res == ERROR_SUCCESS;
}

bool RegKey::OpenKey(const char *keyname)
{
	return OpenKey(keyname, KEY_ALL_ACCESS );
}

bool RegKey::SetValue(const char *valuename, const char *value)
{
	if ( ! valuename || ! value ) return false;
	LONG res = RegSetValueEx( hkey, valuename, 0, REG_SZ,
							  (const unsigned char *)value, lstrlen(value)+1);
	return res == ERROR_SUCCESS;
}


string RegKey::QueryValue(const char *valuename)
{
	DWORD dwType = REG_SZ;

	if (! hkey ) return NULL; // can I do this ?

	LPBYTE lpData = NULL;
	DWORD cbData = 0;

	LONG res = RegQueryValueEx(	hkey, valuename, 0, &dwType, NULL, &cbData );
	
//	if ( res != ERROR_MORE_DATA ) { // this should be true but isn't
//		::MessageBox( NULL, "ERROR_MORE_DATA should have been seen...", 
//			"Whatever", MB_OK );
//		return NULL;	
//	} 

	lpData = (LPBYTE) malloc( cbData );

	if ( !lpData ) {
		::MessageBox( NULL, "Out of memory error occured in QueryValue function.", 
			"SwishCtl Error", MB_OK | MB_ICONINFORMATION);

		return *(new string());
	}

	res = RegQueryValueEx(	hkey, valuename, 0, &dwType, lpData, &cbData );
	
	if (res != ERROR_SUCCESS) {
		free( lpData );
		return *(new string());
	}

	string *temp = new string( (char *)lpData ) ;
	free( lpData );

	return *temp;
}

RegKey::~RegKey()
{
	if ( hkey )	::RegCloseKey(hkey);
}

bool RegKey::DeleteKey(const char *subkey)
{
	return ::RegDeleteKey(hkey, subkey ) == ERROR_SUCCESS;
}

/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RegKey ends
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
