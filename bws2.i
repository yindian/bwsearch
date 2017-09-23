/*
 * See Copyright Notice in bwslib.h
 */
    ifname = (char *) malloc(baselen + 5);
    if (!ifname)
    {
        fprintf(stderr, "malloc failed\n");
        return FAIL_RET;
    }
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        bws_free_csa_index(&csa);\
        free(ifname);\
    } while (0)
    sprintf(ifname, "%.*s.ndx", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "rb");
    fprintf(stderr, "Loading %s csa ... ", ifname);
    TICK;
    ret = bws_load_csa_index(&csa, BWS_FLAG_LOAD_SA | BWS_FLAG_LOAD_ISA
                             | BWS_FLAG_MMAP, fp);
    if (ret)
    {
        fprintf(stderr, "failed %d\n", ret);
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
#undef CLEAN_UP
#define CLEAN_UP do\
    {\
        bws_free_bws_index(&bws);\
        bws_free_csa_index(&csa);\
        free(ifname);\
    } while (0)
    fprintf(stderr, "Loading %s bws ... ", ifname);
    TICK;
    ret = bws_load_bws_index(&bws, BWS_FLAG_LOAD_RANKC | BWS_FLAG_MMAP, fp);
    fclose(fp);
    if (ret)
    {
        fprintf(stderr, "failed %d\n", ret);
        CLEAN_UP;
        return FAIL_RET;
    }
    TOCK;
    sprintf(ifname, "%.*s.bw", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "rb");
    bwfp = bw_file_new_from_fp(fp, BWS_FLAG_MMAP);
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
