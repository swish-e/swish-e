/* this file contains the actual definitions of */
/* the IIDs and CLSIDs */

/* link this file in with the server and any clients */


/* File created by MIDL compiler version 5.01.0164 */
/* at Fri May 23 23:23:35 2003
 */
/* Compiler settings for C:\projects\swish\win32\swishctl\SwishCtl.idl:
    Oicf (OptLev=i2), W1, Zp8, env=Win32, ms_ext, c_ext
    error checks: allocation ref bounds_check enum stub_data 
*/
//@@MIDL_FILE_HEADING(  )
#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

const IID IID_ICSwishCtl = {0x9E204F2D,0x4F04,0x11D7,{0x85,0x8F,0x8C,0xA0,0x8F,0xF5,0x86,0x0C}};


const IID LIBID_SWISHCTLLib = {0x9E204F21,0x4F04,0x11D7,{0x85,0x8F,0x8C,0xA0,0x8F,0xF5,0x86,0x0C}};


const CLSID CLSID_CSwishCtl = {0x9E204F2E,0x4F04,0x11D7,{0x85,0x8F,0x8C,0xA0,0x8F,0xF5,0x86,0x0C}};


#ifdef __cplusplus
}
#endif

