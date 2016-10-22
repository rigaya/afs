#ifdef __cplusplus
extern "C" {
#endif
extern void __stdcall afs_blend_mmx(
  void *dst, void *src1, void *src2, void *src3,
  unsigned char *sip, unsigned int mask, int w, int src_frame_size
);

extern void __stdcall afs_mie_spot_mmx(
    void *dst, void *src1, void *src2,
    void *src3, void *src4, void *src_spot,int w, int src_frame_size
);

extern void __stdcall afs_mie_inter_mmx(
    void *dst, void *src1, void *src2,
    void *src3, void *src4, int w, int src_frame_size
);

extern void __stdcall afs_deint4_mmx(
    void *dst, void *src1, void *src3, void *src4, void *src5, void *src7,
  unsigned char *sip, unsigned int mask, int w, int src_frame_size
);

#ifdef __cplusplus
}
#endif

