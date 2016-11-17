#ifndef AFSVF
// 自動フィールドシフト インターレース解除プラグイン for AviUtl
#else
// 自動フィールドシフト ビデオフィルタプラグイン for AviUtl
#endif

#include <windows.h>
#include <process.h>
#include <stdio.h>
#include <stdarg.h>
#include "filter.h"
#include "afs_server.h"

#include "afs.h"
#include "afs_func.h"

#include "afs_mmx.h"
#include "afs_simd.h"
#include "afs_filter_mmx.h"
#include "afs_filter_simd.h"
#include "afs_analyze_mmx.h"
#include "afs_analyze_simd.h"
#include "afs_yuy2up_mmx.h"
#include "afs_yuy2up_simd.h"


#ifndef AFSVF
static TCHAR filter_name[] = "自動フィールドシフト";
#ifndef AFSNFS
static TCHAR filter_name_ex[] = "自動フィールドシフト インタレース解除 ver7.5a+ by Aji";
#else
static TCHAR filter_name_ex[] = "自動フィールドシフト インタレース解除 ver7.5a- by Aji";
#endif
#else // AFSVF
static char filter_name[] = "自動フィールドシフトVF";
#ifndef AFSNFS
static TCHAR filter_name_ex[] = "自動フィールドシフト ビデオフィルタ ver7.5a+ by Aji";
#else
static TCHAR filter_name_ex[] = "自動フィールドシフト ビデオフィルタ ver7.5a- by Aji";
#endif
#endif // AFSVF

#define max2(x,y) (((x)>=(y))?(x):(y))
#define max3(x,y,z) (max2((x),max2((y),(z))))
#define absdiff(x,y) (((x)>=(y))?(x)-(y):(y)-(x))

AFS_CONTEXT g_afs;
AFS_FUNC afs_func = { 0 };
#define sourcep(x) (g_afs.source_array+((x)&(AFS_SOURCE_CACHE_NUM-1)))
#define scanp(x)   (g_afs.scan_array  +((x)&(AFS_SCAN_CACHE_NUM-1)))
#define stripep(x) (g_afs.stripe_array+((x)&(AFS_STRIPE_CACHE_NUM-1)))

static inline func_yuy2up get_func_copy_frame() {
    return afs_func.yuy2up[g_afs.afs_mode & ~(AFS_MODE_YUY2UP|AFS_MODE_CACHE_NV16)];
}

static inline int afs_cache_nv16(unsigned int afs_mode) {
    static_assert(AFS_MODE_CACHE_NV16 == 0x02, "AFS_MODE_CACHE_NV16 is not 0x02, afs_cache_nv16() will fail.");
    return (afs_mode & AFS_MODE_CACHE_NV16) >> 1;
}

static inline int si_pitch(int x, unsigned int afs_mode) {
    int align_minus_one = afs_func.analyze[afs_cache_nv16(afs_mode)].align_minus_one;
    return (x + align_minus_one) & (~align_minus_one);
}

static __declspec(noinline) void avx2_dummy_if_avail() {
    if (afs_func.simd_avail & AVX2) {
        __asm {
            vpxor ymm0, ymm0, ymm0
            vzeroupper
        };
    }
}

static __declspec(noinline) void error_message(FILTER *fp, LPTSTR m) {
    fp->exfunc->ini_save_str(fp, "error_message", m);
    
    TCHAR t[1024];
    int l = GetDateFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, NULL, NULL, t, 1024);
    if (l) t[l-1] = ' ';
    t[l] = 0;
    l += GetTimeFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, NULL, NULL, t+l, 1024-l);
    fp->exfunc->ini_save_str(fp, "error_time", t);

    return;
}

static __declspec(noinline) void error_message_box(int line, const char *format, ...) {
    va_list args;
    va_start(args, format);
    int buf_size = _vscprintf(format, args) + 256;// _vscprintf doesn't count terminating '\0'
    char *buffer = (char *)malloc(buf_size * sizeof(buffer[0]));
    if (buffer == nullptr)
        return;

    int len = sprintf_s(buffer, buf_size, "Error on line %d\n", line);

    vsprintf_s(buffer + len, buf_size - len, format, args);
    MessageBox(0, buffer, "afs", 0);
    free(buffer);
}

static __declspec(noinline) void error_modal(FILTER *fp, void *editp, LPTSTR m) {
    if (fp->exfunc->is_editing(editp) && !fp->exfunc->is_saving(editp))
        MessageBox(fp->hwnd, m, filter_name, MB_OK|MB_ICONSTOP);
    else
        error_message(fp, m);

    return;
}

// ログ関連

static int log_start_frame, log_end_frame, log_save_check;

static BOOL save_log(FILTER *fp, void *editp) {
    TCHAR path[MAX_PATH];
    SYS_INFO sys_info;
    FILE_INFO file_info;
    fp->exfunc->get_sys_info(editp, &sys_info);
    fp->exfunc->get_file_info(editp, &file_info);
    int frame_n = fp->exfunc->get_frame_n(editp);
    wsprintf(path, "%s.afs", sys_info.output_name);

    HANDLE fh = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fh == INVALID_HANDLE_VALUE) {
        error_message(fp, "ログファイルの作成に失敗しました。");
        return FALSE;
    }
    wsprintf(path, "afs7            %14d  %14d  %14d  ",
        log_end_frame + 1 - log_start_frame, file_info.video_rate, file_info.video_scale);
    path[14] = 13, path[15] = 10, path[30] = 13, path[31] = 10;
    path[46] = 13, path[47] = 10, path[62] = 13, path[63] = 10, path[64] = 0;
    
    DWORD dw;
    WriteFile(fh, path, 64, &dw, NULL);
    path[7] = 13, path[8] = 10, path[9] = 0;
    for (int i = log_start_frame; i <= log_end_frame; i++) {
        unsigned char status = afs_get_status(frame_n, i);
        path[0] = (status & AFS_FLAG_SHIFT0     ) ? '1' : '0';
        path[1] = (status & AFS_FLAG_SHIFT1     ) ? '1' : '0';
        path[2] = (status & AFS_FLAG_SHIFT2     ) ? '1' : '0';
        path[3] = (status & AFS_FLAG_SHIFT3     ) ? '1' : '0';
        path[4] = (status & AFS_FLAG_FRAME_DROP ) ? '1' : '0';
        path[5] = (status & AFS_FLAG_SMOOTHING  ) ? '1' : '0';
        path[6] = (status & AFS_FLAG_FORCE24    ) ? '1' : '0';
        WriteFile(fh, &path, 9, &dw, NULL);
    }
    CloseHandle(fh);

    return TRUE;
}

static BOOL load_log(FILTER *fp, void *editp) {
    TCHAR path[MAX_PATH];
    SYS_INFO sys_info;
    FILE_INFO file_info;
    fp->exfunc->get_sys_info(editp, &sys_info);
    fp->exfunc->get_file_info(editp, &file_info);
    int frame_n = fp->exfunc->get_frame_n(editp);
    wsprintf(path, "%s.afs", sys_info.edit_name);
    HANDLE hf = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hf == INVALID_HANDLE_VALUE) {
        error_message(fp, "ログファイルが開けません。");
        goto log_err;
    }
    
    DWORD dw;
    ReadFile(hf, path, 64, &dw, NULL);
    if (*(LPDWORD)path != '7sfa' || dw != 64) {
        error_message(fp, "自動フィールドシフトv7以降のログファイルではありません。");
        return FALSE;
    }
    int frame = 0;
    for (int i = 16; i < 30; i++)
        frame = frame * 10 + ((path[i] > '0' && path[i] <= '9') ? path[i] - '0' : 0);
    if (frame != frame_n) {
        error_message(fp, "フレーム数がログと一致しません。");
        CloseHandle(hf);
        goto log_err;
    }

    int start = 0;
    for (int i = 0; i < frame; i++) {
        ReadFile(hf, path, 9, &dw, NULL);
        if (dw != 9 || path[7] != 13 || path[8] != 10){
            error_message(fp, "ログファイルが壊れています。");
            CloseHandle(hf);
            start = i;
            goto log_err;
        }
        unsigned char status = AFS_STATUS_DEFAULT |
            ((path[0] == '1') ? AFS_FLAG_SHIFT0     : 0) |
            ((path[1] == '1') ? AFS_FLAG_SHIFT1     : 0) |
            ((path[2] == '1') ? AFS_FLAG_SHIFT2     : 0) |
            ((path[3] == '1') ? AFS_FLAG_SHIFT3     : 0) |
            ((path[4] == '1') ? AFS_FLAG_FRAME_DROP : 0) |
            ((path[5] == '1') ? AFS_FLAG_SMOOTHING  : 0) |
            ((path[6] == '1') ? AFS_FLAG_FORCE24    : 0);
        afs_set(frame_n, i, status);
    }

    CloseHandle(hf);

    return FALSE;

log_err:
    for (int i = start; i < frame_n; i++)
        afs_set(frame_n, i, AFS_STATUS_DEFAULT);
    return FALSE;
}

#if SIMD_DEBUG
static BYTE *debug_buf = nullptr;
static DWORD debug_buf_size = 0;

static BYTE *debug_buf_analyze = nullptr;
static DWORD debug_buf_analyze_size = 0;

template<typename T>
BOOL compare_frame(T *ptr0, T *ptr1, int width, int pitch, int height) {
    BOOL error = FALSE;
    for (int i = 0; i < height; i++)
        error |= (memcmp(ptr0 + i * pitch, ptr1 + i * pitch, sizeof(T) * width));
    return error;
}

BYTE *get_debug_buffer(DWORD size) {
    if (debug_buf_size < size) {
        if (debug_buf != nullptr)
            _aligned_free(debug_buf);
        debug_buf = (unsigned char *)_aligned_malloc(size, 32);
        debug_buf_size = (debug_buf != nullptr) ? size : 0;
    }
    return debug_buf;
}
BYTE *get_debug_analyze_buffer(DWORD size) {
    if (debug_buf_analyze_size < size) {
        if (debug_buf_analyze != nullptr)
            _aligned_free(debug_buf_analyze);
        debug_buf_analyze = (unsigned char *)_aligned_malloc(size, 32);
        debug_buf_analyze_size = (debug_buf_analyze != nullptr) ? size : 0;
    }
    return debug_buf_analyze;
}
#endif

#ifndef AFSVF
void clear_source_cache(void) {
    for (int i = 0; i < AFS_SOURCE_CACHE_NUM; i++)
        g_afs.source_array[i].status = 0;
}

void source_cache_expire(int frame) {
    AFS_SOURCE_DATA* srp = sourcep(frame);
    srp->status = 0;
}

void free_source_cache(void) {
    clear_source_cache();
    if (g_afs.source_array[0].map != NULL) {
        for (int i = 0; i < AFS_SOURCE_CACHE_NUM; i++) {
            if (g_afs.source_array[i].map) {
                _aligned_free(g_afs.source_array[i].map);
                g_afs.source_array[i].map = NULL;
            }
        }
    }
}

BOOL set_source_cache_size(int frame_n, int max_w, int max_h, int afs_mode) {
    const int source_w = si_pitch(max_w, afs_mode);
    const int cache_nv16 = (afs_mode & AFS_MODE_CACHE_NV16) != 0;
    const int size = source_w * max_h;

    if (g_afs.source_array[0].map != NULL) {
        if ((frame_n != 0 && g_afs.source_frame_n != 0 && g_afs.source_frame_n != frame_n) || g_afs.source_w != source_w || g_afs.source_h != max_h || g_afs.cache_nv16 != cache_nv16) {
            free_source_cache();
        }
    }

    const int frame_size_bytes = size * ((cache_nv16) ? 2 : 6);
    if (g_afs.source_array[0].map == NULL) {
        for (int i = 0; i < AFS_SOURCE_CACHE_NUM; i++) {
            if (NULL == (g_afs.source_array[i].map = _aligned_malloc(frame_size_bytes, 64))) {
                free_source_cache();
                return FALSE;
            }
            ZeroMemory(g_afs.source_array[i].map, frame_size_bytes);
            g_afs.source_array[i].status = 0;
        }
    }

    if (frame_n > 0 || g_afs.source_frame_n < 0) g_afs.source_frame_n = frame_n;
    g_afs.source_w = source_w;
    g_afs.source_h = max_h;
    g_afs.cache_nv16 = cache_nv16;

    return TRUE;
}

#if ENABLE_SUB_THREADS
void __stdcall yuy2up(int thread_id) {
    const int y_start = (g_afs.sub_thread.yuy2up_task.height *  thread_id   ) / g_afs.sub_thread.thread_sub_n;
    const int y_end   = (g_afs.sub_thread.yuy2up_task.height * (thread_id+1)) / g_afs.sub_thread.thread_sub_n;
    afs_func.yuy2up[g_afs.sub_thread.afs_mode](g_afs.sub_thread.yuy2up_task.dst, g_afs.sub_thread.yuy2up_task.dst_pitch, g_afs.sub_thread.yuy2up_task.dst_pitch * g_afs.sub_thread.yuy2up_task.max_h,
        g_afs.sub_thread.yuy2up_task.src, g_afs.sub_thread.yuy2up_task.width, g_afs.sub_thread.yuy2up_task.src_pitch, y_start, y_end);
}
#endif

void* get_source_cache(FILTER *fp, void *editp, int frame, int w, int max_w, int h, int *hit) {
    int file_id, video_number;
#ifndef AFSNFS
    if (fp->exfunc->get_source_video_number(editp, frame, &file_id, &video_number) != TRUE)
#endif
        file_id = video_number = 0;
    int yuy2upsample = fp->check[10] ? 1 : 0;

    AFS_SOURCE_DATA *srp = sourcep(frame);

    if (srp->status > 0 && srp->frame == frame && srp->file_id == file_id &&
        srp->video_number == video_number && srp->yuy2upsample == yuy2upsample) {
            if(hit != NULL) *hit = 1;
            return srp->map;
    }
    if (hit != NULL) *hit = 0;

    void *dst = srp->map;
    void *src = fp->exfunc->get_ycp_source_cache(editp, frame, 0);
#if SIMD_DEBUG
    PIXEL_YC *test_buf = (PIXEL_YC *)get_debug_buffer(sizeof(PIXEL_YC) * g_afs.source_w * (h + 2));
    afs_yuy2up_frame_mmx(test_buf, g_afs.source_w, g_afs.source_w * g_afs.source_h, src, w, max_w, h, 0, h);
    afs_func.yuy2up(dst, g_afs.source_w, g_afs.source_w * g_afs.source_h, src, w, max_w, h, 0, h);
    if (compare_frame(dst, test_buf, w, g_afs.source_w, h))
        error_message_box(__LINE__, "afs_func.yuy2up");
#endif
#if ENABLE_SUB_THREADS
    g_afs.sub_thread.yuy2up_task.dst = dst;
    g_afs.sub_thread.yuy2up_task.dst_pitch = g_afs.source_w;
    g_afs.sub_thread.yuy2up_task.max_h = g_afs.source_h;
    g_afs.sub_thread.yuy2up_task.src = src;
    g_afs.sub_thread.yuy2up_task.width = w;
    g_afs.sub_thread.yuy2up_task.src_pitch = max_w;
    g_afs.sub_thread.yuy2up_task.height = h;
    g_afs.sub_thread.afs_mode = g_afs.afs_mode;
    g_afs.sub_thread.sub_task = TASK_YUY2UP;
        
    //g_afs.sub_thread.thread_sub_nは総スレッド数
    //自分を除いた数を起動
    for (int ith = 0; ith < g_afs.sub_thread.thread_sub_n - 1; ith++) {
        SetEvent(g_afs.sub_thread.hEvent_sub_start[ith]);
    }
    //メインスレッド(自分)がスレッドID0を担当
    yuy2up(0);
    //1スレッド(つまり自スレッドのみなら同期は必要ない)
    if (0 < g_afs.sub_thread.thread_sub_n - 1)
        WaitForMultipleObjects(g_afs.sub_thread.thread_sub_n - 1, g_afs.sub_thread.hEvent_sub_fin, TRUE, INFINITE);
#if SIMD_DEBUG
    if (compare_frame(dst, test_buf, w, g_afs.source_w, h))
        error_message_box(__LINE__, "afs_func.yuy2up_mt");
#endif //SIMD_DEBUG
#else //ENABLE_SUB_THREADS
    afs_func.yuy2up(dst, g_afs.source_w, g_afs.source_w * g_afs.source_h, src, w, max_w, h, 0, h);
#endif //ENABLE_SUB_THREADS

    srp->status = 1;
    srp->frame = frame;
    srp->file_id = file_id;
    srp->video_number = video_number;
    srp->yuy2upsample = yuy2upsample;

    return srp->map;
}
#endif

void fill_this_ycp(FILTER *fp, FILTER_PROC_INFO *fpip) {
#ifndef AFSVF
    void *ycp = fp->exfunc->get_ycp_source_cache(fpip->editp, fpip->frame, 0);
#else
    void *ycp = fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &fpip->w, &fpip->h);
#endif

    if (ycp != NULL)
        memcpy_sse<true>(fpip->ycp_edit, ycp, fpip->yc_size * fpip->h * fpip->max_w);
    else
        error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");

    return;
}

void* get_ycp_cache(FILTER *fp, FILTER_PROC_INFO *fpip, int frame, int *hit) {
    int upper_limit = fpip->frame_n - 1;
    frame = max(0, min(frame, upper_limit));

#ifndef AFSVF
    return get_source_cache(fp, fpip->editp, frame, fpip->w, fpip->max_w, fpip->h, hit);
#else
    if (hit != NULL) *hit = 1;
    if (frame == fpip->frame)
        return fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, frame, &fpip->w, &fpip->h);
    else
        return fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, frame, NULL, NULL);
#endif
}

#ifndef AFSNFS
int is_frame_reverse(FILTER *fp, FILTER_PROC_INFO *fpip, int frame) {
    if (frame < 0 || frame >= fpip->frame_n)
        return 0;
    
    FRAME_STATUS fs;
    if (!fp->exfunc->get_frame_status(fpip->editp, frame, &fs)) {
        error_modal(fp, fpip->editp, "フレームステータスが取得できませんでした。");
        return 0;
    } else {
        return (fs.inter == FRAME_STATUS_INTER_REVERSE) ? 1 : 0;
    }
}
#else
int is_frame_reverse(FILTER*, FILTER_PROC_INFO*, int) {
    return 0;
}
#endif

void stripe_info_dirty(int frame) {
    AFS_STRIPE_DATA *stp = stripep(frame);
    if (stp->frame == frame && stp->status > 1)
        stp->status = 1;
}

void stripe_info_expire(int frame) {
    AFS_STRIPE_DATA *stp = stripep(frame);
    if (stp->frame == frame && stp->status > 0)
        stp->status = 0;
}

void free_analyze_cache() {
    g_afs.scan_frame_n = -1;
    for (int i = 0; i < AFS_SCAN_CACHE_NUM; i++)
        g_afs.scan_array[i].status = 0;
    for (int i = 0; i < AFS_STRIPE_CACHE_NUM; i++)
        g_afs.stripe_array[i].status = 0;

    if (g_afs.analyze_cachep[0] != nullptr) {
        g_afs.scan_arg.type = -1;
        for (int i = 0; i < g_afs.scan_worker_n; i++)
            SetEvent(g_afs.hEvent_worker_awake[i]);
        WaitForMultipleObjects(g_afs.scan_worker_n, g_afs.hThread_worker, TRUE, INFINITE);
        for (int i = 0; i < g_afs.scan_worker_n; i++){
            CloseHandle(g_afs.hThread_worker[i]);
            CloseHandle(g_afs.hEvent_worker_awake[i]);
            CloseHandle(g_afs.hEvent_worker_sleep[i]);
        }
        for (int i = 0; i < _countof(g_afs.analyze_cachep); i++) {
            if (nullptr != g_afs.analyze_cachep[i]) {
                _aligned_free(g_afs.analyze_cachep[i]);
            }
        }
        memset(g_afs.analyze_cachep, 0, sizeof(g_afs.analyze_cachep));
        if (g_afs.scan_workp)
            _aligned_free(g_afs.scan_workp);
        g_afs.scan_workp = nullptr;
    }
}

void free_sub_thread() {
#if ENABLE_SUB_THREADS
    if (0 < g_afs.sub_thread.thread_sub_n - 1) {
        g_afs.sub_thread.thread_sub_abort = TRUE;
        for (int ith = 0; ith < g_afs.sub_thread.thread_sub_n - 1; ith++) {
            if (g_afs.sub_thread.hEvent_sub_start[ith]) {
                SetEvent(g_afs.sub_thread.hEvent_sub_start[ith]);
            }
        }
        WaitForMultipleObjects(g_afs.sub_thread.thread_sub_n - 1, g_afs.sub_thread.hThread_sub, TRUE, INFINITE);
        for (int ith = 0; ith < g_afs.sub_thread.thread_sub_n - 1; ith++) {
            if (g_afs.sub_thread.hThread_sub[ith])
                CloseHandle(g_afs.sub_thread.hThread_sub[ith]);
            if (g_afs.sub_thread.hEvent_sub_start[ith])
                CloseHandle(g_afs.sub_thread.hEvent_sub_start[ith]);
            if (g_afs.sub_thread.hEvent_sub_fin[ith])
                CloseHandle(g_afs.sub_thread.hEvent_sub_fin[ith]);
        }
    }
    ZeroMemory(g_afs.sub_thread.hThread_sub, sizeof(g_afs.sub_thread.hThread_sub));
    ZeroMemory(g_afs.sub_thread.hEvent_sub_start, sizeof(g_afs.sub_thread.hEvent_sub_start));
    ZeroMemory(g_afs.sub_thread.hEvent_sub_fin, sizeof(g_afs.sub_thread.hEvent_sub_fin));
    g_afs.sub_thread.thread_sub_abort = FALSE;
#endif
}

void setup_sub_thread(int sub_thread_n) {
#if ENABLE_SUB_THREADS
    if (g_afs.sub_thread.thread_sub_n != sub_thread_n) {
        free_sub_thread();
    
        g_afs.sub_thread.thread_sub_n = sub_thread_n;
        if (NULL == g_afs.sub_thread.hThread_sub[0]) {
            for (int i = 0; i < g_afs.sub_thread.thread_sub_n - 1; i++) {
                g_afs.sub_thread.hEvent_sub_start[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
                g_afs.sub_thread.hEvent_sub_fin[i]   = CreateEvent(NULL, FALSE, FALSE, NULL);
                g_afs.sub_thread.hThread_sub[i] = (HANDLE)_beginthreadex(NULL, 0, sub_thread, (LPVOID)(i + 1), 0, NULL);
            }
        }
    }
#endif
}

// フィルタ定義


#if ENABLE_SUB_THREADS && !NUM_SUB_THREAD
#define TRACK_N 13
TCHAR *track_name[] = { "上", "下", "左", "右", "切替点", "判定比", "縞(ｼﾌﾄ)", "縞(解除)", "Y動き", "C動き", "解除Lv", "ｽﾚｯﾄﾞ数",       "ｻﾌﾞｽﾚｯﾄﾞ" };
int track_s[]       = {    0,    0,    0,    0,       0,        0,         0,          0,       0,       0,        0,         1,              1    };
int track_default[] = {   16,   16,   32,   32,       0,      192,       128,         64,     128,     256,        4,         2,              2    };
int track_e[]       = {  512,  512,  512,  512,     256,      256,      1024,       1024,    1024,    1024,        5, AFS_SCAN_WORKER_MAX, AFS_SUB_WORKER_MAX };
#else
#define TRACK_N 12
TCHAR *track_name[] = { "上", "下", "左", "右", "切替点", "判定比", "縞(ｼﾌﾄ)", "縞(解除)", "Y動き", "C動き", "解除Lv", "ｽﾚｯﾄﾞ数" };
int track_s[]       = {    0,    0,    0,    0,       0,        0,         0,          0,       0,       0,        0,         1  };
int track_default[] = {   16,   16,   32,   32,       0,      192,       128,         64,     128,     256,        4,         2  };
int track_e[]       = {  512,  512,  512,  512,     256,      256,      1024,       1024,    1024,    1024,        5, AFS_SCAN_WORKER_MAX };
#endif

#define CHECK_N 12

TCHAR *check_name[] = { "フィールドシフト",
    "間引き",
    "スムージング",
    "24fps化",
    "シーンチェンジ検出(解除Lv1)",
    "編集モード",
    "調整モード",
    "ログ保存",
    "ログ再生",
    "ログ再実行",
#ifndef AFSVF
    "YUY2補間(ランチョス2法)",
#else
    "フィールドオーダー反転",
#endif
    "シフト・解除なし"
};
int check_default[] = { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 };


FILTER_DLL filter = {
#ifndef AFSVF
    FILTER_FLAG_INTERLACE_FILTER|FILTER_FLAG_EX_INFORMATION|FILTER_FLAG_WINDOW_SIZE|FILTER_FLAG_EX_DATA,
    320,616, //window size
#else
    FILTER_FLAG_NO_INIT_DATA|FILTER_FLAG_EX_INFORMATION,
    0,0, //window size
#endif
    filter_name,
    TRACK_N,
    track_name,
    track_default,
    track_s,track_e,
    CHECK_N,
    check_name,
    check_default,
    func_proc,
    func_init,
    func_exit,
    func_update,
    func_WndProc,
    NULL,NULL, //system reserved
    &g_afs.ex_data, //ex_data_ptr
    sizeof(g_afs.ex_data), //ex_data_size
    filter_name_ex,
    func_save_start,
    func_save_end,
    NULL,NULL,NULL, //exfunc,hwnd,dll_hinst
    NULL, //ex_data_def
#ifndef AFSVF
    func_is_saveframe
#else
    NULL //func_is_saveframe
#endif
};

EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void) {
    return &filter;
}

#ifndef AFSVF
EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTableYUY2(void) {
    return &filter;
}
#endif

BOOL func_init(FILTER*) {
    memset(&g_afs, 0, sizeof(g_afs));
    g_afs.source_frame_n = -1;
    g_afs.scan_worker_n = -1;
    g_afs.scan_frame_n = -1;
    g_afs.ex_data.proc_mode = AFS_MODE_CACHE_NV16;
#if ENABLE_SUB_THREADS
    g_afs.sub_thread.thread_sub_n = -1;
#endif
    get_afs_func_list(&afs_func);
    QPC_FREQ;
    afs_check_share();
    return TRUE;
}

BOOL func_exit(FILTER*) {
    free_sub_thread();
#ifndef AFSVF
    free_source_cache();
#endif
    free_analyze_cache();
    afs_release_share();
    return TRUE;
}

BOOL func_update(FILTER* fp, int status) {
    if (status == FILTER_UPDATE_STATUS_CHECK + 0) {
        if (fp->check[1] == 0)
            fp->check[1] = fp->check[2] = 0;
    }

    if (status == FILTER_UPDATE_STATUS_CHECK + 1) {
        if (fp->check[1] == 1)
            fp->check[0] = 1;
        else
            fp->check[2] = 0;
    }

    if (status == FILTER_UPDATE_STATUS_CHECK + 2) {
        if (fp->check[2] == 1) {
            fp->check[0] = fp->check[1] = 1;
        }
    }

    if (status == FILTER_UPDATE_STATUS_CHECK + 5) {
        if (fp->check[5] == 1)
            fp->check[6] = 0;
    }

    if (status == FILTER_UPDATE_STATUS_CHECK + 6) {
        if (fp->check[6] == 1)
            fp->check[5] = 0;
    }

    if (status == FILTER_UPDATE_STATUS_CHECK + 8) {
        if (fp->check[8] == 1) {
            fp->check[9] = 0;
        }
    }

    if (status == FILTER_UPDATE_STATUS_CHECK + 9) {
        if (fp->check[9] == 1) {
            fp->check[8] = 0;
        }
    }

    if (fp->check[8] == 1) {
        fp->check[7] = 0;
        fp->check[9] = 0;
    }

    if (fp->check[8] == 1) {
        fp->check[0] = 0;
        fp->check[1] = 0;
        fp->check[2] = 0;
        fp->check[3] = 0;
        fp->check[4] = 0;
        fp->check[5] = 0;
        fp->check[6] = 0;
    } else {
        if (fp->check[0] == 0)
            fp->check[1] = fp->check[2] = 0;
        else if (fp->check[1] == 0)
            fp->check[2] = 0;
        if (fp->check[5] == 1)
            fp->check[6] = 0;
    }
    fp->exfunc->filter_window_update(fp);

    return TRUE;
}

BOOL func_save_start(FILTER *fp, int s, int e, void *editp) {
    afs_set_start_frame(s);

    log_start_frame = s;
    log_end_frame = e;
    log_save_check = 0;

    if (fp->check[8] || fp->check[9])
        load_log(fp, editp);

    return FALSE;
}

BOOL func_save_end(FILTER *fp, void *editp) {
    afs_set_start_frame(0);

    if (fp->check[7] && log_save_check)
        save_log(fp, editp);

    log_start_frame = 0;
    log_end_frame = -1;

    return TRUE;
}

// 解析関数ワーカースレッド
template<typename SCR_TYPE>
void thread_func_analyze_frame(const int id) {
    const AFS_FUNC_ANALYZE func_analyze = afs_func.analyze[afs_cache_nv16(g_afs.scan_arg.afs_mode)];
    const int min_analyze_cycle = func_analyze.min_cycle;
    const int max_block_size = func_analyze.max_block_size;
    int analyze_block = min_analyze_cycle;
    SCR_TYPE *const p0 = (SCR_TYPE *)g_afs.scan_arg.p0;
    SCR_TYPE *const p1 = (SCR_TYPE *)g_afs.scan_arg.p1;
    PIXEL_YC *const workp = g_afs.scan_workp + (g_afs.scan_h * max_block_size * id);// workp will be at least (min_analyze_cycle*2) aligned.
    int pos_x = ((int)(g_afs.scan_w * id / (double)g_afs.scan_worker_n + 0.5) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
    int x_fin = ((int)(g_afs.scan_w * (id+1) / (double)g_afs.scan_worker_n + 0.5) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
    AFS_SCAN_CLIP clip_thread = *g_afs.scan_arg.clip;
    int thread_mc_local[2] = { 0 };
    if (id < g_afs.scan_worker_n - 1) {
        if (func_analyze.shrink_info) {
            for (; pos_x < x_fin; pos_x += analyze_block) {
                analyze_block = min(x_fin - pos_x, max_block_size);
                afs_analyze_get_local_scan_clip(&clip_thread, g_afs.scan_arg.clip, pos_x, analyze_block, g_afs.scan_w, 0);
                func_analyze.analyze_main[g_afs.scan_arg.type]((BYTE *)workp, p0 + pos_x, p1 + pos_x, g_afs.scan_arg.tb_order, analyze_block, g_afs.scan_arg.source_w, g_afs.scan_arg.si_pitch, g_afs.scan_h, g_afs.source_h, thread_mc_local, &clip_thread);
                func_analyze.shrink_info(g_afs.scan_arg.dst + pos_x, workp, g_afs.scan_h, analyze_block, g_afs.scan_arg.si_pitch);
            }
        } else {
            for (; pos_x < x_fin; pos_x += analyze_block) {
                analyze_block = min(x_fin - pos_x, max_block_size);
                afs_analyze_get_local_scan_clip(&clip_thread, g_afs.scan_arg.clip, pos_x, analyze_block, g_afs.scan_w, 0);
                func_analyze.analyze_main[g_afs.scan_arg.type](g_afs.scan_arg.dst + pos_x, p0 + pos_x, p1 + pos_x, g_afs.scan_arg.tb_order, analyze_block, g_afs.scan_arg.source_w, g_afs.scan_arg.si_pitch, g_afs.scan_h, g_afs.source_h, thread_mc_local, &clip_thread);
            }
        }
    } else {
#if SIMD_DEBUG
        //MMX版をマルチスレッドでやると結果が変わってしまうので、シングルスレッドで処理する
        BYTE *test_buf = get_debug_analyze_buffer(g_afs.scan_arg.max_w * (g_afs.scan_h + 2));
        auto afs_analyze_mmx = (g_afs.scan_arg.type == 0) ? afs_analyze_12_mmx : afs_analyze_1_mmx;
        for (pos_x = 0; pos_x < g_afs.scan_w - 3; pos_x += 4) {
            afs_analyze_mmx(workp, p0 + pos_x, p1 + pos_x, g_afs.scan_arg.tb_order, g_afs.scan_arg.max_w, g_afs.scan_h, g_afs.source_h, thread_mc_local, &clip_thread);
            afs_analyze_shrink_info_mmx(test_buf + pos_x, workp, g_afs.scan_h, g_afs.scan_arg.si_pitch);
        }
        if (pos_x < g_afs.scan_w) {
            afs_analyze_mmx(workp, p0 + g_afs.scan_w-4, p1 + g_afs.scan_w-4, g_afs.scan_arg.tb_order, g_afs.scan_arg.max_w, g_afs.scan_h, g_afs.source_h, thread_mc_local, &clip_thread);
            afs_analyze_shrink_info_mmx(test_buf + g_afs.scan_w-4, workp, g_afs.scan_h, g_afs.scan_arg.si_pitch);
        }
        pos_x = ((int)(g_afs.scan_w * id / (double)g_afs.scan_worker_n + 0.5) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
#endif
        x_fin = g_afs.scan_w;
        if (func_analyze.shrink_info) {
            for (; x_fin - pos_x > max_block_size; pos_x += analyze_block) {
                analyze_block = min(x_fin - pos_x, max_block_size);
                afs_analyze_get_local_scan_clip(&clip_thread, g_afs.scan_arg.clip, pos_x, analyze_block, g_afs.scan_w, 0);
                func_analyze.analyze_main[g_afs.scan_arg.type]((BYTE *)workp, p0 + pos_x, p1 + pos_x, g_afs.scan_arg.tb_order, analyze_block, g_afs.scan_arg.source_w, g_afs.scan_arg.si_pitch, g_afs.scan_h, g_afs.source_h, thread_mc_local, &clip_thread);
                func_analyze.shrink_info(g_afs.scan_arg.dst + pos_x, workp, g_afs.scan_h, analyze_block, g_afs.scan_arg.si_pitch);
            }
            if (pos_x < g_afs.scan_w) {
                analyze_block = ((g_afs.scan_w - pos_x) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
                afs_analyze_get_local_scan_clip(&clip_thread, g_afs.scan_arg.clip, pos_x, analyze_block, g_afs.scan_w, pos_x - (g_afs.scan_w-analyze_block));
                func_analyze.analyze_main[g_afs.scan_arg.type]((BYTE *)workp, p0 + g_afs.scan_w-analyze_block, p1 + g_afs.scan_w-analyze_block, g_afs.scan_arg.tb_order, analyze_block, g_afs.scan_arg.source_w, g_afs.scan_arg.si_pitch, g_afs.scan_h, g_afs.source_h, thread_mc_local, &clip_thread);
                func_analyze.shrink_info(g_afs.scan_arg.dst + g_afs.scan_w-analyze_block, workp, g_afs.scan_h, analyze_block, g_afs.scan_arg.si_pitch);
            }
        } else {
            for (; x_fin - pos_x > max_block_size; pos_x += analyze_block) {
                analyze_block = min(x_fin - pos_x, max_block_size);
                afs_analyze_get_local_scan_clip(&clip_thread, g_afs.scan_arg.clip, pos_x, analyze_block, g_afs.scan_w, 0);
                func_analyze.analyze_main[g_afs.scan_arg.type](g_afs.scan_arg.dst + pos_x, p0 + pos_x, p1 + pos_x, g_afs.scan_arg.tb_order, analyze_block, g_afs.scan_arg.source_w, g_afs.scan_arg.si_pitch, g_afs.scan_h, g_afs.source_h, thread_mc_local, &clip_thread);
            }
            if (pos_x < g_afs.scan_w) {
                analyze_block = ((g_afs.scan_w - pos_x) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
                afs_analyze_get_local_scan_clip(&clip_thread, g_afs.scan_arg.clip, pos_x, analyze_block, g_afs.scan_w, pos_x - (g_afs.scan_w-analyze_block));
                func_analyze.analyze_main[g_afs.scan_arg.type](g_afs.scan_arg.dst + g_afs.scan_w-analyze_block, p0 + g_afs.scan_w-analyze_block, p1 + g_afs.scan_w-analyze_block, g_afs.scan_arg.tb_order, analyze_block, g_afs.scan_arg.source_w, g_afs.scan_arg.si_pitch, g_afs.scan_h, g_afs.source_h, thread_mc_local, &clip_thread);
            }
        }
    }
    g_afs.thread_motion_count[id][0] = thread_mc_local[0];
    g_afs.thread_motion_count[id][1] = thread_mc_local[1];
}


unsigned __stdcall thread_func(LPVOID worker_id) {
    const int id = (int)worker_id;

    WaitForSingleObject(g_afs.hEvent_worker_awake[id], INFINITE);
    while (g_afs.scan_arg.type >= 0) {
        avx2_dummy_if_avail();
        (g_afs.scan_arg.afs_mode & AFS_MODE_CACHE_NV16) ? thread_func_analyze_frame<uint8_t>(id) : thread_func_analyze_frame<PIXEL_YC>(id);
        SetEvent(g_afs.hEvent_worker_sleep[id]);
        WaitForSingleObject(g_afs.hEvent_worker_awake[id], INFINITE);
    }
    _endthreadex(0);
    return 0;
}

// 解析関数ラッパー

void analyze_stripe(int type, AFS_SCAN_DATA* sp, void* p1, void* p0, int source_w, AFS_SCAN_CLIP *mc_clip) {
#if SIMD_DEBUG
    afs_analyze_set_threshold_mmx(sp->thre_shift, sp->thre_deint, sp->thre_Ymotion, sp->thre_Cmotion);
#endif
    afs_func.analyze[afs_cache_nv16(g_afs.afs_mode)].set_threshold(sp->thre_shift, sp->thre_deint, sp->thre_Ymotion, sp->thre_Cmotion);
    g_afs.scan_arg.type     = type;
    g_afs.scan_arg.afs_mode = g_afs.afs_mode;
    g_afs.scan_arg.dst      = sp->map;
    g_afs.scan_arg.p0       = p0;
    g_afs.scan_arg.p1       = p1;
    g_afs.scan_arg.tb_order = sp->tb_order;
    g_afs.scan_arg.source_w = source_w;
    g_afs.scan_arg.si_pitch = si_pitch(g_afs.scan_w, g_afs.afs_mode);
    g_afs.scan_arg.clip     = mc_clip;
    for (int i = 0; i < g_afs.scan_worker_n; i++)
        SetEvent(g_afs.hEvent_worker_awake[i]);
    WaitForMultipleObjects(g_afs.scan_worker_n, g_afs.hEvent_worker_sleep, TRUE, INFINITE);

    int motion_count[2] = { 0 };
    for (int i = 0; i < g_afs.scan_worker_n; i++) {
        motion_count[0] += g_afs.thread_motion_count[i][0];
        motion_count[1] += g_afs.thread_motion_count[i][1];
    }
    int idx = sp - g_afs.scan_array;
    memcpy(g_afs.scan_motion_count[idx], motion_count, sizeof(motion_count));
    g_afs.scan_motion_clip[idx] = *mc_clip;

#if SIMD_DEBUG
    BYTE *test_buf = get_debug_analyze_buffer(0);
    if (compare_frame(test_buf, g_afs.scan_arg.dst, g_afs.scan_w, g_afs.scan_arg.si_pitch, g_afs.scan_h))
        error_message_box(__LINE__, "afs_func.analyze.analyze_main");
#endif
}

// 縞・動きキャッシュ＆ワークメモリ確保

BOOL check_scan_cache(int afs_mode, int frame_n, int w, int h, int worker_n) {
    const int si_w = si_pitch(w, afs_mode);
    const int size = si_w * (h + 2);

    if (g_afs.analyze_cachep[0] != NULL) {
        if (g_afs.scan_frame_n != frame_n || g_afs.scan_w != w || g_afs.scan_h != h || g_afs.scan_worker_n != worker_n) {
            free_analyze_cache();
        }
    }

    if (g_afs.analyze_cachep[0] == NULL) {
        if (afs_func.analyze[afs_cache_nv16(afs_mode)].shrink_info || SIMD_DEBUG) {
            if (nullptr == (g_afs.scan_workp = (PIXEL_YC*)_aligned_malloc(sizeof(PIXEL_YC) * BLOCK_SIZE_YCP * worker_n * h, 64))) {
                return FALSE;
            }
        }

        for (int i = 0; i < AFS_SCAN_CACHE_NUM + AFS_STRIPE_CACHE_NUM; i++) {
            if (nullptr == (g_afs.analyze_cachep[i] = (unsigned char*)_aligned_malloc(sizeof(unsigned char) * size, 64))) {
                if (g_afs.analyze_cachep[i]) {
                    _aligned_free(g_afs.analyze_cachep[i]);
                    g_afs.analyze_cachep[i] = nullptr;
                }
                return FALSE;
            }
            ZeroMemory(g_afs.analyze_cachep[i], sizeof(unsigned char) * size);
        }
        for (int i = 0; i < AFS_SCAN_CACHE_NUM; i++) {
            g_afs.scan_array[i].status = 0;
            g_afs.scan_array[i].map = g_afs.analyze_cachep[i] + si_w;
        }

        for (int i = 0; i < AFS_STRIPE_CACHE_NUM; i++) {
            g_afs.stripe_array[i].status = 0;
            g_afs.stripe_array[i].map = g_afs.analyze_cachep[AFS_SCAN_CACHE_NUM + i];
        }

        for (int i = 0; i < worker_n; i++) {
            g_afs.hEvent_worker_awake[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
            g_afs.hEvent_worker_sleep[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
            g_afs.hThread_worker[i]      = (HANDLE)_beginthreadex(NULL, 0, thread_func, (LPVOID)i, 0, NULL);
            g_afs.worker_thread_priority[i] = THREAD_PRIORITY_NORMAL;
        }
        g_afs.scan_arg.type = -1;
    }

    g_afs.scan_frame_n = frame_n;
    g_afs.scan_w = w;
    g_afs.scan_h = h;
    g_afs.scan_worker_n = worker_n;

    const int priority = GetThreadPriority(GetCurrentThread());
    for (int i = 0; i < g_afs.scan_worker_n; i++)
        if(g_afs.worker_thread_priority[i] != priority)
            SetThreadPriority(g_afs.hThread_worker[i], g_afs.worker_thread_priority[i] = priority);

    return TRUE;
}

// 縞・動き解析

void scan_frame(int frame, int force, int source_w, void *p1, void *p0,
                int mode, int tb_order, int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion, AFS_SCAN_CLIP *mc_clip) {
    const int si_w = si_pitch(g_afs.scan_w, g_afs.afs_mode);
    AFS_SCAN_DATA *sp = scanp(frame);
    if (!force && sp->status > 0 && sp->frame == frame && sp->tb_order == tb_order && sp->thre_shift == thre_shift &&
        ((mode == 0 ) ||
        (mode == 1 && sp->mode == 1 &&
        sp->thre_deint == thre_deint && sp->thre_Ymotion == thre_Ymotion && sp->thre_Cmotion == thre_Cmotion)))
        return;

    sp->status = 1;
    stripe_info_expire(frame);
    stripe_info_expire(frame - 1);

    sp->frame = frame, sp->mode = mode, sp->tb_order = tb_order;
    sp->thre_shift = thre_shift, sp->thre_deint = thre_deint;
    sp->thre_Ymotion = thre_Ymotion, sp->thre_Cmotion = thre_Cmotion;
    sp->clip.top = sp->clip.bottom = sp->clip.left = sp->clip.right = -1;

    analyze_stripe((mode == 0), sp, p1, p0, source_w, mc_clip);

    return;
}

// フィールドごとに動きピクセル数計算

void count_motion(int frame, AFS_SCAN_CLIP *mc_clip) {
    const int si_w = si_pitch(g_afs.scan_w, g_afs.afs_mode);
    AFS_SCAN_DATA *sp = scanp(frame);
    
    if (0 == memcmp(&sp->clip, mc_clip, sizeof(mc_clip[0])))
        return;

    stripe_info_expire(frame);
    stripe_info_expire(frame - 1);
    
    sp->clip = *mc_clip;

    int motion_count[2] = { 0, 0 };
#if SIMD_DEBUG
    int mc_debug[2] = { 0 };
    auto func_compare_debug =[mc_clip](int *mc_debug, int *motion_count) {
        if (memcmp(mc_debug, motion_count, sizeof(int) * 2))
            error_message_box(__LINE__, "afs_func.get_count.motion\n"
            "clip=t%3d:b%3d:l%3d:r%3d\n"
            "       mmx, new\n"
            "n[0] = %6d:%6d\n[1]= %6d:%6d",
                mc_clip->top, mc_clip->bottom, mc_clip->left, mc_clip->right,
                mc_debug[0], motion_count[0],
                mc_debug[1], motion_count[1]);
    };
    afs_get_motion_count(mc_debug, sp, si_w, g_afs.scan_w, g_afs.scan_h);
    afs_func.get_count.motion(motion_count, sp, si_w, g_afs.scan_w, g_afs.scan_h);
    func_compare_debug(mc_debug, motion_count);
#endif
    int *mc_ptr = motion_count;
    if (afs_func.analyze[afs_cache_nv16(g_afs.afs_mode)].mc_count && 0 == memcmp(&g_afs.scan_motion_clip[frame & 15], mc_clip, sizeof(mc_clip[0]))) {
        mc_ptr = g_afs.scan_motion_count[frame & 15];
#if SIMD_DEBUG
        func_compare_debug(mc_debug, mc_ptr);
#endif
    } else {
        afs_func.get_count.motion(motion_count, sp, si_w, g_afs.scan_w, g_afs.scan_h);
    }
    sp->ff_motion = mc_ptr[0];
    sp->lf_motion = mc_ptr[1];
}

#if ENABLE_SUB_THREADS
void merge_scan(int thread_id) {
    AFS_SCAN_DATA *sp0 = g_afs.sub_thread.merge_scan_task.sp0;
    AFS_SCAN_DATA *sp1 = g_afs.sub_thread.merge_scan_task.sp1;
    AFS_STRIPE_DATA *sp = g_afs.sub_thread.merge_scan_task.sp;
    const int si_w = g_afs.sub_thread.merge_scan_task.si_w;
    const int min_analyze_cycle = afs_func.analyze[afs_cache_nv16(g_afs.sub_thread.afs_mode)].min_cycle;
    const int x_start = (((si_w *  thread_id   ) / g_afs.sub_thread.thread_sub_n) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
    const int x_fin   = (((si_w * (thread_id+1)) / g_afs.sub_thread.thread_sub_n) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
    afs_func.merge_scan(sp->map, sp0->map, sp1->map, si_w, g_afs.scan_h, x_start, x_fin);
}
#endif

// 縞情報統合取得
// mode = 0:判定結果のみ, 1:判定結果+解析マップ
unsigned char* get_stripe_info(int frame, int mode) {
    const int si_w = si_pitch(g_afs.scan_w, g_afs.afs_mode);

    AFS_STRIPE_DATA *sp = stripep(frame);
    if(sp->status > mode && sp->frame == frame)
        return sp->map;
    
    AFS_SCAN_DATA *sp0 = scanp(frame);
    AFS_SCAN_DATA *sp1 = scanp(frame + 1);
    // 縞検出のビット反転、動き検出のフィールド＆フレーム結合
#if SIMD_DEBUG
    BYTE *test_buffer = get_debug_buffer(si_w * (g_afs.scan_h + 2));
    afs_merge_scan_mmx(test_buffer, sp0->map, sp1->map, si_w, g_afs.scan_h, 0, si_w);
    afs_func.merge_scan(sp->map,    sp0->map, sp1->map, si_w, g_afs.scan_h, 0, si_w);
    if (memcmp(test_buffer, sp->map, si_w * g_afs.scan_h))
        error_message_box(__LINE__, "afs_func.merge_scan");
#endif

#if ENABLE_SUB_THREADS
    g_afs.sub_thread.afs_mode = g_afs.afs_mode;
    g_afs.sub_thread.merge_scan_task.sp = sp;
    g_afs.sub_thread.merge_scan_task.sp0 = sp0;
    g_afs.sub_thread.merge_scan_task.sp1 = sp1;
    g_afs.sub_thread.merge_scan_task.si_w = si_w;
    g_afs.sub_thread.sub_task = TASK_MERGE_SCAN;
    
    //g_afs.sub_thread.thread_sub_nは総スレッド数
    //自分を除いた数を起動
    for (int ith = 0; ith < g_afs.sub_thread.thread_sub_n - 1; ith++) {
        SetEvent(g_afs.sub_thread.hEvent_sub_start[ith]);
    }
    //メインスレッド(自分)がスレッドID0を担当
    merge_scan(0);
    //1スレッド(つまり自スレッドのみなら同期は必要ない)
    if (0 < g_afs.sub_thread.thread_sub_n - 1)
        WaitForMultipleObjects(g_afs.sub_thread.thread_sub_n - 1, g_afs.sub_thread.hEvent_sub_fin, TRUE, INFINITE);
  #if SIMD_DEBUG
    if (memcmp(test_buffer, sp->map, si_w * g_afs.scan_h))
        error_message_box(__LINE__, "afs_func.merge_scan_mt");
  #endif //SIMD_DEBUG
#else
    afs_func.merge_scan(sp->map, sp0->map, sp1->map, si_w, g_afs.scan_h, 0, si_w);
#endif //ENABLE_SUB_THREADS

    sp->status = 2;
    sp->frame = frame;

    int count[2] = { 0, 0 };
    const int y_fin = g_afs.scan_h - sp0->clip.bottom - ((g_afs.scan_h - sp0->clip.top - sp0->clip.bottom) & 1);
    afs_func.get_count.stripe(count, sp0, sp1, sp, si_w, g_afs.scan_w, g_afs.scan_h);
    sp->count0 = count[0];
    sp->count1 = count[1];
#if SIMD_DEBUG
    memset(count, 0, sizeof(count));
    afs_get_stripe_count(count, sp0, sp1, sp, si_w, g_afs.scan_w, g_afs.scan_h);
    if(sp->count0 != count[0] || sp->count1 != count[1])
        error_message_box(__LINE__, "afs_func.get_count.stripe");
#endif

    return sp->map;
}

// テレシネパターン推定シフト判定

int detect_telecine_cross(int frame, int coeff_shift) {
    AFS_SCAN_DATA *sp1 = scanp(frame - 1);
    AFS_SCAN_DATA *sp2 = scanp(frame);
    AFS_SCAN_DATA *sp3 = scanp(frame + 1);
    AFS_SCAN_DATA *sp4 = scanp(frame + 2);
    int shift = 0;

    if (max2(absdiff(sp1->lf_motion + sp2->lf_motion, sp2->ff_motion),
        absdiff(sp3->ff_motion + sp4->ff_motion, sp3->lf_motion)) * coeff_shift >
        max3(absdiff(sp1->ff_motion + sp2->ff_motion, sp1->lf_motion),
        absdiff(sp2->ff_motion + sp3->ff_motion, sp2->lf_motion),
        absdiff(sp3->lf_motion + sp4->lf_motion, sp4->ff_motion)) * 256)
        if (max2(sp2->lf_motion, sp3->ff_motion) * coeff_shift > sp2->ff_motion * 256)
            shift = 1;

    if (max2(absdiff(sp1->lf_motion + sp2->lf_motion, sp2->ff_motion),
        absdiff(sp3->ff_motion + sp4->ff_motion, sp3->lf_motion)) * coeff_shift >
        max3(absdiff(sp1->ff_motion + sp2->ff_motion, sp1->lf_motion),
        absdiff(sp2->lf_motion + sp3->lf_motion, sp3->ff_motion),
        absdiff(sp3->lf_motion + sp4->lf_motion, sp4->ff_motion)) * 256)
        if(max2(sp2->lf_motion, sp3->ff_motion) * coeff_shift > sp3->lf_motion * 256)
            shift = 1;

    return shift;
}

// 画面解析

unsigned char analyze_frame(int frame, int drop, int smooth, int force24, int coeff_shift, int method_watershed,
                            int *reverse, int *assume_shift, int *result_stat) {
    for (int i = 0; i < 4; i++)
        assume_shift[i] = detect_telecine_cross(frame + i, coeff_shift);

    AFS_SCAN_DATA *scp = scanp(frame);
    int total = 0;
    if (g_afs.scan_h - scp->clip.bottom - ((g_afs.scan_h - scp->clip.top - scp->clip.bottom) & 1) > scp->clip.top && g_afs.scan_w - scp->clip.right > scp->clip.left)
        total = (g_afs.scan_h - scp->clip.bottom - ((g_afs.scan_h - scp->clip.top - scp->clip.bottom) & 1) - scp->clip.top) * (g_afs.scan_w - scp->clip.right - scp->clip.left);
    int threshold = (total * method_watershed) >> 12;

    for (int i = 0; i < 4; i++) {
        get_stripe_info(frame + i, 0);
        AFS_STRIPE_DATA *stp = stripep(frame + i);

        result_stat[i] = (stp->count0 * coeff_shift > stp->count1 * 256) ? 1 : 0;
        if (threshold > stp->count1 && threshold > stp->count0)
            result_stat[i] += 2;
    }

    unsigned char status = AFS_STATUS_DEFAULT;
    if (result_stat[0] & 2)
        status |= assume_shift[0] ? AFS_FLAG_SHIFT0 : 0;
    else
        status |= (result_stat[0] & 1) ? AFS_FLAG_SHIFT0 : 0;
    if (reverse[0]) status ^= AFS_FLAG_SHIFT0;

    if (result_stat[1] & 2)
        status |= assume_shift[1] ? AFS_FLAG_SHIFT1 : 0;
    else
        status |= (result_stat[1] & 1) ? AFS_FLAG_SHIFT1 : 0;
    if (reverse[1]) status ^= AFS_FLAG_SHIFT1;

    if (result_stat[2] & 2)
        status |= assume_shift[2] ? AFS_FLAG_SHIFT2 : 0;
    else
        status |= (result_stat[2] & 1) ? AFS_FLAG_SHIFT2 : 0;
    if (reverse[2]) status ^= AFS_FLAG_SHIFT2;

    if (result_stat[3] & 2)
        status |= assume_shift[3] ? AFS_FLAG_SHIFT3 : 0;
    else
        status |= (result_stat[3] & 1) ? AFS_FLAG_SHIFT3 : 0;
    if (reverse[3]) status ^= AFS_FLAG_SHIFT3;

    if (drop) {
        status |= AFS_FLAG_FRAME_DROP;
        if (smooth) status |= AFS_FLAG_SMOOTHING;
    }
    if (force24) status |= AFS_FLAG_FORCE24;
    if (frame < 1) status &= AFS_MASK_SHIFT0;

    return status;
}

// シーンチェンジ解析

BOOL analyze_scene_change(int afs_mode, unsigned char* sip, int tb_order, int source_w, int w, int h,
                          void *p0, void *p1, int top, int bottom, int left, int right) {
    const int si_w = si_pitch(w, afs_mode);
    //どうやらスタックに確保すると死ぬようなので、mallocする
    static int *hist3d[2] = { nullptr, nullptr };
    if (hist3d[0] == nullptr) {
        hist3d[0] = (int *)malloc(sizeof(int) * 4096 * 2);
        hist3d[1] = hist3d[0] + 4096;
    }
    memset(hist3d[0], 0, sizeof(int) * 4096 * 2);

    int count = 0, count0 = 0;
    for (int pos_y = top; pos_y < h - bottom; pos_y++) {
        if (!is_latter_field(pos_y, tb_order)) {
            unsigned char *sip0 = sip + pos_y * si_w + left;
            for (int pos_x = left; pos_x < w - right; pos_x++) {
                count++;
                count0 += (!(*sip0 & 0x06));
                sip0++;
            }
        }
    }

    if (count0 < (count - count0) * 3) return FALSE;

    if (afs_mode & AFS_MODE_CACHE_NV16) {
        for (int pos_y = top; pos_y < h - bottom; pos_y++) {
            uint8_t *ptr0 = (uint8_t *)p0 + pos_y * source_w + (left & (~1));
            uint8_t *ptr1 = (uint8_t *)p1 + pos_y * source_w + (left & (~1));
            for (int pos_x = left; pos_x < w - right; pos_x += 2, ptr0 += 2, ptr1 += 2) {
                int y0_0 = ptr0[0];
                int y0_1 = ptr0[1];
                int u0   = (ptr0 + source_w * h)[0];
                int v0   = (ptr0 + source_w * h)[0];
                int y1_0 = ptr1[0];
                int y1_1 = ptr1[1];
                int u1   = (ptr1 + source_w * h)[0];
                int v1   = (ptr1 + source_w * h)[0];
                hist3d[0][(y0_0 & 0xf0) << 4 | (u0 & 0xf0) | (v0 & 0xf0) >> 4]++;
                hist3d[0][(y0_1 & 0xf0) << 4 | (u0 & 0xf0) | (v0 & 0xf0) >> 4]++;
                hist3d[1][(y1_0 & 0xf0) << 4 | (u1 & 0xf0) | (v1 & 0xf0) >> 4]++;
                hist3d[1][(y1_1 & 0xf0) << 4 | (u1 & 0xf0) | (v1 & 0xf0) >> 4]++;
            }
        }
    } else {
        for (int pos_y = top; pos_y < h - bottom; pos_y++) {
            PIXEL_YC *ycp0 = (PIXEL_YC *)p0 + pos_y * source_w + left;
            PIXEL_YC *ycp1 = (PIXEL_YC *)p1 + pos_y * source_w + left;
            for (int pos_x = left; pos_x < w - right; pos_x++, ycp0++, ycp1++) {
                hist3d[0][((ycp0->y + 128) & 0xf00) | (((ycp0->cr + 128) >> 4) & 0x0f0) | (((ycp0->cb + 128) >> 8) & 0x00f)]++;
                hist3d[1][((ycp1->y + 128) & 0xf00) | (((ycp1->cr + 128) >> 4) & 0x0f0) | (((ycp1->cb + 128) >> 8) & 0x00f)]++;
            }
        }
    }

    count0 = 0;
    for (int i = 0; i < 4096; i++) {
        count0 += hist3d[(hist3d[0][i] >= hist3d[1][i])][i];
    }

    return (count0 < count);
}

// 結果表示

DWORD icon_hantei[] = {
    0x00000000,0x00000030,0x01800030,0x01806030,0x01806030,0x01806030,0x21806d32,0x7ffe6db6,
    0x60066cb4,0x60066cb4,0x20006c34,0x08006c34,0x1ff86d30,0x01806dfc,0x01806dfe,0x01806c30,
    0x01906c30,0x01906c30,0x11906c30,0x3f986d30,0x3f886fff,0x01886dfe,0x01986c30,0x01986c30,
    0x01b86c30,0x01ac6030,0x01e46030,0x01c46030,0x7f867830,0x3f023030,0x00002030,0x00000000,
};

DWORD icon_sima[] = {
    0x00700180,0x007000c0,0x3fffc060,0x00000630,0x07ff031c,0x060301b0,0x07ff04c0,0x00000c60,
    0x3fffdbfe,0x3000c0c0,0x33fcccd8,0x330cd8cc,0x33fcd8c4,0x3000c0c2,0x1c00c0c0,0x00000000,
};

DWORD icon_23[] = {
    0x03800000,0x07c003c0,0x0e0007e0,0x0c000e70,0x0e018e30,0x07c18e00,0x07c00700,0x07000380,
    0x0e0001c0,0x0e00c0e0,0x0e0cc07c,0x0e0c01fc,0x0f1c0f80,0x07f80f00,0x01f00000,0x00000000,
};

DWORD icon_hanten[] = {
    0x00000000,0x000c0000,0x003f3ffc,0x1f8c0004,0x000c0004,0x003f0804,0x102d1ffc,0x3fbf0824,
    0x032d0424,0x093f0444,0x190c0284,0x30bf8104,0x3c8c0282,0x23ec0c62,0x000c3019,0x00000000,
};

DWORD icon_arrow_bold[] = {
    0x00000000,0x00000000,0x02000040,0x06000060,0x0e000070,0x1ffffff8,0x3ffffffc,0x7ffffffe,
    0x7ffffffe,0x3ffffffc,0x1ffffff8,0x0e000070,0x06000060,0x02000040,0x00000000,0x00000000,
};

DWORD icon_arrow_reverse[] = {
    0x04000000,0x0c000000,0x1fffff80,0x3fffff80,0x1fffff80,0x0c007e00,0x0401f800,0x0007e000,
    0x001f8020,0x007e0030,0x01fffff8,0x01fffffc,0x01fffff8,0x00000030,0x00000020,0x00000000,
};

DWORD icon_arrow_thin[] = {
    0x04000020,0x0c000030,0x1ffffff8,0x3ffffffc,0x1ffffff8,0x0c000030,0x04000020,0x00000000,
};

static PIXEL_YC icon_color[6] = {
    {4096,0,0}, {2038,0,0}, {1225,-692,2048}, {460,-260,768}, {467,2048,-332}, {233,1024,-166},
};

enum {
    YC48_WHITE,
    YC48_GREY,
    YC48_RED,
    YC48_DARKRED,
    YC48_BLUE,
    YC48_DARKBLUE
};

void __declspec(noinline) dot_yc48(PIXEL_YC *ycp, int color, int max_w, int width, int height, int x, int y) {
    if (x < 0 || x >= width || y < 0 || y >= height) return;

    ycp[y * max_w + x] = icon_color[color];
    return;
}

void __declspec(noinline) disp_icon(PIXEL_YC *ycp, int max_w, int width, int height, DWORD* icon, int x, int y, int lines, int color) {
    ycp += y * max_w + x;
    PIXEL_YC *colorp = &icon_color[color];
    for (int pos_y = y; pos_y < y + lines; pos_y++) {
        DWORD pixbits = *icon++;
        PIXEL_YC *ycp_temp = ycp;
        ycp += max_w;
        for (int pos_x = x; pos_x < x + 32; pos_x++) {
            if (pos_x >= 0 && pos_x < width && pos_y >= 0 && y < height && (pixbits & 1)) {
                *ycp_temp = *colorp;
            }
            pixbits >>= 1;
            ycp_temp++;
        }
    }
}

void __declspec(noinline) disp_icon2(PIXEL_YC *ycp, int max_w, int width, int height, DWORD* icon, int x, int y, int lines, int color) {
    disp_icon(ycp, max_w, width, height, icon, x+1, y+1, lines, 1);
    disp_icon(ycp, max_w, width, height, icon, x, y, lines, color);
}

void disp_status(PIXEL_YC *ycp, int *result_stat, int *assume_shift, int *reverse,
                 int max_w, int width, int height, int top, int bottom, int left, int right) {
    for (int pos_y = top; pos_y < top + 80; pos_y++) {
        dot_yc48(ycp, YC48_GREY, max_w, width, height, left +  48, pos_y);
        dot_yc48(ycp, YC48_GREY, max_w, width, height, left +  80, pos_y);
        dot_yc48(ycp, YC48_GREY, max_w, width, height, left + 112, pos_y);
        dot_yc48(ycp, YC48_GREY, max_w, width, height, left + 144, pos_y);
        dot_yc48(ycp, YC48_GREY, max_w, width, height, left + 176, pos_y);
    }
    for (int pos_x = left+48; pos_x < left + 176; pos_x++) {
        dot_yc48(ycp, YC48_GREY, max_w, width, height, pos_x , top + 16);
        dot_yc48(ycp, YC48_GREY, max_w, width, height, pos_x , top + 32);
        dot_yc48(ycp, YC48_GREY, max_w, width, height, pos_x , top + 48);
        dot_yc48(ycp, YC48_GREY, max_w, width, height, pos_x , top + 80);
    }
    if (left + right < width - 1 && top + bottom < height - 1) {
        for (int pos_x = left; pos_x < width - 1 - right; pos_x++) {
            dot_yc48(ycp, YC48_WHITE, max_w, width, height, pos_x, top);
            dot_yc48(ycp, YC48_WHITE, max_w, width, height, pos_x, height - 1 - bottom);
        }
        for (int pos_y = top; pos_y < height - 1 - bottom; pos_y++) {
            dot_yc48(ycp, YC48_WHITE, max_w, width, height, left, pos_y);
            dot_yc48(ycp, YC48_WHITE, max_w, width, height, width - 1 - right, pos_y);
        }
    }

    disp_icon2(ycp, max_w, width, height, icon_sima,   left, top,    16, 2);
    disp_icon2(ycp, max_w, width, height, icon_23,     left, top+16, 16, 4);
    disp_icon2(ycp, max_w, width, height, icon_hanten, left, top+32, 16, 0);
    disp_icon2(ycp, max_w, width, height, icon_hantei, left, top+48, 32, 0);

    for (int i = 0; i < 4; i++)
        if (result_stat[i] & 2)
            if (result_stat[i] & 1)
                disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+32+32*i, top,    8,  3);
            else
                disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+48+32*i, top+8,  8,  3);
        else
            if (result_stat[i] & 1)
                disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+32+32*i, top,    8,  2);
            else
                disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+48+32*i, top+8,  8,  2);

    for (int i = 0; i < 4; i++)
        if (result_stat[i] & 2)
            if (assume_shift[i] & 1)
                disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+32+32*i, top+16, 8,  4);
            else
                disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+48+32*i, top+24, 8,  4);
        else
            if (assume_shift[i] & 1)
                disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+32+32*i, top+16, 8,  5);
            else
                disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+48+32*i, top+24, 8,  5);

    for (int i = 0; i < 4; i++)
        if (reverse[i])
            disp_icon2(ycp, max_w, width, height, icon_arrow_reverse, left+40+32*i, top+32, 16, 0);

    for (int i = 0; i < 4; i++)
        if (result_stat[i] & 2)
            if (((assume_shift[i] & 1) && !reverse[i]) || ((~assume_shift[i] & 1) && reverse[i]))
                disp_icon2(ycp, max_w, width, height, icon_arrow_bold, left+32+32*i, top+48, 16, 4);
            else
                disp_icon2(ycp, max_w, width, height, icon_arrow_bold, left+48+32*i, top+64, 16, 4);
        else
            if (((result_stat[i] & 1) && !reverse[i]) || ((~result_stat[i] & 1) && reverse[i]))
                disp_icon2(ycp, max_w, width, height, icon_arrow_bold, left+32+32*i, top+48, 16, 2);
            else
                disp_icon2(ycp, max_w, width, height, icon_arrow_bold, left+48+32*i, top+64, 16, 2);
}

//
// フィルタ処理関数
//

#if ENABLE_SUB_THREADS
#define DEFINE_SYNTHESIZE_LOCAL \
    const FILTER_PROC_INFO *fpip = g_afs.sub_thread.synthesize_task.fpip; \
    SRC_TYPE *p0 = (SRC_TYPE *)g_afs.sub_thread.synthesize_task.p0; \
    SRC_TYPE *p1 = (SRC_TYPE *)g_afs.sub_thread.synthesize_task.p1; \
    unsigned char *sip = g_afs.sub_thread.synthesize_task.sip; \
    unsigned char status = g_afs.sub_thread.synthesize_task.status; \
    const int si_w = g_afs.sub_thread.synthesize_task.si_w; \
    const int tb_order = g_afs.sub_thread.synthesize_task.tb_order; \
    const int y_start = (fpip->h *  thread_id   ) / g_afs.sub_thread.thread_sub_n; \
    const int y_end   = (fpip->h * (thread_id+1)) / g_afs.sub_thread.thread_sub_n; \
    const int max_w   = fpip->max_w; \
    const int w       = fpip->w; \
    const int h       = fpip->h; \
    const int source_w = g_afs.source_w; \
    const int src_frame_pixels = g_afs.source_w * g_afs.source_h; \
    const func_copy_line copy_line = afs_func.copy_line[g_afs.sub_thread.afs_mode];

#define CHECK_Y_RANGE(y) ((DWORD)(y - y_start) < (DWORD)(y_end - y_start))

PIXEL_YUV to_yuv(PIXEL_YC ycp) {
    PIXEL_YUV p;
    p.y = (BYTE) (((ycp.y         * 219 + 383)>>12) + 16);
    p.u = (BYTE)((((ycp.cb + 2048)*   7 +  66)>> 7) + 16);
    p.v = (BYTE)((((ycp.cr + 2048)*   7 +  66)>> 7) + 16);
    return p;
}
static inline short gety(PIXEL_YC *ycp) {
    return ycp->y;
}
static inline uint8_t gety(uint8_t *ycp) {
    return *ycp;
}
static inline PIXEL_YC getavg(PIXEL_YC *ycpa, PIXEL_YC *ycpb, int pos_x, int src_frame_pixels) {
    PIXEL_YC ycp;
    ycp.y  = (ycpa->y  + ycpb->y  + 1) >> 1;
    ycp.cr = (ycpa->cr + ycpb->cr + 1) >> 1;
    ycp.cb = (ycpa->cb + ycpb->cb + 1) >> 1;
    return ycp;
}
static inline PIXEL_YC getyc48(int y, int u, int v) {
    PIXEL_YC ycp;
    ycp.y  = ((y * 1197) >> 6) - 299;
    ycp.cr = ((u - 128) * 4681 + 164) >> 8;
    ycp.cb = ((v - 128) * 4681 + 164) >> 8;
    return ycp;
}
static inline PIXEL_YC getavg(uint8_t *ycpa, uint8_t *ycpb, int pos_x, int src_frame_pixels) {
    int uv_offset = (pos_x & 1) ? -2 : 0;
    int y = (*ycpa + *ycpb + 1) >> 1;

    uint8_t *ptrua = (uint8_t *)((size_t)(ycpa + src_frame_pixels) & (~1));
    uint8_t *ptrub = (uint8_t *)((size_t)(ycpb + src_frame_pixels) & (~1));
    int ua = ((size_t)ycpa & 1) ? ptrua[0] + ptrua[2] + 1 : (ptrua[0] << 1);
    int va = ((size_t)ycpa & 1) ? ptrua[1] + ptrua[3] + 1 : (ptrua[1] << 1);
    int ub = ((size_t)ycpb & 1) ? ptrub[0] + ptrub[2] + 1 : (ptrub[0] << 1);
    int vb = ((size_t)ycpb & 1) ? ptrub[1] + ptrub[3] + 1 : (ptrub[1] << 1);
    int u = (ua + ub + 2) >> 2;
    int v = (va + vb + 2) >> 2;
    return getyc48(y, u, v);
}
static inline PIXEL_YC getyc48(PIXEL_YC *ycp, int src_frame_pixels) {
    return *ycp;
}
static inline PIXEL_YC getyc48(uint8_t *ycp, int src_frame_pixels) {
    uint8_t *ptr = (uint8_t *)((size_t)(ycp + src_frame_pixels) & (~1));
    int y = *ycp;
    int u = ((size_t)ycp & 1) ? (ptr[0] + ptr[2] + 1) >> 1 : ptr[0];
    int v = ((size_t)ycp & 1) ? (ptr[1] + ptr[3] + 1) >> 1 : ptr[1];
    return getyc48(y, u, v);
}
void store_pixel_yc(uint16_t *_ptr, PIXEL_YC ycp, int pos_x) {
    PIXEL_YUV yuv_temp = to_yuv(ycp);
    BYTE *ptr = (BYTE *)_ptr;
    if (pos_x & 1) {
        ptr[0] = yuv_temp.y;
    } else {
        ptr[0] = yuv_temp.y;
        ptr[1] = yuv_temp.v;
        ptr[3] = yuv_temp.u;
    }
}
void store_pixel_yc(PIXEL_YC *ptr, PIXEL_YC ycp, int pos_x) {
    *(PIXEL_YC *)ptr = ycp;
}

template<typename DST_TYPE, typename SRC_TYPE>
void synthesize_mode_5(int thread_id) {
    DEFINE_SYNTHESIZE_LOCAL;
    SRC_TYPE *p01, *p02, *p03, *p11, *p12, *p13;
    SRC_TYPE *ycp01, *ycp02, *ycp03, *ycp11, *ycp12, *ycp13;
    p01 = p03 = p0,   p11 = p13 = p1;
    p02 = p0 + source_w, p12 = p1 + source_w;
    for (int pos_y = 0; pos_y < h; pos_y++) {
        p01 = p02, p11 = p12;
        p02 = p03, p12 = p13;
        if (pos_y < h - 1) {
            p03 += source_w, p13 += source_w;
        } else {
            p03 = p01, p13 = p11;
        }
        if (CHECK_Y_RANGE(pos_y)) {
            DST_TYPE *ycp  = (DST_TYPE *)fpip->ycp_edit + pos_y * max_w;
            unsigned char *sip0 = sip + pos_y * si_w;
            if (status & AFS_FLAG_SHIFT0) {
                if (!is_latter_field(pos_y, tb_order)) {
                    ycp02 = p02, ycp11 = p11, ycp12 = p12, ycp13 = p13;
                    for (int pos_x = 0; pos_x < w; pos_x++) {
                        PIXEL_YC ycp_temp;
                        if (!(*sip0 & 0x06)) {
                            SRC_TYPE *pix1, *pix2;
                            if (pos_x >= 2 && pos_x < w - 2){
                                int d, d_min;
                                d_min = d = absdiff(gety(ycp11-2), gety(ycp13+2));
                                pix1 = ycp11-2, pix2 = ycp13+2;
                                d = absdiff(gety(ycp11+2), gety(ycp13-2));
                                if (d < d_min) d = d_min, pix1 = ycp11+2, pix2 = ycp13-2;
                                d = absdiff(gety(ycp11-1), gety(ycp13+1));
                                if (d < d_min) d = d_min, pix1 = ycp11-1, pix2 = ycp13+1;
                                d = absdiff(gety(ycp11+1), gety(ycp13-1));
                                if (d < d_min) d = d_min, pix1 = ycp11+1, pix2 = ycp13-1;
                                d = absdiff(gety(ycp11), gety(ycp13));
                                if (d < d_min || ((gety(ycp11) + gety(ycp11) - gety(pix1) - gety(pix2))^(gety(pix1)  + gety(pix2) - gety(ycp13) - gety(ycp13))) < 0)
                                    pix1 = ycp11, pix2 = ycp13;
                            } else {
                                pix1 = ycp11, pix2 = ycp13;
                            }
                            ycp_temp = getavg(pix1, pix2, pos_x, src_frame_pixels);
                        } else {
                            ycp_temp = getyc48(ycp02, src_frame_pixels);
                        }
                        store_pixel_yc(ycp, ycp_temp, pos_x);
                        ycp++, ycp02++, ycp11++, ycp12++, ycp13++, sip0++;
                    }
                } else {
                    copy_line(ycp, p12, w, src_frame_pixels);
                }
            } else {
                if (is_latter_field(pos_y, tb_order)) {
                    ycp01 = p01, ycp02 = p02, ycp03 = p03, ycp12 = p12;
                    for (int pos_x = 0; pos_x < w; pos_x++) {
                        PIXEL_YC ycp_temp;
                        if (!(*sip0 & 0x05)) {
                            SRC_TYPE *pix1, *pix2;
                            if (pos_x >= 2 && pos_x < w - 2) {
                                int d, d_min;
                                d_min = d = absdiff(gety(ycp01-2), gety(ycp03+2));
                                pix1 = ycp01-2, pix2 = ycp03+2;
                                d = absdiff(gety(ycp01+2), gety(ycp03-2));
                                if (d < d_min) d = d_min, pix1 = ycp01+2, pix2 = ycp03-2;
                                d = absdiff(gety(ycp01-1), gety(ycp03+1));
                                if (d < d_min) d = d_min, pix1 = ycp01-1, pix2 = ycp03+1;
                                d = absdiff(gety(ycp01+1), gety(ycp03-1));
                                if (d < d_min) d = d_min, pix1 = ycp01+1, pix2 = ycp03-1;
                                d = absdiff(gety(ycp01), gety(ycp03));
                                if (d < d_min || ((gety(ycp01) + gety(ycp01) - gety(pix1) - gety(pix2))^(gety(pix1)  + gety(pix2) - gety(ycp03) - gety(ycp03))) < 0)
                                    pix1 = ycp01, pix2 = ycp03;
                            } else {
                                pix1 = ycp01, pix2 = ycp03;
                            }
                            ycp_temp = getavg(pix1, pix2, pos_x, src_frame_pixels);
                        } else {
                            ycp_temp = getyc48(ycp02, src_frame_pixels);
                        }
                        store_pixel_yc(ycp, ycp_temp, pos_x);
                        ycp++, ycp01++, ycp02++, ycp03++, ycp12++, sip0++;
                    }
                } else {
                    copy_line(ycp, p02, w, src_frame_pixels);
                }
            }
        }
    }
}

template<typename DST_TYPE, typename SRC_TYPE>
void synthesize_mode_4(int thread_id) {
    DEFINE_SYNTHESIZE_LOCAL;
    SRC_TYPE *p01, *p02, *p03, *p04, *p05, *p06, *p07;
    SRC_TYPE *p11, *p12, *p13, *p14, *p15, *p16, *p17;
    const func_deint4 deint4 = afs_func.deint4[g_afs.sub_thread.afs_mode];

    p01 = p03 = p05 = p0;
    p02 = p04 = p06 = p0 + source_w;
    p07 = p0 + source_w * 2;
    p11 = p13 = p15 = p1;
    p12 = p14 = p16 = p1 + source_w;
    p17 = p1 + source_w * 2;
    for (int pos_y = 0; pos_y < h; pos_y++) {
        p01 = p02; p02 = p03; p03 = p04; p04 = p05; p05 = p06; p06 = p07;
        p11 = p12; p12 = p13; p13 = p14; p14 = p15; p15 = p16; p16 = p17;
        if (pos_y < h - 3) {
            p07 += source_w;
            p17 += source_w;
        } else {
            p07 = p05;
            p17 = p15;
        }
        if (CHECK_Y_RANGE(pos_y)) {
            DST_TYPE *ycp  = (DST_TYPE *)fpip->ycp_edit + pos_y * max_w;
            unsigned char *sip0 = sip + pos_y * si_w;
            if (status & AFS_FLAG_SHIFT0) {
                if (!is_latter_field(pos_y, tb_order)) {
                    deint4(ycp, p11, p13, p04, p15, p17, sip0, 0x06060606, w, src_frame_pixels);
                } else {
                    copy_line(ycp, p14, w, src_frame_pixels);
                }
            } else {
                if (is_latter_field(pos_y, tb_order)) {
                    deint4(ycp, p01, p03, p04, p05, p07, sip0, 0x05050505, w, src_frame_pixels);
                } else {
                    copy_line(ycp, p04, w, src_frame_pixels);
                }
            }
        }
    }
}

template<typename DST_TYPE, typename SRC_TYPE>
void synthesize_mode_3(int thread_id) {
    DEFINE_SYNTHESIZE_LOCAL;
    SRC_TYPE *p01, *p02, *p03, *p11, *p12, *p13;
    const func_blend blend = afs_func.blend[g_afs.sub_thread.afs_mode];

    p01 = p03 = p0,   p11 = p13 = p1;
    p02 = p0 + source_w, p12 = p1 + source_w;
    for (int pos_y = 0; pos_y < h; pos_y++) {
        p01 = p02, p11 = p12;
        p02 = p03, p12 = p13;
        if (pos_y < h - 1) {
            p03 += source_w, p13 += source_w;
        } else {
            p03 = p01, p13 = p11;
        }
        if (CHECK_Y_RANGE(pos_y)) {
            DST_TYPE *ycp = (DST_TYPE *)fpip->ycp_edit + pos_y * max_w;
            unsigned char *sip0 = sip + pos_y * si_w;
            if (status & AFS_FLAG_SHIFT0) {
                if (!is_latter_field(pos_y, tb_order)) {
                    blend(ycp, p11, p02, p13, sip0, 0x06060606, w, src_frame_pixels);
                } else {
                    blend(ycp, p01, p12, p03, sip0, 0x06060606, w, src_frame_pixels);
                }
            } else {
                blend(ycp, p01, p02, p03, sip0, 0x05050505, w, src_frame_pixels);
            }
        }
    }
}

template<typename DST_TYPE, typename SRC_TYPE>
void synthesize_mode_2(int thread_id) {
    DEFINE_SYNTHESIZE_LOCAL;
    SRC_TYPE *p01, *p02, *p03, *p11, *p12, *p13;
    const func_blend blend = afs_func.blend[g_afs.sub_thread.afs_mode];

    p01 = p03 = p0,   p11 = p13 = p1;
    p02 = p0 + source_w, p12 = p1 + source_w;
    for (int pos_y = 0; pos_y < h; pos_y++) {
        p01 = p02, p11 = p12;
        p02 = p03, p12 = p13;
        if (pos_y < h - 1) {
            p03 += source_w, p13 += source_w;
        } else {
            p03 = p01, p13 = p11;
        }
        if (CHECK_Y_RANGE(pos_y)) {
            DST_TYPE *ycp = (DST_TYPE *)fpip->ycp_edit + pos_y * max_w;
            unsigned char *sip0 = sip + pos_y * si_w;
            if (status & AFS_FLAG_SHIFT0) {
                if (!is_latter_field(pos_y, tb_order)) {
                    blend(ycp, p11, p02, p13, sip0, 0x02020202, w, src_frame_pixels);
                } else {
                    blend(ycp, p01, p12, p03, sip0, 0x02020202, w, src_frame_pixels);
                }
            } else {
                blend(ycp, p01, p02, p03, sip0, 0x01010101, w, src_frame_pixels);
            }
        }
    }
}

template<typename DST_TYPE, typename SRC_TYPE>
void synthesize_mode_1(int thread_id) {
    DEFINE_SYNTHESIZE_LOCAL;
    const int clip_t = g_afs.sub_thread.synthesize_task.clip.top;
    const int clip_b = g_afs.sub_thread.synthesize_task.clip.bottom;
    const int clip_l = g_afs.sub_thread.synthesize_task.clip.left;
    const int clip_r = g_afs.sub_thread.synthesize_task.clip.right;
    SRC_TYPE *p01, *p02, *p03, *p11, *p12, *p13;
    const func_mie_spot  mie_spot  = afs_func.mie_spot[g_afs.sub_thread.afs_mode];
    const func_mie_inter mie_inter = afs_func.mie_inter[g_afs.sub_thread.afs_mode];

    p01 = p03 = p0,   p11 = p13 = p1;
    p02 = p0 + source_w, p12 = p1 + source_w;
    if (g_afs.sub_thread.synthesize_task.detect_sc && (~status & AFS_FLAG_SHIFT0))
        if (analyze_scene_change(g_afs.sub_thread.afs_mode, sip, tb_order, source_w, w, h, p0, p1, clip_t, clip_b, clip_l, clip_r))
            p11 = p01, p12 = p02, p13 = p03;
    for (int pos_y = 0; pos_y < h; pos_y++) {
        p01 = p02, p11 = p12;
        p02 = p03, p12 = p13;
        if(pos_y < h - 1) {
            p03 += source_w, p13 += source_w;
        } else {
            p03 = p01, p13 = p11;
        }
        if (CHECK_Y_RANGE(pos_y)) {
            DST_TYPE *ycp = (DST_TYPE *)fpip->ycp_edit + pos_y * max_w;
            if (status & AFS_FLAG_SHIFT0) {
                if (!is_latter_field(pos_y, tb_order)) {
                    mie_inter(ycp, p02, p11, p12, p13, w, src_frame_pixels);
                } else {
                    mie_spot(ycp, p01, p03, p11, p13, p12, w, src_frame_pixels);
                }
            } else {
                if (is_latter_field(pos_y, tb_order)) {
                    mie_inter(ycp, p01, p02, p03, p12, w, src_frame_pixels);
                } else {
                    mie_spot(ycp, p01, p03, p11, p13, p02, w, src_frame_pixels);
                }
            }
        }
    }
}

template<typename DST_TYPE, typename SRC_TYPE>
void synthesize_mode_0(int thread_id) {
    DEFINE_SYNTHESIZE_LOCAL;
    for (int pos_y = y_start; pos_y < y_end; pos_y++) {
        DST_TYPE *ycp = (DST_TYPE *)fpip->ycp_edit + pos_y * max_w;
        SRC_TYPE *ycp0 = p0 + pos_y * source_w;
        SRC_TYPE *ycp1 = p1 + pos_y * source_w;
        if (is_latter_field(pos_y, tb_order) && (status & AFS_FLAG_SHIFT0))
            copy_line(ycp, ycp1, w, src_frame_pixels);
        else
            copy_line(ycp, ycp0, w, src_frame_pixels);
    }
}

typedef void(*func_synthesize)(int thread_id);

void synthesize(int thread_id) {
    func_synthesize func_list[][4] = {
        { synthesize_mode_0<PIXEL_YC, PIXEL_YC>, synthesize_mode_0<PIXEL_YC, uint8_t>, nullptr, synthesize_mode_0<uint16_t, uint8_t> },
        { synthesize_mode_1<PIXEL_YC, PIXEL_YC>, synthesize_mode_1<PIXEL_YC, uint8_t>, nullptr, synthesize_mode_1<uint16_t, uint8_t> },
        { synthesize_mode_2<PIXEL_YC, PIXEL_YC>, synthesize_mode_2<PIXEL_YC, uint8_t>, nullptr, synthesize_mode_2<uint16_t, uint8_t> },
        { synthesize_mode_3<PIXEL_YC, PIXEL_YC>, synthesize_mode_3<PIXEL_YC, uint8_t>, nullptr, synthesize_mode_3<uint16_t, uint8_t> },
        { synthesize_mode_4<PIXEL_YC, PIXEL_YC>, synthesize_mode_4<PIXEL_YC, uint8_t>, nullptr, synthesize_mode_4<uint16_t, uint8_t> },
        { synthesize_mode_5<PIXEL_YC, PIXEL_YC>, synthesize_mode_5<PIXEL_YC, uint8_t>, nullptr, synthesize_mode_5<uint16_t, uint8_t> },
    };
    static_assert(AFS_MODE_AVIUTL_YUY2 == 0x04, "AFS_MODE_AVIUTL_YUY2 is not 0x04, synthesize() will fail.");
    static_assert(AFS_MODE_CACHE_NV16  == 0x02, "AFS_MODE_CACHE_NV16  is not 0x02, synthesize() will fail.");
    func_list[g_afs.sub_thread.synthesize_task.mode][g_afs.sub_thread.afs_mode >> 1](thread_id);
}

unsigned int __stdcall sub_thread(void *prm) {
    //メインスレッドがスレッドID0なので、ここではIDは1から
    const int thread_id = (int)prm;
    //イベントの配列はメインスレッドの分はないのでインデックスから1引く
    WaitForSingleObject(g_afs.sub_thread.hEvent_sub_start[thread_id-1], INFINITE);
    while (!g_afs.sub_thread.thread_sub_abort) {
        avx2_dummy_if_avail();
        switch (g_afs.sub_thread.sub_task) {
        case TASK_MERGE_SCAN: merge_scan(thread_id); break;
        case TASK_SYNTHESIZE: synthesize(thread_id); break;
#ifndef AFSVF
        case TASK_YUY2UP:     yuy2up(thread_id); break;
#endif
        default: break;
        }
        SetEvent(g_afs.sub_thread.hEvent_sub_fin[thread_id-1]);
        WaitForSingleObject(g_afs.sub_thread.hEvent_sub_start[thread_id-1], INFINITE);
    }
    return 0;
}
#endif

static void set_frame_tune_mode(FILTER_PROC_INFO *fpip, BYTE *sip, int si_w, int status) {
    const int h = fpip->h;
    static const PIXEL_YC YC48_BLACK      = { 0,       0,     0 };
    static const PIXEL_YC YC48_GREY       = { 1536,    0,     0 };
    static const PIXEL_YC YC48_BLUE       = { 467,  2048,  -322 };
    static const PIXEL_YC YC48_LIGHT_BLUE = { 2871,  692, -2048 };
    if (g_afs.afs_mode & AFS_MODE_AVIUTL_YUY2) {
        static const PIXEL_YUV YUY2_BLACK      = to_yuv(YC48_BLACK);
        static const PIXEL_YUV YUY2_GREY       = to_yuv(YC48_GREY);
        static const PIXEL_YUV YUY2_BLUE       = to_yuv(YC48_BLUE);
        static const PIXEL_YUV YUY2_LIGHT_BLUE = to_yuv(YC48_LIGHT_BLUE);
        for (int pos_y = 0; pos_y < h; pos_y++) {
            BYTE *ptr_yuv = (BYTE *)fpip->ycp_edit + pos_y * fpip->max_w * 2;
            BYTE *sip0 = sip + pos_y * si_w;
            const int w = fpip->w;
            if (status & AFS_FLAG_SHIFT0) {
                for (int pos_x = 0; pos_x < w; pos_x += 2, ptr_yuv += 4, sip0 += 2) {
                    PIXEL_YUV yuv0, yuv1;
                    BYTE temp = sip[0];
                    if (!(temp & 0x06))
                        yuv0 = YUY2_LIGHT_BLUE;
                    else if (~temp & 0x02)
                        yuv0 = YUY2_GREY;
                    else if (~temp & 0x04)
                        yuv0 = YUY2_BLUE;
                    else
                        yuv0 = YUY2_BLACK;

                    temp = sip[1];
                    if (!(temp & 0x06))
                        yuv1 = YUY2_LIGHT_BLUE;
                    else if (~temp & 0x02)
                        yuv1 = YUY2_GREY;
                    else if (~temp & 0x04)
                        yuv1 = YUY2_BLUE;
                    else
                        yuv1 = YUY2_BLACK;

                    ptr_yuv[0] = yuv0.y;
                    ptr_yuv[1] = (yuv0.u + yuv1.u + 1) >> 1;
                    ptr_yuv[2] = yuv1.y;
                    ptr_yuv[3] = (yuv0.v + yuv1.v + 1) >> 1;
                }
            } else {
                for (int pos_x = 0; pos_x < w; pos_x += 2, ptr_yuv += 4, sip0 += 2) {
                    PIXEL_YUV yuv0, yuv1;
                    BYTE temp = sip[0];
                    if (!(*sip0 & 0x05))
                        yuv0 = YUY2_LIGHT_BLUE;
                    else if (~*sip0 & 0x01)
                        yuv0 = YUY2_GREY;
                    else if (~*sip0 & 0x04)
                        yuv0 = YUY2_BLUE;
                    else
                        yuv0 = YUY2_BLACK;

                    temp = sip[1];
                    if (!(temp & 0x06))
                        yuv1 = YUY2_LIGHT_BLUE;
                    else if (~temp & 0x02)
                        yuv1 = YUY2_GREY;
                    else if (~temp & 0x04)
                        yuv1 = YUY2_BLUE;
                    else
                        yuv1 = YUY2_BLACK;

                    ptr_yuv[0] = yuv0.y;
                    ptr_yuv[1] = (yuv0.u + yuv1.u + 1) >> 1;
                    ptr_yuv[2] = yuv1.y;
                    ptr_yuv[3] = (yuv0.v + yuv1.v + 1) >> 1;
                }
            }
        }
    } else {
        for (int pos_y = 0; pos_y < h; pos_y++) {
            PIXEL_YC *ptr_yc  = fpip->ycp_edit + pos_y * fpip->max_w;
            BYTE *sip0 = sip + pos_y * si_w;
            const int w = fpip->w;
            if (status & AFS_FLAG_SHIFT0) {
                for (int pos_x = 0; pos_x < w; pos_x++, ptr_yc++, sip0++) {
                    if (!(*sip0 & 0x06))
                        *ptr_yc = YC48_LIGHT_BLUE;
                    else if (~*sip0 & 0x02)
                        *ptr_yc = YC48_GREY;
                    else if (~*sip0 & 0x04)
                        *ptr_yc = YC48_BLUE;
                    else
                        *ptr_yc = YC48_BLACK;
                }
            } else {
                for (int pos_x = 0; pos_x < w; pos_x++, ptr_yc++, sip0++) {
                    if (!(*sip0 & 0x05))
                        *ptr_yc = YC48_LIGHT_BLUE;
                    else if (~*sip0 & 0x01)
                        *ptr_yc = YC48_GREY;
                    else if (~*sip0 & 0x04)
                        *ptr_yc = YC48_BLUE;
                    else
                        *ptr_yc = YC48_BLACK;
                }
            }
        }
    }
}

BOOL func_proc( FILTER *fp,FILTER_PROC_INFO *fpip )
{
    //
    //    fp->track[n]            : トラックバーの数値
    //    fp->check[n]            : チェックボックスの数値
    //    fpip->w                 : 実際の画像の横幅
    //    fpip->h                 : 実際の画像の縦幅
    //    fpip->max_w                : 画像領域の横幅
    //    fpip->max_h                : 画像領域の縦幅
    //    fpip->ycp_edit            : 画像領域へのポインタ
    //    fpip->ycp_temp            : テンポラリ領域へのポインタ
    //    fpip->ycp_edit[n].y        : 画素(輝度    )データ (     0 ～ 4095 )
    //    fpip->ycp_edit[n].cb    : 画素(色差(青))データ ( -2048 ～ 2047 )
    //    fpip->ycp_edit[n].cr    : 画素(色差(赤))データ ( -2048 ～ 2047 )
    //
    //    インターレース解除フィルタは画像サイズを変えたり
    //    画像領域とテンポラリ領域を入れ替えたりは出来ません。
    //
    //    画像領域に初期画像データは入っていません。
    //    get_ycp_source_cache()を使って自分で読み込む必要があります。
    //
    unsigned char *sip, *sip0;
    void *p0, *p1, *ycp0, *ycp1;
    int hit, prev_hit;

#ifdef AFSVF
    if(fp->exfunc->set_ycp_filtering_cache_size(fp, fpip->max_w, fpip->max_h, AFS_SOURCE_CACHE_NUM, NULL) == NULL){
        error_modal(fp, fpip->editp, "フィルタキャッシュの確保に失敗しました。");
        return FALSE;
    };
#endif
    QPC_GET_COUNTER(QPC_START);

    // 設定値読み出し
    AFS_SCAN_CLIP clip = scan_clip(fp->track[0], fp->track[1], fp->track[2], fp->track[3]);
    const int method_watershed = (fp->check[0]) ? fp->track[4] : 0;
    const int coeff_shift = (fp->check[0]) ? fp->track[5] : 0;
    const int thre_shift = fp->track[6];
    const int thre_deint = fp->track[7];
    const int thre_Ymotion = fp->track[8];
    const int thre_Cmotion = fp->track[9];
    int       analyze = fp->track[10];
    const int num_thread = fp->track[11];
    const int num_sub_thread = (ENABLE_SUB_THREADS) ? ((!NUM_SUB_THREAD) ? fp->track[12] : NUM_SUB_THREAD) : 1;
    const int drop = (fp->check[0] && fp->check[1]);
    const int smooth = (drop && fp->check[2]);
    const int force24 = fp->check[3];
    const int detect_sc = fp->check[4];
    int       edit_mode = fp->check[5];
    int       tune_mode = fp->check[6];
    const int log_save = fp->check[7];
    const int trace_mode = fp->check[8];
    const int replay_mode = fp->check[9];
    const int yuy2upsample = fp->check[10] ? 1 : 0;
    const int through_mode = fp->check[11];
#ifndef AFSVF
    const int cache_nv16_mode = (g_afs.ex_data.proc_mode & AFS_MODE_CACHE_NV16) || fpip->yc_size < 6;
#else
    const int cache_nv16_mode = 0;
#endif

    const int is_saving = fp->exfunc->is_saving(fpip->editp);
    const int is_editing = fp->exfunc->is_editing(fpip->editp);
    int tb_order = ((fpip->flag & FILTER_PROC_INFO_FLAG_INVERT_FIELD_ORDER) != 0);
    g_afs.afs_mode = 0x00;
    g_afs.afs_mode |= (fpip->yc_size == 2) ? AFS_MODE_AVIUTL_YUY2 : AFS_MODE_AVIUTL_YC48;
    g_afs.afs_mode |= (yuy2upsample & (fpip->yc_size != 2)) ? AFS_MODE_YUY2UP : 0x00;
    g_afs.afs_mode |= (cache_nv16_mode) ? AFS_MODE_CACHE_NV16 : AFS_MODE_CACHE_YC48;

#ifdef AFSVF
    tb_order = fp->check[10] ? !tb_order : tb_order;

    if(trace_mode){
        fill_this_ycp(fp, fpip);
        return TRUE;
    }
#endif
    avx2_dummy_if_avail();

    setup_sub_thread(num_sub_thread);

    if (is_saving || !is_editing)
        edit_mode = tune_mode = 0;
    else if (edit_mode)
        analyze = 0;
    else if (tune_mode)
        analyze += !analyze;

    // 反転情報取得
    int reverse[4];
    for (int i = 0; i < 4; i++)
        reverse[i] = is_frame_reverse(fp, fpip, fpip->frame + i);

    if (!afs_is_ready()) {
        fill_this_ycp(fp, fpip);
        error_modal(fp, fpip->editp, "共有エラー");
        return TRUE;
    }

#ifndef AFSVF
    if (trace_mode) {
        fill_this_ycp(fp, fpip);
        return TRUE;
    }

    if (set_source_cache_size(fpip->frame_n, fpip->max_w, fpip->max_h, g_afs.afs_mode) != TRUE) {
        fill_this_ycp(fp, fpip);
        error_modal(fp, fpip->editp, "フィルタキャッシュの確保に失敗しました。");
        return TRUE;
    };
#else
    g_afs.source_w = fpip->max_w;
    g_afs.source_h = fpip->max_h;
#endif

    p0 = get_ycp_cache(fp, fpip, fpip->frame, NULL);
    if (p0 == NULL) {
        error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
        return TRUE;
    }

    // 解析情報キャッシュ確保
    if (!check_scan_cache(g_afs.afs_mode, fpip->frame_n, fpip->w, fpip->h, num_thread)) {
        fill_this_ycp(fp, fpip);
        error_modal(fp, fpip->editp, "解析用メモリが確保できません。");
        return TRUE;
    }

    if (through_mode) {
        if (!trace_mode && !replay_mode) {
            afs_set(fpip->frame_n, fpip->frame, AFS_STATUS_DEFAULT);
        }
        p1 = get_ycp_cache(fp, fpip, fpip->frame - 1, NULL);
        if (p1 == NULL) {
            error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
            return TRUE;
        }

        uint8_t *ptr_dst = (uint8_t *)fpip->ycp_edit;
        int src_offset = 0;
        const int src_frame_pixels = g_afs.source_w * g_afs.source_h;
        const func_copy_line copy_line = afs_func.copy_line[g_afs.afs_mode];
        const int dst_w_byte = fpip->max_w * ((g_afs.afs_mode & AFS_MODE_AVIUTL_YUY2) ? 2 : 6);
        const int source_w_byte = g_afs.source_w * ((g_afs.afs_mode & AFS_MODE_CACHE_NV16) ? 1 : 6);
        for (int pos_y = 0; pos_y < fpip->h; pos_y++, ptr_dst += dst_w_byte, src_offset += source_w_byte) {
            auto ptr_src = (uint8_t *)((is_latter_field(pos_y, tb_order) && reverse[0]) ? p1 : p0) + src_offset;
            copy_line(ptr_dst, ptr_src, fpip->w, src_frame_pixels);
        }

        return TRUE;
    }
    
    QPC_GET_COUNTER(QPC_INIT);
    // 不足解析補充
    ycp0 = get_ycp_cache(fp, fpip, fpip->frame - 2, &hit);
    if (ycp0 == NULL) {
        error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
        return TRUE;
    }
    QPC_GET_COUNTER(QPC_YCP_CACHE);
    QPC_ADD(QPC_INIT, QPC_INIT, QPC_START);
    QPC_ADD(QPC_YCP_CACHE, QPC_YCP_CACHE, QPC_INIT);
    for (int i = -1; i <= 7; i++) {
        QPC_GET_COUNTER(QPC_INIT);
        ycp1 = ycp0;
        prev_hit = hit;
        ycp0 = get_ycp_cache(fp, fpip, fpip->frame + i, &hit);
        if (ycp0 == NULL) {
            error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
            return TRUE;
        }
        QPC_GET_COUNTER(QPC_YCP_CACHE);
        scan_frame(fpip->frame + i, (!prev_hit || !hit), g_afs.source_w, ycp1, ycp0,
            (analyze == 0 ? 0 : 1), tb_order, thre_shift, thre_deint, thre_Ymotion, thre_Cmotion, &clip);
        QPC_GET_COUNTER(QPC_SCAN_FRAME);
        count_motion(fpip->frame + i, &clip);
        QPC_GET_COUNTER(QPC_COUNT_MOTION);
        
        QPC_ADD(QPC_YCP_CACHE, QPC_YCP_CACHE, QPC_INIT);
        QPC_ADD(QPC_SCAN_FRAME, QPC_SCAN_FRAME, QPC_YCP_CACHE);
        QPC_ADD(QPC_COUNT_MOTION, QPC_COUNT_MOTION, QPC_SCAN_FRAME);
    }
    // 共有メモリ、解析情報キャッシュ読み出し
    QPC_GET_COUNTER(QPC_INIT);
    int assume_shift[4], result_stat[4];
    unsigned char status;
    if (fpip->frame + 2 < fpip->frame_n) {
        status = analyze_frame(fpip->frame + 2, drop, smooth, force24, coeff_shift, method_watershed, reverse, assume_shift, result_stat);
        if (!replay_mode) afs_set(fpip->frame_n, fpip->frame + 2, status);
    }
    if (fpip->frame + 1 < fpip->frame_n) {
        status = analyze_frame(fpip->frame + 1, drop, smooth, force24, coeff_shift, method_watershed, reverse, assume_shift, result_stat);
        if (!replay_mode) afs_set(fpip->frame_n, fpip->frame + 1, status);
    }
    status = analyze_frame(fpip->frame, drop, smooth, force24, coeff_shift, method_watershed, reverse, assume_shift, result_stat);
    if (replay_mode) {
        status = afs_get_status(fpip->frame_n, fpip->frame);
        status &= AFS_MASK_FRAME_DROP & AFS_MASK_SMOOTHING & AFS_MASK_FORCE24;
        if (drop) {
            status |= AFS_FLAG_FRAME_DROP;
            if (smooth) status |= AFS_FLAG_SMOOTHING;
        }
        if (force24) status |= AFS_FLAG_FORCE24;
        if (fpip->frame < 1) status &= AFS_MASK_SHIFT0;
    }
    afs_set(fpip->frame_n, fpip->frame, status);
    log_save_check = 1;
    QPC_GET_COUNTER(QPC_ANALYZE_FRAME);

    sip = get_stripe_info(fpip->frame, 1);
    const int si_w = si_pitch(fpip->w, g_afs.afs_mode);
    QPC_GET_COUNTER(QPC_STRIP_COUNT);

    // 解析マップをフィルタ
    if (analyze > 1) {
#if SIMD_DEBUG
        BYTE *test_buf = get_debug_buffer(si_w * fpip->h);
        memcpy(test_buf, sip, si_w * fpip->h);
#endif
        afs_func.analyzemap_filter(sip, si_w, fpip->w, fpip->h);
#if SIMD_DEBUG
        afs_analyzemap_filter_mmx(test_buf, si_w, fpip->w, fpip->h);
        if (compare_frame(sip, test_buf, fpip->w, si_w, fpip->h)) {
            error_message_box(__LINE__, "afs_func.analyzemap_filter");
        }
#endif
        stripe_info_dirty(fpip->frame);
    }
    QPC_GET_COUNTER(QPC_MAP_FILTER);
    QPC_ADD(QPC_ANALYZE_FRAME, QPC_ANALYZE_FRAME, QPC_INIT);
    QPC_ADD(QPC_STRIP_COUNT, QPC_STRIP_COUNT, QPC_ANALYZE_FRAME);
    QPC_ADD(QPC_MAP_FILTER, QPC_MAP_FILTER, QPC_STRIP_COUNT);

    // 解除済み画面合成
    p0 = get_ycp_cache(fp, fpip, fpip->frame, NULL);
    if (p0 == NULL) {
        error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
        return TRUE;
    }
    p1 = get_ycp_cache(fp, fpip, fpip->frame - 1, NULL);
    if (p1 == NULL) {
        if ((g_afs.afs_mode & AFS_MODE_AVIUTL_YUY2) == 0) {
            get_func_copy_frame()(fpip->ycp_edit, fpip->max_w, fpip->max_w * fpip->max_h, p0, fpip->w, g_afs.source_w, 0, fpip->h);
        }
        error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
        return TRUE;
    }
    QPC_GET_COUNTER(QPC_YCP_CACHE);
    QPC_ADD(QPC_YCP_CACHE, QPC_YCP_CACHE, QPC_MAP_FILTER);

    if (tune_mode) {
        set_frame_tune_mode(fpip, sip, si_w, status);
    } else {
#if SIMD_DEBUG
        PIXEL_YC *test_synthesize = (PIXEL_YC *)get_debug_buffer(sizeof(PIXEL_YC) * fpip->max_w * fpip->max_h * 2);
#endif
#if ENABLE_SUB_THREADS
        if (analyze <= 5) {
            g_afs.sub_thread.synthesize_task.mode = analyze;
            g_afs.sub_thread.synthesize_task.fpip = fpip;
            g_afs.sub_thread.synthesize_task.p0 = p0;
            g_afs.sub_thread.synthesize_task.p1 = p1;
            g_afs.sub_thread.synthesize_task.sip = sip;
            g_afs.sub_thread.synthesize_task.si_w = si_w;
            g_afs.sub_thread.synthesize_task.status = status;
            g_afs.sub_thread.synthesize_task.tb_order = tb_order;
            g_afs.sub_thread.synthesize_task.mode = analyze;
            g_afs.sub_thread.synthesize_task.detect_sc = detect_sc;
            g_afs.sub_thread.synthesize_task.clip = clip;
            g_afs.sub_thread.afs_mode = g_afs.afs_mode;

            g_afs.sub_thread.sub_task = TASK_SYNTHESIZE;
            //g_afs.sub_thread.thread_sub_nは総スレッド数
            //自分を除いた数を起動
            for (int ith = 0; ith < g_afs.sub_thread.thread_sub_n - 1; ith++) {
                SetEvent(g_afs.sub_thread.hEvent_sub_start[ith]);
            }
            //メインスレッド(自分)がスレッドID0を担当
            synthesize(0);
            //1スレッド(つまり自スレッドのみなら同期は必要ない)
            if (0 < g_afs.sub_thread.thread_sub_n - 1)
                WaitForMultipleObjects(g_afs.sub_thread.thread_sub_n - 1, g_afs.sub_thread.hEvent_sub_fin, TRUE, INFINITE);
        }
    #if SIMD_DEBUG
        PIXEL_YC *test_synthesize_mt = test_synthesize + fpip->max_w * fpip->max_h;
        memcpy(test_synthesize_mt, fpip->ycp_edit, sizeof(PIXEL_YC) * fpip->max_w * fpip->max_h);
    #endif
#endif
#if !ENABLE_SUB_THREADS || SIMD_DEBUG
    #if SIMD_DEBUG
        PIXEL_YC *sdp;
        #define _SD(x) x
    #else
        #define _SD(x)
    #endif
        if (analyze == 5) {
            PIXEL_YC *p01, *p02, *p03, *p11, *p12, *p13;
            PIXEL_YC *ycp01, *ycp02, *ycp03, *ycp11, *ycp12, *ycp13;

            p01 = p03 = p0, p11 = p13 = p1;
            p02 = p0 + g_afs.source_w, p12 = p1 + g_afs.source_w;
            for (int pos_y = 0; pos_y <= fpip->h - 1; pos_y++) {
                p01 = p02, p11 = p12;
                p02 = p03, p12 = p13;
                if (pos_y < fpip->h - 1) {
                    p03 += g_afs.source_w, p13 += g_afs.source_w;
                } else {
                    p03 = p01, p13 = p11;
                }
                ycp  = fpip->ycp_edit + pos_y * fpip->max_w;
                sip0 = sip + pos_y * si_w;
                if (status & AFS_FLAG_SHIFT0) {
                    if (!is_latter_field(pos_y, tb_order)) {
                        ycp02 = p02, ycp11 = p11, ycp12 = p12, ycp13 = p13;
                        for (int pos_x = 0; pos_x < fpip->w; pos_x++) {
                            if (!(*sip0 & 0x06)) {
                                PIXEL_YC *pix1, *pix2;
                                if (pos_x >= 2 && pos_x < fpip->w - 2) {
                                    int d, d_min;
                                    d_min = d = absdiff((ycp11-2)->y, (ycp13+2)->y);
                                    pix1 = ycp11-2, pix2 = ycp13+2;
                                    d = absdiff((ycp11+2)->y, (ycp13-2)->y);
                                    if (d < d_min) d = d_min, pix1 = ycp11+2, pix2 = ycp13-2;
                                    d = absdiff((ycp11-1)->y, (ycp13+1)->y);
                                    if (d < d_min) d = d_min, pix1 = ycp11-1, pix2 = ycp13+1;
                                    d = absdiff((ycp11+1)->y, (ycp13-1)->y);
                                    if (d < d_min) d = d_min, pix1 = ycp11+1, pix2 = ycp13-1;
                                    d = absdiff(ycp11->y, ycp13->y);
                                    if (d < d_min || ((ycp11->y + ycp11->y - pix1->y - pix2->y)^(pix1->y  + pix2->y - ycp13->y - ycp13->y)) < 0)
                                        pix1 = ycp11, pix2 = ycp13;
                                } else {
                                    pix1 = ycp11, pix2 = ycp13;
                                }
                                ycp->y  = (pix1->y  + pix2->y  + 1) >> 1;
                                ycp->cr = (pix1->cr + pix2->cr + 1) >> 1;
                                ycp->cb = (pix1->cb + pix2->cb + 1) >> 1;
                            } else
                                *ycp = *ycp02;
                            ycp++, ycp02++, ycp11++, ycp12++, ycp13++, sip0++;
                        }
                    } else {
                        memcpy(ycp, p12, sizeof(PIXEL_YC) * fpip->w);
                    }
                } else {
                    if (is_latter_field(pos_y, tb_order)) {
                        ycp01 = p01, ycp02 = p02, ycp03 = p03, ycp12 = p12;
                        for (int pos_x = 0; pos_x < fpip->w; pos_x++) {
                            if (!(*sip0 & 0x05)) {
                                PIXEL_YC *pix1, *pix2;
                                if (pos_x >= 2 && pos_x < fpip->w - 2) {
                                    int d, d_min;
                                    d_min = d = absdiff((ycp01-2)->y, (ycp03+2)->y);
                                    pix1 = ycp01-2, pix2 = ycp03+2;
                                    d = absdiff((ycp01+2)->y, (ycp03-2)->y);
                                    if (d < d_min) d = d_min, pix1 = ycp01+2, pix2 = ycp03-2;
                                    d = absdiff((ycp01-1)->y, (ycp03+1)->y);
                                    if (d < d_min) d = d_min, pix1 = ycp01-1, pix2 = ycp03+1;
                                    d = absdiff((ycp01+1)->y, (ycp03-1)->y);
                                    if (d < d_min) d = d_min, pix1 = ycp01+1, pix2 = ycp03-1;
                                    d = absdiff(ycp01->y, ycp03->y);
                                    if (d < d_min || ((ycp01->y + ycp01->y - pix1->y - pix2->y)^(pix1->y  + pix2->y - ycp03->y - ycp03->y)) < 0)
                                        pix1 = ycp01, pix2 = ycp03;
                                } else {
                                    pix1 = ycp01, pix2 = ycp03;
                                }
                                ycp->y  = (pix1->y  + pix2->y  + 1) >> 1;
                                ycp->cr = (pix1->cr + pix2->cr + 1) >> 1;
                                ycp->cb = (pix1->cb + pix2->cb + 1) >> 1;
                            } else
                                *ycp = *ycp02;
                            ycp++, ycp01++, ycp02++, ycp03++, ycp12++, sip0++;
                        }
                    } else {
                        memcpy(ycp, p02, sizeof(PIXEL_YC) * fpip->w);
                    }
                }
            }
        } else if (analyze == 4) {
            PIXEL_YC *p01, *p02, *p03, *p04, *p05, *p06, *p07;
            PIXEL_YC *p11, *p12, *p13, *p14, *p15, *p16, *p17;

            p01 = p03 = p05 = p0;
            p02 = p04 = p06 = p0 + g_afs.source_w;
            p07 = p0 + g_afs.source_w * 2;
            p11 = p13 = p15 = p1;
            p12 = p14 = p16 = p1 + g_afs.source_w;
            p17 = p1 + g_afs.source_w * 2;
            for (int pos_y = 0; pos_y <= fpip->h - 1; pos_y++) {
                p01 = p02; p02 = p03; p03 = p04; p04 = p05; p05 = p06; p06 = p07;
                p11 = p12; p12 = p13; p13 = p14; p14 = p15; p15 = p16; p16 = p17;
                if (pos_y < fpip->h - 3) {
                    p07 += g_afs.source_w;
                    p17 += g_afs.source_w;
                } else {
                    p07 = p05;
                    p17 = p15;
                }
                ycp     = fpip->ycp_edit  + pos_y * fpip->max_w;
                _SD(sdp = test_synthesize + pos_y * fpip->max_w);
                sip0 = sip + pos_y * si_w;
                if (status & AFS_FLAG_SHIFT0) {
                    if (!is_latter_field(pos_y, tb_order)) {
                        afs_func.deint4(ycp, p11, p13, p04, p15, p17, sip0, 0x06060606, fpip->w);
                        _SD(afs_deint4_mmx(sdp, p11, p13, p04, p15, p17, sip0, 0x06060606, fpip->w));
                    } else {
                        memcpy(ycp, p14, sizeof(PIXEL_YC) * fpip->w);
                        _SD(memcpy(sdp, p14, sizeof(PIXEL_YC) * fpip->w));
                    }
                } else {
                    if (is_latter_field(pos_y, tb_order)) {
                        afs_func.deint4(ycp, p01, p03, p04, p05, p07, sip0, 0x05050505, fpip->w);
                        _SD(afs_deint4_mmx(ycp, p01, p03, p04, p05, p07, sip0, 0x05050505, fpip->w));
                    } else {
                        memcpy(ycp, p04, sizeof(PIXEL_YC) * fpip->w);
                        _SD(memcpy(sdp, p04, sizeof(PIXEL_YC) * fpip->w));
                    }
                }
            }
        } else if (analyze == 3) {
            PIXEL_YC *p01, *p02, *p03, *p11, *p12, *p13;

            p01 = p03 = p0, p11 = p13 = p1;
            p02 = p0 + g_afs.source_w, p12 = p1 + g_afs.source_w;
            for (int pos_y = 0; pos_y <= fpip->h - 1; pos_y++) {
                p01 = p02, p11 = p12;
                p02 = p03, p12 = p13;
                if (pos_y < fpip->h - 1) {
                    p03 += g_afs.source_w, p13 += g_afs.source_w;
                } else {
                    p03 = p01, p13 = p11;
                }
                ycp     = fpip->ycp_edit  + pos_y * fpip->max_w;
                _SD(sdp = test_synthesize + pos_y * fpip->max_w);
                sip0 = sip + pos_y * si_w;
                if (status & AFS_FLAG_SHIFT0) {
                    if (!is_latter_field(pos_y, tb_order)) {
                        afs_func.blend(ycp, p11, p02, p13, sip0, 0x06060606, fpip->w);
                        _SD(afs_blend_mmx(sdp, p11, p02, p13, sip0, 0x06060606, fpip->w));
                    } else {
                        afs_func.blend(ycp, p01, p12, p03, sip0, 0x06060606, fpip->w);
                        _SD(afs_blend_mmx(sdp, p01, p12, p03, sip0, 0x06060606, fpip->w));
                    }
                } else {
                    afs_func.blend(ycp, p01, p02, p03, sip0, 0x05050505, fpip->w);
                    _SD(afs_blend_mmx(sdp, p01, p02, p03, sip0, 0x05050505, fpip->w));
                }
            }
        } else if (analyze == 2) {
            PIXEL_YC *p01, *p02, *p03, *p11, *p12, *p13;

            p01 = p03 = p0, p11 = p13 = p1;
            p02 = p0 + g_afs.source_w, p12 = p1 + g_afs.source_w;
            for (int pos_y = 0; pos_y <= fpip->h - 1; pos_y++) {
                p01 = p02, p11 = p12;
                p02 = p03, p12 = p13;
                if (pos_y < fpip->h - 1) {
                    p03 += g_afs.source_w, p13 += g_afs.source_w;
                } else {
                    p03 = p01, p13 = p11;
                }
                ycp     = fpip->ycp_edit  + pos_y * fpip->max_w;
                _SD(sdp = test_synthesize + pos_y * fpip->max_w);
                sip0 = sip + pos_y * si_w;
                if (status & AFS_FLAG_SHIFT0) {
                    if (!is_latter_field(pos_y, tb_order)) {
                        afs_func.blend(ycp, p11, p02, p13, sip0, 0x02020202, fpip->w);
                        _SD(afs_blend_mmx(sdp, p11, p02, p13, sip0, 0x02020202, fpip->w));
                    } else {
                        afs_func.blend(ycp, p01, p12, p03, sip0, 0x02020202, fpip->w);
                        _SD(afs_blend_mmx(sdp, p01, p12, p03, sip0, 0x02020202, fpip->w));
                    }
                } else {
                    afs_func.blend(ycp, p01, p02, p03, sip0, 0x01010101, fpip->w);
                    _SD(afs_blend_mmx(sdp, p01, p02, p03, sip0, 0x01010101, fpip->w));
                }
            }
        } else if (analyze == 1) {
            PIXEL_YC *p01, *p02, *p03, *p11, *p12, *p13;

            p01 = p03 = p0, p11 = p13 = p1;
            p02 = p0 + g_afs.source_w, p12 = p1 + g_afs.source_w;
            if (detect_sc && (~status & AFS_FLAG_SHIFT0))
                if (analyze_scene_change(g_afs.mode, sip, tb_order, g_afs.source_w, fpip->w, fpip->h, p0, p1, clip.top, clip.bottom, clip.left, clip.right))
                    p11 = p01, p12 = p02, p13 = p03;
            for (int pos_y = 0; pos_y <= fpip->h - 1; pos_y++) {
                p01 = p02, p11 = p12;
                p02 = p03, p12 = p13;
                if (pos_y < fpip->h - 1) {
                    p03 += g_afs.source_w, p13 += g_afs.source_w;
                } else {
                    p03 = p01, p13 = p11;
                }
                ycp     = fpip->ycp_edit  + pos_y * fpip->max_w;
                _SD(sdp = test_synthesize + pos_y * fpip->max_w);
                if (status & AFS_FLAG_SHIFT0) {
                    if (!is_latter_field(pos_y, tb_order)) {
                        afs_func.mie_inter(ycp, p02, p11, p12, p13, fpip->w);
                        _SD(afs_mie_inter_mmx(sdp, p02, p11, p12, p13, fpip->w));
                    } else {
                        afs_func.mie_spot(ycp, p01, p03, p11, p13, p12, fpip->w);
                        _SD(afs_mie_spot_mmx(sdp, p01, p03, p11, p13, p12, fpip->w));
                    }
                } else {
                    if (is_latter_field(pos_y, tb_order)) {
                        afs_func.mie_inter(ycp, p01, p02, p03, p12, fpip->w);
                        _SD(afs_mie_inter_mmx(sdp, p01, p02, p03, p12, fpip->w));
                    } else {
                        afs_func.mie_spot(ycp, p01, p03, p11, p13, p02, fpip->w);
                        _SD(afs_mie_spot_mmx(sdp, p01, p03, p11, p13, p02, fpip->w));
                    }
                }
            }
        } else {
            for (int pos_y = 0; pos_y < fpip->h; pos_y++) {
                const int offset = pos_y * fpip->max_w;
                ycp = fpip->ycp_edit + offset;
                _SD(sdp = test_synthesize + offset);
                ycp0 = p0 + pos_y * g_afs.source_w;
                ycp1 = p1 + pos_y * g_afs.source_w;
                if (is_latter_field(pos_y, tb_order) && (status & AFS_FLAG_SHIFT0)) {
                    memcpy(ycp, ycp1, sizeof(PIXEL_YC) * fpip->w);
                    _SD(memcpy(sdp, ycp1, sizeof(PIXEL_YC) * fpip->w));
                } else {
                    memcpy(ycp, ycp0, sizeof(PIXEL_YC) * fpip->w);
                    _SD(memcpy(sdp, ycp0, sizeof(PIXEL_YC) * fpip->w));
                }
            }
        }
    #if SIMD_DEBUG
        if (compare_frame(fpip->ycp_edit, test_synthesize, fpip->w, fpip->max_w, fpip->h)) {
            error_message_box(__LINE__, "afs_func.synthesize");
        }
    #if ENABLE_SUB_THREADS
        if (compare_frame(fpip->ycp_edit, test_synthesize_mt, fpip->w, fpip->max_w, fpip->h)) {
            error_message_box(__LINE__, "afs_func.synthesize_mt");
        }
    #endif //ENABLE_SUB_THREADS
    #endif //SIMD_DEBUG
#endif
    }
    // 解析結果表示
    if (edit_mode)
        disp_status(fpip->ycp_edit, result_stat, assume_shift, reverse,
        fpip->max_w, fpip->w, fpip->h, clip.top, clip.bottom, clip.left, clip.right);
    
    QPC_GET_COUNTER(QPC_BLEND);
    QPC_ADD(QPC_BLEND, QPC_BLEND, QPC_YCP_CACHE);
    QPC_ADD(QPC_START, QPC_BLEND, QPC_START);

#if CHECK_PERFORMANCE
    //適当に速度を計測して時たま吐く
    if ((fpip->frame & 1023) == 0) {
        FILE *fp = NULL;
        if (0 == fopen_s(&fp, "afs_log.csv", "ab")) {
            fprintf(fp, "frame count        ,     %d\n", fpip->frame);
            fprintf(fp, "total              , %12.3f, ms\r\n", QPC_MS(QPC_START));
            fprintf(fp, "init               , %12.3f, ms\r\n", QPC_MS(QPC_INIT));
            fprintf(fp, "get_ycp_cache      , %12.3f, ms\r\n", QPC_MS(QPC_YCP_CACHE));
            fprintf(fp, "scan_frame         , %12.3f, ms\r\n", QPC_MS(QPC_SCAN_FRAME));
            fprintf(fp, "count_motion       , %12.3f, ms\r\n", QPC_MS(QPC_COUNT_MOTION));
            fprintf(fp, "analyze_frame      , %12.3f, ms\r\n", QPC_MS(QPC_ANALYZE_FRAME));
            fprintf(fp, "get_strip_count    , %12.3f, ms\r\n", QPC_MS(QPC_STRIP_COUNT));
            fprintf(fp, "analyze_map_filter , %12.3f, ms\r\n", QPC_MS(QPC_MAP_FILTER));
            fprintf(fp, "blend              , %12.3f, ms\r\n", QPC_MS(QPC_BLEND));
            fprintf(fp, "\r\n\r\n");
            fclose(fp);
        }
    }
#endif
    return TRUE;
}

#ifndef AFSVF
BOOL func_is_saveframe(FILTER *fp, void *editp, int saveno, int frame, int fps, int edit_flag, int )
{
    static int prev_frame = 0, phase24 = 0, position24 = 4;
    int frame_n, drop24, rate, scale, i;
    unsigned char status;

    if (fps != 24) {
        rate = fps, scale = 30;
        for(i = 2; i < scale;)
            if (rate % i == 0 && scale % i == 0)
                rate /= i, scale /= i;
            else
                i++;
        return ((saveno + 1) * rate / scale > saveno * rate / scale);
    }

    //24fps
    if (saveno == 0) {
        phase24 = 4;
        position24 = 1;
        prev_frame = frame;
        return TRUE;
    }
    frame_n = fp->exfunc->get_frame_n(editp);
    status = afs_get_status(frame_n, prev_frame);
    prev_frame = frame;
    if (saveno == 1) {
        if (!(status & AFS_FLAG_SHIFT0) &&
            (status & AFS_FLAG_SHIFT1) &&
            (status & AFS_FLAG_SHIFT2))
            phase24 = 0;
    }
    drop24 = !(status & AFS_FLAG_SHIFT1) &&
        (status & AFS_FLAG_SHIFT2) &&
        (status & AFS_FLAG_SHIFT3);
    if (drop24 || (edit_flag & EDIT_FRAME_EDIT_FLAG_DELFRAME)) phase24 = (position24 + 100) % 5;
    drop24 = 0;
    if (position24 >= phase24 &&
        ((position24 + 100) % 5 == phase24 ||
        (position24 +  99) % 5 == phase24)) {
            position24 -= 5;
            drop24 = 1;
    }
    position24++;
    return drop24 ? FALSE : TRUE;
}
#endif

#define ID_BUTTON_DEFAULT  40001
#define ID_BUTTON_LV1      40002
#define ID_BUTTON_LV2      40003
#define ID_BUTTON_LV3      40004
#define ID_BUTTON_LV4      40005
#define ID_BUTTON_24FPS    40006
#define ID_BUTTON_24FPS_HD 40007
#define ID_BUTTON_30FPS    40008
#define ID_BUTTON_STRIPE   40009
#define ID_BUTTON_MOTION   40010
#define ID_BUTTON_HDTV     40011
#define ID_LABEL_PROC_MODE 40012
#define ID_COMBO_PROC_MODE 40013

HFONT b_font;
HWND b_default, b_lv1, b_lv2, b_lv3, b_lv4, b_24fps, b_24fps_hd, b_30fps, b_stripe, b_motion, b_hdtv, lb_proc_mode, cx_proc_mode;

static void init_dialog(HWND hwnd, FILTER *fp);
static void on_lvdefault_button(FILTER *fp);
static void on_lv1_button(FILTER *fp);
static void on_lv2_button(FILTER *fp);
static void on_lv3_button(FILTER *fp);
static void on_lv4_button(FILTER *fp);
static void on_24fps_button(FILTER *fp);
static void on_24fps_hd_button(FILTER *fp);
static void on_30fps_button(FILTER *fp);
static void on_stripe_button(FILTER *fp);
static void on_motion_button(FILTER *fp);
static void on_hdtv_button(FILTER *fp);
static int set_combo_item(void *string, int data);
static void change_cx_param();
static void update_cx(int proc_mode);

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void*, FILTER *fp) {
    switch (message) {
    case WM_FILTER_FILE_OPEN:
    case WM_FILTER_FILE_CLOSE:
        if (!afs_check_share()) break;
        afs_free_cache();
#ifndef AFSVF
        free_source_cache();
#endif
        free_analyze_cache();
        break;
    case WM_FILTER_INIT:
        init_dialog(hwnd, fp);
        return TRUE;
    case WM_FILTER_UPDATE: // フィルタ更新
    case WM_FILTER_SAVE_END: // セーブ終了
        update_cx(g_afs.ex_data.proc_mode);
        break;
    case WM_COMMAND:
        switch(LOWORD(wparam)) {
        case ID_BUTTON_DEFAULT:
            on_lvdefault_button(fp);
            break;
        case ID_BUTTON_LV1:
            on_lv1_button(fp);
            break;
        case ID_BUTTON_LV2:
            on_lv2_button(fp);
            break;
        case ID_BUTTON_LV3:
            on_lv3_button(fp);
            break;
        case ID_BUTTON_LV4:
            on_lv4_button(fp);
            break;
        case ID_BUTTON_24FPS:
            on_24fps_button(fp);
            break;
        case ID_BUTTON_24FPS_HD:
            on_24fps_hd_button(fp);
            break;
        case ID_BUTTON_30FPS:
            on_30fps_button(fp);
            break;
        case ID_BUTTON_STRIPE:
            on_stripe_button(fp);
            break;
        case ID_BUTTON_MOTION:
            on_motion_button(fp);
            break;
        case ID_BUTTON_HDTV:
            on_hdtv_button(fp);
            break;
#ifndef AFSVF
        case ID_COMBO_PROC_MODE: // コンボボックス
            switch (HIWORD(wparam)) {
            case CBN_SELCHANGE: // 選択変更
                change_cx_param();
            }
            break;
#endif
        default:
            return FALSE;
        }
        return fp->exfunc->is_filter_active(fp);
    case WM_FILTER_EXIT:
        DeleteObject(b_font);
        break;
    case WM_KEYUP:
    case WM_KEYDOWN:
        //    case WM_MOUSEWHEEL:
    case 0x020A:
        SendMessage(GetWindow(hwnd, GW_OWNER), message, wparam, lparam);
        break;
    }

    return FALSE;
}

static void init_dialog(HWND hwnd, FILTER *fp) {
    int top = 330;
    HINSTANCE hinst = fp->dll_hinst;

    b_font = CreateFont(14, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, "Meiryo UI");

    b_default = CreateWindow("BUTTON", "デフォルト", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top, 90, 18, hwnd, (HMENU)ID_BUTTON_DEFAULT, hinst, NULL);
    SendMessage(b_default, WM_SETFONT, (WPARAM)b_font, 0);

    b_lv1 = CreateWindow("BUTTON", "動き重視", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+20, 90, 18, hwnd, (HMENU)ID_BUTTON_LV1, hinst, NULL);
    SendMessage(b_lv1, WM_SETFONT, (WPARAM)b_font, 0);

    b_lv2 = CreateWindow("BUTTON", "二重化", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+40, 90, 18, hwnd, (HMENU)ID_BUTTON_LV2, hinst, NULL);
    SendMessage(b_lv2, WM_SETFONT, (WPARAM)b_font, 0);

    b_lv3 = CreateWindow("BUTTON", "映画/アニメ", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+60, 90, 18, hwnd, (HMENU)ID_BUTTON_LV3, hinst, NULL);
    SendMessage(b_lv3, WM_SETFONT, (WPARAM)b_font, 0);

    b_lv4 = CreateWindow("BUTTON", "残像最小化", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+80, 90, 18, hwnd, (HMENU)ID_BUTTON_LV4, hinst, NULL);
    SendMessage(b_lv4, WM_SETFONT, (WPARAM)b_font, 0);

    b_24fps = CreateWindow("BUTTON", "24fps固定", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+100, 90, 18, hwnd, (HMENU)ID_BUTTON_24FPS, hinst, NULL);
    SendMessage(b_24fps, WM_SETFONT, (WPARAM)b_font, 0);

    b_24fps_hd = CreateWindow("BUTTON", "24fps固定 (HD)", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+120, 90, 18, hwnd, (HMENU)ID_BUTTON_24FPS_HD, hinst, NULL);
    SendMessage(b_24fps_hd, WM_SETFONT, (WPARAM)b_font, 0);

    b_30fps = CreateWindow("BUTTON", "→ 30fps固定", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+140, 90, 18, hwnd, (HMENU)ID_BUTTON_30FPS, hinst, NULL);
    SendMessage(b_30fps, WM_SETFONT, (WPARAM)b_font, 0);

    b_stripe = CreateWindow("BUTTON", "→ 解除強(縞)", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+160, 90, 18, hwnd, (HMENU)ID_BUTTON_STRIPE, hinst, NULL);
    SendMessage(b_stripe, WM_SETFONT, (WPARAM)b_font, 0);

    b_motion = CreateWindow("BUTTON", "→ 解除強(動)", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+180, 90, 18, hwnd, (HMENU)ID_BUTTON_MOTION, hinst, NULL);
    SendMessage(b_motion, WM_SETFONT, (WPARAM)b_font, 0);

    b_hdtv   = CreateWindow("BUTTON", "解除Lv0(HD)", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+200, 90, 18, hwnd, (HMENU)ID_BUTTON_HDTV, hinst, NULL);
    SendMessage(b_hdtv, WM_SETFONT, (WPARAM)b_font, 0);

#ifndef AFSVF
    lb_proc_mode = CreateWindow("static", "", SS_SIMPLE|WS_CHILD|WS_VISIBLE, 8, top+238, 60, 24, hwnd, (HMENU)ID_LABEL_PROC_MODE, hinst, NULL);
    SendMessage(lb_proc_mode, WM_SETFONT, (WPARAM)b_font, 0);
    SendMessage(lb_proc_mode, WM_SETTEXT, 0, (LPARAM)"解析モード");

    cx_proc_mode = CreateWindow("COMBOBOX", "", WS_CHILD|WS_VISIBLE|CBS_DROPDOWNLIST|WS_VSCROLL, 70, top+234, 234, 100, hwnd, (HMENU)ID_COMBO_PROC_MODE, hinst, NULL);
    SendMessage(cx_proc_mode, WM_SETFONT, (WPARAM)b_font, 0);

    set_combo_item("フル解析",     AFS_MODE_CACHE_YC48);
    set_combo_item("簡易高速解析", AFS_MODE_CACHE_NV16);
    SendMessage(cx_proc_mode, CB_SETCURSEL, 0, 0);
#endif
}

static void on_lvdefault_button(FILTER *fp) {
    SendMessage(b_default, WM_KILLFOCUS, 0, 0);
    fp->track[0]  = track_default[0];
    fp->track[1]  = track_default[1];
    fp->track[2]  = track_default[2];
    fp->track[3]  = track_default[3];
    fp->track[4]  = track_default[4];
    fp->track[5]  = track_default[5];
    fp->track[6]  = track_default[6];
    fp->track[7]  = track_default[7];
    fp->track[8]  = track_default[8];
    fp->track[9]  = track_default[9];
    fp->track[10] = track_default[10];
    fp->check[0]  = check_default[0];
    fp->check[1]  = check_default[1];
    fp->check[2]  = check_default[2];
    fp->check[3]  = check_default[3];
    fp->check[4]  = check_default[4];
    fp->check[5]  = check_default[5];
    fp->check[6]  = check_default[6];
    fp->check[7]  = check_default[7];
    fp->check[8]  = check_default[8];
    fp->check[9]  = check_default[9];
    fp->check[10] = check_default[10];
    fp->check[11] = check_default[11];
    fp->exfunc->filter_window_update(fp);
}

static void on_lv1_button(FILTER *fp) {
    SendMessage(b_lv1, WM_KILLFOCUS,0,0);
    fp->track[4]  = 0;
    fp->track[5]  = 192;
    fp->track[6]  = 128;
    fp->track[7]  = 64;
    fp->track[8]  = 128;
    fp->track[9]  = 256;
    fp->track[10] = 1;
    fp->check[0]  = 0;
    fp->check[1]  = 0;
    fp->check[2]  = 0;
    fp->check[3]  = 0;
    fp->check[4]  = 1;
    fp->check[8]  = 0;
    fp->check[9]  = 0;
    fp->check[11] = 0;
    fp->exfunc->filter_window_update(fp);
}

static void on_lv2_button(FILTER *fp) {
    SendMessage(b_lv2, WM_KILLFOCUS,0,0);
    fp->track[4]  = 0;
    fp->track[5]  = 192;
    fp->track[6]  = 128;
    fp->track[7]  = 64;
    fp->track[8]  = 128;
    fp->track[9]  = 256;
    fp->track[10] = 2;
    fp->check[0]  = 1;
    fp->check[1]  = 1;
    fp->check[2]  = 1;
    fp->check[3]  = 0;
    fp->check[4]  = 0;
    fp->check[8]  = 0;
    fp->check[9]  = 0;
    fp->check[11] = 0;
    fp->exfunc->filter_window_update(fp);
}

static void on_lv3_button(FILTER *fp) {
    SendMessage(b_lv3, WM_KILLFOCUS,0,0);
    fp->track[4]  = 64;
    fp->track[5]  = 128;
    fp->track[6]  = 128;
    fp->track[7]  = 64;
    fp->track[8]  = 128;
    fp->track[9]  = 256;
    fp->track[10] = 3;
    fp->check[0]  = 1;
    fp->check[1]  = 1;
    fp->check[2]  = 1;
    fp->check[3]  = 0;
    fp->check[4]  = 0;
    fp->check[8]  = 0;
    fp->check[9]  = 0;
    fp->check[11] = 0;
    fp->exfunc->filter_window_update(fp);
}

static void on_lv4_button(FILTER *fp) {
    SendMessage(b_lv4, WM_KILLFOCUS,0,0);
    fp->track[4]  = 0;
    fp->track[5]  = 192;
    fp->track[6]  = 128;
    fp->track[7]  = 64;
    fp->track[8]  = 128;
    fp->track[9]  = 256;
    fp->track[10] = 4;
    fp->check[0]  = 1;
    fp->check[1]  = 1;
    fp->check[2]  = 1;
    fp->check[3]  = 0;
    fp->check[4]  = 0;
    fp->check[8]  = 0;
    fp->check[9]  = 0;
    fp->check[11] = 0;
    fp->exfunc->filter_window_update(fp);
}

static void on_24fps_button(FILTER *fp) {
    SendMessage(b_24fps, WM_KILLFOCUS,0,0);
    fp->track[4]  = 64;
    fp->track[5]  = 128;
    fp->track[6]  = 128;
    fp->track[7]  = 64;
    fp->track[8]  = 128;
    fp->track[9]  = 256;
    fp->track[10] = 3;
    fp->check[0]  = 1;
    fp->check[1]  = 1;
    fp->check[2]  = 0;
    fp->check[3]  = 1;
    fp->check[4]  = 0;
    fp->check[8]  = 0;
    fp->check[9]  = 0;
    fp->check[11] = 0;
    fp->exfunc->filter_window_update(fp);
}

static void on_24fps_hd_button(FILTER *fp) {
    SendMessage(b_24fps_hd, WM_KILLFOCUS, 0, 0);
    fp->track[0]  = 8;
    fp->track[1]  = 8;
    fp->track[2]  = 16;
    fp->track[3]  = 16;
    fp->track[4]  = 92;
    fp->track[5]  = 192;
    fp->track[6]  = 448;
    fp->track[7]  = 48;
    fp->track[8]  = 112;
    fp->track[9]  = 224;
    fp->track[10] = 3;
    fp->check[0]  = 1;
    fp->check[1]  = 1;
    fp->check[2]  = 1;
    fp->check[3]  = 1;
    fp->check[4]  = 0;
    fp->check[8]  = 0;
    fp->check[9]  = 0;
    fp->check[11] = 0;
    fp->exfunc->filter_window_update(fp);
}

static void on_30fps_button(FILTER *fp) {
    SendMessage(b_30fps, WM_KILLFOCUS,0,0);
    fp->check[0]  = 0;
    fp->check[1]  = 0;
    fp->check[2]  = 0;
    fp->check[3]  = 0;
    fp->check[8]  = 0;
    fp->check[9]  = 0;
    fp->check[11] = 0;
    fp->exfunc->filter_window_update(fp);
}

static void on_stripe_button(FILTER *fp) {
    SendMessage(b_stripe, WM_KILLFOCUS,0,0);
    fp->track[7]  = 24;
    fp->exfunc->filter_window_update(fp);
}

static void on_motion_button(FILTER *fp) {
    SendMessage(b_motion, WM_KILLFOCUS,0,0);
    fp->track[8]  = 64;
    fp->track[9]  = 128;
    fp->exfunc->filter_window_update(fp);
}

static void on_hdtv_button(FILTER *fp) {
    SendMessage(b_hdtv, WM_KILLFOCUS,0,0);
    fp->track[0]  = 64;
    fp->track[1]  = 64;
    fp->track[2]  = 64;
    fp->track[3]  = 64;
    fp->track[4]  = 0;
    fp->track[5]  = 256;
    fp->track[6]  = 128;
    fp->track[7]  = 64;
    fp->track[8]  = 128;
    fp->track[9]  = 256;
    fp->track[10] = 0;
    fp->check[0]  = 1;
    fp->check[1]  = 1;
    fp->check[2]  = 1;
    fp->check[8]  = 0;
    fp->check[9]  = 0;
    fp->check[11] = 0;
    fp->exfunc->filter_window_update(fp);
}

static void change_cx_param() {
    LRESULT ret;

    // 選択番号取得
    ret = SendMessage(cx_proc_mode, CB_GETCURSEL, 0, 0);
    ret = SendMessage(cx_proc_mode, CB_GETITEMDATA, ret, 0);

    if (ret != CB_ERR) {
        g_afs.ex_data.proc_mode = ret;
    }
}

static int set_combo_item(void *string, int data) {
    // コンボボックスアイテム数
    int num = SendMessage(cx_proc_mode, CB_GETCOUNT, 0, 0);

    // 最後尾に追加
    SendMessage(cx_proc_mode, CB_INSERTSTRING, num, (LPARAM)string);
    SendMessage(cx_proc_mode, CB_SETITEMDATA, num, (LPARAM)data);

    return num;
}

static void update_cx(int proc_mode) {
    const int num = SendMessage(cx_proc_mode, CB_GETCOUNT, 0, 0);
    // コンボボックス検索
    for (int i = 0; i < num; i++) {
        if (proc_mode == SendMessage(cx_proc_mode, CB_GETITEMDATA, i, 0)) {
            SendMessage(cx_proc_mode, CB_SETCURSEL, i, 0); // カーソルセット
        }
    }
}