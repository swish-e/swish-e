/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RegKey Starts
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
class RegKey {
public:
	RegKey() {hkey=NULL; startkey=HKEY_CURRENT_USER;};
	RegKey(HKEY startkey);
	~RegKey();
	bool CreateKey(const char *keyname);
	bool OpenKey( const char * keyname);
	bool OpenKey( const char * keyname, REGSAM samdesired);
	bool SetValue(const char *valuename, const char *value);
	bool DeleteKey(const char *subkey);
	string QueryValue(const char *valuename);
private:
	HKEY startkey;
	HKEY hkey;
};

/* - - - - - - - - - - - - - - - - - - - - - - - - - - -
 * RegKey ends
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - */
