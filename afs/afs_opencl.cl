﻿
#define BLOCK_INT_X  (32) //work groupサイズ(x) = スレッド数/work group
#define BLOCK_Y       (8) //work groupサイズ(y) = スレッド数/work group
#define BLOCK_LOOP_Y (16) //work groupのy方向反復数

#define SHARED_INT_X (BLOCK_INT_X) //SLMの幅
#define SHARED_Y     (16) //SLMの縦

#ifdef __OPENCL_VERSION__

//__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_CLAMP_TO_EDGE | CLK_FILTER_NEAREST;
__constant sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_NONE | CLK_FILTER_NEAREST;

//      7       6         5        4        3        2        1       0
// | motion  |         non-shift        | motion  |          shift          |
// |  shift  |  sign  |  shift |  deint |  flag   | sign  |  shift |  deint |      
__constant uchar motion_flag = 0x08u;
__constant uchar motion_shift = 0x80u;

__constant uchar non_shift_sign  = 0x40u;
__constant uchar non_shift_shift = 0x20u;
__constant uchar non_shift_deint = 0x10u;

__constant uchar shift_sign  = 0x04u;
__constant uchar shift_shift = 0x02u;
__constant uchar shift_deint = 0x01u;

#if PREFER_SHORT4
short4 analyze_motion(short4 p0, short4 p1, uchar thre_motion, uchar thre_shift) {
    short4 absy = as_short4(abs(convert_short4(p1) - convert_short4(p0)));
    short4 mask = 0;
    mask |= ((short4)thre_motion > absy) ? (short4)motion_flag  : (short4)0;
    mask |= ((short4)thre_shift  > absy) ? (short4)motion_shift : (short4)0;
    return mask;
}

short4 analyze_stripe(short4 p0, short4 p1, uchar flag_sign, uchar flag_deint, uchar flag_shift, uchar thre_deint, uchar thre_shift) {
    short4 new_sign = (p0 >= p1) ? (short4)flag_sign : (short4)0;
    //short4 new_diff = (p1 > p0) ? p1 - p0 : p0 - p1;
    short4 absy = as_short4(abs(convert_short4(p1) - convert_short4(p0)));
    short4 mask = new_sign;
    mask |= (absy > (short4)thre_deint) ? (short4)flag_deint : (short4)0;
    mask |= (absy > (short4)thre_shift) ? (short4)flag_shift : (short4)0;
    return mask;
}

uchar4 analyze(
    __read_only image2d_t img_p0,
    __read_only image2d_t img_p1,
    int imgx, int imgy, int tb_order,
    uchar thre_motion, uchar thre_deint, uchar thre_shift) {
    short4 p0, p1, mask = 0;
    //motion
    p0 = convert_short4(read_imageui(img_p0, sampler, (int2)(imgx,imgy)));
    p1 = convert_short4(read_imageui(img_p1, sampler, (int2)(imgx,imgy)));
    mask = analyze_motion(p0, p1, thre_motion, thre_shift);

    if (imgy >= 1) {
        //non-shift
        p1 = convert_short4(read_imageui(img_p0, sampler, (int2)(imgx,imgy-1)));
        mask |= analyze_stripe(p0, p1, non_shift_sign, non_shift_deint, non_shift_shift, thre_deint, thre_shift);

        //shift
        if ((tb_order + imgy) & 1) {
            p0 = convert_short4(read_imageui(img_p1, sampler, (int2)(imgx,imgy-1)));
            p1 = convert_short4(read_imageui(img_p0, sampler, (int2)(imgx,imgy  )));
        } else {
            p0 = convert_short4(read_imageui(img_p0, sampler, (int2)(imgx,imgy-1)));
            p1 = convert_short4(read_imageui(img_p1, sampler, (int2)(imgx,imgy  )));
        }
        mask |= analyze_stripe(p0, p1, shift_sign, shift_deint, shift_shift, thre_deint, thre_shift);
    }
    return convert_uchar4(mask);
}
#else
uchar4 analyze_motion(uchar4 p0, uchar4 p1, uchar thre_motion, uchar thre_shift) {
    uchar4 absy = convert_uchar4(abs(convert_short4(p1) - convert_short4(p0)));
    uchar4 mask = 0;
    mask |= ((uchar4)thre_motion > absy) ? (uchar4)motion_flag  : (uchar4)0;
    mask |= ((uchar4)thre_shift  > absy) ? (uchar4)motion_shift : (uchar4)0;
    return mask;
}

uchar4 analyze_stripe(uchar4 p0, uchar4 p1, uchar flag_sign, uchar flag_deint, uchar flag_shift, uchar thre_deint, uchar thre_shift) {
    uchar4 new_sign = (p0 >= p1) ? (uchar4)flag_sign : (uchar4)0;
    //uchar4 new_diff = (p1 > p0) ? p1 - p0 : p0 - p1;
    uchar4 absy = as_uchar4((p1 > p0) ? as_char4(p1) - as_char4(p0) : as_char4(p0) - as_char4(p1));
    //uchar4 absy = convert_uchar4(abs(convert_short4(p1) - convert_short4(p0)));
    uchar4 mask = new_sign;
    mask |= (absy > (uchar4)thre_deint) ? (uchar4)flag_deint : (uchar4)0;
    mask |= (absy > (uchar4)thre_shift) ? (uchar4)flag_shift : (uchar4)0;
    return mask;
}

uchar4 analyze(
    __read_only image2d_t img_p0,
    __read_only image2d_t img_p1,
    int imgx, int imgy, int tb_order,
    uchar thre_motion, uchar thre_deint, uchar thre_shift) {
    uchar4 p0, p1, mask = 0;
    //motion
    p0 = read_imageui(img_p0, sampler, (int2)(imgx,imgy));
    p1 = read_imageui(img_p1, sampler, (int2)(imgx,imgy));
    mask = analyze_motion(p0, p1, thre_motion, thre_shift);

    if (imgy >= 1) {
        //non-shift
        p1 = read_imageui(img_p0, sampler, (int2)(imgx,imgy-1));
        mask |= analyze_stripe(p0, p1, non_shift_sign, non_shift_deint, non_shift_shift, thre_deint, thre_shift);

        //shift
        if ((tb_order + imgy) & 1) {
            p0 = read_imageui(img_p1, sampler, (int2)(imgx,imgy-1));
            p1 = read_imageui(img_p0, sampler, (int2)(imgx,imgy  ));
        } else {
            p0 = read_imageui(img_p0, sampler, (int2)(imgx,imgy-1));
            p1 = read_imageui(img_p1, sampler, (int2)(imgx,imgy  ));
        }
        mask |= analyze_stripe(p0, p1, shift_sign, shift_deint, shift_shift, thre_deint, thre_shift);
    }
    return mask;
}
#endif

int shared_int_idx(int x, int y, int dep) {
    return dep * SHARED_INT_X * SHARED_Y + (y&15) * SHARED_INT_X + x;
}

void count_flags(ushort2 dat0, ushort2 dat1, ushort2 *restrict count_deint, ushort2 *restrict count_shift) {
    ushort2 deint, shift, mask;
    deint = dat0 & (ushort2)(non_shift_deint | shift_deint | (non_shift_deint<<8) | (shift_deint<<8));
    shift = dat0 & (ushort2)(non_shift_shift | shift_shift | (non_shift_shift<<8) | (shift_shift<<8));
    mask = (dat0 & (ushort2)(non_shift_sign  | shift_sign  | (non_shift_sign <<8) | (shift_sign <<8)))
         ^ (dat1 & (ushort2)(non_shift_sign  | shift_sign  | (non_shift_sign <<8) | (shift_sign <<8)));
    mask |= (mask<<1);
    mask |= (mask>>2);
    *count_deint &= mask;
    *count_shift &= mask;
    *count_deint += deint;
    *count_shift += shift;
}

ushort2 generate_mask(int ly, int idepth, __local int *restrict ptr_shared) {
    ushort2 count_deint = (ushort2)0;
    ushort2 count_shift = (ushort2)0;
    ushort2 dat0, dat1, deint, shift, motion;

    //sharedメモリはあらかじめ-4もデータを作ってあるので、問題なく使用可能
    dat1 = as_ushort2(ptr_shared[shared_int_idx(0, ly-3, idepth)]);
    //deint = dat1 & (ushort2)(non_shift_deint | shift_deint | (non_shift_deint<<8) | (shift_deint<<8));
    shift = dat1 & (ushort2)(non_shift_shift | shift_shift | (non_shift_shift<<8) | (shift_shift<<8));
    //count_deint += deint;
    count_shift += shift;

    dat0 = as_ushort2(ptr_shared[shared_int_idx(0, ly-2, idepth)]);
    count_flags(dat0, dat1, &count_deint, &count_shift);

    dat1 = as_ushort2(ptr_shared[shared_int_idx(0, ly-1, idepth)]);
    count_flags(dat1, dat0, &count_deint, &count_shift);

    dat0 = as_ushort2(ptr_shared[shared_int_idx(0, ly+0, idepth)]);
    count_flags(dat0, dat1, &count_deint, &count_shift);

    //      7       6         5        4        3        2        1       0
    // | motion  |         non-shift        | motion  |          shift          |
    // |  shift  |  sign  |  shift |  deint |  flag   | sign  |  shift |  deint |  
    ushort2 mask = 0;
    //motion 0x8888 -> 0x4444 とするため右シフト 
    mask |= ((dat0 & (ushort2)(motion_flag | motion_shift | (motion_flag << 8) | (motion_shift << 8))) >> (ushort2)1); //motion flag / motion shift
    //nonshift deint - countbit:654 / setbit 0x01
    mask |= (count_deint & (ushort2)(0x70u << 0)) > (ushort2)(2<<(4+0)) ? (ushort2)(0x01<< 0) : (ushort2)0x00; //nonshift deint(0)
    mask |= (count_deint & (ushort2)(0x70u << 8)) > (ushort2)(2<<(4+8)) ? (ushort2)(0x01<< 8) : (ushort2)0x00; //nonshift deint(1)
    //nonshift shift - countbit:765 / setbit 0x10
    mask |= (count_shift & (ushort2)(0xE0u << 0)) > (ushort2)(3<<(5+0)) ? (ushort2)(0x01<< 4) : (ushort2)0x00; //nonshift shift(0)
    mask |= (count_shift & (ushort2)(0xE0u << 8)) > (ushort2)(3<<(5+8)) ? (ushort2)(0x01<<12) : (ushort2)0x00; //nonshift shift(1)
    //shift deint - countbit:210 / setbit 0x02
    mask |= (count_deint & (ushort2)(0x07u << 0)) > (ushort2)(2<<(0+0)) ? (ushort2)(0x01<< 1) : (ushort2)0x00; //shift deint(0)
    mask |= (count_deint & (ushort2)(0x07u << 8)) > (ushort2)(2<<(0+8)) ? (ushort2)(0x01<< 9) : (ushort2)0x00; //shift deint(1)
    //shift shift - countbit:321 / setbit 0x20
    mask |= (count_shift & (ushort2)(0x0Eu << 0)) > (ushort2)(3<<(1+0)) ? (ushort2)(0x01<< 5) : (ushort2)0x00; //shift shift(0)
    mask |= (count_shift & (ushort2)(0x0Eu << 8)) > (ushort2)(3<<(1+8)) ? (ushort2)(0x01<<13) : (ushort2)0x00; //shift shift(1)

    return mask;
}

void merge_mask(ushort2 masky, ushort2 maskc, ushort2 *restrict mask0, ushort2 *restrict mask1) {
    // | v2 | u2 | v0 | u0 |  maskc
    // | u2 | u2 | u0 | u0 |  masku
    // | v2 | v2 | v0 | v0 |  maskv
    ushort2 masku = (maskc & (ushort2)0x00ff) | (maskc << (ushort2)8);
    ushort2 maskv = (maskc & (ushort2)0xff00) | (maskc >> (ushort2)8);

    ushort2 mask0u = as_ushort2( as_int(masku) | (as_int(masku) << 8) | (as_int(masku) >> 8) );
    ushort2 mask0v = as_ushort2( as_int(maskv) | (as_int(maskv) << 8) | (as_int(maskv) >> 8) );

    *mask0 = masky & masku & maskv;
    *mask1 = masky | masku | maskv;

    *mask0 &= (ushort2)0xcccc;
    *mask1 &= (ushort2)0x3333;

    *mask0 |= *mask1;
}

__kernel void afs_analyze_12_nv16_kernel(
    __global int *restrict ptr_dst,
    __global int *restrict ptr_count,
    __read_only image2d_t img_p0y,
    __read_only image2d_t img_p0c,
    __read_only image2d_t img_p1y,
    __read_only image2d_t img_p1c,
    int tb_order, int width_int, int si_pitch_int, int h,
    uchar thre_Ymotion, uchar thre_Cmotion, uchar thre_deint, uchar thre_shift,
    uint scan_left, uint scan_width, uint scan_top, uint scan_height) {
    __local int shared[BLOCK_INT_X * SHARED_Y * 4]; //int単位でアクセスする
    int lx = get_local_id(0); //スレッド数=BLOCK_INT_X
    int ly = get_local_id(1); //スレッド数=BLOCK_Y
    int gid = get_group_id(1);
    int imgx = get_global_id(0);
    int imgy_start = gid * BLOCK_LOOP_Y * BLOCK_Y + ly;
    int imgy_block_fin = (gid * BLOCK_LOOP_Y + 1) * BLOCK_Y;
    ushort2 motion_count_01 = (ushort2)0;

    __local int *ptr_shared = shared + shared_int_idx(lx,0,0);
    ptr_dst += (imgy_start-4) * si_pitch_int + imgx;

    //前の4ライン分、計算しておく
    //sharedの SHARED_Y-4 ～ SHARED_Y-1 を埋める
    if (ly < 4) {
        uchar4 mask;
        int imgy = imgy_start-4+ly;
        mask = (imgy >= 0) ? analyze(img_p0y, img_p1y, imgx, imgy, tb_order, thre_Ymotion, thre_deint, thre_shift) : (uchar4)0;
        ptr_shared[shared_int_idx(0, -4+ly, 0)] = as_int(mask);
        mask = (imgy >= 0) ? analyze(img_p0c, img_p1c, imgx, imgy, tb_order, thre_Cmotion, thre_deint, thre_shift) : (uchar4)0;
        ptr_shared[shared_int_idx(0, -4+ly, 1)] = as_int(mask);
    }
    ptr_shared[shared_int_idx(0, ly+BLOCK_Y, 2)] = 0;
    ptr_shared[shared_int_idx(0, ly+BLOCK_Y, 3)] = 0;

    int imgy = imgy_start;
    for (uint iloop = 0; iloop <= BLOCK_LOOP_Y; iloop++, ptr_dst += BLOCK_Y * si_pitch_int, imgy += BLOCK_Y, imgy_block_fin += BLOCK_Y, ly += BLOCK_Y) {
        { //差分情報を計算
            uchar4 mask;
            mask = (imgy < h) ? analyze(img_p0y, img_p1y, imgx, imgy, tb_order, thre_Ymotion, thre_deint, thre_shift) : (uchar4)0;
            ptr_shared[shared_int_idx(0, ly, 0)] = as_int(mask);
            mask = (imgy < h) ? analyze(img_p0c, img_p1c, imgx, imgy, tb_order, thre_Cmotion, thre_deint, thre_shift) : (uchar4)0;
            ptr_shared[shared_int_idx(0, ly, 1)] = as_int(mask);
            barrier(CLK_LOCAL_MEM_FENCE);
        }
        { //マスク生成
            ushort2 masky = generate_mask(ly, 0, ptr_shared);
            ushort2 maskc = generate_mask(ly, 1, ptr_shared);
            ushort2 mask0, mask1;
            merge_mask(masky, maskc, &mask0, &mask1);
            ptr_shared[shared_int_idx(0, ly, 2)] = as_int(mask0);
            ptr_shared[shared_int_idx(0, ly, 3)] = as_int(mask1);
            barrier(CLK_LOCAL_MEM_FENCE);
        }
        { //最終出力
            //ly+4とか使っているので準備ができてないうちから、次の列のデータを使うことになってまずい
            ushort2 mask1, mask4, mask5, mask6, mask7;
            mask7 = as_ushort2(ptr_shared[shared_int_idx(0, ly-4, 2)]);
            mask6 = as_ushort2(ptr_shared[shared_int_idx(0, ly-3, 2)]);
            mask5 = as_ushort2(ptr_shared[shared_int_idx(0, ly-2, 2)]);
            mask4 = as_ushort2(ptr_shared[shared_int_idx(0, ly-1, 2)]);
            mask1 = as_ushort2(ptr_shared[shared_int_idx(0, ly+0, 3)]);
            mask4 |= mask5 | mask6;
            mask4 &= (ushort2)0x3333;
            mask1 &= (ushort2)0x3030;
            mask1 |= mask4 | mask7;
            if (imgx < width_int && (imgy - 4) < min(h, imgy_block_fin) && ly - 4 >= 0) {
                //motion_countの実行
                if ((((uint)imgx - scan_left) < scan_width) && (((uint)(imgy - 4) - scan_top) < scan_height)) {
                    ushort2 motion_flag = ((~mask1) & (ushort2)0x4040) >> (ushort2)6;
                    motion_flag = (motion_flag & (ushort2)0xff) + (motion_flag >> (ushort2)8);
                    // 16                     8                    0
                    //  | motion_count_latter | motion_count_first |      
                    motion_count_01 += motion_flag << (((ly + tb_order) & 1) << 3);
                }
                //判定結果の出力
                ptr_dst[0] = as_int(mask1);
            }
            barrier(CLK_LOCAL_MEM_FENCE);
        }
    }

    //motion countの総和演算
    //lyはループを回して加算されているので、get_local_id(1)を取り直す
    const int lid = get_local_id(1) * BLOCK_INT_X + get_local_id(0);
    uint temp = as_uint(motion_count_01);
    // 32              24              16               8              0
    //  | count_latter1 | count_first1 || count_latter0 | count_first0 |
    //
    //  |               |              || count_latter0 | count_first0 |
    //  |               |              ||      +        |      +       |   temp
    //  |               |              || count_latter1 | count_first1 |
    temp = (temp & 0xffff) + (temp >> 16);
    //  |       0       | count_latter ||      0        | count_first |
    shared[lid] = ((temp & 0xff00) << 8) | (temp & 0xff);
    barrier(CLK_LOCAL_MEM_FENCE);
    for (int offset = BLOCK_Y * BLOCK_INT_X >> 1; offset > 0; offset >>= 1) {
        if (lid < offset) {
            shared[lid] += shared[lid + offset];
        }
        barrier(CLK_LOCAL_MEM_FENCE);
    }
    if (lid == 0) {
        ptr_count[get_group_id(1) * get_num_groups(0) + get_group_id(0)] = shared[0];
    }
}

#endif //#ifdef __OPENCL_VERSION__
