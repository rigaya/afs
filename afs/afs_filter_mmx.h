#ifdef __cplusplus
extern "C" {
#endif
extern void __stdcall afs_analyzemap_filter_mmx(
  unsigned char* sip, int si_w, int w, int h
);

extern void __stdcall afs_merge_scan_mmx(
  unsigned char* dst, unsigned char* src0, unsigned char* src1, int w, int si_w, int h, int x_start, int x_fin, int tb_order, int *stripe_count, AFS_SCAN_CLIP *mc_clip
);
#ifdef __cplusplus
}
#endif

