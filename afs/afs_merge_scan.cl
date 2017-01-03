
#define MERGE_BLOCK_INT_X  (32) //work groupサイズ(x) = スレッド数/work group
#define MERGE_BLOCK_Y       (8) //work groupサイズ(y) = スレッド数/work group
#define MERGE_BLOCK_LOOP_Y  (1) //work groupのy方向反復数

#define MERGE_PROC_TYPE       uint4
#define MERGE_PROC_TYPE_CL cl_uint4

#ifdef __OPENCL_VERSION__

__kernel void afs_merge_scan_kernel(
    __global MERGE_PROC_TYPE *restrict ptr_dst,
    const __global MERGE_PROC_TYPE *restrict src_p0,
    const __global MERGE_PROC_TYPE *restrict src_p1,
    int si_w_type, int h) {
    //int lx = get_local_id(0); //スレッド数=MERGE_BLOCK_INT_X
    int imgx = get_global_id(0);

    if (imgx < si_w_type) {
        int ly = get_local_id(1); //スレッド数=MERGE_BLOCK_Y
        int gid = get_group_id(1);
        int imgy = gid * MERGE_BLOCK_LOOP_Y * MERGE_BLOCK_Y + ly;
        src_p0  += imgx + si_w_type * imgy;
        src_p1  += imgx + si_w_type * imgy;
        ptr_dst += imgx + si_w_type * imgy;
        for (uint iloop = 0; iloop < MERGE_BLOCK_LOOP_Y; iloop++,
            imgy    += MERGE_BLOCK_Y,
            ptr_dst += MERGE_BLOCK_Y * si_w_type,
            src_p0  += MERGE_BLOCK_Y * si_w_type,
            src_p1  += MERGE_BLOCK_Y * si_w_type
        ) {
            if (imgy < h) {
                const int offsetm = (imgy == 0  ) ? 0 : -si_w_type;
                const int offsetp = (imgy >= h-1) ? 0 :  si_w_type;
                MERGE_PROC_TYPE p0m = src_p0[offsetm];
                MERGE_PROC_TYPE p0c = src_p0[0];
                MERGE_PROC_TYPE p0p = src_p0[offsetp];

                MERGE_PROC_TYPE p1m = src_p1[offsetm];
                MERGE_PROC_TYPE p1c = src_p1[0];
                MERGE_PROC_TYPE p1p = src_p1[offsetp];

                MERGE_PROC_TYPE m4 = (p0m | p0p | (MERGE_PROC_TYPE)0xf3f3f3f3) & p0c;
                MERGE_PROC_TYPE m5 = (p1m | p1p | (MERGE_PROC_TYPE)0xf3f3f3f3) & p1c;

                ptr_dst[0] = (m4 & m5 & (MERGE_PROC_TYPE)0x44444444) | (~(p0c) & (MERGE_PROC_TYPE)0x33333333);
            }
        }
    }
}

#endif //#ifdef __OPENCL_VERSION__
