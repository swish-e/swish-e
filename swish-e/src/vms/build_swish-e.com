$ arch_name = f$edit(f$getsyi("arch_name"),"upcase")
$ if arch_name .eqs. "ALPHA" then arch_name = "axp"
$ proc =  f$environment("procedure")
$ dev = f$parse(proc,,,"DEVICE")
$ dir = f$parse(proc,,,"DIRECTORY")
$ set default 'dev''dir'
$ set default [-]
$ if p1 .eqs. "LIBXML2" 
$ then
$! Don't work on VAX
$   mms/descr=[.vms]descrip_libxml2.mms 'p2'
$ else
$   mms/descr=[.vms]descrip_'arch_name'.mms 'p2'
$ endif
