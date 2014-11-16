#pragma once

#include "filter.h"

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
	PIXEL_YC *p0;
	PIXEL_YC *p1;
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
#define NUM_SUB_THREAD     2 //サブスレッド数の指定 0でトラックバーによる設定が可能に
#define SCAN_BACKGROUND    0 //scan処理をバックグラウンドで行う (バグってるので使用中止)
#define ANALYZE_BACKGROUND (0 & SCAN_BACKGROUND) //analyze処理をバックグラウンドで行う (バグってるので使用中止)
#define BACKGROUND_THREAD_BELOW_NORMAL 0 //バックグラウンドスレッドの優先度を下げる
#define SIMD_DEBUG         0 //SIMD処理をデバッグする
#define CHECK_PERFORMANCE  0 //パフォーマンスレポートを出力する

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
