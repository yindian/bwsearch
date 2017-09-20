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
    sprintf(ifname, "%.*s.idx", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "rb");
    fprintf(stderr, "Loading %s ... ", ifname);
    TICK;
    ret = bws_load_csa_index(&csa, BWS_FLAG_LOAD_ISA, fp);
    fclose(fp);
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
    sprintf(ifname, "%.*s.bws", baselen, base);
    CHECK_OPEN_FILE(fp, ifname, "rb");
    fprintf(stderr, "Loading %s ... ", ifname);
    TICK;
    ret = bws_load_bws_index(&bws, BWS_FLAG_LOAD_RANKC, fp);
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
/* vim: set ts=4 sw=4 et cino=l1,t0,(0,w1,W2s,M1 fo+=mM tw=80 cc=80 : */
