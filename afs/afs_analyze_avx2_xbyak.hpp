#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#include "afs_analyze_xbyak.h"

//afs_analyze
//32bit
//free ecx, edx
//restore ebx, esi, edi, ebp
//return eax, edx
//BYTE *dst, void *_p0, void *_p1, int tb_order, int width, int step, int si_pitch, int h_start, int h_fin, int height, int h_max, int *motion_count, AFS_SCAN_CLIP *mc_clip
// [esp + 04] BYTE *dst
// [esp + 08] void *_p0
// [esp + 12] void *_p1
// [esp + 16] void tb_order
// [esp + 20] int width
// [esp + 24] int step
// [esp + 28] int si_pitch
// [esp + 32] int h_start
// [esp + 36] int h_fin
// [esp + 40] int height
// [esp + 44] int h_max
// [esp + 48] int *motion_count
// [esp + 52] AFS_SCAN_CLIP *mc_clip

int AFSAnalyzeXbyak::checkprm(int tb_order, int step, int si_pitch, int h, int max_h, int mc_scan_top, int mc_scan_bottom) const {
    AFSAnalyzeParam prmcmp;
    memcpy(&prmcmp, &param, sizeof(param));
    prmcmp.h = h;
    prmcmp.si_pitch = si_pitch;
    prmcmp.max_h = max_h;
    prmcmp.tb_order = tb_order;
    prmcmp.si_pitch = si_pitch;
    prmcmp.step = step;
    prmcmp.mc_scan_top = mc_scan_top;
    prmcmp.mc_scan_bottom = mc_scan_bottom;
    return memcmp(&prmcmp, &param, sizeof(param));
}

AFSAnalyzeXbyakAVX2::AFSAnalyzeXbyakAVX2(
    bool amd_avx2_slow,
    int tb_order, int step, int si_pitch, int h, int max_h, int mc_scan_top, int mc_scan_bottom,
    size_t size, void *userPtr) : AFSAnalyzeXbyak(size, userPtr) {
    using namespace Xbyak;
    const int step6 = step * 6;
    const int STACK_ALIGN = 32;
    avx2_slow = amd_avx2_slow;
    param.h = h;
    param.si_pitch = si_pitch;
    param.max_h = max_h;
    param.tb_order = tb_order;
    param.si_pitch = si_pitch;
    param.step = step;
    param.mc_scan_top = mc_scan_top;
    param.mc_scan_bottom = mc_scan_bottom;
    push(ebp);
    mov(ebp, esp);
    and(esp, -STACK_ALIGN);
    mov(eax, ebp);
    sub(eax, esp); //and(esp, -32)の差分

    //待避
    push(edi); push(esi); push(ebx);

    static const int STACK_PUSH              = 12;
    const int stack_ptr_dst_offset           =  0;
    const int stack_ptr_p0_offset            =  4;
    const int stack_ptr_p1_offset            =  8;
    const int stack_ptr_width_offset         = 12;
    const int stack_ptr_h_start_plus4_offset = 16;
    const int stack_ptr_h_fin_offset         = 20;
    const int stack_ptr_h_fin_l2_offset      = 24;
    const int stack_ptr_h_fin_l3_offset      = 28;
    const int stack_ptr_tmp16pix_offset      = 32; //size = 64
    const int stack_ptr_buffer_offset        = 96; //size = BUFFER_SIZE
    const int stack_ptr_buffer2_offset       = stack_ptr_buffer_offset  + BUFFER_SIZE;        //size = BUFFER_SIZE
    const int stack_ptr_mc_mask_offset       = stack_ptr_buffer2_offset + BLOCK_SIZE_YCP * 8; //size = BLOCK_SIZE_YCP
    const int stack_ptr_pw_thre_motion_offset = stack_ptr_mc_mask_offset + BLOCK_SIZE_YCP;  //size = BLOCK_SIZE_YCP
    const int stack_fin  = stack_ptr_pw_thre_motion_offset + 32 + 4 + 4;
    const int STACK_SIZE = ((stack_fin + (STACK_ALIGN-1) + STACK_PUSH) & ~(STACK_ALIGN-1)) - STACK_PUSH;
    static_assert(STACK_SIZE >= stack_fin, "STACK_SIZE is too small.");
    static_assert((STACK_SIZE + STACK_PUSH) % 32 == 0, "STACK_SIZE must be mod32.");
    {
        //スタック確保
        sub(esp, STACK_SIZE);
        int stack_alloc_remain = STACK_SIZE;
        do {
            stack_alloc_remain -= 4096;
            mov(dword[esp + (std::max)(0, stack_alloc_remain)], ebx); //dummy
        } while (stack_alloc_remain > 0);
    }
    const Address& stack_orig_esp_offset    = dword[esp + STACK_SIZE - 4];                 //size = 4
    const Address& stack_ptr_motion_count   = dword[esp + STACK_SIZE - 8];                 //size = 4
    const Address& stack_ptr_dst            = dword[esp + stack_ptr_dst_offset];           //size = 4
    const Address& stack_ptr_p0             = dword[esp + stack_ptr_p0_offset];            //size = 4
    const Address& stack_ptr_p1             = dword[esp + stack_ptr_p1_offset];            //size = 4
    const Address& stack_ptr_width          = dword[esp + stack_ptr_width_offset];         //size = 4
    const Address& stack_ptr_h_start_plus4  = dword[esp + stack_ptr_h_start_plus4_offset];  //size = 4
    const Address& stack_ptr_h_fin          = dword[esp + stack_ptr_h_fin_offset];         //size = 4
    const Address& stack_ptr_h_fin_l2       = dword[esp + stack_ptr_h_fin_l2_offset];      //size = 4
    const Address& stack_ptr_h_fin_l3       = dword[esp + stack_ptr_h_fin_l3_offset];      //size = 4
    const Address& stack_ptr_tmp16pix       = yword[esp + stack_ptr_tmp16pix_offset];      //size = 64
    const Address& stack_ptr_buffer         = yword[esp + stack_ptr_buffer_offset];        //size = BUFFER_SIZE
    const Address& stack_ptr_buffer2        = yword[esp + stack_ptr_buffer2_offset];       //size = BLOCK_SIZE_YCP * 8
    const Address& stack_ptr_mc_mask        = byte[esp + stack_ptr_mc_mask_offset];        //size = BLOCK_SIZE_YCP
    const Address& stack_ptr_pw_thre_motion = yword[esp + stack_ptr_pw_thre_motion_offset];  //size = 32

    mov(stack_orig_esp_offset, eax); //and(esp, -32)の差分を保存

    //bufferを0初期化
    cld();
    mov(ecx, BUFFER_SIZE);
    xor(eax, eax);
    lea(edi, stack_ptr_buffer);
    rep(); db(0xAA); //rep stosb

    //関数引数の取り出し
    mov(ecx, dword[ebp + 4 +  4]); //関数引数: BYTE *dst
    mov(stack_ptr_dst, ecx);
    mov(ebx, dword[ebp + 4 + 36]); //関数引数: int h_fin
    mov(stack_ptr_h_fin, ebx);

    //h_fin_loop2, h_fin_loop3の算出
    //const int h_fin_loop2 = std::min(h_fin +4, h);
    //const int h_fin_loop3 = std::min(h_fin +4, h + 4);
    mov(ecx, h); //h
    add(ebx, 4); //h_fin+4
    lea(eax, ptr[ecx +4]); //h+4
    cmp(ebx, ecx);
    cmovle(ecx, ebx); //min(h_fin+4, h)
    mov(stack_ptr_h_fin_l2, ecx);
    cmp(ebx, eax);
    cmovle(eax, ebx); //min(h_fin+4, h+4)
    mov(stack_ptr_h_fin_l3, eax);

    //関数引数の取り出し
    mov(esi, dword[ebp + 4 + 8]); //関数引数: void *_p0
    mov(edi, dword[ebp + 4 + 12]); //関数引数: void *_p1
    mov(ebx, dword[ebp + 4 + 20]); //関数引数: int width
    mov(stack_ptr_width, ebx);
    mov(edx, dword[ebp + 4 + 48]); //関数引数: int *motion_count
    mov(stack_ptr_motion_count, edx);
    mov(ecx, edi);
    sub(ecx, esi); //p1 - p0
    movd(mm7, ecx); // 64| 0 | p1 - p0 |0 ... 下位がp1 - p0

    mov(ecx, dword[ebp + 4 + 52]); //関数引数: AFS_SCAN_CLIP *mc_clip
    mov(edx, dword[ebp + 4 + 32]); //関数引数: int h_start
    lea(eax, ptr[edx + 4]);
    mov(stack_ptr_h_start_plus4, eax);

    //mc_clipの初期化
    init_mc_mask(stack_ptr_mc_mask_offset);

    //pw_thre_motionのコピー
    copy_pw_thre_motion_to_stack(stack_ptr_pw_thre_motion);

    test(edx, edx);
    jnz("afs_analyze_loop1_fin_else", T_NEAR); { //if (ih != 0)
        mov(stack_ptr_p0, esi); //保存
        mov(stack_ptr_p1, edi); //保存
        afs_analyze_loop1(stack_ptr_p0, stack_ptr_p1, stack_ptr_pw_thre_motion, step6, stack_ptr_buffer2_offset);
        inc(edx);
        jmp("afs_analyze_loop1_fin");
    } L("afs_analyze_loop1_fin_else"); {
        imul(ecx, edx, si_pitch);
        add(stack_ptr_dst, ecx); // dst += si_pitch * h_start;

        //少し、解析領域をオーバーラップさせる
        //こうすることで、縦方向分割の縞検出が安定する
        mov(eax, edx);
        sub(eax, 2);   //h_start-2
        cmovg(edx, eax); //if (h_start>2) h_start = h_start-2

        mov(eax, edx);
        dec(eax);
        imul(eax, eax, step6);
        add(esi, eax);          //p0 += step6 * (h_start - 1);
        add(edi, eax);          //p1 += step6 * (h_start - 1);
        mov(stack_ptr_p0, esi); //保存
        mov(stack_ptr_p1, edi); //保存
    }
    L("afs_analyze_loop1_fin");

    //必要に応じshift
    //((ih & 1) ^ tb_order) == 0なら、 そのまま(下位がp1 - p0)
    //((ih & 1) ^ tb_order) == 1なら、 shift (上位にp1 - p0)
    lea(eax, ptr[edx + tb_order + 1]);
    and(eax, 1);
    //tb_order=0, (ih & 1)=0 -> 0
    //tb_order=0, (ih & 1)=1 -> 32
    //tb_order=1, (ih & 1)=1 -> 32
    //tb_order=1, (ih & 1)=0 -> 0
    shl(eax, 5);
    movd(mm0, eax);
    psllq(mm7, mm0);

    //カウンタをクリア
    pxor(mm0, mm0);

    afs_analyze_loop2(step6, si_pitch,
        stack_ptr_dst,
        stack_ptr_p0,
        stack_ptr_p1,
        stack_ptr_width,
        stack_ptr_h_fin_l2,
        stack_ptr_pw_thre_motion,
        stack_ptr_h_start_plus4,
        stack_ptr_tmp16pix_offset,
        stack_ptr_buffer_offset,
        stack_ptr_buffer2_offset,
        stack_ptr_mc_mask_offset);

    afs_analyze_loop3(step6, si_pitch,
        stack_ptr_dst,
        stack_ptr_width,
        stack_ptr_h_fin_l3,
        stack_ptr_tmp16pix_offset,
        stack_ptr_buffer_offset,
        stack_ptr_buffer2_offset,
        stack_ptr_mc_mask_offset);

    //カウンタの処理
    //hなどにより、前後を調整する
    //ここでは edx = ih = h_fin_l3
    //if (((tb_order + h_fin_loop3) & 1) == 0)なら反転
    and(edx, 1); //(h_fin_loop3) & 1
    mov(eax, (size_t)pb_mshufmask);
    //qword[eax + 0] -> 反転
    //qwordfeax + 4] -> そのまま
    //qword[eax + 8] -> 反転
    //tb_order =0, (ih & 1) = 0 -> +0
    //tb_order =0, (ih & 1) = 1 -> +4
    //tb_order =1, (ih & 1) = 0 -> +4
    //tb_order =1, (ih & 1) = 1 -> +8
    pshufb(mm0, qword[eax + edx * 4 + ((tb_order) ? 4 : 0)]);
    mov(eax, stack_ptr_motion_count);
    paddd(mm0, qword[eax]);
    movq(qword[eax], mm0);

    //終了
    vzeroupper();
    emms();
    mov(eax, stack_orig_esp_offset);
    add(esp, STACK_SIZE);
    pop(ebx); pop(esi); pop(edi);
    add(esp, eax); //本来のespを復元
    pop(ebp);
    ret(52);
    
    afs_analyze_loop2_1_internal(step6, si_pitch, h);
}

#define AVX2I_MEM_BROADCAST(inst, ymm0, ymm1, addr, ymmtmp) if (avx2_slow) { vbroadcasti128(ymmtmp, xword[addr]); inst(ymm0, ymm1, ymmtmp); } else { inst(ymm0, ymm1, yword[addr]); }
#define AVX2L_MEM_BROADCAST(ymm0, addr) if (avx2_slow) { vbroadcasti128(ymm0, xword[addr]); } else { vmovdqa(ymm0, yword[addr]); }

void AFSAnalyzeXbyakAVX2::copy_pw_thre_motion_to_stack(const Xbyak::Address& stack_ptr_pw_thre_motion) {
    vmovdqa(ymm0, yword[pw_thre_motion]);
    vmovdqa(stack_ptr_pw_thre_motion, ymm0);
}

//ebx ... width
//edi ... dst
//esi ... 変更せず
//edx ... ih
//
//ecx ... ptr_mc_clip -> tmp
//eax ... tmp
//ymm0-ymm7 ... 使用
void AFSAnalyzeXbyakAVX2::init_mc_mask(const int stack_ptr_mc_mask_offset) {
    using namespace Xbyak;

    vmovdqa(ymm2, yword[pw_index]); //[i+ 0, i+ 8]
    vpcmpeqb(ymm7, ymm7, ymm7);
    vpsrlw(ymm7, ymm7, 15);
    vpsllw(ymm7, ymm7, 4); //16
    vpaddw(ymm1, ymm2, ymm7); //[i+16, i+24]
    vpsllw(ymm7, ymm7, 1); //32
    vperm2i128(ymm0, ymm2, ymm1, (2<<4) + 0); //あとでvpacksswbするので [i+ 0, i+16]
    vperm2i128(ymm1, ymm2, ymm1, (3<<4) + 1); //あとでvpacksswbするので [i+ 8, i+24]

    xor(eax, eax);
    dec(eax);
    add(eax, dword[ecx + offsetof(AFS_SCAN_CLIP, left)]); //mc_clip->left-1
    movd(xmm2, eax);
    vpbroadcastw(ymm2, xmm2); //mc_clip->left - 1
    mov(ecx, dword[ecx + offsetof(AFS_SCAN_CLIP, right)]); //mc_clip->right
    mov(eax, ebx); //width
    sub(eax, ecx); //width - mc_clip->right
    movd(xmm3, eax);
    vpbroadcastw(ymm3, xmm3); //width - mc_clip->right

    lea(eax, ptr[esp + stack_ptr_mc_mask_offset]);
    mov(ecx, BLOCK_SIZE_YCP / 32);
    static_assert(BLOCK_SIZE_YCP % 32 == 0, "BLOCK_SIZE_YCP should be mod32.");
    L("init_mc_mask_loop1"); {
        vpcmpgtw(ymm4, ymm0, ymm2); // ([i+ 0] > (mc_clip->left - 1)) = ([i+ 0] >= mc_clip->left)
        vpcmpgtw(ymm5, ymm3, ymm0); // mc_clip->right > [i+ 0]
        vpand(ymm4, ymm4, ymm5);    // mc_clip->left <= [i+ 0] < mc_clip->right

        vpcmpgtw(ymm5, ymm1, ymm2); // ([i+16] > (mc_clip->left - 1)) = ([i+16] >= mc_clip->left)
        vpcmpgtw(ymm6, ymm3, ymm1); // mc_clip->right > [i+16]
        vpand(ymm5, ymm5, ymm6);    // mc_clip->left <= [i+16] < mc_clip->right

        vpacksswb(ymm4, ymm4, ymm5);
        vmovdqa(yword[eax], ymm4);

        vpaddw(ymm0, ymm0, ymm7); //[i+ 0] += 32 
        vpaddw(ymm1, ymm1, ymm7); //[i+16] += 32 
        add(eax, 32);
        dec(ecx);
        jnz("init_mc_mask_loop1");
    }
}

//ebx ... width
//ebp ... ih
//esi ... p0
//edi ... p1
//
//ymm6 ... pw_thre_shift
//ymm7 ... pw_mask_12motion_0
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
void AFSAnalyzeXbyakAVX2::afs_analyze_loop1(
    const Xbyak::Address& stack_ptr_p0, const Xbyak::Address& stack_ptr_p1,
    const Xbyak::Address& stack_ptr_pw_thre_motion,
    int step6, int stack_ptr_buffer2_offset) {
    lea(ecx, ptr[esp + stack_ptr_buffer2_offset]); //buf2ptr
    lea(ebp, ptr[ecx + ebx/* width */]); //buf2ptr_fin
    AVX2L_MEM_BROADCAST(ymm7, pw_mask_12motion_0);
    AVX2L_MEM_BROADCAST(ymm6, pw_thre_shift);
    L("afs_analyze_loop1"); {
        //ymm2のpe_thre_motionは、afs_shrink_infoの呼び出しで破棄されるのでここでロード
        vmovdqa(ymm2, stack_ptr_pw_thre_motion); //broadcast不可
        afs_analyze_loop_1_internal(ymm5, ymm2, ymm7, ymm6, step6, false,  0);
        afs_analyze_loop_1_internal(ymm4, ymm2, ymm7, ymm6, step6, false, 32);
        afs_analyze_loop_1_internal(ymm3, ymm2, ymm7, ymm6, step6, true,  64);
        afs_shrink_info(true, ecx, ymm5, ymm4, ymm3);
        add(ecx, 16);
        cmp(ecx, ebp);
        jb("afs_analyze_loop1");
    }
}

//ymm3 - ymm7
//esi ... p0
//edi ... p1
//eax ... width
//ecx ... outer loop counter
//ebp ... outer loop range
//edx ... ih
//
//ymm0 - ymm1
void AFSAnalyzeXbyakAVX2::afs_analyze_loop_1_internal(
    const Xbyak::Ymm& ymm_out, /*ymm3 - ymm5*/
    const Xbyak::Ymm& ymm2_pw_thre_motion,
    const Xbyak::Ymm& ymm7_pw_mask_12motion_0,
    const Xbyak::Ymm& ymm6_pw_thre_shift,
    int step6, bool third_call, int offset) {
    vmovdqu(ymm0, yword[esi + offset]);
    vpsubw(ymm0, ymm0, yword[edi + offset]);
    prefetcht0(ptr[esi + step6 + offset]);
    prefetcht0(ptr[edi + step6 + offset]);
    vpabsw(ymm0, ymm0);
    vpcmpgtw(ymm_out, ymm2_pw_thre_motion, ymm0); //0x0400
    vpcmpgtw(ymm0, ymm6_pw_thre_shift, ymm0);     //0x4000
    if (!third_call) {
        //最後の呼び出し(third_call = true)以外では、
        //次の呼び出しのため、ymm2_pw_thre_motionをシャッフル
        vpermq(ymm2_pw_thre_motion, ymm2_pw_thre_motion, _MM_SHUFFLE(1, 0, 2, 1));
    }
    vpsrlw(ymm_out, ymm_out, 4); //下位12bitに 
    vpsllw(ymm0, ymm0, 12);      //上位4bitに
    vpor(ymm_out, ymm_out, ymm0);
    vpand(ymm_out, ymm_out, ymm7_pw_mask_12motion_0);
}

// ----------------------------------
//loop1 = true
//ymm6 - ymm7 ... 使用済み
//ymm3 - ymm5 ... 引数
//esi ... p0
//edi ... p1
//eax ... width
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
//edx ... ih
//ymm0 - ymm2 ... 一時変数
// ----------------------------------
//loop1 = false
// ----------------------------------
void AFSAnalyzeXbyakAVX2::afs_shrink_info(
    bool loop1,
    const Xbyak::Reg32& ecx, /*buf2_out*/
    const Xbyak::Ymm& ymm5, const Xbyak::Ymm& ymm4, const Xbyak::Ymm& ymm3) {
    vpblendd(  ymm0, ymm5, ymm4, 0xf0);
    vperm2i128(ymm1, ymm5, ymm3, (0x02<<4)+0x01);
    vpblendd(  ymm2, ymm4, ymm3, 0xf0);

    vpblendw(ymm3, ymm0, ymm2, 0x20+0x04);
    vpblendw(ymm4, ymm0, ymm2, 0x40+0x08+0x01);
    vpblendw(ymm5, ymm0, ymm2, 0x80+0x10+0x02);

    vpblendw(ymm3, ymm3, ymm1, 0x80+0x10+0x02);
    vpblendw(ymm4, ymm4, ymm1, 0x20+0x04);
    vpblendw(ymm5, ymm5, ymm1, 0x40+0x08+0x01);

    vpcmpeqb(ymm2, ymm2, ymm2);
    vpsrlw(ymm2, ymm2, 8);
    vpalignr(ymm4, ymm4, ymm4, 2);
    vpalignr(ymm5, ymm5, ymm5, 4);
    vpor(ymm1, ymm3, ymm4);
    vpand(ymm0, ymm3, ymm4);
    vpor(ymm1, ymm1, ymm5);
    vpand(ymm0, ymm0, ymm5);
    vpand(ymm1, ymm1, ymm2);
    vpsraw(ymm0, ymm0, 8);
    vpacksswb(ymm0, ymm0, ymm1);
    AVX2I_MEM_BROADCAST(vpshufb, ymm0, ymm0, Array_SUFFLE_YCP_COMPRESSED, ymm1);
    AVX2I_MEM_BROADCAST(vpand, ymm0, ymm0, pb_mask_12motion_stripe_01, ymm2);
    vpsrldq(ymm1, ymm0, 8);
    vpor(ymm0, ymm0, ymm1);
    vpermq(ymm0, ymm0, _MM_SHUFFLE(3, 1, 2, 0));
    if (loop1) {
        vmovdqa(xword[ecx], xmm0);
    } else {
        mov(eax, edx);
        and(eax, 7);
        shl(eax, BLOCK_SIZE_YCP_LOG2);
        add(eax, ecx);
        vmovdqa(xword[eax], xmm0);

        lea(eax, ptr[edx+1]);
        and(eax, 7);
        shl(eax, BLOCK_SIZE_YCP_LOG2);
        add(eax, ecx);
        vextracti128(xword[eax], ymm0, 1);
    }
}

//afs_analyze_loop2_w1 ループ内
//eax  ... tmp
//esi  ... p0
//edi  ... p1
//ebx  ... bufptr
//ecx  ... buf2ptr
//ebp  ... buf2ptr_fin
//ymm5 ... pw_thre_motion
//ymm6 ... pw_thre_shift
//ymm7 ... pb_thre_count
//
//afs_analyze_loop2_w2 ループ内
//edx  ... ih
//eax  ... tmp
//ebx  ... dst
//ecx  ... buf2ptr
//ebp  ... buf2ptr_fin
//esi  ... tmp
//edi  ... mc_mask_ptr
//ymm4 ... pb_mask_12stripe_01
//ymm5 ... pb_mask_1stripe_01
//ymm6 ... pw_thre_shift
//ymm7 ... pb_thre_count
void AFSAnalyzeXbyakAVX2::afs_analyze_loop2(int step6, int si_pitch,
    const Xbyak::Address& stack_ptr_dst,
    const Xbyak::Address& stack_ptr_p0,
    const Xbyak::Address& stack_ptr_p1,
    const Xbyak::Address& stack_ptr_width,
    const Xbyak::Address& stack_ptr_h_fin_l2,
    const Xbyak::Address& stack_ptr_pw_thre_motion,
    const Xbyak::Address& stack_ptr_h_start_plus4,
    const int stack_ptr_tmp16pix_offset,
    const int stack_ptr_buffer_offset,
    const int stack_ptr_buffer2_offset,
    const int stack_ptr_mc_mask_offset) {
    AVX2L_MEM_BROADCAST(ymm7, pb_thre_count);
    AVX2L_MEM_BROADCAST(ymm6, pw_thre_shift);
    L("afs_analyze_loop2_h"); {
        lea(ecx, ptr[esp + stack_ptr_buffer2_offset]); //buf2ptr
        mov(ebp, ecx);
        add(ebp, stack_ptr_width); //buf2ptr_fin

        lea(ebx, ptr[esp + stack_ptr_buffer_offset]); //buf_ptr
        mov(esi, stack_ptr_p0);
        mov(edi, stack_ptr_p1);
        lea(eax, ptr[esi + step6]);
        mov(stack_ptr_p0, eax);
        lea(eax, ptr[edi + step6]);
        mov(stack_ptr_p1, eax);
        L("afs_analyze_loop2_w1"); {
            vmovdqa(ymm5, stack_ptr_pw_thre_motion); //broadcast不可
            call("afs_analyze_loop2_1_internal");
            vmovdqa(yword[esp + stack_ptr_tmp16pix_offset + 0], ymm3);

            add(esi, 32);
            add(edi, 32);
            add(ebx, 64);
            vpermq(ymm5, ymm5, _MM_SHUFFLE(1, 0, 2, 1));
            call("afs_analyze_loop2_1_internal");
            vmovdqa(yword[esp + stack_ptr_tmp16pix_offset + 32], ymm3);

            add(esi, 32);
            add(edi, 32);
            add(ebx, 64);
            vpermq(ymm5, ymm5, _MM_SHUFFLE(1, 0, 2, 1));
            call("afs_analyze_loop2_1_internal");

            add(esi, 32);
            add(edi, 32);
            add(ebx, 64);

            vmovdqa(ymm5, yword[esp + stack_ptr_tmp16pix_offset +  0]);
            vmovdqa(ymm4, yword[esp + stack_ptr_tmp16pix_offset + 32]);
            afs_shrink_info(false, ecx, ymm5, ymm4, ymm3);
            add(ecx, 16);
            cmp(ecx, ebp);
            jb("afs_analyze_loop2_w1");
        }
        cmp(edx, stack_ptr_h_start_plus4);
        jl("afs_analyze_loop2_h_fin", T_NEAR); {
            mov(ebx, stack_ptr_dst); //dst
            lea(eax, ptr[ebx + si_pitch]); //next dst
            mov(stack_ptr_dst, eax);

            lea(ecx, ptr[esp + stack_ptr_buffer2_offset]);
            mov(ebp, ecx);
            add(ebp, stack_ptr_width);
            lea(edi, ptr[esp + stack_ptr_mc_mask_offset]);
            AVX2L_MEM_BROADCAST(ymm4, pb_mask_12stripe_01);
            AVX2L_MEM_BROADCAST(ymm5, pb_mask_1stripe_01);
            L("afs_analyze_loop2_w2"); {
                afs_analyze_loop2_2_internal(stack_ptr_mc_mask_offset);
                add(ebx, 32);
                add(edi, 32);
                add(ecx, 32);
                cmp(ecx, ebp);
                jb("afs_analyze_loop2_w2");
            }
        }
        L("afs_analyze_loop2_h_fin");
        pshufw(mm0, mm0, _MM_SHUFFLE(1, 0, 3, 2));
        pshufw(mm7, mm7, _MM_SHUFFLE(1, 0, 3, 2));
        inc(edx);
        cmp(edx, stack_ptr_h_fin_l2);
        jb("afs_analyze_loop2_h", T_NEAR);
    }
}

//eax ... tmp
//ebx ... bufptr
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
//edx ... ih
//esi ... p0
//edi ... p1
//esp ... call内なのでoffsetが必要
//ymm5 ... pw_thre_motion
//ymm6 ... pw_thre_shift
//ymm7 ... pb_thre_count
//mm0  ... counter
//mm7  ... p1-p0
//
//ymm3 ... out
//m6   ... tmp
void AFSAnalyzeXbyakAVX2::afs_analyze_loop2_1_internal(int step6, int si_pitch, int h) {
    using namespace Xbyak;
    Ymm ymm7_pb_thre_count(ymm7);
    Ymm ymm6_pw_thre_shift(ymm6);
    Ymm ymm5_pw_thre_motion(ymm5);
    align();
    L("afs_analyze_loop2_1_internal");
    //analyze motion
    vmovdqa(ymm0, yword[esi + step6]);
    vmovdqa(ymm1, ymm0);
    vpsubw(ymm0, ymm0, yword[edi + step6]);
    vpabsw(ymm0, ymm0);
    vpcmpgtw(ymm3, ymm5_pw_thre_motion, ymm0); //0x0400
    vpcmpgtw(ymm2, ymm6_pw_thre_shift, ymm0);  //0x4000
    vpsrlw(ymm3, ymm3, 4);
    vpsllw(ymm2, ymm2, 12);
    vpor(ymm3, ymm3, ymm2);
    AVX2I_MEM_BROADCAST(vpand, ymm3, ymm3, pw_mask_12motion_0, ymm4);

    prefetcht0(ptr[esi + step6 * 2]);
    prefetcht0(ptr[edi + step6 * 2]);

    //analyze non-shift
    vmovdqa(ymm2, yword[esi]);
    vpsubw(ymm2, ymm2, ymm1);
    vpabsw(ymm0, ymm2);
    vpcmpeqw(ymm2, ymm2, ymm0);
    AVX2I_MEM_BROADCAST(vpcmpgtw, ymm1, ymm0, pw_thre_deint, ymm4);
    vpcmpgtw(ymm0, ymm0, ymm6_pw_thre_shift);
    vpsllw(ymm0, ymm0, 8);
    vpsrlw(ymm1, ymm1, 8);
    vpor(ymm1, ymm1, ymm0);

    vmovdqa(ymm0, yword[ebx]);
    vpabsw(ymm4, ymm0);       //絶対値がcountの値
    vpsraw(ymm0, ymm0, 15);   //符号付きとしてシフトし、0xffffか0x0000を作る
    vpxor(ymm0, ymm2, ymm0);
    vpand(ymm4, ymm4, ymm1);
    vpand(ymm0, ymm4, ymm0);
    vpsubsb(ymm0, ymm0, ymm1);
    vpandn(ymm4, ymm2, ymm0);  //フラグが負(0xffff)なら、カウントを0に
    vpsignw(ymm2, ymm0, ymm2); //フラグが負(0xffff)なら、カウントを負に
    vpor(ymm2, ymm2, ymm4);    //フラグが0だった場合の加算
    vmovdqa(yword[ebx], ymm2);

    vpcmpgtb(ymm0, ymm0, ymm7_pb_thre_count);
    vpsrlw(ymm0, ymm0, 4);
    AVX2I_MEM_BROADCAST(vpand, ymm0, ymm0, pw_mask_12stripe_0, ymm4);
    vpor(ymm3, ymm3, ymm0);

    //analyze shift
    movd(eax, mm7); //p1-p0 or 0
    vmovdqa(ymm2, yword[esi + eax]); //p0 あるいは p1
    pshufw(mm6, mm7, _MM_SHUFFLE(1, 0, 3, 2));
    movd(eax, mm6); //さきほどの逆
    vpsubw(ymm2, ymm2, yword[esi + eax + step6]);
    vpabsw(ymm0, ymm2);
    vpcmpeqw(ymm2, ymm2, ymm0);
    AVX2I_MEM_BROADCAST(vpcmpgtw, ymm1, ymm0, pw_thre_deint, ymm4);
    vpcmpgtw(ymm0, ymm0, ymm6_pw_thre_shift);
    vpsllw(ymm0, ymm0, 8);
    vpsrlw(ymm1, ymm1, 8);
    vpor(ymm1, ymm1, ymm0);

    vmovdqa(ymm0, yword[ebx+32]);
    vpabsw(ymm4, ymm0);      //絶対値がcountの値
    vpsraw(ymm0, ymm0, 15);  //符号付きとしてシフトし、0xffffか0x0000を作る
    vpxor(ymm0, ymm2, ymm0);
    vpand(ymm4, ymm4, ymm1);
    vpand(ymm0, ymm4, ymm0);
    vpsubsb(ymm0, ymm0, ymm1);
    vpandn(ymm4, ymm2, ymm0);  //フラグが負(0xffff)なら、カウントを0に
    vpsignw(ymm2, ymm0, ymm2); //フラグが負(0xffff)なら、カウントを負に
    vpor(ymm2, ymm2, ymm4);    //フラグが0だった場合の加算
    vmovdqa(yword[ebx+32], ymm2);

    vpcmpgtb(ymm0, ymm0, ymm7_pb_thre_count);
    vpsrlw(ymm0, ymm0, 4);
    AVX2I_MEM_BROADCAST(vpand, ymm0, ymm0, pw_mask_12stripe_1, ymm4);
    vpor(ymm3, ymm3, ymm0);
    ret();
}

//edx . . ih
//esi . . tmp
//edi . . mc_mask_ptr
//eax . . tmp
//ebx . . dst
//ecx . . buf2ptr
//ebp . . buf2ptr_fin
//ymm4 . . pb_mask_12stripe_01
//ymm5 . . pb_mask_1stripe_01
//ymm6 . . pw_thre_shift
//ymm7 . . pb_thre_count
void AFSAnalyzeXbyakAVX2::afs_analyze_loop2_2_internal(int stack_ptr_mc_mask_offset) {
    Xbyak::Ymm ymm4_pb_mask_12stripe_01(ymm4);
    Xbyak::Ymm ymm5_pb_mask_1stripe_01(ymm5);

    lea(eax, ptr[edx+5]); //edx-3
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vmovdqa(ymm0, yword[ecx + eax]);

    lea(esi, ptr[edx+6]); //edx-2
    and(esi, 7);
    shl(esi, BLOCK_SIZE_YCP_LOG2);
    vpor(ymm0, ymm0, yword[ecx + esi]);

    lea(eax, ptr[edx+1]); //edx+1
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vpand(ymm1, ymm5_pb_mask_1stripe_01, yword[ecx + eax]);

    lea(esi, ptr[edx+7]); //edx-1
    and(esi, 7);
    shl(esi, BLOCK_SIZE_YCP_LOG2);
    vpor(ymm0, ymm0, yword[ecx + esi]);
    vpand(ymm0, ymm0, ymm4_pb_mask_12stripe_01);

    lea(eax, ptr[edx+4]); //edx-4
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vpor(ymm1, ymm1, yword[ecx + eax]);
    vpor(ymm0, ymm0, ymm1);
    vmovdqu(yword[ebx], ymm0);

    afs_analyze_count_motion(stack_ptr_mc_mask_offset);
}

//edx ... ih
//eax ... tmp
//esi ... tmp
//edi ... mc_mask_ptr
//ebx ... dst
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
//ymm0 ... 対象
//ymm4 ... pb_mask_12stripe_01
//ymm5 ... pb_mask_1stripe_01
//ymm6 ... pw_thre_shift
//ymm7 ... pb_thre_count
void AFSAnalyzeXbyakAVX2::afs_analyze_count_motion(int stack_ptr_mc_mask_offset) {
    const int top = param.mc_scan_top;
    const int mc_scan_y_limit = (param.h - param.mc_scan_bottom - top) & ~1;
    //vpmovmskbは最上位ビットの取り出しであるから、最上位ビットさえ意識すればよい
    //ここで行いたいのは、「0x40ビットが立っていないこと」であるから、
    //左シフトで最上位ビットに移動させたのち、andnotで反転させながらmc_maskとの論理積をとればよい
    vpsllw(ymm0, ymm0, 1);
    vpandn(ymm0, ymm0, yword[edi]);

    vpmovmskb(esi, ymm0);
    and(esi, eax);
    popcnt(esi, esi);

    mov(eax, edx);
    sub(eax, top+4); //ih - 4 - top
    cmp(eax, mc_scan_y_limit);
    mov(eax, 0); //フラグ変化なし
    cmovb(eax, esi); //((DW0RD)(y -top) < (DW0RD)y_limit) ? count(esi) : 0x00;

    movd(mm1, eax);
    paddd(mm0, mm1);
}

//edx ... ih
//ymm4 ... pb_mask_12stripe_01
//ymm5 ... pb_mask_1stripe_01 -> zero
//ymm6 ... pw_thre_shift
//ymm7 ... pb_thre_count
//
//esi ... tmp
//edi ... mc_nask_ptr
//eax ... tmp
//ebx ... dst
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
void AFSAnalyzeXbyakAVX2::afs_analyze_loop3(int step6, int si_pitch,
    const Xbyak::Address& stack_ptr_dst,
    const Xbyak::Address& stack_ptr_width,
    const Xbyak::Address& stack_ptr_h_fin_l3,
    const int stack_ptr_tmp16pix_offset,
    const int stack_ptr_buffer_offset,
    const int stack_ptr_buffer2_offset,
    const int stack_ptr_mc_mask_offset) {
    vpxor(ymm5, ymm5, ymm5);
    AVX2L_MEM_BROADCAST(ymm4, pb_mask_12stripe_01);

    L("afs_analyze_loop3_h"); {
        cmp(edx, stack_ptr_h_fin_l3);
        jge("afs_analyze_loop3_h_fin", T_NEAR);
        //dstの取り出しと更新
        mov(ebx, stack_ptr_dst);
        lea(eax, ptr[ebx + si_pitch]);
        mov(stack_ptr_dst, eax);

        //buf2_ptr
        lea(ecx, ptr[esp + stack_ptr_buffer2_offset]);
        mov(ebp, ecx);
        add(ebp, stack_ptr_width);
        lea(edi, ptr[esp + stack_ptr_mc_mask_offset]);

        L("afs_analyze_loop3_w"); {
            afs_analyze_loop3_internal(stack_ptr_mc_mask_offset);
            add(ebx, 32);
            add(edi, 32);
            add(ecx, 32);
            cmp(ecx, ebp);
            jb("afs_analyze_loop3_w");
        }
        pshufw(mm0, mm0, _MM_SHUFFLE(1, 0, 3, 2));
        inc(edx);
        jmp("afs_analyze_loop3_h", T_NEAR);
    }
    L("afs_analyze_loop3_h_fin");
}

//edx ... ih
//esi ... tmp
//edi ... mc_mask_ptr
//eax ... tmp
//ebx ... dst
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
//ymm4 ... pb_mask_12stripe_01
//ymm5 ... zero
//ymm6 ... pw_thre_shi ft
//ymm7 ... pb_thre_count
void AFSAnalyzeXbyakAVX2::afs_analyze_loop3_internal(int stack_ptr_mc_mask_offset) {
    Xbyak::Ymm ymm4_pb_mask_12stripe_01(ymm4);
    Xbyak::Ymm ymm5_zero(ymm5);

    lea(eax, ptr[edx + 5]); //edx-3
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vmovdqa(ymm0, yword[ecx + eax]);

    lea(esi, ptr[edx + 6]); //edx-2
    and(esi, 7);
    shl(esi, BLOCK_SIZE_YCP_LOG2);
    vpor(ymm0, ymm0, yword[ecx + esi]);

    lea(eax, ptr[edx + 7]); //edx-1
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vpor(ymm0, ymm0, yword[ecx + eax]);
    vpand(ymm0, ymm0, ymm4_pb_mask_12stripe_01);

    lea(eax, ptr[edx +5]); //edx-4
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vpor(ymm0, ymm0, yword[ecx + eax]);

    vmovdqu(yword[ebx], ymm0);
    mov(esi, edx); //edx
    and(esi, 7);
    shl(esi, BLOCK_SIZE_YCP_LOG2);
    vmovdqa(yword[ecx + esi], ymm5_zero);

    afs_analyze_count_motion(stack_ptr_mc_mask_offset);
};
