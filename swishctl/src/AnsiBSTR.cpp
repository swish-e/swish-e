/*
 * $Id$
 * 
 * SwishCtl - SWISH-E API ActiveX Control
 * Copyright (c) 2003 Peoples Resource Centre Wellington NZ
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */ 

// AnsiBSTR.cpp: implementation of the CAnsiBSTR class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "AnsiBSTR.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CAnsiBSTR::CAnsiBSTR(const char *source)
{
	int sourcelen = strlen( source );
	//m_cstr = NULL;	
	wchar_t *wbuffer = (wchar_t *)calloc( sourcelen + 1, sizeof(wchar_t));

	if ( ! wbuffer ) return; 	

	MultiByteToWideChar(  CP_ACP,         // code page
		NULL,         // character-type options
		source, // string to map
		sourcelen,       // number of bytes in string
		wbuffer,  // wide-character buffer
		sourcelen + 1        // size of buffer
		);
	this->Attach(SysAllocString( wbuffer ));
	free( wbuffer );
}

CAnsiBSTR::~CAnsiBSTR()
{
	//if ( m_cstr ) free( m_cstr ); 
}



CAnsiBSTR::CAnsiBSTR(BSTR bstr)
{
	//m_cstr = NULL;	
	this->Attach( bstr );
}

// -----------------------------


CAnsiStr::~CAnsiStr()
{
	if ( m_cstr ) free( m_cstr ); 
}

char * CAnsiStr::c_str()
{
	return m_cstr;
}

CAnsiStr::CAnsiStr(BSTR bstr)
{
	m_cstr = NULL;	

	int sourcelen = SysStringLen(bstr);
	
	m_cstr = (char *) calloc( sourcelen + 1, sizeof(char));

	if ( ! m_cstr ) return; 	
	
	WideCharToMultiByte(  
		CP_ACP,		 // code page
		NULL,        // performance and mapping flags 
		bstr,    // wide-character string 
		sourcelen,          // number of chars in string
		m_cstr ,     // buffer for new string
		sourcelen + 1,          // size of buffer
		NULL,     // default for unmappable chars NULL
		NULL  // set when default char used
		);
	
	m_cstr [sourcelen] = '\0';
}

