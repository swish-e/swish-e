// AnsiBSTR.h: interface for the CAnsiBSTR class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ANSIBSTR_H__5C10CD8E_238E_4483_A5C3_702741F49B3E__INCLUDED_)
#define AFX_ANSIBSTR_H__5C10CD8E_238E_4483_A5C3_702741F49B3E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CAnsiBSTR : public CComBSTR  
{
public:
	CAnsiBSTR( BSTR bstr);
	CAnsiBSTR(const char *source);
	virtual ~CAnsiBSTR();

};

class CAnsiStr  
{
public:
	CAnsiStr( BSTR bstr);
	char * c_str();
	virtual ~CAnsiStr();
private:
		char * m_cstr;
};
#endif // !defined(AFX_ANSIBSTR_H__5C10CD8E_238E_4483_A5C3_702741F49B3E__INCLUDED_)
