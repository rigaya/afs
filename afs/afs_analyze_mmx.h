#ifdef __cplusplus
extern "C" {
#endif
extern void __stdcall afs_analyze_set_threshold_mmx(
  int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion
);

extern void __stdcall afs_analyze_12_mmx(
  PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int step, int h
);

extern void __stdcall afs_analyze_shrink_info_mmx(
  unsigned char *dst, PIXEL_YC *src, int h, int si_pitch
);

extern void __stdcall afs_analyze_1_mmx(
  PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int step, int h
);

extern void __stdcall afs_analyze_2_mmx(
  PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int step, int h
);

static void __stdcall afs_analyze_12_mmx_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	afs_analyze_12_mmx(dst, p0, p1, tb_order, step, h);
}
static void __stdcall afs_analyze_1_mmx_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	afs_analyze_1_mmx(dst, p0, p1, tb_order, step, h);
}
static void __stdcall afs_analyze_2_mmx_plus(PIXEL_YC *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int h) {
	afs_analyze_2_mmx(dst, p0, p1, tb_order, step, h);
}
static void __stdcall afs_analyze_shrink_info_mmx_plus(unsigned char *dst, PIXEL_YC *src, int h, int width, int si_pitch) {
	afs_analyze_shrink_info_mmx(dst, src, h, si_pitch);
}
static void __stdcall afs_analyze_12_mmx_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_12_mmx((PIXEL_YC *)dst, p0, p1, tb_order, step, h);
}
static void __stdcall afs_analyze_1_mmx_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_1_mmx((PIXEL_YC *)dst, p0, p1, tb_order, step, h);
}
static void __stdcall afs_analyze_2_mmx_plus2(unsigned char *dst, PIXEL_YC *p0, PIXEL_YC *p1, int tb_order, int width, int step, int si_pitch, int h) {
	afs_analyze_2_mmx((PIXEL_YC *)dst, p0, p1, tb_order, step, h);
}
#ifdef __cplusplus
}
#endif

