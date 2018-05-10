#ifndef __AFS_ANALYZE_XBYAK_H__
#define __AFS_ANALYZE_XBYAK_H__

#include "xbyak/xbyak.h"
#include "afs.h"

static const uint8_t TL_R0 = 0xf0;
static const uint8_t TL_R1 = 0xcc;
static const uint8_t TL_R2 = 0xaa;

struct AFSAnalyzeParam {
    int tb_order;
    int step;
    int si_pitch;
    int h;
    int max_h;
    int mc_scan_top;
    int mc_scan_bottom;
};

class AFSAnalyzeXbyak : public Xbyak::CodeGenerator {
protected:
    AFSAnalyzeParam param;
private:
    void operator=(const AFSAnalyzeXbyak&) {};
    AFSAnalyzeXbyak(const AFSAnalyzeXbyak& src) {};
public:
    AFSAnalyzeXbyak(size_t size = Xbyak::DEFAULT_MAX_CODE_SIZE, void *userPtr = nullptr)
        : Xbyak::CodeGenerator(size, userPtr) { };

    virtual ~AFSAnalyzeXbyak() { };
    virtual int checkprm(int tb_order, int step, int si_pitch, int h, int max_h, int mc_scan_top, int mc_scan_bottom) const;
};

class AFSAnalyzeXbyakAVX2 : public AFSAnalyzeXbyak {
private:
    void operator=(const AFSAnalyzeXbyakAVX2&) {};

    static const int BLOCK_SIZE_YCP_LOG2 = 8;
    static const int BLOCK_SIZE_YCP = 1 << BLOCK_SIZE_YCP_LOG2;
    static const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 2;
    bool avx2_slow;
public:
    virtual ~AFSAnalyzeXbyakAVX2() {};
    AFSAnalyzeXbyakAVX2(
        bool amd_avx2_slow,
        int tb_order, int step, int si_pitch, int h, int max_h, int mc_scan_top, int mc_scan_bottom,
        size_t size = Xbyak::DEFAULT_MAX_CODE_SIZE, void *userPtr = nullptr);
private:
    void copy_pw_thre_motion_to_stack(const Xbyak::Address& stack_ptr_pw_thre_motion);
    void init_mc_mask(const int stack_ptr_mc_mask_offset);
    void afs_analyze_loop1(
        const Xbyak::Address& stack_ptr_p0, const Xbyak::Address& stack_ptr_p1,
        const Xbyak::Address& stack_ptr_pw_thre_motion,
        int step6, int stack_ptr_buffer2_offset);
    void afs_analyze_loop_1_internal(
        const Xbyak::Ymm& ymm_out,
        const Xbyak::Ymm& ymm2_pw_thre_motion,
        const Xbyak::Ymm& ymm7_pw_mask_12motion_0,
        const Xbyak::Ymm& ymm6_pw_thre_shift,
        int step6, bool third_call, int offset);
    void afs_shrink_info(
        bool loop1,
        const Xbyak::Reg32& ecx,
        const Xbyak::Ymm& ymm5, const Xbyak::Ymm& ymm4, const Xbyak::Ymm& ymm3);
    void afs_analyze_loop2(int step6, int si_pitch,
        const Xbyak::Address& stack_ptr_dst,
        const Xbyak::Address& stack_ptr_p0,
        const Xbyak::Address& stack_ptr_p1,
        const Xbyak::Address& stack_ptr_width,
        const Xbyak::Address& stack_ptr_h_fin_l2,
        const Xbyak::Address& stack_ptr_pw_thre_motion,
        const Xbyak::Address& stack_ptr_h_start,
        const int stack_ptr_tmp16pix_offset,
        const int stack_ptr_buffer_offset,
        const int stack_ptr_buffer2_offset,
        const int stack_ptr_mc_mask_offset);
    void afs_analyze_loop2_1_internal(int step6, int si_pitch, int h);
    void afs_analyze_loop2_2_internal(int stack_ptr_mc_mask_offset);
    void afs_analyze_count_motion(int stack_ptr_mc_mask_offset);
    void afs_analyze_loop3(int step6, int si_pitch,
        const Xbyak::Address& stack_ptr_dst,
        const Xbyak::Address& stack_ptr_width,
        const Xbyak::Address& stack_ptr_h_fin_l3,
        const int stack_ptr_tmp16pix_offset,
        const int stack_ptr_buffer_offset,
        const int stack_ptr_buffer2_offset,
        const int stack_ptr_mc_mask_offset);
    void afs_analyze_loop3_internal(int stack_ptr_mc_mask_offset);
};


class AFSAnalyzeXbyakAVX512 : public AFSAnalyzeXbyak {
private:
    void operator=(const AFSAnalyzeXbyakAVX512&) {};

    static const int BLOCK_SIZE_YCP_LOG2 = 8;
    static const int BLOCK_SIZE_YCP = 1 << BLOCK_SIZE_YCP_LOG2;
    static const int BUFFER_SIZE = BLOCK_SIZE_YCP * 6 * 2;
public:
    virtual ~AFSAnalyzeXbyakAVX512() {};
    AFSAnalyzeXbyakAVX512(
        int tb_order, int step, int si_pitch, int h, int max_h, int mc_scan_top, int mc_scan_bottom,
        size_t size = Xbyak::DEFAULT_MAX_CODE_SIZE, void *userPtr = nullptr);
private:
    void copy_pw_thre_motion_to_stack(const Xbyak::Address& stack_ptr_pw_thre_motion);
    void init_mc_mask(const int stack_ptr_mc_mask_offset);
    void afs_analyze_loop1(
        const Xbyak::Address& stack_ptr_p0, const Xbyak::Address& stack_ptr_p1,
        const Xbyak::Address& stack_ptr_pw_thre_motion,
        int step6, int stack_ptr_buffer2_offset);
    void afs_analyze_loop_1_internal(
        const Xbyak::Zmm& zmm_out,
        const Xbyak::Zmm& zmm2_pw_thre_motion,
        const Xbyak::Zmm& zmm7_pw_mask_12motion_0,
        const Xbyak::Zmm& zmm6_pw_thre_shift,
        int step6, bool third_call, int offset);
    void afs_shrink_info(
        bool loop1,
        const Xbyak::Reg32& ecx,
        const Xbyak::Zmm& zmm5, const Xbyak::Zmm& zmm4, const Xbyak::Zmm& zmm3);
    void afs_analyze_loop2(int step6, int si_pitch,
        const Xbyak::Address& stack_ptr_dst,
        const Xbyak::Address& stack_ptr_p0,
        const Xbyak::Address& stack_ptr_p1,
        const Xbyak::Address& stack_ptr_width,
        const Xbyak::Address& stack_ptr_h_fin_l2,
        const Xbyak::Address& stack_ptr_pw_thre_motion,
        const Xbyak::Address& stack_ptr_h_start,
        const int stack_ptr_tmp16pix_offset,
        const int stack_ptr_buffer_offset,
        const int stack_ptr_buffer2_offset,
        const int stack_ptr_mc_mask_offset);
    void afs_analyze_loop2_1_internal(int step6, int si_pitch, int h);
    void afs_analyze_loop2_2_internal(int stack_ptr_mc_mask_offset);
    void afs_analyze_count_motion(int stack_ptr_mc_mask_offset);
    void afs_analyze_loop3(int step6, int si_pitch,
        const Xbyak::Address& stack_ptr_dst,
        const Xbyak::Address& stack_ptr_width,
        const Xbyak::Address& stack_ptr_h_fin_l3,
        const int stack_ptr_tmp16pix_offset,
        const int stack_ptr_buffer_offset,
        const int stack_ptr_buffer2_offset,
        const int stack_ptr_mc_mask_offset);
    void afs_analyze_loop3_internal(int stack_ptr_mc_mask_offset);
};

#endif //#if __AFS_ANALYZE_XBYAK_H__
