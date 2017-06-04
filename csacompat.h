#ifndef _CSACOMPAT_H
#define _CSACOMPAT_H

#define CSA_VERSION 2009071800
#define CSA_ID_HEADER 0x00
#define CSA_ID_SA     0x01
#define CSA_ID_ISA    0x02
#define CSA_K 4        /* #bytes of integer */
#define CSA_D 128      /* interval between two SA values stored explicitly */
#define CSA_D2 256/* interval between two inverse SA values stored explicitly */
#define CSA_SIGMA 256  /* alphabet size */
                       
#endif
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
