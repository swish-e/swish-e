$ proc =  f$environment("procedure")
$ dev = f$parse(proc,,,"DEVICE")
$ dir = f$parse(proc,,,"DIRECTORY")
$ set default 'dev''dir'
$ set default [-]
$ if p2 .eqs. "LIBXML" 
$ then
$   mms/descr=[.vms]descrip.mms 'p1'
$ else
$   mms/descr=[.vms]descrip_axp.mms 'p1'
$ endif
