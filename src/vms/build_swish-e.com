$ proc =  f$environment("procedure")
$ dev = f$parse(proc,,,"DEVICE")
$ dir = f$parse(proc,,,"DIRECTORY")
$ set default 'dev''dir'
$ set default [-]
$ mms/descr=[.vms]descrip.mms 'p1'
