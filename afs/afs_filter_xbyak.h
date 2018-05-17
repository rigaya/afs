#ifndef __AFS_FILTER_XBYAK_H__
#define __AFS_FILTER_XBYAK_H__

#include "xbyak/xbyak.h"
#include "afs.h"

// void __stdcall afs_merge_scan_avx2_plus(BYTE* dst, BYTE* src0, BYTE* src1, int w, int si_w, int h, int x_start, int x_fin, int tb_order, int *stripe_count, AFS_SCAN_CLIP *mc_clip) {
//[esp + 04] BYTE* dst
//[esp + 08] BYTE* src0
//[esp + 12] BYTE* src1
//[esp + 16] int w
//[esp + 20] int si_w
//[esp + 24] int h
//[esp + 28] int x_start
//[esp + 32] int x_fin
//[esp + 36] int tb_order
//[esp + 40] int *stripe_count
//[esp + 44] AFS_SCAN_CLIP *mc_clip

struct AFSMergeScaneParam {
    int si_w;
    int h;
    int mc_scan_top;
    int mc_scan_bottom;
};

class AFSMergeScanXbyak : public Xbyak::CodeGenerator {
private:
    static const uint8_t TL_R0 = 0xf0;
    static const uint8_t TL_R1 = 0xcc;
    static const uint8_t TL_R2 = 0xaa;

    enum LOOP_PHASE {
        LOOP_PRE,
        LOOP_MAIN,
        LOOP_POST
    };
    AFSMergeScaneParam param;
    bool avx512;
    static const int STACK_ALIGN = 64;
    static const int BLOCK_SIZE = 1920;
    void set_mc_y_mask(int stack_ptr_mc_mask_y_offset);
    void load_vec_const();
    void init_mc_mask(const int stack_ptr_mc_mask_x_offset, const Xbyak::Address& stack_ptr_w, const Xbyak::Address& stack_ptr_mc_clip);
    void init_mc_mask_avx2(const int stack_ptr_mc_mask_x_offset, const Xbyak::Address& stack_ptr_w, const Xbyak::Address& stack_ptr_mc_clip);
    void init_mc_mask_avx512(const int stack_ptr_mc_mask_x_offset, const Xbyak::Address& stack_ptr_w, const Xbyak::Address& stack_ptr_mc_clip);

    void loop(LOOP_PHASE loop_phase, const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset);
    void loop_avx2(LOOP_PHASE loop_phase, const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset);
    void loop_avx512(LOOP_PHASE loop_phase, const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset);

    void count_stripe_avx2(const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset);
    void count_stripe_avx512(const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset);

    void operator=(const AFSMergeScanXbyak&) {};
    AFSMergeScanXbyak(const AFSMergeScanXbyak& src) {};
public:
    AFSMergeScanXbyak(int si_w, int h, const AFS_SCAN_CLIP *mc_clip, bool avx512, size_t size = Xbyak::DEFAULT_MAX_CODE_SIZE, void *userPtr = nullptr);
    virtual ~AFSMergeScanXbyak() {};

    int checkprm(int si_w, int h, const AFS_SCAN_CLIP *mc_clip) const {
        return param.si_w != si_w
            || param.h != h
            || param.mc_scan_top != mc_clip->top
            || param.mc_scan_bottom != mc_clip->bottom;
    }
};

#endif //__AFS_FILTER_XBYAK_H__
