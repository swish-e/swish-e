// setup.cpp : Defines the entry point for the application.
//
#include "stdafx.h"
#include "resource.h"
#include <string>

#include "paths.h" 

using namespace std;

#include "../regkey.h"

// One day I'll get around to doing the 
// ugly GetFileVersion code...
#define SWISHCTL_VERSION ("1007")

const char *AUTORUN = "AutoRun";

// Global Variables:
enum dllnames { SWISHCTL=0, PCRE=1, ZLIB=2 };


// Names of files to be installed in System directory
char * installdlls[] = {
		"SwishCtl.dll", "pcre.dll", "zlib.dll" };

const char registrykey[] = "Software\\Kauranga\\ACCIndex\\1.0\\Options";

static const char uninstallkey[] = 
	"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" ;


// Foward declarations of functions included in this code module:
LRESULT CALLBACK SetupHandler(HWND, UINT, WPARAM, LPARAM);


/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * class SetupApp manages application instance data 
 * and Ole initialisation
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
class SetupApp {
public:
	static HINSTANCE getInstance() {return hInst;};
	~SetupApp();
	SetupApp(HINSTANCE hInstance);
	static HINSTANCE hInst;			// current instance

};

SetupApp::SetupApp(HINSTANCE hInstance)
{
	hInst = hInstance; // Store instance handle in our global variable
	OleInitialize(NULL);

}

SetupApp::~SetupApp()
{
	OleUninitialize();	
}

HINSTANCE SetupApp::hInst = 0; // static instance variable

/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * end of Setup App class
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */


/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * LibModule class 
 * encapsulates instance loaded with LoadLibrary
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
class LibModule {
public:
	LibModule() {m_hLib = NULL;};
	~LibModule();
	bool LoadLibrary( const char *pathname );
	HINSTANCE getInstance(){ return m_hLib;};

private:
	HINSTANCE m_hLib;
};

LibModule::~LibModule()
{
	if (m_hLib) ::FreeLibrary(m_hLib); //ensure FreeLibrary gets called
}

bool LibModule::LoadLibrary( const char *pathname )
{
	m_hLib = ::LoadLibrary(pathname);
	return m_hLib != NULL;
}
/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * end of LibModule class
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */


typedef enum InstallResult { INSTALL_OK, ERR_INUSE, ERR_FAIL, ERR_ONREBOOT };


// Add a new Folder (FolderName) to Start | Programs
// appends the Programs folder path to give FullPathToFolder
// Make sure FullPathToFolder is big enough to hold the path
bool AddStartProgramsFolder(char *FolderName, char *FullPathToFolder)
{
	LPITEMIDLIST   pidlPrograms;

	//get the pidl for Start|Programs
	SHGetSpecialFolderLocation(NULL, CSIDL_PROGRAMS, &pidlPrograms);

	if ( !SHGetPathFromIDList( pidlPrograms, FullPathToFolder) ) 
	{
		return false;
	}

	strcat( FullPathToFolder, FolderName);
	//create the folder
	CreateDirectory(FullPathToFolder, NULL);
	SHChangeNotify(SHCNE_MKDIR, SHCNF_PATH, FullPathToFolder, 0);

	return true;
}


HRESULT CreateShortcut(  LPCSTR Source, 
                     LPSTR Target, 
                     LPSTR Description) 
{ 
	HRESULT hresult; 
	IShellLink* ShellLink; 

	//CoInitialize must be called before this
	// Get a pointer to the IShellLink interface. 
	hresult = CoCreateInstance(   
		CLSID_ShellLink, 
		NULL, 
		CLSCTX_INPROC_SERVER, 
		IID_IShellLink, 
		(LPVOID*)&ShellLink); 
	if (SUCCEEDED(hresult)) 
	{ 
		IPersistFile* PersistFile; 

		// Set the path to the shortcut target, and add the description. 
		ShellLink->SetPath(Source); 
		ShellLink->SetDescription(Description); 

		// Query IShellLink for the IPersistFile interface for saving the 
		// shortcut in persistent storage. 
		hresult = ShellLink->QueryInterface(IID_IPersistFile, (LPVOID*)&PersistFile); 

		if (SUCCEEDED(hresult)) 
		{ 
			WCHAR WideTarget[MAX_PATH]; 

			// Ensure that the string is ANSI. 
			MultiByteToWideChar( CP_ACP, 
							   0, 
							   Target, 
							   -1, 
							   WideTarget, 
							   MAX_PATH); 

			// Save the link by calling IPersistFile::Save. 
			hresult = PersistFile->Save(WideTarget, TRUE); 


			PersistFile->Release(); 
		  } 

		ShellLink->Release(); 
	} 

	return hresult; 
}

// Remove the ACCIndex key from the registry
// should remove the Kauranga key if it is empty
bool UnregisterIndexFile()
{
	RegKey key(HKEY_LOCAL_MACHINE);

	if ( ! key.OpenKey( "Software\\Kauranga"  ) ) return false;
	return key.DeleteKey( "ACCIndex" );
}

// Create the ACCIndex registry key and add the 
// IndexLocation, IndexFiles and DLLVersion keys 
bool RegisterIndexFile(char *indexdir, char *indexname)
{
	RegKey key(HKEY_LOCAL_MACHINE);
	string keyname(registrykey);
	if ( ! key.CreateKey( keyname.c_str() ) ) return false;
	
	bool kv = key.SetValue( "IndexLocation", indexdir );
	kv &= key.SetValue( "IndexFiles", indexname );

	kv &= key.SetValue( "DLLVersion", SWISHCTL_VERSION );

	return kv;
}

// Remove reference to ACCIndex from the Uninstall section 
// of the registry 
bool UnregisterUninstall()
{
	RegKey key(HKEY_LOCAL_MACHINE);

	if ( ! key.OpenKey( uninstallkey ) ) return false;
	return key.DeleteKey( "ACCIndex" );
}

// Register Setup.exe /u in the registry Uninstall section 
bool RegisterUninstall(const char *uninstallcommand)
{

	string keyname(uninstallkey);
	keyname += "ACCIndex";

	RegKey key(HKEY_LOCAL_MACHINE);
	if ( ! key.CreateKey( keyname.c_str() ) ) return false;
	
	bool result = key.SetValue( "DisplayName", "ACC Index Search Tool" );
	result &= key.SetValue( "UninstallString", uninstallcommand );

	return result;
}

BOOL LaunchUrl( HWND hwnd, char *url )
{
   if (ShellExecute(
		hwnd, 
		"open", 
		url,
		NULL, NULL, SW_SHOWNORMAL) < (HINSTANCE)32 ) 
   {
		char *buff[256] ;
		sprintf( (char *) buff, 
			"Setup could not launch the search page (%s) \n"
			"please try opening the file from within Internet Explorer.",
			url
		);
		MessageBox(
				NULL, // should pass in the dialog box handle
				(char *) buff,
				"Swish Control Setup",
				MB_OK | MB_ICONASTERISK );
		return FALSE;
   }
   return TRUE;
}

// remove the shortcuts in the start menu
bool DeleteShortcuts(char *shortcut, char *subdir) 
{
	CStartProgramsDirectory startprograms;
	
	CDirectory newdir = startprograms + subdir;

	CPath Shortcut = newdir + shortcut; 

	bool result = DeleteFile( Shortcut.getBuffer()) != 0;
	
	RemoveDirectory( newdir.getBuffer());
	return result;

}

// add a shortcut to the search page in the startmenu
bool AddShortcut(CDirectory &appdir)
{
	CStartProgramsDirectory startprograms;
	
	CDirectory newdir = startprograms.CreateChild( "ACC Index" );

	CPath Shortcut = newdir + "index.lnk"; 
	CPath IndexPage = appdir + "index.htm";


	CreateShortcut( IndexPage.getBuffer(), Shortcut.getBuffer(), 
			"ACC Index database" );
	startprograms.NotifyChanged();
	return true;
}


typedef HRESULT (STDAPICALLTYPE *CTLREGPROC)() ; // Requires stdole.h

// 
bool registerOleDll( const char * pathname )
{
	LibModule dll;
	if ( ! dll.LoadLibrary(pathname) ) return false;

	CTLREGPROC lpDllEntryPoint;
	lpDllEntryPoint = (CTLREGPROC) GetProcAddress (dll.getInstance(), 
												_T("DllRegisterServer"));

	if (!lpDllEntryPoint) return false; 	//unable to locate entry point

	HRESULT res = (*lpDllEntryPoint)(); 
	return res == NOERROR;
}

// 
bool unregisterOleDll( const char * pathname )
{
	LibModule dll;
	if ( ! dll.LoadLibrary(pathname) ) return false;

	CTLREGPROC lpDllEntryPoint;
	lpDllEntryPoint = (CTLREGPROC)GetProcAddress (	dll.getInstance(), 
												  _T("DllUnregisterServer"));
	if (!lpDllEntryPoint) return false;

	HRESULT res = (*lpDllEntryPoint)(); //unable to locate entry point
	return res == NOERROR;
}

// Check whether the Class ID is present
bool AppIsRegistered() 
{
	CLSID clsid;
    return CLSIDFromProgID(L"SwishCtl.CSwishCtl", &clsid) == S_OK;    
}

BOOL ShowInstallDialog()
{
	return DialogBox (SetupApp::getInstance	(), 
						(LPCTSTR) IDD_SETUP, 
						NULL, 
						(DLGPROC) SetupHandler);
}

void SetAutorun( bool on )
{
		RegKey key(HKEY_CURRENT_USER);
		key.OpenKey( registrykey, KEY_ALL_ACCESS );
		if (on) {
			key.SetValue( AUTORUN, "true" );
		} else {
			key.SetValue( AUTORUN, "false" );
		}
}

typedef enum VERSION_AGE { NO_VER, OLDER_VER, CURRENT_VER, NEWER_VER };

VERSION_AGE CheckInstalledVersion()
{
	RegKey hkcu(HKEY_LOCAL_MACHINE) ;
	if (!hkcu.OpenKey( registrykey, KEY_READ )) {
		return NO_VER;
	}
	string verstring = hkcu.QueryValue( "DLLVersion" );

	int installed = atoi( verstring.c_str() );
	int current = atoi( SWISHCTL_VERSION );
	if ( !installed ) {
		return NO_VER;
	} else if (installed < current) {
		return OLDER_VER;
	} else if (installed == current) {
		return CURRENT_VER;
	} else {
		return NEWER_VER;
	}
}

bool AllowAutorun()
{
	// look up autorun key for this user.
	RegKey hkcu(HKEY_CURRENT_USER) ;
	if (hkcu.OpenKey( registrykey, KEY_READ )) {
		string autorun = hkcu.QueryValue( AUTORUN );
		if ( autorun != "true") {
			return false;
		}
	}
	return true;
}

DWORD FindFile (char *filename, CDirectory &curdir, CDirectory &destdir)
{
	CWinDirectory win;
	unsigned int curdirsize = curdir.getSize();
	unsigned int destdirsize =  destdir.getSize();
	DWORD result = VerFindFile( VFFF_ISSHAREDFILE,      
						filename, win.getBuffer(), win.getBuffer(),     //appdir=null - directory for related files 
						curdir.getBuffer(), &curdirsize , 
						destdir.getBuffer(), &destdirsize);

	return result;
}

bool ConfirmDiskAccessRetry(const char * windir) 
{
	char *buff[256] ;
	sprintf( (char *) buff, 
		"An error occured trying to write to your Windows drive "
		"you don't appear to have the correct access priviledges "
		"to write to %s, are you sure you want to replace the file?", 
		windir 
	);

	int choice = MessageBox(
		NULL, // should pass in the dialog box handle
		(char *) buff, "Swish Control Installation",
		MB_YESNO | MB_DEFBUTTON2 | MB_ICONINFORMATION );
	return ( choice == IDYES ) ;
}

bool NoSpaceConfirmRetry() 
{
	int choice = MessageBox(
		NULL, // should pass in the dialog box handle
		"Your Windows drive appears to be out of disk space"
		" - try emptying your recycle bin or deleting some files "
		"and try again...",
		"Swish Control Installation",
		MB_RETRYCANCEL | MB_DEFBUTTON2 | MB_ICONINFORMATION );
	return ( choice == IDRETRY ) ;
}

bool InUseConfirmRetry(const char * filename) 
{
	char *buff[256] ;
	sprintf( (char *) buff, 
		"A file (%s) is in use, please close any open applications and retry.", 
		filename 
	);

	int choice = MessageBox(
		NULL, // should pass in the dialog box handle
		(char *) buff, "Swish Control Installation",
		MB_RETRYCANCEL | MB_DEFBUTTON2 | MB_ICONINFORMATION );
	return ( choice == IDRETRY ) ;
}

bool ConfirmOverwriteOlder(const char * filename) 
{
	char *buff[256] ;
	sprintf( (char *) buff, 
		"An newer version of the file (%s) is already installed "
		"are you sure you want to overwrite it?", 
		filename 
	);
	int  choice = MessageBox(
			NULL, // should pass in the dialog box handle
			(char *) buff,
			"Swish Control Installation",
			MB_YESNO | MB_ICONASTERISK | MB_DEFBUTTON2 );
	return ( choice == IDYES );
	
}

void MissingSourceFile( char *filename, char * sourcedir )
{
	char *buff[256] ;
	sprintf( (char *) buff, 
		"A required file (%s) is missing from the setup directory (%s). " 
		"Please obtain an original copy of all the setup files "
		" and rerun Setup.",
		filename, sourcedir 
	);
	MessageBox(
			NULL, // should pass in the dialog box handle
			(char *) buff,
			"Swish Control Installation",
			MB_OK | MB_ICONASTERISK );
	
}

bool InstallFile (char * filename,
				  CDirectory &SourceDir, CDirectory &DestDir)
{
	CPath TempFile;
	unsigned int buffsize = TempFile.getSize();

	// ROUND ONE - try a simple install
	DWORD flags = VIFF_DONTDELETEOLD;
	DWORD result = VerInstallFile(
		  flags,       // bit flags that condition function behavior
		  filename,    // file to install
		  filename,    // new name of file to install
		  SourceDir.getBuffer(),    // source directory of file to install
		  DestDir.getBuffer(),      // directory in which to install file
		  NULL,        // directory where file is currently installed
		  TempFile.getBuffer(),     // receives name of temporary copy of file 
								    // used during installation
		  &buffsize			        // size of string in szTmpFile
		);

	while ( result ) { 
		flags = VIFF_DONTDELETEOLD;
		bool retry = false;

		if ( (result & VIF_FILEINUSE) && InUseConfirmRetry(filename) ) 
		{
				retry = true;
		} 
		else if ( result & VIF_CANNOTREADSRC ) // fatal error
		{
				MissingSourceFile(filename, SourceDir.getBuffer()); 
		} 
		else if ( (result & VIF_SRCOLD ) && ConfirmOverwriteOlder(filename) ) 
		{
				flags |= VIFF_FORCEINSTALL;
				retry = true;
		
		} 
		else if ( result & VIF_TEMPFILE  ) {
			// todo: work out what to do with the tempfile?

		} else if ( result 
			& (VIF_SHARINGVIOLATION|VIF_WRITEPROT|VIF_ACCESSVIOLATION) )
		{
			if ( ConfirmDiskAccessRetry( DestDir.getBuffer() ) ) {
				retry = true;
				flags |= VIFF_FORCEINSTALL;
			}

		} else if ( (result & VIF_OUTOFSPACE) && NoSpaceConfirmRetry()  ) {
			retry = true;
		}
		if ( ! retry ) break; 
		
		result = VerInstallFile(
		  flags, filename,  filename, 
		  SourceDir.getBuffer(), DestDir.getBuffer(),
		  NULL, TempFile.getBuffer(), &buffsize);
		
	}
	return (result == 0);
}

void ShowRegistrationFailed(HWND hwnd)
{
	MessageBox(
			hwnd, // should pass in the dialog box handle
			"An (OLE) error occured registering the search application "
			"on your system. Setup cannot continue.",
			"Swish Control Installation",
			MB_OK | MB_ICONASTERISK );

}

bool Uninstall( HWND hwnd ) 
{
	CDirectory curdir, destdir;
	FindFile( installdlls[SWISHCTL], curdir, destdir );

	CPath swishctlpath = curdir + installdlls[SWISHCTL];

	unregisterOleDll(swishctlpath.getBuffer());
	UnregisterUninstall();

	BOOL result = DeleteFile( swishctlpath.getBuffer() );

	// don't worry about deleting these too much
	swishctlpath = curdir + installdlls[PCRE];
	DeleteFile( swishctlpath.getBuffer() );
	swishctlpath = curdir + installdlls[PCRE];
	DeleteFile( swishctlpath.getBuffer() );
	
	DeleteShortcuts("index.lnk", "ACC Index"); 
	
	if (result) {

		MessageBox( hwnd, 
				"ACC Index Search Tool has been uninstalled", 
				"Swish Control Setup", 
				MB_OK
			   );
	} else {
		MessageBox( hwnd, 
				"ACC Index Search Tool has been uninstalled"
				" but some files could not be deleted perhaps"
				" they have been removed already." , 
				"Swish Control Setup", 
				MB_OK
			   );
	}

	return TRUE;
}


bool Install( HWND hwnd )
{
	CDirectory curdir, destdir;
	CAppDirectory sourcedir(SetupApp::getInstance());

	FindFile( installdlls[SWISHCTL], curdir, destdir );
	if (!InstallFile( installdlls[SWISHCTL], sourcedir, destdir )) return false;

	CPath swishctlpath = destdir + installdlls[SWISHCTL];

	if (!registerOleDll( swishctlpath.getBuffer()))
	{
		ShowRegistrationFailed(hwnd);
		return false;
	}


	if ( ! RegisterIndexFile(sourcedir.getBuffer(), "docs.idx") ) {
		MessageBox( hwnd, 
				"Setup was not able to write configuration details to "
				"your system registry. Please try running setup again", 
				"Swish Control Setup Error", 
				MB_OK | MB_ICONEXCLAMATION
			   );
		return false;
	}
	FindFile( installdlls[PCRE], curdir, destdir );
	if (!InstallFile( installdlls[PCRE], sourcedir, destdir )) return false;

	FindFile( installdlls[ZLIB], curdir, destdir );
	if(!InstallFile( installdlls[ZLIB], sourcedir, destdir )) return false;

	AddShortcut(sourcedir);

	CPath uninstaller = sourcedir + "setup.exe /u";
	RegisterUninstall(uninstaller.getBuffer());

	MessageBox( hwnd, 
			"Setup Complete",
			"Swish Control Setup", 
			MB_OK | MB_ICONINFORMATION
		   );
	return true;
}


#if 0
void DoTests()
{
	char curdir[_MAX_PATH]; 
	char destdir[_MAX_PATH];
	UINT curdirlen = _MAX_PATH;
	UINT destdirlen = _MAX_PATH;

	AppDir appdir;
	char *ad = appdir.c_str();

	FilePath fp("C:\\CRAP", _MAX_PATH);
	fp = fp + (const char *)"crap.exe";

	FilePath fp2;

	fp2 = fp2 + (const char *)"whatever";

	fp2 = fp + fp2;
	

	FilePath fp3(NULL, _MAX_PATH);
	DWORD copied = GetModuleFileName( SetupApp::getInstance(),
		fp3.c_str(), fp3.getSize());

	string fname(_MAX_PATH, '\0');

	copied = GetModuleFileName( SetupApp::getInstance(),
		(char *)fname.c_str(), fname.length());
}
#endif 

bool ConfirmUpgrade() 
{
	int  choice = MessageBox(
			GetDesktopWindow(), // could try GetDesktopWindow()
			"An older version of the search tool is installed."
			" Would you like to upgrade?",
			"Swish Control Installation",
			MB_YESNO | MB_ICONQUESTION );
	return ( choice == IDYES );
}

BOOL LaunchSearchPage( )
{
	CAppDirectory appdir( SetupApp::getInstance());
	CPath indexfile = appdir + "index.htm";
	return LaunchUrl( NULL, indexfile.getBuffer() );	

}

// - - - - - - - - - - - - - - - - - - - - - - - - - - -
// WinMain 
// Check the command line and registry options 
// to decide what to do or whether to do nothing at all!
// - - - - - - - - - - - - - - - - - - - - - - - - - - -
int APIENTRY WinMain(HINSTANCE hInstance,
                     HINSTANCE hPrevInstance,
                     LPSTR     lpCmdLine,
                     int       nCmdShow)
{

	
	// Perform application initialization:
	SetupApp _app( hInstance );

	//	TestPathsModule(SetupApp::getInstance());
	//  DoTests(); return 0;

	// Check for [/u] (uninstall) on command line
	if ( strstr( lpCmdLine, "/U" ) || strstr( lpCmdLine, "/u" )) {
		return Uninstall(NULL);
	} 

	// Check for [/f] (force display of dialog) on command line
	if ( strstr( lpCmdLine, "/F" ) 
		|| strstr( lpCmdLine, "/f" ) 
		|| !AppIsRegistered() ) 
	{
		if( ShowInstallDialog() ) {
			LaunchSearchPage ();
		}
		return 0;
	}

	switch ( CheckInstalledVersion() ) {
	case NO_VER:
		if( ShowInstallDialog() ) {
			LaunchSearchPage ();
		}
		break;
	case OLDER_VER:
		if ( !ConfirmUpgrade() ) break;

		if( ShowInstallDialog() ) {
			LaunchSearchPage ();
		}
		break;
	case CURRENT_VER:
		if ( AllowAutorun() ) 
		{
			LaunchSearchPage ();
		}
		break;
	case NEWER_VER:
		if ( AllowAutorun() ) 
		{
			LaunchSearchPage ();
		}
		break;
	}
	return 0;
}


// Mesage handler for about box.
// does the actual install !
LRESULT CALLBACK SetupHandler(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case WM_INITDIALOG:
				return TRUE;

		case WM_COMMAND:
			if (LOWORD(wParam) == IDOK ) 
			{
				bool autorun =
					(IsDlgButtonChecked (hDlg, IDC_AUTORUN) != BST_CHECKED);

				SetAutorun( autorun );

				Install( hDlg );

				EndDialog(hDlg, (BOOL) autorun );
				return TRUE;					
			} 
			else if ( LOWORD(wParam) == IDC_EXIT) 
			{
				EndDialog(hDlg, FALSE);
				return TRUE;
			}
			break;
	}
    return FALSE;
}


