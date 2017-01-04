#pragma once

#include <CL/cl.h>
#include <thread>
#include <future>

struct AFS_CONTEXT;

enum {
    AFS_OPENCL_DEVICE_UNCHECKED = 0,
    AFS_OPENCL_DEVICE_CHECK_FAIL
};

typedef struct _AFS_OPENCL {
    int device_check; //AFS_OPENCL_DEVICE_xxx
    cl_platform_id platform;
    cl_device_id device;
    cl_context ctx;
    cl_command_queue queue;
    cl_program program_analyze[2]; //それぞれtb_order = 0,1 用
    cl_kernel kernel_analyze[2];   //それぞれtb_order = 0,1 用
    cl_program program_merge_scan;
    cl_kernel kernel_merge_scan;
    std::future<cl_int> build_ret;                //program_xxx/kernel_xxxの非同期でのビルド結果を格納
    cl_mem source_img[AFS_SOURCE_CACHE_NUM][2];   //[0]=nv16の輝度, [1]=(PREFER_IMAGE＝1の場合、nv16の色差)/(PREFER_IMAGE＝0の場合、未使用)
    cl_mem source_buf[AFS_SOURCE_CACHE_NUM][2];   //[0]=nv16の輝度, [1]=(PREFER_IMAGE＝1の場合、nv16の色差)/(PREFER_IMAGE＝0の場合、未使用)
    int source_w, source_h;                       //source_imgのwidth(生のピクセル数)とheight
    cl_mem scan_mem[AFS_SCAN_CACHE_NUM];
    cl_mem stripe_mem[AFS_STRIPE_CACHE_NUM];
    int scan_w, scan_h;                           //scan_mem, stripe_memのwidth(生のピクセル数)とheight
    cl_mem motion_count_temp[AFS_SCAN_CACHE_NUM]; //scan_frameのmotion_count用
    unsigned short *motion_count_temp_map[AFS_SCAN_CACHE_NUM]; //scan_frameのmotion_count用
    int motion_count_temp_max; //motion_count_temp_mapの最大要素数(確保したメモリの量)
    HMODULE hModuleDLL;        //afs.aufのハンドル (リソース取得時に使用)
    bool bSVMAvail;            //SVM (Fine Grain)が利用可能かどうか
} AFS_OPENCL;

int afs_opencl_open_device(AFS_CONTEXT *afs, HMODULE hModuleDLL);
int afs_opencl_init(AFS_CONTEXT *afs);

int afs_opencl_source_buffer_pitch(AFS_CONTEXT *afs, int w, int h, int *pitch, int *baseAddressAlign);

int afs_opencl_create_source_buffer(AFS_CONTEXT *afs, int w, int h);
int afs_opencl_create_scan_buffer(AFS_CONTEXT *afs, int si_w, int h);

cl_int afs_opencl_source_buffer_map(AFS_CONTEXT *afs, int i);
cl_int afs_opencl_source_buffer_unmap(AFS_CONTEXT *afs, int i, bool force = false);
int    afs_opencl_source_buffer_index(AFS_CONTEXT *afs, void *p0);
cl_int afs_opencl_scan_buffer_map(AFS_CONTEXT *afs, int i);
cl_int afs_opencl_scan_buffer_unmap(AFS_CONTEXT *afs, int i, bool force = false);
int    afs_opencl_scan_buffer_index(AFS_CONTEXT *afs, void *p0);
cl_int afs_opencl_count_motion_temp_map(AFS_CONTEXT *afs, int i);
cl_int afs_opencl_count_motion_temp_unmap(AFS_CONTEXT *afs, int i, bool force = false);
cl_int afs_opencl_stripe_buffer_map(AFS_CONTEXT *afs, int i);
cl_int afs_opencl_stripe_buffer_unmap(AFS_CONTEXT *afs, int i, bool force = false);
int    afs_opencl_stripe_buffer_index(AFS_CONTEXT *afs, void *p0);

cl_int afs_opencl_queue_finish(AFS_CONTEXT *afs);

void afs_opencl_release_buffer(AFS_CONTEXT *afs);

int afs_opencl_analyze_12_nv16(AFS_CONTEXT *afs, int dst_idx, int p0_idx, int p1_idx, int tb_order, int width, int si_pitch, int h, int max_h,
    int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion, const void *scan_clip, int *global_block_count);
int afs_opencl_merge_scan_nv16(AFS_CONTEXT *afs, int dst_idx, int p0_idx, int p1_idx, int si_w, int h);
