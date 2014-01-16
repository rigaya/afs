#ifndef AFSVF
// 自動フィールドシフト インターレース解除プラグイン for AviUtl
#else
// 自動フィールドシフト ビデオフィルタプラグイン for AviUtl
#endif

#include <windows.h>
#include <process.h>
#include <stdio.h>
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


#define AFS_SOURCE_CACHE_NUM 16
#define AFS_SCAN_CACHE_NUM   16
#define AFS_SCAN_WORKER_MAX  16
#define AFS_STRIPE_CACHE_NUM  8

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

static AFS_FUNC afs_func = { 0 };
static PERFORMANCE_CHECKER afs_qpc = { 0 };

static inline int si_pitch(int x) {
	int align_minus_one = afs_func.analyze.align_minus_one;
	return (x + align_minus_one) & (~align_minus_one);
}

static void error_message(FILTER *fp, LPTSTR m)
{
	TCHAR t[1024];
	int l;

	fp->exfunc->ini_save_str(fp, "error_message", m);

	l = GetDateFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, NULL, NULL, t, 1024);
	if(l) t[l-1] = ' ';
	t[l] = 0;
	l += GetTimeFormat(LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, NULL, NULL, t+l, 1024-l);
	fp->exfunc->ini_save_str(fp, "error_time", t);

	return;
}

static void error_modal(FILTER *fp, void *editp, LPTSTR m)
{
	if(fp->exfunc->is_editing(editp) && !fp->exfunc->is_saving(editp))
		MessageBox(fp->hwnd, m, filter_name, MB_OK|MB_ICONSTOP);
	else
		error_message(fp, m);

	return;
}

#ifndef AFSVF
// インタレース解除フィルタ用ソースキャッシュ

static PIXEL_YC *source_cachep = NULL;
static int source_frame_n = -1, source_w, source_h;

typedef struct {
	PIXEL_YC *map;
	int status, frame, file_id, video_number, yuy2upsample;
} AFS_SOURCE_DATA;

static AFS_SOURCE_DATA source_array[AFS_SOURCE_CACHE_NUM];
#define sourcep(x) (source_array+((x)&(AFS_SOURCE_CACHE_NUM-1)))

void clear_source_cache(void)
{
	int i;

	for(i = 0; i < AFS_SOURCE_CACHE_NUM; i++)
		source_array[i].status = 0;
}

void source_cache_expire(int frame)
{
	AFS_SOURCE_DATA* srp;

	srp = sourcep(frame);
	srp->status = 0;
}

void free_source_cache(void)
{
	clear_source_cache();

	if(source_cachep != NULL){
		_aligned_free(source_cachep);
		source_cachep = NULL;
	}
}

BOOL set_source_cache_size(int frame_n, int max_w, int max_h)
{
	int size, i;

	size = max_w * max_h;

	if(source_cachep != NULL)
		if((frame_n != 0 && source_frame_n != 0 && source_frame_n != frame_n) || source_w != max_w || source_h != max_h){
			_aligned_free(source_cachep);
			source_cachep = NULL;
		}

		if(source_cachep == NULL){
			source_cachep = (PIXEL_YC*)_aligned_malloc(sizeof(PIXEL_YC) * size * AFS_SOURCE_CACHE_NUM, 32);
			ZeroMemory(source_cachep, sizeof(PIXEL_YC) * size * AFS_SOURCE_CACHE_NUM);
			if(source_cachep == NULL)
				return FALSE;

			for(i = 0; i < AFS_SOURCE_CACHE_NUM; i++){
				source_array[i].map = source_cachep + size * i;
				source_array[i].status = 0;
			}
		}

		if(frame_n > 0 || source_frame_n < 0) source_frame_n = frame_n;
		source_w = max_w;
		source_h = max_h;

		return TRUE;
}

PIXEL_YC* get_source_cache(FILTER *fp, void *editp, int frame, int w, int h, int *hit)
{
	AFS_SOURCE_DATA *srp;
	PIXEL_YC *src, *dst;
	int file_id, video_number, yuy2upsample;

#ifndef AFSNFS
	if(fp->exfunc->get_source_video_number(editp, frame, &file_id, &video_number) != TRUE)
#endif
		file_id = video_number = 0;
	yuy2upsample = fp->check[10] ? 1 : 0;

	srp = sourcep(frame);

	if(srp->status > 0 && srp->frame == frame && srp->file_id == file_id &&
		srp->video_number == video_number && srp->yuy2upsample == yuy2upsample){
			if(hit != NULL) *hit = 1;
			return srp->map;
	}
	if(hit != NULL) *hit = 0;

	dst = srp->map;
	src = (PIXEL_YC *)fp->exfunc->get_ycp_source_cache(editp, frame, 0);
	if(yuy2upsample)
		afs_func.yuy2up(dst, src, w, source_w, h);
	else
		memcpy(dst, src, source_w * source_h * sizeof(PIXEL_YC));

	srp->status = 1;
	srp->frame = frame;
	srp->file_id = file_id;
	srp->video_number = video_number;
	srp->yuy2upsample = yuy2upsample;

	return srp->map;
}
#endif

void fill_this_ycp(FILTER *fp, FILTER_PROC_INFO *fpip)
{
	PIXEL_YC* ycp;

#ifndef AFSVF
	ycp = (PIXEL_YC *)fp->exfunc->get_ycp_source_cache(fpip->editp, fpip->frame, 0);
#else
	ycp = (PIXEL_YC *)fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, fpip->frame, &fpip->w, &fpip->h);
#endif

	if(ycp != NULL)
		memcpy(fpip->ycp_edit, ycp, sizeof(PIXEL_YC) * fpip->h * fpip->max_w);
	else
		error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");

	return;
}

PIXEL_YC* get_ycp_cache(FILTER *fp, FILTER_PROC_INFO *fpip, int frame, int *hit)
{
	if(frame < 0)
		frame = 0;
	if(frame >= fpip->frame_n)
		frame = fpip->frame_n - 1;

#ifndef AFSVF
	return get_source_cache(fp, fpip->editp, frame, fpip->w, fpip->h, hit);
#else
	if(hit != NULL) *hit = 1;
	if(frame == fpip->frame)
		return (PIXEL_YC *)fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, frame, &fpip->w, &fpip->h);
	else
		return (PIXEL_YC *)fp->exfunc->get_ycp_filtering_cache_ex(fp, fpip->editp, frame, NULL, NULL);
#endif
}

#ifndef AFSNFS
int is_frame_reverse(FILTER *fp, FILTER_PROC_INFO *fpip, int frame)
{
	FRAME_STATUS fs;

	if(frame < 0 || frame >= fpip->frame_n)
		return 0;

	if(!fp->exfunc->get_frame_status(fpip->editp, frame, &fs)){
		error_modal(fp, fpip->editp, "フレームステータスが取得できませんでした。");
		return 0;
	}else
		return (fs.inter == FRAME_STATUS_INTER_REVERSE) ? 1 : 0;
}
#else
int is_frame_reverse(FILTER*, FILTER_PROC_INFO*, int)
{
	return 0;
}
#endif

// ログ関連

static int log_start_frame, log_end_frame, log_save_check;

static BOOL save_log(FILTER *fp, void *editp)
{
	SYS_INFO sys_info;
	FILE_INFO file_info;
	HANDLE fh;
	TCHAR path[MAX_PATH];
	DWORD dw;
	unsigned char status;
	int frame_n, i;

	fp->exfunc->get_sys_info(editp, &sys_info);
	fp->exfunc->get_file_info(editp, &file_info);
	frame_n = fp->exfunc->get_frame_n(editp);
	wsprintf(path, "%s.afs", sys_info.output_name);
	if((fh = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE){
		error_message(fp, "ログファイルの作成に失敗しました。");
		return FALSE;
	}
	wsprintf(path, "afs7            %14d  %14d  %14d  ",
		log_end_frame + 1 - log_start_frame, file_info.video_rate, file_info.video_scale);
	path[14] = 13, path[15] = 10, path[30] = 13, path[31] = 10;
	path[46] = 13, path[47] = 10, path[62] = 13, path[63] = 10, path[64] = 0;
	WriteFile(fh, path, 64, &dw, NULL);
	path[7] = 13, path[8] = 10, path[9] = 0;
	for(i = log_start_frame; i <= log_end_frame; i++){
		status = afs_get_status(frame_n, i);
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

static BOOL load_log(FILTER *fp, void *editp)
{
	SYS_INFO sys_info;
	FILE_INFO file_info;
	HANDLE hf;
	TCHAR path[MAX_PATH];
	DWORD dw;
	unsigned char status;
	int frame_n, frame, start, i;

	start = 0;

	fp->exfunc->get_sys_info(editp, &sys_info);
	fp->exfunc->get_file_info(editp, &file_info);
	frame_n = fp->exfunc->get_frame_n(editp);
	wsprintf(path, "%s.afs", sys_info.edit_name);
	if((hf = CreateFile(path, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL)) == INVALID_HANDLE_VALUE){
		error_message(fp, "ログファイルが開けません。");
		goto log_err;
	}

	ReadFile(hf, path, 64, &dw, NULL);
	if(*(LPDWORD)path != '7sfa' || dw != 64){
		error_message(fp, "自動フィールドシフトv7以降のログファイルではありません。");
		return FALSE;
	}
	frame = 0;
	for(i = 16; i < 30; i++)
		frame = frame * 10 + ((path[i] > '0' && path[i] <= '9') ? path[i] - '0' : 0);
	if(frame != frame_n){
		error_message(fp, "フレーム数がログと一致しません。");
		CloseHandle(hf);
		goto log_err;
	}

	for(i = 0; i < frame; i++){
		ReadFile(hf, path, 9, &dw, NULL);
		if(dw != 9 || path[7] != 13 || path[8] != 10){
			error_message(fp, "ログファイルが壊れています。");
			CloseHandle(hf);
			start = i;
			goto log_err;
		}
		status = AFS_STATUS_DEFAULT |
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
	for(i = start; i < frame_n; i++)
		afs_set(frame_n, i, AFS_STATUS_DEFAULT);
	return FALSE;
}

// 縞、動きスキャン＋合成縞情報キャッシュ

static unsigned char* analyze_cachep = NULL;
static PIXEL_YC* scan_workp = NULL;
static int scan_worker_n = 1;
static int scan_frame_n = -1, scan_w, scan_h;
static HANDLE hThread_worker[AFS_SCAN_WORKER_MAX];
static HANDLE hEvent_worker_awake[AFS_SCAN_WORKER_MAX];
static HANDLE hEvent_worker_sleep[AFS_SCAN_WORKER_MAX];
static int worker_thread_priority[AFS_SCAN_WORKER_MAX];
static AFS_SCAN_ARG scan_arg;

static AFS_SCAN_DATA scan_array[AFS_SCAN_CACHE_NUM];
#define scanp(x) (scan_array+((x)&(AFS_SCAN_CACHE_NUM-1)))


static AFS_STRIPE_DATA stripe_array[AFS_STRIPE_CACHE_NUM];
#define stripep(x) (stripe_array+((x)&(AFS_STRIPE_CACHE_NUM-1)))

void stripe_info_dirty(int frame)
{
	AFS_STRIPE_DATA *stp;
	stp = stripep(frame);
	if(stp->frame == frame && stp->status > 1)
		stp->status = 1;
}

void stripe_info_expire(int frame)
{
	AFS_STRIPE_DATA *stp;
	stp = stripep(frame);
	if(stp->frame == frame && stp->status > 0)
		stp->status = 0;
}

void free_analyze_cache(void)
{
	int i;

	scan_frame_n = -1;
	for(i = 0; i < AFS_SCAN_CACHE_NUM; i++)
		scan_array[i].status = 0;
	for(i = 0; i < AFS_STRIPE_CACHE_NUM; i++)
		stripe_array[i].status = 0;

	if(analyze_cachep != NULL){
		scan_arg.type = -1;
		for(i = 0; i < scan_worker_n; i++)
			SetEvent(hEvent_worker_awake[i]);
		WaitForMultipleObjects(scan_worker_n, hThread_worker, TRUE, INFINITE);
		for(i = 0; i < scan_worker_n; i++){
			CloseHandle(hThread_worker[i]);
			CloseHandle(hEvent_worker_awake[i]);
			CloseHandle(hEvent_worker_sleep[i]);
		}
		_aligned_free(analyze_cachep);
		if (scan_workp)
			_aligned_free(scan_workp);
		analyze_cachep = NULL;
		scan_workp = NULL;
	}
}

// フィルタ定義

#define TRACK_N 12

TCHAR *track_name[] = { "上", "下", "左", "右", "切替点", "判定比", "縞(ｼﾌﾄ)", "縞(解除)", "Y動き", "C動き", "解除Lv", "ｽﾚｯﾄﾞ数" };
int track_s[]       = {    0,    0,    0,    0,       0,        0,         0,          0,       0,       0,        0,         1  };
int track_default[] = {   16,   16,   32,   32,       0,      192,       128,         64,     128,     256,        4,         2  };
int track_e[]       = {  512,  512,  512,  512,     256,      256,      1024,       1024,    1024,    1024,        5,  AFS_SCAN_WORKER_MAX };

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
	"シフト・解除なし" };
#ifndef AFSVF
int check_default[] = { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 1, 0 };
#else
int check_default[] = { 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
#endif


FILTER_DLL filter = {
#ifndef AFSVF
	FILTER_FLAG_INTERLACE_FILTER|FILTER_FLAG_EX_INFORMATION,
#else
	FILTER_FLAG_NO_INIT_DATA|FILTER_FLAG_EX_INFORMATION,
#endif
	0,0, //window size
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
	NULL, //ex_data_ptr
	NULL, //ex_data_size
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

EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable( void )
{
	return &filter;
}

BOOL func_init(FILTER*)
{
	get_afs_func_list(&afs_func);
#if CHECK_PERFORMANCE
	QueryPerformanceFrequency((LARGE_INTEGER *)&afs_qpc.qpc_freq);
#endif
	afs_check_share();
	return TRUE;
}

BOOL func_exit(FILTER*)
{
#ifndef AFSVF
	free_source_cache();
#endif
	free_analyze_cache();
	afs_release_share();
	return TRUE;
}

BOOL func_update(FILTER* fp, int status)
{
	if(status == FILTER_UPDATE_STATUS_CHECK + 0){
		if(fp->check[1] == 0)
			fp->check[1] = fp->check[2] = 0;
	}

	if(status == FILTER_UPDATE_STATUS_CHECK + 1){
		if(fp->check[1] == 1)
			fp->check[0] = 1;
		else
			fp->check[2] = 0;
	}

	if(status == FILTER_UPDATE_STATUS_CHECK + 2){
		if(fp->check[2] == 1){
			fp->check[0] = fp->check[1] = 1;
		}
	}

	if(status == FILTER_UPDATE_STATUS_CHECK + 5){
		if(fp->check[5] == 1)
			fp->check[6] = 0;
	}

	if(status == FILTER_UPDATE_STATUS_CHECK + 6){
		if(fp->check[6] == 1)
			fp->check[5] = 0;
	}

	if(status == FILTER_UPDATE_STATUS_CHECK + 8){
		if(fp->check[8] == 1){
			fp->check[9] = 0;
		}
	}

	if(status == FILTER_UPDATE_STATUS_CHECK + 9){
		if(fp->check[9] == 1){
			fp->check[8] = 0;
		}
	}

	if(fp->check[8] == 1){
		fp->check[7] = 0;
		fp->check[9] = 0;
	}

	if(fp->check[8] == 1){
		fp->check[0] = 0;
		fp->check[1] = 0;
		fp->check[2] = 0;
		fp->check[3] = 0;
		fp->check[4] = 0;
		fp->check[5] = 0;
		fp->check[6] = 0;
	}else{
		if(fp->check[0] == 0)
			fp->check[1] = fp->check[2] = 0;
		else if(fp->check[1] == 0)
			fp->check[2] = 0;
		if(fp->check[5] == 1)
			fp->check[6] = 0;
	}
	fp->exfunc->filter_window_update(fp);

	return TRUE;
}

BOOL func_save_start(FILTER *fp, int s, int e, void *editp)
{
	afs_set_start_frame(s);

	log_start_frame = s;
	log_end_frame = e;
	log_save_check = 0;

	if(fp->check[8] || fp->check[9])
		load_log(fp, editp);

	return FALSE;
}

BOOL func_save_end(FILTER *fp, void *editp)
{
	afs_set_start_frame(0);

	if(fp->check[7] && log_save_check)
		save_log(fp, editp);

	log_start_frame = 0;
	log_end_frame = -1;

	return TRUE;
}

// 解析関数ワーカースレッド


unsigned __stdcall thread_func(LPVOID worker_id)
{
	PIXEL_YC* workp;

	const int id = (int)worker_id;
	const int min_analyze_cycle = afs_func.analyze.min_cycle;
	const int max_block_size = afs_func.analyze.max_block_size;
	int analyze_block = min_analyze_cycle;

	WaitForSingleObject(hEvent_worker_awake[id], INFINITE);
	while(scan_arg.type >= 0){
		workp = scan_workp + (scan_h * max_block_size * id);// workp will be at least (min_analyze_cycle*2) aligned.
		int pos_x = ((int)(scan_w * id / (double)scan_worker_n + 0.5) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
		int x_fin = ((int)(scan_w * (id+1) / (double)scan_worker_n + 0.5) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
		if (id < scan_worker_n - 1) {
			if (afs_func.analyze.shrink_info) {
				for( ; pos_x < x_fin; pos_x += analyze_block) {
					analyze_block = min(x_fin - pos_x, max_block_size);
					afs_func.analyze.analyze_main[scan_arg.type]((BYTE *)workp, scan_arg.p0 + pos_x, scan_arg.p1 + pos_x, scan_arg.tb_order, analyze_block, scan_arg.max_w, scan_arg.si_pitch, scan_h);
					afs_func.analyze.shrink_info(scan_arg.dst + pos_x, workp, scan_h, analyze_block, scan_arg.si_pitch);
				}
			} else {
				for( ; pos_x < x_fin; pos_x += analyze_block) {
					analyze_block = min(x_fin - pos_x, max_block_size);
					afs_func.analyze.analyze_main[scan_arg.type](scan_arg.dst + pos_x, scan_arg.p0 + pos_x, scan_arg.p1 + pos_x, scan_arg.tb_order, analyze_block, scan_arg.max_w, scan_arg.si_pitch, scan_h);
				}
			}
		} else {
			x_fin = scan_w;
			if (afs_func.analyze.shrink_info) {
				for(; x_fin - pos_x > max_block_size; pos_x += analyze_block) {
					analyze_block = min(x_fin - pos_x, max_block_size);
					afs_func.analyze.analyze_main[scan_arg.type]((BYTE *)workp, scan_arg.p0 + pos_x, scan_arg.p1 + pos_x, scan_arg.tb_order, analyze_block, scan_arg.max_w, scan_arg.si_pitch, scan_h);
					afs_func.analyze.shrink_info(scan_arg.dst + pos_x, workp, scan_h, analyze_block, scan_arg.si_pitch);
				}
				if(pos_x < scan_w){
					analyze_block = ((scan_w - pos_x) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
					afs_func.analyze.analyze_main[scan_arg.type]((BYTE *)workp, scan_arg.p0 + scan_w-analyze_block, scan_arg.p1 + scan_w-analyze_block, scan_arg.tb_order, analyze_block, scan_arg.max_w, scan_arg.si_pitch, scan_h);
					afs_func.analyze.shrink_info(scan_arg.dst + scan_w-analyze_block, workp, scan_h, analyze_block, scan_arg.si_pitch);
				}
			} else {
				for( ; x_fin - pos_x > max_block_size; pos_x += analyze_block) {
					analyze_block = min(x_fin - pos_x, max_block_size);
					afs_func.analyze.analyze_main[scan_arg.type](scan_arg.dst + pos_x, scan_arg.p0 + pos_x, scan_arg.p1 + pos_x, scan_arg.tb_order, analyze_block, scan_arg.max_w, scan_arg.si_pitch, scan_h);
				}
				if(pos_x < scan_w){
					analyze_block = ((scan_w - pos_x) + (min_analyze_cycle-1)) & ~(min_analyze_cycle-1);
					afs_func.analyze.analyze_main[scan_arg.type](scan_arg.dst + scan_w-analyze_block, scan_arg.p0 + scan_w-analyze_block, scan_arg.p1 + scan_w-analyze_block, scan_arg.tb_order, analyze_block, scan_arg.max_w, scan_arg.si_pitch, scan_h);
				}
			}
		}
		SetEvent(hEvent_worker_sleep[id]);
		WaitForSingleObject(hEvent_worker_awake[id], INFINITE);
	}
	_endthreadex(0);
	return 0;
}

// 解析関数ラッパー

void analyze_stripe_full(AFS_SCAN_DATA* sp, PIXEL_YC* p1, PIXEL_YC* p0, int max_w)
{
	int i;

	afs_func.analyze.set_threshold(sp->thre_shift, sp->thre_deint, sp->thre_Ymotion, sp->thre_Cmotion);
	scan_arg.type     = 0;
	scan_arg.dst      = sp->map;
	scan_arg.p0       = p0;
	scan_arg.p1       = p1;
	scan_arg.tb_order = sp->tb_order;
	scan_arg.max_w    = max_w;
	scan_arg.si_pitch = si_pitch(scan_w);
	for(i = 0; i < scan_worker_n; i++)
		SetEvent(hEvent_worker_awake[i]);
	WaitForMultipleObjects(scan_worker_n, hEvent_worker_sleep, TRUE, INFINITE);
}

void analyze_stripe_pass1(AFS_SCAN_DATA* sp, PIXEL_YC* p1, PIXEL_YC* p0, int max_w)
{
	int i;

	afs_func.analyze.set_threshold(sp->thre_shift, sp->thre_deint, sp->thre_Ymotion, sp->thre_Cmotion);
	scan_arg.type     = 1;
	scan_arg.dst      = sp->map;
	scan_arg.p0       = p0;
	scan_arg.p1       = p1;
	scan_arg.tb_order = sp->tb_order;
	scan_arg.max_w    = max_w;
	scan_arg.si_pitch = si_pitch(scan_w);
	for(i = 0; i < scan_worker_n; i++)
		SetEvent(hEvent_worker_awake[i]);
	WaitForMultipleObjects(scan_worker_n, hEvent_worker_sleep, TRUE, INFINITE);
}

void analyze_stripe_pass2(AFS_SCAN_DATA* sp, PIXEL_YC* p1, PIXEL_YC* p0, int max_w)
{
	int i;

	afs_func.analyze.set_threshold(sp->thre_shift, sp->thre_deint, sp->thre_Ymotion, sp->thre_Cmotion);
	scan_arg.type     = 2;
	scan_arg.dst      = sp->map;
	scan_arg.p0       = p0;
	scan_arg.p1       = p1;
	scan_arg.tb_order = sp->tb_order;
	scan_arg.max_w    = max_w;
	scan_arg.si_pitch = si_pitch(scan_w);
	for(i = 0; i < scan_worker_n; i++)
		SetEvent(hEvent_worker_awake[i]);
	WaitForMultipleObjects(scan_worker_n, hEvent_worker_sleep, TRUE, INFINITE);
}

// 縞・動きキャッシュ＆ワークメモリ確保

BOOL check_scan_cache(int frame_n, int w, int h, int worker_n)
{
	int si_w, size, i, priority;

	si_w = si_pitch(w);
	size = si_w * (h + 2);

	if(analyze_cachep != NULL)
		if(scan_frame_n != frame_n || scan_w != w || scan_h != h || scan_worker_n != worker_n)
			free_analyze_cache();

	if(analyze_cachep == NULL){
		analyze_cachep = (unsigned char*)_aligned_malloc(sizeof(unsigned char) * size * (AFS_SCAN_CACHE_NUM + AFS_STRIPE_CACHE_NUM), 64);
		ZeroMemory(analyze_cachep, sizeof(unsigned char) * size * (AFS_SCAN_CACHE_NUM + AFS_STRIPE_CACHE_NUM));
		if(analyze_cachep == NULL)
			return FALSE;

		if (afs_func.analyze.shrink_info) {
			scan_workp = (PIXEL_YC*)_aligned_malloc(sizeof(PIXEL_YC) * BLOCK_SIZE_YCP * worker_n * h, 64);
			if (scan_workp == NULL){
				_aligned_free(analyze_cachep);
				analyze_cachep = NULL;
				return FALSE;
			}
		}

		for(i = 0; i < AFS_SCAN_CACHE_NUM; i++){
			scan_array[i].status = 0;
			scan_array[i].map = analyze_cachep + size * i + si_w;
		}

		for(i = 0; i < AFS_STRIPE_CACHE_NUM; i++){
			stripe_array[i].status = 0;
			stripe_array[i].map = analyze_cachep + size * (i + AFS_SCAN_CACHE_NUM) + si_w;
		}

		for(i = 0; i < worker_n; i++){
			hEvent_worker_awake[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
			hEvent_worker_sleep[i] = CreateEvent(NULL, FALSE, FALSE, NULL);
			hThread_worker[i]      = (HANDLE)_beginthreadex(NULL, 0, thread_func, (LPVOID)i, 0, NULL);
			worker_thread_priority[i] = THREAD_PRIORITY_NORMAL;
		}
	}

	scan_frame_n = frame_n;
	scan_w = w;
	scan_h = h;
	scan_worker_n = worker_n;
	scan_arg.type = -1;

	priority = GetThreadPriority(GetCurrentThread());
	for(i = 0; i < scan_worker_n; i++)
		if(worker_thread_priority[i] != priority)
			SetThreadPriority(hThread_worker[i], worker_thread_priority[i] = priority);

	return TRUE;
}

// 縞・動き解析

void scan_frame(int frame, int force, int max_w, PIXEL_YC *p1, PIXEL_YC *p0,
				int mode, int tb_order, int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion)
{
	AFS_SCAN_DATA *sp;
	int si_w;

	si_w = si_pitch(scan_w);

	sp = scanp(frame);
	if(!force && sp->status > 0 && sp->frame == frame && sp->tb_order == tb_order && sp->thre_shift == thre_shift &&
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
	sp->top = sp->bottom = sp->left = sp->right = -1;

	if(mode == 0)
		analyze_stripe_pass1(sp, p1, p0, max_w);
	else 
		analyze_stripe_full(sp, p1, p0, max_w);

	return;
}

// フィールドごとに動きピクセル数計算

void count_motion(int frame, int top, int bottom, int left, int right)
{
	AFS_SCAN_DATA *sp;
	int si_w;

	si_w = si_pitch(scan_w);
	sp = scanp(frame);

	if(sp->top == top && sp->bottom == bottom && sp->left == left && sp->right == right)
		return;

	stripe_info_expire(frame);
	stripe_info_expire(frame - 1);

	sp->top = top, sp->bottom = bottom, sp->left = left, sp->right = right;

	int motion_count[2] = { 0, 0 };
	afs_func.get_count.motion(motion_count, sp, si_w, scan_w, scan_h);

	sp->ff_motion = motion_count[0];
	sp->lf_motion = motion_count[1];
}

// 縞情報統合取得
// mode = 0:判定結果のみ, 1:判定結果+解析マップ
unsigned char* get_stripe_info(int frame, int mode)
{
	AFS_SCAN_DATA *sp0, *sp1;
	AFS_STRIPE_DATA *sp;
	int si_w = si_pitch(scan_w);

	sp = stripep(frame);
	if(sp->status > mode && sp->frame == frame)
		return sp->map;

	// 縞検出のビット反転、動き検出のフィールド＆フレーム結合
	sp0 = scanp(frame);
	sp1 = scanp(frame + 1);
	afs_func.merge_scan(sp->map, sp0->map, sp1->map, si_w, scan_h);

	sp->status = 2;
	sp->frame = frame;

	int count[2] = { 0, 0 };
	const int y_fin = scan_h - sp0->bottom - ((scan_h - sp0->top - sp0->bottom) & 1);
	afs_func.get_count.stripe(count, sp0, sp1, sp, si_w, scan_w, scan_h);
	sp->count0 = count[0];
	sp->count1 = count[1];

	return sp->map;
}

// テレシネパターン推定シフト判定

int detect_telecine_cross(int frame, int coeff_shift)
{
	AFS_SCAN_DATA *sp1, *sp2, *sp3, *sp4;
	int shift;

	sp1 = scanp(frame - 1);
	sp2 = scanp(frame);
	sp3 = scanp(frame + 1);
	sp4 = scanp(frame + 2);
	shift = 0;

	if(max2(absdiff(sp1->lf_motion + sp2->lf_motion, sp2->ff_motion),
		absdiff(sp3->ff_motion + sp4->ff_motion, sp3->lf_motion)) * coeff_shift >
		max3(absdiff(sp1->ff_motion + sp2->ff_motion, sp1->lf_motion),
		absdiff(sp2->ff_motion + sp3->ff_motion, sp2->lf_motion),
		absdiff(sp3->lf_motion + sp4->lf_motion, sp4->ff_motion)) * 256)
		if(max2(sp2->lf_motion, sp3->ff_motion) * coeff_shift > sp2->ff_motion * 256)
			shift = 1;

	if(max2(absdiff(sp1->lf_motion + sp2->lf_motion, sp2->ff_motion),
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
							int *reverse, int *assume_shift, int *result_stat)
{
	AFS_SCAN_DATA *scp;
	AFS_STRIPE_DATA *stp;
	int total, threshold, i;
	unsigned char status;

	for(i = 0; i < 4; i++)
		assume_shift[i] = detect_telecine_cross(frame + i, coeff_shift);

	scp = scanp(frame);
	total = 0;
	if(scan_h - scp->bottom - ((scan_h - scp->top - scp->bottom) & 1) > scp->top && scan_w - scp->right > scp->left)
		total = (scan_h - scp->bottom - ((scan_h - scp->top - scp->bottom) & 1) - scp->top) * (scan_w - scp->right - scp->left);
	threshold = (total * method_watershed) >> 12;

	for(i = 0; i < 4; i++){
		get_stripe_info(frame + i, 0);
		stp = stripep(frame + i);

		result_stat[i] = (stp->count0 * coeff_shift > stp->count1 * 256) ? 1 : 0;
		if(threshold > stp->count1 && threshold > stp->count0)
			result_stat[i] += 2;
	}

	status = AFS_STATUS_DEFAULT;
	if(result_stat[0] & 2)
		status |= assume_shift[0] ? AFS_FLAG_SHIFT0 : 0;
	else
		status |= (result_stat[0] & 1) ? AFS_FLAG_SHIFT0 : 0;
	if(reverse[0]) status ^= AFS_FLAG_SHIFT0;

	if(result_stat[1] & 2)
		status |= assume_shift[1] ? AFS_FLAG_SHIFT1 : 0;
	else
		status |= (result_stat[1] & 1) ? AFS_FLAG_SHIFT1 : 0;
	if(reverse[1]) status ^= AFS_FLAG_SHIFT1;

	if(result_stat[2] & 2)
		status |= assume_shift[2] ? AFS_FLAG_SHIFT2 : 0;
	else
		status |= (result_stat[2] & 1) ? AFS_FLAG_SHIFT2 : 0;
	if(reverse[2]) status ^= AFS_FLAG_SHIFT2;

	if(result_stat[3] & 2)
		status |= assume_shift[3] ? AFS_FLAG_SHIFT3 : 0;
	else
		status |= (result_stat[3] & 1) ? AFS_FLAG_SHIFT3 : 0;
	if(reverse[3]) status ^= AFS_FLAG_SHIFT3;

	if(drop){
		status |= AFS_FLAG_FRAME_DROP;
		if(smooth) status |= AFS_FLAG_SMOOTHING;
	}
	if(force24) status |= AFS_FLAG_FORCE24;
	if(frame < 1) status &= AFS_MASK_SHIFT0;

	return status;
}

// シーンチェンジ解析

BOOL analyze_scene_change(unsigned char* sip, int tb_order, int max_w, int w, int h,
						  PIXEL_YC *p0, PIXEL_YC *p1, int top, int bottom, int left, int right)
{
	PIXEL_YC *ycp0, *ycp1;
	unsigned char* sip0;
	int pos_x, pos_y;
	int count, count0;
	//int hist3d0[4096], hist3d1[4096];
	int hist3d[2][4096];
	int si_w = si_pitch(w);
	int i;

	count = count0 = 0;
	for(pos_y = top; pos_y < h - bottom; pos_y++){
		if(!is_latter_field(pos_y, tb_order)){
			sip0 = sip + pos_y * si_w + left;
			for(pos_x = left; pos_x < w - right; pos_x++){
				count++;
				count0 += (!(*sip0 & 0x06));
				//if(!(*sip0 & 0x06)) count0++;
				sip0++;
			}
		}
	}

	if(count0 < (count - count0) * 3) return FALSE;

	//for(i = 0; i < 4096; i++) hist3d0[i] = hist3d1[i] = 0;
	memset(hist3d, 0, sizeof(hist3d));

	for(pos_y = top; pos_y < h - bottom; pos_y++){
		ycp0 = p0 + pos_y * max_w + left;
		ycp1 = p1 + pos_y * max_w + left;
		for(pos_x = left; pos_x < w - right; pos_x++){
			//hist3d0[((ycp0->y + 128) & 0xf00) | (((ycp0->cr + 128) >> 4) & 0x0f0) | (((ycp0->cb + 128) >> 8) & 0x00f)]++;
			//hist3d1[((ycp1->y + 128) & 0xf00) | (((ycp1->cr + 128) >> 4) & 0x0f0) | (((ycp1->cb + 128) >> 8) & 0x00f)]++;
			hist3d[0][((ycp0->y + 128) & 0xf00) | (((ycp0->cr + 128) >> 4) & 0x0f0) | (((ycp0->cb + 128) >> 8) & 0x00f)]++;
			hist3d[1][((ycp1->y + 128) & 0xf00) | (((ycp1->cr + 128) >> 4) & 0x0f0) | (((ycp1->cb + 128) >> 8) & 0x00f)]++;
			ycp0++;
			ycp1++;
		}
	}

	count0 = 0;
	for(i = 0; i < 4096; i++)
		count0 += hist3d[(hist3d[0][i] >= hist3d[1][i])][i];
		//if(hist3d0[i] < hist3d1[i])
		//	count0 += hist3d0[i];
		//else
		//	count0 += hist3d1[i];

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

void dot_white(PIXEL_YC *ycp, int max_w, int width, int height, int x, int y)
{
	if(x < 0 || x >= width || y < 0 || y >= height) return;

	ycp += y * max_w + x;
	ycp->y = 4096, ycp->cb = 0, ycp->cr = 0;

	return;
}

void dot_grey(PIXEL_YC *ycp, int max_w, int width, int height, int x, int y)
{
	if(x < 0 || x >= width || y < 0 || y >= height) return;

	ycp += y * max_w + x;
	ycp->y = 2048, ycp->cb = 0, ycp->cr = 0;

	return;
}

void dot_red(PIXEL_YC *ycp, int max_w, int width, int height, int x, int y)
{
	if(x < 0 || x >= width || y < 0 || y >= height) return;

	ycp += y * max_w + x;
	ycp->y = 1225, ycp->cb = -692, ycp->cr = 2048;

	return;
}

void dot_darkred(PIXEL_YC *ycp, int max_w, int width, int height, int x, int y)
{
	if(x < 0 || x >= width || y < 0 || y >= height) return;

	ycp += y * max_w + x;
	ycp->y = 460, ycp->cb = -260, ycp->cr = 768;

	return;
}

void dot_blue(PIXEL_YC *ycp, int max_w, int width, int height, int x, int y)
{
	if(x < 0 || x >= width || y < 0 || y >= height) return;

	ycp += y * max_w + x;
	ycp->y = 467, ycp->cb = 2048, ycp->cr = -332;

	return;
}

void dot_darkblue(PIXEL_YC *ycp, int max_w, int width, int height, int x, int y)
{
	if(x < 0 || x >= width || y < 0 || y >= height) return;

	ycp += y * max_w + x;
	ycp->y = 233, ycp->cb = 1024, ycp->cr = -166;

	return;
}

static short icon_color[6][3] = {
	{4096,0,0}, {2038,0,0}, {1225,-692,2048}, {460,-260,768}, {467,2048,-332}, {233,1024,-166},
};

void disp_icon(PIXEL_YC *ycp, int max_w, int width, int height, DWORD* icon, int x, int y, int lines, int color)
{
	int pos_x, pos_y;
	short *colorp;
	DWORD pixbits;
	PIXEL_YC *ycp_temp;

	ycp += y * max_w + x;
	colorp = icon_color[color];
	for(pos_y = y; pos_y < y + lines; pos_y++){
		pixbits = *icon++;
		ycp_temp = ycp;
		ycp += max_w;
		for(pos_x = x; pos_x < x + 32; pos_x++){
			if(pos_x >= 0 && pos_x < width && pos_y >= 0 && y < height && (pixbits & 1)){
				ycp_temp->y  = colorp[0];
				ycp_temp->cb = colorp[1];
				ycp_temp->cr = colorp[2];
			}
			pixbits >>= 1;
			ycp_temp++;
		}
	}
}

void disp_icon2(PIXEL_YC *ycp, int max_w, int width, int height, DWORD* icon, int x, int y, int lines, int color)
{
	disp_icon(ycp, max_w, width, height, icon, x+1, y+1, lines, 1);
	disp_icon(ycp, max_w, width, height, icon, x, y, lines, color);
}

void disp_status(PIXEL_YC *ycp, int *result_stat, int *assume_shift, int *reverse,
				 int max_w, int width, int height, int top, int bottom, int left, int right)
{
	int pos_x, pos_y, i;

	for(pos_y = top; pos_y < top + 80; pos_y++){
		dot_grey(ycp, max_w, width, height, left +  48, pos_y);
		dot_grey(ycp, max_w, width, height, left +  80, pos_y);
		dot_grey(ycp, max_w, width, height, left + 112, pos_y);
		dot_grey(ycp, max_w, width, height, left + 144, pos_y);
		dot_grey(ycp, max_w, width, height, left + 176, pos_y);
	}
	for(pos_x = left+48; pos_x < left + 176; pos_x++){
		dot_grey(ycp, max_w, width, height, pos_x , top + 16);
		dot_grey(ycp, max_w, width, height, pos_x , top + 32);
		dot_grey(ycp, max_w, width, height, pos_x , top + 48);
		dot_grey(ycp, max_w, width, height, pos_x , top + 80);
	}
	if(left + right < width - 1 && top + bottom < height - 1){
		for(pos_x = left; pos_x < width - 1 - right; pos_x++){
			dot_white(ycp, max_w, width, height, pos_x, top);
			dot_white(ycp, max_w, width, height, pos_x, height - 1 - bottom);
		}
		for(pos_y = top; pos_y < height - 1 - bottom; pos_y++){
			dot_white(ycp, max_w, width, height, left, pos_y);
			dot_white(ycp, max_w, width, height, width - 1 - right, pos_y);
		}
	}

	disp_icon2(ycp, max_w, width, height, icon_sima,   left, top,    16, 2);
	disp_icon2(ycp, max_w, width, height, icon_23,     left, top+16, 16, 4);
	disp_icon2(ycp, max_w, width, height, icon_hanten, left, top+32, 16, 0);
	disp_icon2(ycp, max_w, width, height, icon_hantei, left, top+48, 32, 0);

	for(i = 0; i < 4; i++)
		if(result_stat[i] & 2)
			if(result_stat[i] & 1)
				disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+32+32*i, top,    8,  3);
			else
				disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+48+32*i, top+8,  8,  3);
		else
			if(result_stat[i] & 1)
				disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+32+32*i, top,    8,  2);
			else
				disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+48+32*i, top+8,  8,  2);

	for(i = 0; i < 4; i++)
		if(result_stat[i] & 2)
			if(assume_shift[i] & 1)
				disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+32+32*i, top+16, 8,  4);
			else
				disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+48+32*i, top+24, 8,  4);
		else
			if(assume_shift[i] & 1)
				disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+32+32*i, top+16, 8,  5);
			else
				disp_icon2(ycp, max_w, width, height, icon_arrow_thin, left+48+32*i, top+24, 8,  5);

	for(i = 0; i < 4; i++)
		if(reverse[i])
			disp_icon2(ycp, max_w, width, height, icon_arrow_reverse, left+40+32*i, top+32, 16, 0);

	for(i = 0; i < 4; i++)
		if(result_stat[i] & 2)
			if(((assume_shift[i] & 1) && !reverse[i]) || ((~assume_shift[i] & 1) && reverse[i]))
				disp_icon2(ycp, max_w, width, height, icon_arrow_bold, left+32+32*i, top+48, 16, 4);
			else
				disp_icon2(ycp, max_w, width, height, icon_arrow_bold, left+48+32*i, top+64, 16, 4);
		else
			if(((result_stat[i] & 1) && !reverse[i]) || ((~result_stat[i] & 1) && reverse[i]))
				disp_icon2(ycp, max_w, width, height, icon_arrow_bold, left+32+32*i, top+48, 16, 2);
			else
				disp_icon2(ycp, max_w, width, height, icon_arrow_bold, left+48+32*i, top+64, 16, 2);
}

//
// フィルタ処理関数
//

BOOL func_proc( FILTER *fp,FILTER_PROC_INFO *fpip )
{
	//
	//	fp->track[n]			: トラックバーの数値
	//	fp->check[n]			: チェックボックスの数値
	//	fpip->w 				: 実際の画像の横幅
	//	fpip->h 				: 実際の画像の縦幅
	//	fpip->max_w				: 画像領域の横幅
	//	fpip->max_h				: 画像領域の縦幅
	//	fpip->ycp_edit			: 画像領域へのポインタ
	//	fpip->ycp_temp			: テンポラリ領域へのポインタ
	//	fpip->ycp_edit[n].y		: 画素(輝度    )データ (     0 ～ 4095 )
	//	fpip->ycp_edit[n].cb	: 画素(色差(青))データ ( -2048 ～ 2047 )
	//	fpip->ycp_edit[n].cr	: 画素(色差(赤))データ ( -2048 ～ 2047 )
	//
	//	インターレース解除フィルタは画像サイズを変えたり
	//	画像領域とテンポラリ領域を入れ替えたりは出来ません。
	//
	//	画像領域に初期画像データは入っていません。
	//	get_ycp_source_cache()を使って自分で読み込む必要があります。
	//
	unsigned char *sip, *sip0;
	int pos_x, pos_y, i;
	PIXEL_YC *p0, *p1, *ycp, *ycp0, *ycp1;
	int hit, prev_hit;
	int clip_t, clip_b, clip_l, clip_r;
	int method_watershed, coeff_shift;
	int thre_shift, thre_deint, thre_Ymotion, thre_Cmotion;
	int analyze, drop, smooth, force24, detect_sc, edit_mode, tune_mode;
	int log_save, trace_mode, replay_mode, through_mode;
	int tb_order, reverse[4], is_saving, is_editing;
	unsigned char status;
	int assume_shift[4], result_stat[4];
	int si_w;
	int num_thread;

#ifdef AFSVF
	if(fp->exfunc->set_ycp_filtering_cache_size(fp, fpip->max_w, fpip->max_h, AFS_SOURCE_CACHE_NUM, NULL) == NULL){
		error_modal(fp, fpip->editp, "フィルタキャッシュの確保に失敗しました。");
		return FALSE;
	};
#endif
	get_qp_counter(&afs_qpc.qpc_tmp[0]);

	// 設定値読み出し
	clip_t = fp->track[0];
	clip_b = fp->track[1];
	clip_l = fp->track[2];
	clip_r = fp->track[3];
	method_watershed = fp->track[4];
	coeff_shift = fp->track[5];
	thre_shift = fp->track[6];
	thre_deint = fp->track[7];
	thre_Ymotion = fp->track[8];
	thre_Cmotion = fp->track[9];
	analyze = fp->track[10];
	num_thread = fp->track[11];
	if(!fp->check[0]){
		method_watershed = 0;
		coeff_shift = 0;
	}
	drop = (fp->check[0] && fp->check[1]);
	smooth = (drop && fp->check[2]);
	force24 = fp->check[3];
	detect_sc = fp->check[4];
	edit_mode = fp->check[5];
	tune_mode = fp->check[6];
	log_save = fp->check[7];
	trace_mode = fp->check[8];
	replay_mode = fp->check[9];
	through_mode = fp->check[11];

	is_saving = fp->exfunc->is_saving(fpip->editp);
	is_editing = fp->exfunc->is_editing(fpip->editp);
	tb_order = ((fpip->flag & FILTER_PROC_INFO_FLAG_INVERT_FIELD_ORDER) != 0);
#ifdef AFSVF
	tb_order = fp->check[10] ? !tb_order : tb_order;

	if(trace_mode){
		fill_this_ycp(fp, fpip);
		return TRUE;
	}
#endif

	if(is_saving || !is_editing)
		edit_mode = tune_mode = 0;
	else if(edit_mode)
		analyze = 0;
	else if(tune_mode)
		analyze += !analyze;

	// 反転情報取得
	for(i = 0; i < 4; i++)
		reverse[i] = is_frame_reverse(fp, fpip, fpip->frame + i);

	if(!afs_is_ready()){
		fill_this_ycp(fp, fpip);
		error_modal(fp, fpip->editp, "共有エラー");
		return TRUE;
	}

#ifndef AFSVF
	if(trace_mode){
		fill_this_ycp(fp, fpip);
		return TRUE;
	}

	if(set_source_cache_size(fpip->frame_n, fpip->max_w, fpip->max_h) != TRUE){
		fill_this_ycp(fp, fpip);
		error_modal(fp, fpip->editp, "フィルタキャッシュの確保に失敗しました。");
		return TRUE;
	};
#endif

	p0 = get_ycp_cache(fp, fpip, fpip->frame, NULL);
	if(p0 == NULL){
		error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
		return TRUE;
	}

	// 解析情報キャッシュ確保
	if(!check_scan_cache(fpip->frame_n, fpip->w, fpip->h, num_thread)){
		fill_this_ycp(fp, fpip);
		error_modal(fp, fpip->editp, "解析用メモリが確保できません。");
		return TRUE;
	}

	if(through_mode){
		if(!trace_mode && !replay_mode){
			afs_set(fpip->frame_n, fpip->frame, AFS_STATUS_DEFAULT);
		}
		p1 = get_ycp_cache(fp, fpip, fpip->frame - 1, NULL);
		if(p1 == NULL){
			error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
			return TRUE;
		}

		for(pos_y = 0; pos_y < fpip->h; pos_y++){
			ycp = fpip->ycp_edit + pos_y * fpip->max_w;
			ycp0 = p0 + pos_y * fpip->max_w;
			ycp1 = p1 + pos_y * fpip->max_w;
			if(is_latter_field(pos_y, tb_order) && reverse[0])
				memcpy(ycp, ycp1, sizeof(PIXEL_YC) * fpip->w);
			else
				memcpy(ycp, ycp0, sizeof(PIXEL_YC) * fpip->w);
		}

		return TRUE;
	}
	
	get_qp_counter(&afs_qpc.qpc_tmp[1]);
	// 不足解析補充
	ycp0 = get_ycp_cache(fp, fpip, fpip->frame - 2, &hit);
	if(ycp0 == NULL){
		error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
		return TRUE;
	}
	get_qp_counter(&afs_qpc.qpc_tmp[2]);
	add_qpctime(&afs_qpc.qpc_value[0], afs_qpc.qpc_tmp[1] - afs_qpc.qpc_tmp[0]);
	add_qpctime(&afs_qpc.qpc_value[1], afs_qpc.qpc_tmp[2] - afs_qpc.qpc_tmp[1]);
	for(i = -1; i <= 7; i++){
		get_qp_counter(&afs_qpc.qpc_tmp[1]);
		ycp1 = ycp0;
		prev_hit = hit;
		ycp0 = get_ycp_cache(fp, fpip, fpip->frame + i, &hit);
		if(ycp0 == NULL){
			error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
			return TRUE;
		}
		get_qp_counter(&afs_qpc.qpc_tmp[2]);
		scan_frame(fpip->frame + i, (!prev_hit || !hit), fpip->max_w, ycp1, ycp0,
			(analyze == 0 ? 0 : 1), tb_order, thre_shift, thre_deint, thre_Ymotion, thre_Cmotion);
		get_qp_counter(&afs_qpc.qpc_tmp[3]);
		count_motion(fpip->frame + i, clip_t, clip_b, clip_l, clip_r);
		get_qp_counter(&afs_qpc.qpc_tmp[4]);
		add_qpctime(&afs_qpc.qpc_value[1], afs_qpc.qpc_tmp[2] - afs_qpc.qpc_tmp[1]);
		add_qpctime(&afs_qpc.qpc_value[2], afs_qpc.qpc_tmp[3] - afs_qpc.qpc_tmp[2]);
		add_qpctime(&afs_qpc.qpc_value[3], afs_qpc.qpc_tmp[4] - afs_qpc.qpc_tmp[3]);
	}

	// 共有メモリ、解析情報キャッシュ読み出し
	get_qp_counter(&afs_qpc.qpc_tmp[1]);
	if(fpip->frame + 2 < fpip->frame_n){
		status = analyze_frame(fpip->frame + 2, drop, smooth, force24, coeff_shift, method_watershed, reverse, assume_shift, result_stat);
		if(!replay_mode) afs_set(fpip->frame_n, fpip->frame + 2, status);
	}
	if(fpip->frame + 1 < fpip->frame_n){
		status = analyze_frame(fpip->frame + 1, drop, smooth, force24, coeff_shift, method_watershed, reverse, assume_shift, result_stat);
		if(!replay_mode) afs_set(fpip->frame_n, fpip->frame + 1, status);
	}
	status = analyze_frame(fpip->frame, drop, smooth, force24, coeff_shift, method_watershed, reverse, assume_shift, result_stat);
	if(replay_mode){
		status = afs_get_status(fpip->frame_n, fpip->frame);
		status &= AFS_MASK_FRAME_DROP & AFS_MASK_SMOOTHING & AFS_MASK_FORCE24;
		if(drop){
			status |= AFS_FLAG_FRAME_DROP;
			if(smooth) status |= AFS_FLAG_SMOOTHING;
		}
		if(force24) status |= AFS_FLAG_FORCE24;
		if(fpip->frame < 1) status &= AFS_MASK_SHIFT0;
	}
	afs_set(fpip->frame_n, fpip->frame, status);
	log_save_check = 1;
	get_qp_counter(&afs_qpc.qpc_tmp[2]);
	add_qpctime(&afs_qpc.qpc_value[4], afs_qpc.qpc_tmp[2] - afs_qpc.qpc_tmp[1]);

	sip = get_stripe_info(fpip->frame, 1);
	si_w = si_pitch(fpip->w);
	get_qp_counter(&afs_qpc.qpc_tmp[3]);

	// 解析マップをフィルタ
	if(analyze > 1){
		afs_func.analyzemap_filter(sip, si_w, fpip->w, fpip->h);
		stripe_info_dirty(fpip->frame);
	}
	get_qp_counter(&afs_qpc.qpc_tmp[4]);
	add_qpctime(&afs_qpc.qpc_value[5], afs_qpc.qpc_tmp[3] - afs_qpc.qpc_tmp[2]);
	add_qpctime(&afs_qpc.qpc_value[6], afs_qpc.qpc_tmp[4] - afs_qpc.qpc_tmp[3]);

	// 解除済み画面合成
	p0 = get_ycp_cache(fp, fpip, fpip->frame, NULL);
	if(p0 == NULL){
		error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
		return TRUE;
	}
	p1 = get_ycp_cache(fp, fpip, fpip->frame - 1, NULL);
	if(p1 == NULL){
		memcpy(fpip->ycp_edit, p0, sizeof(PIXEL_YC) * fpip-> h * fpip->max_w);
		error_modal(fp, fpip->editp, "ソース画像の読み込みに失敗しました。");
		return TRUE;
	}
	get_qp_counter(&afs_qpc.qpc_tmp[5]);
	add_qpctime(&afs_qpc.qpc_value[7], &afs_qpc.qpc_tmp[5] - &afs_qpc.qpc_tmp[4]);

	if(tune_mode){
		for(pos_y = 0; pos_y <= fpip->h - 1; pos_y++){
			ycp  = fpip->ycp_edit + pos_y * fpip->max_w;
			sip0 = sip + pos_y * si_w;
			if(status & AFS_FLAG_SHIFT0){
				for(pos_x = 0; pos_x < fpip->w; pos_x++){
					if(!(*sip0 & 0x06))
						ycp->y = 2871, ycp->cb = 692, ycp->cr = -2048;
					else if(~*sip0 & 0x02)
						ycp->y = 1536, ycp->cb = ycp->cr = 0;
					else if(~*sip0 & 0x04)
						ycp->y = 467, ycp->cb = 2048, ycp->cr =  -332;
					else
						ycp->y = ycp->cb = ycp->cr = 0;
					ycp++;
					sip0++;
				}
			}else{
				for(pos_x = 0; pos_x < fpip->w; pos_x++){
					if(!(*sip0 & 0x05))
						ycp->y = 2871, ycp->cb = 692, ycp->cr = -2048;
					else if(~*sip0 & 0x01)
						ycp->y = 1536, ycp->cb = ycp->cr = 0;
					else if(~*sip0 & 0x04)
						ycp->y = 467, ycp->cb = 2048, ycp->cr =  -332;
					else
						ycp->y = ycp->cb = ycp->cr = 0;
					ycp++;
					sip0++;
				}
			}
		}
	}else if(analyze == 5){
		PIXEL_YC *p01, *p02, *p03, *p11, *p12, *p13;
		PIXEL_YC *ycp01, *ycp02, *ycp03, *ycp11, *ycp12, *ycp13;

		p01 = p03 = p0,         p11 = p13 = p1;
		p02 = p0 + fpip->max_w, p12 = p1 + fpip->max_w;
		for(pos_y = 0; pos_y <= fpip->h - 1; pos_y++){
			p01 = p02, p11 = p12;
			p02 = p03, p12 = p13;
			if(pos_y < fpip->h - 1){
				p03 += fpip->max_w, p13 += fpip->max_w;
			}else{
				p03 = p01, p13 = p11;
			}
			ycp  = fpip->ycp_edit + pos_y * fpip->max_w;
			sip0 = sip + pos_y * si_w;
			if(status & AFS_FLAG_SHIFT0){
				if(!is_latter_field(pos_y, tb_order)){
					ycp02 = p02, ycp11 = p11, ycp12 = p12, ycp13 = p13;
					for(pos_x = 0; pos_x < fpip->w; pos_x++){
						if(!(*sip0 & 0x06)){
							PIXEL_YC *pix1, *pix2;
							if(pos_x >= 2 && pos_x < fpip->w - 2){
								int d, d_min;
								d_min = d = absdiff((ycp11-2)->y, (ycp13+2)->y);
								pix1 = ycp11-2, pix2 = ycp13+2;
								d = absdiff((ycp11+2)->y, (ycp13-2)->y);
								if(d < d_min) d = d_min, pix1 = ycp11+2, pix2 = ycp13-2;
								d = absdiff((ycp11-1)->y, (ycp13+1)->y);
								if(d < d_min) d = d_min, pix1 = ycp11-1, pix2 = ycp13+1;
								d = absdiff((ycp11+1)->y, (ycp13-1)->y);
								if(d < d_min) d = d_min, pix1 = ycp11+1, pix2 = ycp13-1;
								d = absdiff(ycp11->y, ycp13->y);
								if(d < d_min || ((ycp11->y + ycp11->y - pix1->y - pix2->y)^(pix1->y  + pix2->y - ycp13->y - ycp13->y)) < 0)
									pix1 = ycp11, pix2 = ycp13;
							}else
								pix1 = ycp11, pix2 = ycp13;
							ycp->y  = (pix1->y  + pix2->y  + 1) >> 1;
							ycp->cr = (pix1->cr + pix2->cr + 1) >> 1;
							ycp->cb = (pix1->cb + pix2->cb + 1) >> 1;
						}else
							*ycp = *ycp02;
						ycp++, ycp02++, ycp11++, ycp12++, ycp13++, sip0++;
					}
				}else
					memcpy(ycp, p12, sizeof(PIXEL_YC) * fpip->w);
			}else{
				if(is_latter_field(pos_y, tb_order)){
					ycp01 = p01, ycp02 = p02, ycp03 = p03, ycp12 = p12;
					for(pos_x = 0; pos_x < fpip->w; pos_x++){
						if(!(*sip0 & 0x05)){
							PIXEL_YC *pix1, *pix2;
							if(pos_x >= 2 && pos_x < fpip->w - 2){
								int d, d_min;
								d_min = d = absdiff((ycp01-2)->y, (ycp03+2)->y);
								pix1 = ycp01-2, pix2 = ycp03+2;
								d = absdiff((ycp01+2)->y, (ycp03-2)->y);
								if(d < d_min) d = d_min, pix1 = ycp01+2, pix2 = ycp03-2;
								d = absdiff((ycp01-1)->y, (ycp03+1)->y);
								if(d < d_min) d = d_min, pix1 = ycp01-1, pix2 = ycp03+1;
								d = absdiff((ycp01+1)->y, (ycp03-1)->y);
								if(d < d_min) d = d_min, pix1 = ycp01+1, pix2 = ycp03-1;
								d = absdiff(ycp01->y, ycp03->y);
								if(d < d_min || ((ycp01->y + ycp01->y - pix1->y - pix2->y)^(pix1->y  + pix2->y - ycp03->y - ycp03->y)) < 0)
									pix1 = ycp01, pix2 = ycp03;
							}else
								pix1 = ycp01, pix2 = ycp03;
							ycp->y  = (pix1->y  + pix2->y  + 1) >> 1;
							ycp->cr = (pix1->cr + pix2->cr + 1) >> 1;
							ycp->cb = (pix1->cb + pix2->cb + 1) >> 1;
						}else
							*ycp = *ycp02;
						ycp++, ycp01++, ycp02++, ycp03++, ycp12++, sip0++;
					}
				}else
					memcpy(ycp, p02, sizeof(PIXEL_YC) * fpip->w);
			}
		}
	}else if(analyze == 4){
		PIXEL_YC *p01, *p02, *p03, *p04, *p05, *p06, *p07;
		PIXEL_YC *p11, *p12, *p13, *p14, *p15, *p16, *p17;

		p01 = p03 = p05 = p0;
		p02 = p04 = p06 = p0 + fpip->max_w;
		p07 = p0 + fpip->max_w * 2;
		p11 = p13 = p15 = p1;
		p12 = p14 = p16 = p1 + fpip->max_w;
		p17 = p1 + fpip->max_w * 2;
		for(pos_y = 0; pos_y <= fpip->h - 1; pos_y++){
			p01 = p02; p02 = p03; p03 = p04; p04 = p05; p05 = p06; p06 = p07;
			p11 = p12; p12 = p13; p13 = p14; p14 = p15; p15 = p16; p16 = p17;
			if(pos_y < fpip->h - 3){
				p07 += fpip->max_w;
				p17 += fpip->max_w;
			}else{
				p07 = p05;
				p17 = p15;
			}
			ycp  = fpip->ycp_edit + pos_y * fpip->max_w;
			sip0 = sip + pos_y * si_w;
			if(status & AFS_FLAG_SHIFT0){
				if(!is_latter_field(pos_y, tb_order)) {
					afs_func.deint4(ycp, p11, p13, p04, p15, p17, sip0, 0x06060606, fpip->w);
				} else {
					memcpy(ycp, p14, sizeof(PIXEL_YC) * fpip->w);
				}
			}else{
				if(is_latter_field(pos_y, tb_order)) {
					afs_func.deint4(ycp, p01, p03, p04, p05, p07, sip0, 0x05050505, fpip->w);
				} else {
					memcpy(ycp, p04, sizeof(PIXEL_YC) * fpip->w);
				}
			}
		}
	}else if(analyze == 3){
		PIXEL_YC *p01, *p02, *p03, *p11, *p12, *p13;

		p01 = p03 = p0,         p11 = p13 = p1;
		p02 = p0 + fpip->max_w, p12 = p1 + fpip->max_w;
		for(pos_y = 0; pos_y <= fpip->h - 1; pos_y++){
			p01 = p02, p11 = p12;
			p02 = p03, p12 = p13;
			if(pos_y < fpip->h - 1){
				p03 += fpip->max_w, p13 += fpip->max_w;
			}else{
				p03 = p01, p13 = p11;
			}
			ycp  = fpip->ycp_edit + pos_y * fpip->max_w;
			sip0 = sip + pos_y * si_w;
			if(status & AFS_FLAG_SHIFT0){
				if(!is_latter_field(pos_y, tb_order)) {
					afs_func.blend(ycp, p11, p02, p13, sip0, 0x06060606, fpip->w);
				} else {
					afs_func.blend(ycp, p01, p12, p03, sip0, 0x06060606, fpip->w);
				}
			}else{
				afs_func.blend(ycp, p01, p02, p03, sip0, 0x05050505, fpip->w);
			}
		}
	}else if(analyze == 2){
		PIXEL_YC *p01, *p02, *p03, *p11, *p12, *p13;

		p01 = p03 = p0,         p11 = p13 = p1;
		p02 = p0 + fpip->max_w, p12 = p1 + fpip->max_w;
		for(pos_y = 0; pos_y <= fpip->h - 1; pos_y++){
			p01 = p02, p11 = p12;
			p02 = p03, p12 = p13;
			if(pos_y < fpip->h - 1){
				p03 += fpip->max_w, p13 += fpip->max_w;
			}else{
				p03 = p01, p13 = p11;
			}
			ycp  = fpip->ycp_edit + pos_y * fpip->max_w;
			sip0 = sip + pos_y * si_w;
			if(status & AFS_FLAG_SHIFT0){
				if(!is_latter_field(pos_y, tb_order)) {
					afs_func.blend(ycp, p11, p02, p13, sip0, 0x02020202, fpip->w);
				} else {
					afs_func.blend(ycp, p01, p12, p03, sip0, 0x02020202, fpip->w);
				}
			}else {
				afs_func.blend(ycp, p01, p02, p03, sip0, 0x01010101, fpip->w);
			}
		}
	}else if(analyze == 1){
		PIXEL_YC *p01, *p02, *p03, *p11, *p12, *p13;

		p01 = p03 = p0,         p11 = p13 = p1;
		p02 = p0 + fpip->max_w, p12 = p1 + fpip->max_w;
		if(detect_sc && (~status & AFS_FLAG_SHIFT0))
			if(analyze_scene_change(sip, tb_order, fpip->max_w, fpip->w, fpip->h, p0, p1, clip_t, clip_b, clip_l, clip_r))
				p11 = p01, p12 = p02, p13 = p03;
		for(pos_y = 0; pos_y <= fpip->h - 1; pos_y++){
			p01 = p02, p11 = p12;
			p02 = p03, p12 = p13;
			if(pos_y < fpip->h - 1){
				p03 += fpip->max_w, p13 += fpip->max_w;
			}else{
				p03 = p01, p13 = p11;
			}
			ycp  = fpip->ycp_edit + pos_y * fpip->max_w;
			if(status & AFS_FLAG_SHIFT0){
				if(!is_latter_field(pos_y, tb_order)) {
					afs_func.mie_inter(ycp, p02, p11, p12, p13, fpip->w);
				}else{
					afs_func.mie_spot(ycp, p01, p03, p11, p13, p12, fpip->w);
				}
			}else{
				if(is_latter_field(pos_y, tb_order)){
					afs_func.mie_inter(ycp, p01, p02, p03, p12, fpip->w);
				}else{
					afs_func.mie_spot(ycp, p01, p03, p11, p13, p02, fpip->w);
				}
			}
		}
	}else
		for(pos_y = 0; pos_y < fpip->h; pos_y++){
			ycp = fpip->ycp_edit + pos_y * fpip->max_w;
			ycp0 = p0 + pos_y * fpip->max_w;
			ycp1 = p1 + pos_y * fpip->max_w;
			if(is_latter_field(pos_y, tb_order) && (status & AFS_FLAG_SHIFT0))
				memcpy(ycp, ycp1, sizeof(PIXEL_YC) * fpip->w);
			else
				memcpy(ycp, ycp0, sizeof(PIXEL_YC) * fpip->w);
		}

		// 解析結果表示
		if(edit_mode)
			disp_status(fpip->ycp_edit, result_stat, assume_shift, reverse,
			fpip->max_w, fpip->w, fpip->h, clip_t, clip_b, clip_l, clip_r);

	get_qp_counter(&afs_qpc.qpc_tmp[6]);
	add_qpctime(&afs_qpc.qpc_value[8], afs_qpc.qpc_tmp[6] - afs_qpc.qpc_tmp[5]);
	add_qpctime(&afs_qpc.qpc_value[9], afs_qpc.qpc_tmp[6] - afs_qpc.qpc_tmp[0]);

#if CHECK_PERFORMANCE
	//適当に速度を計測して時たま吐く
	if ((fpip->frame & 1023) == 0) {
		FILE *fp = NULL;
		if (0 == fopen_s(&fp, "afs_log.csv", "ab")) {
			fprintf(fp, "frame count        ,     %d\n", fpip->frame);
			fprintf(fp, "total              , %12.3f, ms\r\n", afs_qpc.qpc_value[9] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "init               , %12.3f, ms\r\n", afs_qpc.qpc_value[0] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "get_ycp_cache      , %12.3f, ms\r\n", afs_qpc.qpc_value[1] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "scan_frame         , %12.3f, ms\r\n", afs_qpc.qpc_value[2] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "count_motion       , %12.3f, ms\r\n", afs_qpc.qpc_value[3] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "analyze_frame      , %12.3f, ms\r\n", afs_qpc.qpc_value[4] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "get_strip_count    , %12.3f, ms\r\n", afs_qpc.qpc_value[5] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "analyze_map_filter , %12.3f, ms\r\n", afs_qpc.qpc_value[6] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "get_ycp_cache      , %12.3f, ms\r\n", afs_qpc.qpc_value[7] * 1000.0 / (double)afs_qpc.qpc_freq);
			fprintf(fp, "blend              , %12.3f, ms\r\n", afs_qpc.qpc_value[8] * 1000.0 / (double)afs_qpc.qpc_freq);
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

	if(fps != 24){
		rate = fps, scale = 30;
		for(i = 2; i < scale;)
			if(rate % i == 0 && scale % i == 0)
				rate /= i, scale /= i;
			else
				i++;
		return ((saveno + 1) * rate / scale > saveno * rate / scale);
	}

	//24fps
	if(saveno == 0){
		phase24 = 4;
		position24 = 1;
		prev_frame = frame;
		return TRUE;
	}
	frame_n = fp->exfunc->get_frame_n(editp);
	status = afs_get_status(frame_n, prev_frame);
	prev_frame = frame;
	if(saveno == 1){
		if(!(status & AFS_FLAG_SHIFT0) &&
			(status & AFS_FLAG_SHIFT1) &&
			(status & AFS_FLAG_SHIFT2))
			phase24 = 0;
	}
	drop24 = !(status & AFS_FLAG_SHIFT1) &&
		(status & AFS_FLAG_SHIFT2) &&
		(status & AFS_FLAG_SHIFT3);
	if(drop24 || (edit_flag & EDIT_FRAME_EDIT_FLAG_DELFRAME)) phase24 = (position24 + 100) % 5;
	drop24 = 0;
	if(position24 >= phase24 &&
		((position24 + 100) % 5 == phase24 ||
		(position24 +  99) % 5 == phase24)){
			position24 -= 5;
			drop24 = 1;
	}
	position24++;
	return drop24 ? FALSE : TRUE;
}
#endif

#define ID_BUTTON_LV1    40001
#define ID_BUTTON_LV2    40002
#define ID_BUTTON_LV3    40003
#define ID_BUTTON_LV4    40004
#define ID_BUTTON_24FPS  40005
#define ID_BUTTON_30FPS  40006
#define ID_BUTTON_STRIPE 40007
#define ID_BUTTON_MOTION 40008
#define ID_BUTTON_HDTV   40009

HFONT b_font;
HWND b_lv1, b_lv2, b_lv3, b_lv4, b_24fps, b_30fps, b_stripe, b_motion, b_hdtv;

static void init_dialog(HWND hwnd, HINSTANCE hinst);
static void on_lv1_button(FILTER *fp);
static void on_lv2_button(FILTER *fp);
static void on_lv3_button(FILTER *fp);
static void on_lv4_button(FILTER *fp);
static void on_24fps_button(FILTER *fp);
static void on_30fps_button(FILTER *fp);
static void on_stripe_button(FILTER *fp);
static void on_motion_button(FILTER *fp);
static void on_hdtv_button(FILTER *fp);

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam, void*, FILTER *fp)
{
	switch(message){
	case WM_FILTER_FILE_OPEN:
	case WM_FILTER_FILE_CLOSE:
		if(!afs_check_share()) break;
		afs_free_cache();
#ifndef AFSVF
		free_source_cache();
#endif
		free_analyze_cache();
		break;
	case WM_FILTER_INIT:
		init_dialog(hwnd, fp->dll_hinst);
		return TRUE;
	case WM_COMMAND:
		switch(LOWORD(wparam)){
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

static void init_dialog(HWND hwnd, HINSTANCE hinst)
{
	int top = 302;

	b_font = CreateFont(12, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, PROOF_QUALITY, DEFAULT_PITCH | FF_MODERN, "ＭＳ Ｐゴシック");

	b_lv1 = CreateWindow("BUTTON", "動き重視", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top, 90, 18, hwnd, (HMENU)ID_BUTTON_LV1, hinst, NULL);
	SendMessage(b_lv1, WM_SETFONT, (WPARAM)b_font, 0);

	b_lv2 = CreateWindow("BUTTON", "二重化", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+20, 90, 18, hwnd, (HMENU)ID_BUTTON_LV2, hinst, NULL);
	SendMessage(b_lv2, WM_SETFONT, (WPARAM)b_font, 0);

	b_lv3 = CreateWindow("BUTTON", "映画/アニメ", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+40, 90, 18, hwnd, (HMENU)ID_BUTTON_LV3, hinst, NULL);
	SendMessage(b_lv3, WM_SETFONT, (WPARAM)b_font, 0);

	b_lv4 = CreateWindow("BUTTON", "残像最小化", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+60, 90, 18, hwnd, (HMENU)ID_BUTTON_LV4, hinst, NULL);
	SendMessage(b_lv4, WM_SETFONT, (WPARAM)b_font, 0);

	b_24fps = CreateWindow("BUTTON", "24fps固定", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+80, 90, 18, hwnd, (HMENU)ID_BUTTON_24FPS, hinst, NULL);
	SendMessage(b_24fps, WM_SETFONT, (WPARAM)b_font, 0);

	b_30fps = CreateWindow("BUTTON", "→ 30fps固定", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+100, 90, 18, hwnd, (HMENU)ID_BUTTON_30FPS, hinst, NULL);
	SendMessage(b_30fps, WM_SETFONT, (WPARAM)b_font, 0);

	b_stripe = CreateWindow("BUTTON", "→ 解除強(縞)", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+120, 90, 18, hwnd, (HMENU)ID_BUTTON_STRIPE, hinst, NULL);
	SendMessage(b_stripe, WM_SETFONT, (WPARAM)b_font, 0);

	b_motion = CreateWindow("BUTTON", "→ 解除強(動)", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+140, 90, 18, hwnd, (HMENU)ID_BUTTON_MOTION, hinst, NULL);
	SendMessage(b_motion, WM_SETFONT, (WPARAM)b_font, 0);

	b_hdtv   = CreateWindow("BUTTON", "解除Lv0(HD)", WS_CHILD|WS_VISIBLE|WS_GROUP|WS_TABSTOP|BS_PUSHBUTTON|BS_VCENTER, 212, top+160, 90, 18, hwnd, (HMENU)ID_BUTTON_HDTV, hinst, NULL);
	SendMessage(b_hdtv, WM_SETFONT, (WPARAM)b_font, 0);
}

static void on_lv1_button(FILTER *fp)
{
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

static void on_lv2_button(FILTER *fp)
{
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

static void on_lv3_button(FILTER *fp)
{
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

static void on_lv4_button(FILTER *fp)
{
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

static void on_24fps_button(FILTER *fp)
{
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

static void on_30fps_button(FILTER *fp)
{
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

static void on_stripe_button(FILTER *fp)
{
	SendMessage(b_stripe, WM_KILLFOCUS,0,0);
	fp->track[7]  = 24;
	fp->exfunc->filter_window_update(fp);
}

static void on_motion_button(FILTER *fp)
{
	SendMessage(b_motion, WM_KILLFOCUS,0,0);
	fp->track[8]  = 64;
	fp->track[9]  = 128;
	fp->exfunc->filter_window_update(fp);
}

static void on_hdtv_button(FILTER *fp)
{
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
