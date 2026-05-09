################################################################################
#
# ------ For K computer -----
#
################################################################################

##### Fortran setting

FC  = mpifrtpx

FFLAGS_FAST  = -Kfast,parallel,ocl,preex,array_private,noalias=s,mfunc=2 \
               -Qi -Qt -X03 -Ncompdisp -Koptmsg=1 -Cpp -Free             \
               -Ksimd,auto,threadsafe

FFLAGS_DEBUG = -O0                                                   \
               -Qi -Qt -X03 -Ncompdisp -Koptmsg=1 -Cpp -Free         \
               -Ec -Eg -Ha -He -Hf -Ho -Hs -Hu -Hx -v03s -v03d -v03o \
               -Ncheck_cache_arraysize -Ncheck_global

# if ILS_DEBUG=T is set, FFLAGS_DEBUG is selected in Mkinclude

##### C setting

CC = mpifccpx

CFLAGS_FAST  = -Kfast,parallel,ocl,preex,array_private,region_extension,restp=all -Ksimd
CFLAGS_DEBUG = -O0

CFLAGS = $(CFLAGS_FAST)

##### Linker setting

LD      = $(FC)
LDFLAGS = $(FFLAGS)

################################################################################

##### for frontend
INSTALL = install
AR      = ar
ARFLAGS = r
RANLIB  = ranlib

RM        = rm
RMFLAGS   = -f
F2PY      = f2py
FCOMPILER = gnu95
