
SwishCtlps.dll: dlldata.obj SwishCtl_p.obj SwishCtl_i.obj
	link /dll /out:SwishCtlps.dll /def:SwishCtlps.def /entry:DllMain dlldata.obj SwishCtl_p.obj SwishCtl_i.obj \
		kernel32.lib rpcndr.lib rpcns4.lib rpcrt4.lib oleaut32.lib uuid.lib \

.c.obj:
	cl /c /Ox /DWIN32 /D_WIN32_WINNT=0x0400 /DREGISTER_PROXY_DLL \
		$<

clean:
	@del SwishCtlps.dll
	@del SwishCtlps.lib
	@del SwishCtlps.exp
	@del dlldata.obj
	@del SwishCtl_p.obj
	@del SwishCtl_i.obj
