I have test this version of SWISH-E on OpenVMS 7.3 AXP.

Building procedure:
$ @BUILD_SWISH-E
or
$ @BUILD_SWISH-E LIBXML2

If you have LIBXML2 installed

If you used LIBXML2 SWISH-E is compiled using /float=ieee

On AXP SWISH-E is compiled using /name=(short,as_is)


The build generate SWISH-E.EXE SWISH-SEARCH.EXE LIBTEST.EXE

Testing:
$ @BUILD_SWISH-E test


Port made by Piéronne Jean-François (jf.pieronne@laposte.net)
