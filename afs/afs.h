#pragma once

#include "filter.h"

#define AFS_SOURCE_CACHE_NUM 16
#define AFS_SCAN_CACHE_NUM   16
#define AFS_SCAN_WORKER_MAX  16
#define AFS_STRIPE_CACHE_NUM  8
#define AFS_SUB_WORKER_MAX    4

enum {
    AFS_MODE_YUY2UP      = 0x01,
    AFS_MODE_CACHE_YC48  = 0x00,
    AFS_MODE_CACHE_NV16  = 0x02,
    AFS_MODE_AVIUTL_YC48 = 0x00,
    AFS_MODE_AVIUTL_YUY2 = 0x04,
    AFS_MODE_YC48_YC48UP = AFS_MODE_AVIUTL_YC48 | AFS_MODE_CACHE_YC48 | AFS_MODE_YUY2UP,
    AFS_MODE_YC48_YC48   = AFS_MODE_AVIUTL_YC48 | AFS_MODE_CACHE_YC48,
    AFS_MODE_YC48_NV16UP = AFS_MODE_AVIUTL_YC48 | AFS_MODE_CACHE_NV16 | AFS_MODE_YUY2UP,
    AFS_MODE_YC48_NV16   = AFS_MODE_AVIUTL_YC48 | AFS_MODE_CACHE_NV16,
};

typedef struct {
    int top, bottom, left, right;
} AFS_SCAN_CLIP;

static inline AFS_SCAN_CLIP scan_clip(int top, int bottom, int left, int right) {
    AFS_SCAN_CLIP clip;
    clip.top = top;
    clip.bottom = bottom;
    clip.left = left;
    clip.right = right;
    return clip;
}

typedef struct {
    int type;
    unsigned char *dst;
    void *p0;
    void *p1;
    int tb_order;
    int max_w;
    int si_pitch;
    AFS_SCAN_CLIP *clip;
} AFS_SCAN_ARG;

typedef struct {
    unsigned char *map;
    int status, frame, mode, tb_order, thre_shift, thre_deint, thre_Ymotion, thre_Cmotion;
    AFS_SCAN_CLIP clip;
    int ff_motion, lf_motion;
} AFS_SCAN_DATA;

typedef struct {
    unsigned char *map;
    int status, frame, count0, count1;
} AFS_STRIPE_DATA;

// 後方フィールド判定
static inline BOOL is_latter_field(int pos_y, int tb_order) {
    return ((pos_y & 1) == tb_order);
}

const int BLOCK_SIZE_YCP = 256;

#define ENABLE_SUB_THREADS 1 //サブスレッドを有効にする
#define NUM_SUB_THREAD     1 //サブスレッド数の指定 0でトラックバーによる設定が可能に
#define SCAN_BACKGROUND    0 //scan処理をバックグラウンドで行う (バグってるので使用中止)
#define ANALYZE_BACKGROUND (0 & SCAN_BACKGROUND) //analyze処理をバックグラウンドで行う (バグってるので使用中止)
#define BACKGROUND_THREAD_BELOW_NORMAL 0 //バックグラウンドスレッドの優先度を下げる
#define SIMD_DEBUG         0 //SIMD処理をデバッグする
#define CHECK_PERFORMANCE  0 //パフォーマンスレポートを出力する

#if ENABLE_SUB_THREADS
//サブスレッド scan_frame スレッド用 --------------------------------------------------------
typedef struct SYNTHESIZE_TASK {
    FILTER_PROC_INFO *fpip;
    void *p0, *p1;
    unsigned char *sip;
    unsigned char status;
    int si_w;
    int tb_order;
    int mode;
    BOOL detect_sc;
    AFS_SCAN_CLIP clip;
} SYNTHESIZE_TASK;

typedef struct MERGE_SCAN_TASK {
    AFS_STRIPE_DATA *sp;
    AFS_SCAN_DATA *sp0, *sp1;
    int si_w;
} MERGE_SCAN_TASK;

typedef struct YUY2UPSAMPLE_TASK {
    void *dst, *src;
    int dst_pitch, max_h;
    int width, src_pitch, height;
} YUY2UPSAMPLE_TASK;

enum SUB_THREAD_TASK {
    TASK_NONE = 0,
    TASK_MERGE_SCAN,
    TASK_SYNTHESIZE,
    TASK_YUY2UP,
};

typedef struct AFS_SUB_THREAD {
    SUB_THREAD_TASK sub_task;
    YUY2UPSAMPLE_TASK yuy2up_task;
    MERGE_SCAN_TASK merge_scan_task;
    SYNTHESIZE_TASK synthesize_task;
    int thread_sub_n;
    BOOL thread_sub_abort;
    HANDLE hThread_sub[AFS_SUB_WORKER_MAX-1];
    HANDLE hEvent_sub_start[AFS_SUB_WORKER_MAX-1];
    HANDLE hEvent_sub_fin[AFS_SUB_WORKER_MAX-1];
} AFS_SUB_THREAD;

unsigned int __stdcall sub_thread(void *prm);
#endif //#if ENABLE_SUB_THREADS


//解析スレッド管理 scan_frame スレッド用 --------------------------------------------------------

#define AFS_MAX_BACKGROUND_TASK  AFS_SOURCE_CACHE_NUM

enum {
    TASK_TYPE_UNKNOWN = 0,
    TASK_TYPE_SCAN,
    TASK_TYPE_ANALYZE,
};

typedef struct BACKGROUND_TASK {
    HANDLE hEvent_start; //タスク開始
    HANDLE hEvent_fin;  //タスクの終了
    union {
        //scan_task
        struct {
            int i_frame, force, max_w;
            PIXEL_YC *p1, *p0;
            int mode, tb_order, thre_shift, thre_deint, thre_Ymotion, thre_Cmotion;
            AFS_SCAN_CLIP *mc_clip;
        } scan;
        //analyze_task
        struct {
            int frame, drop, smooth, force24, coeff_shift, method_watershed;
            int *reverse;
            int frame_n, replay_mode;
        } analyze;
    };
} BACKGROUND_TASK;

#if SCAN_BACKGROUND
typedef struct BACKGROUND_THREAD {
    HANDLE hThread;     //スレッドのハンドル
    DWORD work_task_id; //現在処理中のタスクID
    DWORD set_task_id;  //追加するタスクのID
    BOOL abort;         //スレッド中断(終了)指示用
    BACKGROUND_TASK task_array[AFS_MAX_BACKGROUND_TASK]; //タスクを格納する配列
} BACKGROUND_THREAD;

#define scantask_workp (scan_thread.task_array + ((scan_thread.work_task_id)&(AFS_MAX_BACKGROUND_TASK-1))) //次に処理するタスクへのポインタ
#define scantask_setp  (scan_thread.task_array + ((scan_thread.set_task_id) &(AFS_MAX_BACKGROUND_TASK-1))) //次に指示を格納する空きタスクへのポインタ

unsigned int __stdcall scan_frame_thread(void *prm);

#if ANALYZE_BACKGROUND
#define analyzetask_workp (analyze_thread.task_array + ((analyze_thread.work_task_id)&(AFS_MAX_BACKGROUND_TASK-1))) //次に処理するタスクへのポインタ
#define analyzetask_setp  (analyze_thread.task_array + ((analyze_thread.set_task_id) &(AFS_MAX_BACKGROUND_TASK-1))) //次に指示を格納する空きタスクへのポインタ

unsigned int __stdcall analyze_frame_thread(void *prm);
#endif //ANALYZE_BACKGROUND
#endif //SCAN_BACKGROUND

typedef struct {
    void *map;
    int status, frame, file_id, video_number, yuy2upsample;
} AFS_SOURCE_DATA;


typedef struct AFS_CONTEXT {
#if ENABLE_SUB_THREADS
    AFS_SUB_THREAD sub_thread;
#endif //ENABLE_SUB_THREADS
#if SCAN_BACKGROUND
    static BACKGROUND_THREAD scan_thread;
#if ANALYZE_BACKGROUND
    static BACKGROUND_THREAD analyze_thread;
#endif //ANALYZE_BACKGROUND
#endif //SCAN_BACKGROUND

    int cache_nv16;
    // インタレース解除フィルタ用ソースキャッシュ
    unsigned int mode;
    int source_frame_n;
    int source_w;
    int source_h;
    AFS_SOURCE_DATA source_array[AFS_SOURCE_CACHE_NUM];

    // 縞、動きスキャン＋合成縞情報キャッシ

    unsigned char* analyze_cachep[AFS_SCAN_CACHE_NUM + AFS_STRIPE_CACHE_NUM];
    PIXEL_YC* scan_workp;
    int scan_worker_n;
    int scan_frame_n, scan_w, scan_h;
    HANDLE hThread_worker[AFS_SCAN_WORKER_MAX];
    HANDLE hEvent_worker_awake[AFS_SCAN_WORKER_MAX];
    HANDLE hEvent_worker_sleep[AFS_SCAN_WORKER_MAX];
    int worker_thread_priority[AFS_SCAN_WORKER_MAX];
    int thread_motion_count[AFS_SCAN_WORKER_MAX][2];
    AFS_SCAN_ARG scan_arg;

    AFS_SCAN_DATA scan_array[AFS_SCAN_CACHE_NUM];
    int scan_motion_count[AFS_SCAN_CACHE_NUM][2];
    AFS_SCAN_CLIP scan_motion_clip[AFS_SCAN_CACHE_NUM];

    AFS_STRIPE_DATA stripe_array[AFS_STRIPE_CACHE_NUM];
} AFS_CONTEXT;

enum {
    QPC_START = 0,
    QPC_INIT,
    QPC_YCP_CACHE,
    QPC_SCAN_FRAME,
    QPC_COUNT_MOTION,
    QPC_ANALYZE_FRAME,
    QPC_STRIP_COUNT,
    QPC_MAP_FILTER,
    QPC_BLEND
};

#if CHECK_PERFORMANCE
typedef struct {
    __int64 tmp[16];
    __int64 value[15];
    __int64 freq;
} PERFORMANCE_CHECKER;

static PERFORMANCE_CHECKER afs_qpc = { 0 };

#define QPC_FREQ QueryPerformanceFrequency((LARGE_INTEGER *)&afs_qpc.freq)
#define QPC_GET_COUNTER(idx) QueryPerformanceCounter((LARGE_INTEGER *)&afs_qpc.tmp[idx])
#define QPC_ADD(idx, after, before) { afs_qpc.value[idx] += (afs_qpc.tmp[after] - afs_qpc.tmp[before]); }
#define QPC_MS(idx) (afs_qpc.value[idx] * 1000.0 / (double)afs_qpc.freq)
#else
#define QPC_FREQ
#define QPC_GET_COUNTER(x)
#define QPC_ADD(idx, after, before)
#define QPC_MS(idx)
#endif
