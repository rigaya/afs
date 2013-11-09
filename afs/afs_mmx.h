#ifdef __cplusplus
extern "C" {
#endif
extern void __stdcall afs_blend_mmx(
  PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2, PIXEL_YC *src3,
  unsigned char *sip, unsigned int mask, int w
);

extern void __stdcall afs_mie_spot_mmx(
  PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2,
  PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src_spot,int w
);

extern void __stdcall afs_mie_inter_mmx(
  PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src2,
  PIXEL_YC *src3, PIXEL_YC *src4, int w
);

extern void __stdcall afs_deint4_mmx(
  PIXEL_YC *dst, PIXEL_YC *src1, PIXEL_YC *src3, PIXEL_YC *src4, PIXEL_YC *src5, PIXEL_YC *src7,
  unsigned char *sip, unsigned int mask, int w
);

#ifdef __cplusplus
}
#endif

