// stringbuffer.h: interface for the stringbuffer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STRINGBUFFER_H__C9ABFB60_4DFC_4C9D_AE7C_1224825A1DDB__INCLUDED_)
#define AFX_STRINGBUFFER_H__C9ABFB60_4DFC_4C9D_AE7C_1224825A1DDB__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class stringbuffer  
{
public:
	stringbuffer();
	virtual ~stringbuffer();

protected:
	char * m_buffer;
};

#endif // !defined(AFX_STRINGBUFFER_H__C9ABFB60_4DFC_4C9D_AE7C_1224825A1DDB__INCLUDED_)
