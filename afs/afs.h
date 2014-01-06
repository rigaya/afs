#pragma once

typedef struct {
	int type;
	unsigned char *dst;
	PIXEL_YC *p0;
	PIXEL_YC *p1;
	int tb_order;
	int max_w;
	int si_pitch;
} AFS_SCAN_ARG;

typedef struct {
	unsigned char *map;
	int status, frame, mode, tb_order, thre_shift, thre_deint, thre_Ymotion, thre_Cmotion;
	int top, bottom, left, right;
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

#define CHECK_PERFORMANCE 0

typedef struct {
	__int64 qpc_tmp[16];
	__int64 qpc_value[15];
	__int64 qpc_freq;
} PERFORMANCE_CHECKER;


static __forceinline void get_qp_counter(__int64 *qpc) {
#if CHECK_PERFORMANCE
	QueryPerformanceCounter((LARGE_INTEGER *)qpc);
#endif
}

static __forceinline void add_qpctime(__int64 *qpc, __int64 add) {
#if CHECK_PERFORMANCE
	*qpc += add;
#endif
}
