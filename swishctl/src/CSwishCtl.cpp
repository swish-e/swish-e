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



// CSwishCtl.cpp : Implementation of CCSwishCtl

// N.B. can't add swish-e/src to build settings for this project 

// because the folder contains a string.h which overrides the 

// default search location for string.h ! 

#include "stdafx.h"

#include <string>

using namespace std;



#include "swish-e.h"

#include "SwishCtl.h"

#include "CSwishCtl.h"

#include "AnsiBSTR.h"

#include "regkey.h"



/////////////////////////////////////////////////////////////////////////////

// CCSwishCtl

CCSwishCtl::CCSwishCtl()

{

	swish_handle = NULL;

	swish_results = NULL;

	current_result = NULL; 

}



CCSwishCtl::~CCSwishCtl(void)

{

	

	if ( swish_handle ) {

        SwishClose( swish_handle );  /* free the memory used */        

	}

}



// I didn't know where to put this prototype...

// so it can go here for the meantime

bool GetModulePath(char filename[ _MAX_PATH ]);





STDMETHODIMP CCSwishCtl::Init(BSTR IndexFiles)

{

	// In order to claim "safe for scripting", 

	// don't allow location of resources to be specified 

	// as a parameter - for this application, the dll must

	// and the index files must reside in the same folder



	// So now the IndexFiles parameter is the name of a 

	// registry value in the options key for this app



	//BstrConverter bstr_indexfiles(IndexFiles);



	const char registrykey[] = "Software\\Kauranga\\ACCIndex\\1.0\\Options";



	CAnsiStr bstr_indexfiles(IndexFiles);



	RegKey hkcu(HKEY_LOCAL_MACHINE) ;





	string indexfilepath; 

	string indexfilename; 





	if (hkcu.OpenKey( registrykey, KEY_QUERY_VALUE )) {

		indexfilepath = hkcu.QueryValue( "IndexLocation"  );

		indexfilename = hkcu.QueryValue( "IndexFiles"  );

	}

		



	chdir( indexfilepath.c_str() );



	swish_handle = SwishInit((char *)indexfilename.c_str());

	

	if ( ! swish_handle ) {

		::MessageBox( NULL, "SwishInit call failed...", indexfilepath.c_str(), MB_OK );

		return E_FAIL;

	}

    if ( SwishError( swish_handle ) )    {

		::MessageBox( NULL, indexfilepath.c_str(), "Swish-e Index File error...", MB_OK );



        SwishClose( swish_handle );  /* free the memory used */        

		return E_FAIL;

    }



	swish_search = NULL;



	return S_OK;

}





STDMETHODIMP CCSwishCtl::GetInterfaceSafetyOptions(REFIID riid, DWORD *pdwSupportedOptions, DWORD *pdwEnabledOptions)

{

   ATLTRACE(_T("CObjectSafetyImpl::GetInterfaceSafetyOptions\n"));

   if (!pdwSupportedOptions || !pdwEnabledOptions)

	   return E_FAIL;

   LPUNKNOWN pUnk;

   if (_InternalQueryInterface (riid, (void**)&pUnk) == E_NOINTERFACE) 

   {

      // Our object doesn't even support this interface.

      return E_NOINTERFACE;   

   } else {

	   // Cleanup after ourselves.

       pUnk->Release();      

	   pUnk = NULL;   

   }   

   if (riid == IID_IDispatch) {

      // IDispatch is an interface used for scripting. If your

      // control supports other IDispatch or Dual interfaces, you

      // may decide to add them here as well. Client wants to know

      // if object is safe for scripting. Only indicate safe for

      // scripting when the interface is safe.

      *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_CALLER;

      *pdwEnabledOptions = m_dwSafety &

                           INTERFACESAFE_FOR_UNTRUSTED_CALLER;      

	  return S_OK;

   } else if ((riid == IID_IPersistStreamInit) ||

             (riid == IID_IPersistStorage)) 

   {

      // IID_IPersistStreamInit and IID_IPersistStorage are

      // interfaces used for Initialization. If your control

      // supports other Persistence interfaces, you may decide to

      // add them here as well. Client wants to know if object is

      // safe for initializing. Only indicate safe for initializing

      // when the interface is safe.

      *pdwSupportedOptions = INTERFACESAFE_FOR_UNTRUSTED_DATA;

      *pdwEnabledOptions = m_dwSafety &

                           INTERFACESAFE_FOR_UNTRUSTED_DATA;

      return S_OK;

   } else {

	  // We are saying that no other interfaces in this control are

      // safe for initializing or scripting.      

	  *pdwSupportedOptions = 0;

      *pdwEnabledOptions = 0;      

	  return E_FAIL;   

	}

}



STDMETHODIMP CCSwishCtl::SetInterfaceSafetyOptions(REFIID riid, DWORD dwOptionSetMask, DWORD dwEnabledOptions)

{

   ATLTRACE(_T("CObjectSafetyImpl::SetInterfaceSafetyOptions\n"));

   if (!dwOptionSetMask && !dwEnabledOptions) return E_FAIL;   



   LPUNKNOWN pUnk;

   if (_InternalQueryInterface (riid, (void**)&pUnk) == E_NOINTERFACE) 

   {

      // Our object doesn't even support this interface.

      return E_NOINTERFACE;   

   } else {

	   // Cleanup after ourselves.

       pUnk->Release();      

	   pUnk = NULL;   

   }

   // Store our current safety level to return in   

   // GetInterfaceSafetyOptions

   m_dwSafety |= dwEnabledOptions & dwOptionSetMask;

   if ((riid == IID_IDispatch) &&

       (m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_CALLER)) 

   {

      // Client wants us to disable any functionality that would

      // make the control unsafe for scripting. The same applies to

      // any other IDispatch or Dual interfaces your control may

      // support. Because our control is safe for scripting by

      // default we just return S_OK. 

	  return S_OK;

   } else if (((riid == IID_IPersistStreamInit) ||

              (riid == IID_IPersistStorage)) &&

             (m_dwSafety & INTERFACESAFE_FOR_UNTRUSTED_DATA)) 

   {

      // Client wants us to make the control safe for initializing

      // from persistent data. For these interfaces, this control

      // is safe so we return S_OK. For Any interfaces that are not

      // safe, we would return E_FAIL.      

	   return S_OK;   

   } else {

      // This control doesn't allow Initialization or Scripting

      // from any other interfaces so return E_FAIL.      

	   return E_FAIL;   

	}   

}





STDMETHODIMP CCSwishCtl::Error(int *errcode)

{

	*errcode = SwishError( swish_handle );

	return S_OK;

}





STDMETHODIMP CCSwishCtl::Query(BSTR words, int *errcode)

{



	HRESULT hresult = NewSearch( words, errcode );



	if ( hresult != S_OK ) return hresult;



	return Execute( NULL, errcode );	

}



STDMETHODIMP CCSwishCtl::ErrorString(BSTR *errstring)

{

	if ( ! swish_handle ) { 

		return E_FAIL;

	}



	char *err = SwishErrorString(swish_handle);

	

	CAnsiBSTR bstrTemp( err );



    if (!bstrTemp)

           return E_OUTOFMEMORY;

 

	*errstring = bstrTemp.Detach();

	return S_OK;

}



STDMETHODIMP CCSwishCtl::Hits(int *hits)

{

	if (! swish_results ) return E_FAIL;

	*hits = SwishHits( swish_results );

	return S_OK;

}



STDMETHODIMP CCSwishCtl::NextResult(int *more)

{

	*more = FALSE;

	if (!swish_results) return E_FAIL;



	current_result = SwishNextResult( swish_results );

	if ( current_result ) *more = TRUE;

	

	return S_OK;

}





STDMETHODIMP CCSwishCtl::ResultPropertyString(

					BSTR propertyname, BSTR *result)

{





	CAnsiStr bstr_convert(propertyname);



	char *propertyval = SwishResultPropertyStr(

		current_result, bstr_convert.c_str());



	CAnsiBSTR bstrTemp( propertyval );



    if (!bstrTemp)

           return E_OUTOFMEMORY;

 

	*result = bstrTemp.Detach();



	return S_OK;



}



STDMETHODIMP CCSwishCtl::Close()

{

	current_result = NULL;

	if ( swish_handle ) {

		if ( swish_results ) {

			Free_Results_Object( swish_results );

			swish_results = NULL;

		}

		if (swish_search) {

			Free_Search_Object( swish_search );

			swish_search = NULL;

		}

        SwishClose( swish_handle );  /* free the memory used */        

		swish_handle = NULL;

	}



	return S_OK;

}



STDMETHODIMP CCSwishCtl::SeekResult(int position, int *result)

{

	*result = -1; // an error

	if (!swish_results) return S_OK;



	*result = SwishSeekResult( swish_results, position );

	

	return S_OK;

}



STDMETHODIMP CCSwishCtl::SetSort(BSTR sort)

{

	if ( ! (swish_handle && swish_search) ){ 

		return E_FAIL;

	}



	CAnsiStr bstr_sort( sort );



	SwishSetSort( swish_search, bstr_sort.c_str() );



	return S_OK;

}





STDMETHODIMP CCSwishCtl::NewSearch(BSTR query, int *errcode)

{

	*errcode = -1; 

	if ( ! swish_handle ) { 

		::MessageBox( NULL, "swish_handle is null...", "New Search", MB_OK );

		return E_FAIL;

	}

	CAnsiStr bstr_query( query );



	if ( swish_results ) {

		Free_Results_Object(swish_results);

		swish_results = NULL;

	}



	if ( swish_search ) {

		Free_Search_Object( swish_search );

	}

	swish_search = New_Search_Object( swish_handle, bstr_query.c_str() );





	SwishResetSearchLimit( swish_search );



	if ( ! swish_search ) {

		::MessageBox( NULL, "swish_search is null...", "Error", MB_OK );

	}



	*errcode = SwishError( swish_handle );



	return S_OK;

}



STDMETHODIMP CCSwishCtl::Execute(BSTR words, int *errcode)

{

	*errcode = -1; 

	if ( ! swish_handle ) {

		

		return E_FAIL;

	}



	if ( !words ) {

		// use existing query string - no need to convert string

		swish_results = SwishExecute( swish_search, NULL );

		*errcode = SwishError( swish_handle );

		return S_OK;

	}	

	CAnsiStr bstr_words( words );

	swish_results = SwishExecute( swish_search, bstr_words.c_str() );



	*errcode = SwishError( swish_handle );



	return S_OK;

}







STDMETHODIMP CCSwishCtl::SetSearchLimit(BSTR propertyname, BSTR low, BSTR hi)

{

	if ( ! (swish_handle && swish_search) ){ 

		return E_FAIL;

	}

	CAnsiStr bstr_propertyname(propertyname);

	CAnsiStr bstr_low(low);

	CAnsiStr bstr_hi(hi);



	SwishSetSearchLimit( swish_search, 

		bstr_propertyname.c_str(), 

		bstr_low.c_str(),

		bstr_hi.c_str() );

	return S_OK;

}



