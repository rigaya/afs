#include "afs_filter_xbyak.h"

static const _declspec(align(64)) USHORT pw_index[32] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
};
static const BYTE pb_mshufmask[] = { 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3 };
static const uint32_t pd_mask_const[3] = { 0xf3f3f3f3, 0x44444444, 0x33333333 };

AFSMergeScanXbyak::AFSMergeScanXbyak(
    int si_w, int h, const AFS_SCAN_CLIP *mc_clip, bool enableAVX512, size_t size, void *userPtr)
    : Xbyak::CodeGenerator(size, userPtr) {
    using namespace Xbyak;
    param.si_w = si_w;
    param.h = h;
    param.mc_scan_top = mc_clip->top;
    param.mc_scan_bottom = mc_clip->bottom;
    avx512 = enableAVX512;
    push(ebp);
    mov(ebp, esp);
    and(esp, -STACK_ALIGN);
    mov(eax, ebp);
    sub(eax, esp); //and(esp, -STACK_ALIGN)の差分
    push(edi); push(esi); push(ebx);

    static const int STACK_PUSH                = 12;
    const int stack_ptr_orig_esp_offset        = 0;
    const int stack_ptr_dst_offset             = 4;
    const int stack_ptr_src0_offset            = 8;
    const int stack_ptr_src1_offset            = 12;
    const int stack_ptr_step_offset            = 16;
    const int stack_ptr_w_offset               = 20;
    const int stack_ptr_dst_block_fin_offset   = 24;
    const int stack_ptr_dst_line_offset        = 28;
    const int stack_ptr_src0_line_offset       = 32;
    const int stack_ptr_src1_line_offset       = 36;
    const int stack_ptr_tb_order_offset        = 40;
    const int stack_ptr_x_start_offset         = 44;
    const int stack_ptr_stripe_count_offset    = 48;
    const int stack_ptr_stripe_mask_offset     = 52; //size = 8
    const int stack_ptr_mc_clip_offset         = 60;
    const int stack_ptr_mc_mask_y_offset       = 64; //size = 8
    const int stack_ptr_mc_mask_x_offset       = 72; //size = BLOCK_SIZE / 8
    const int stack_fin = stack_ptr_mc_mask_x_offset + BLOCK_SIZE / 8;
    const int STACK_SIZE = ((stack_fin + (STACK_ALIGN-1) + STACK_PUSH) & ~(STACK_ALIGN-1)) - STACK_PUSH;
    static_assert(STACK_SIZE >= stack_fin, "STACK_SIZE is too small.");
    static_assert((STACK_SIZE + STACK_PUSH) % STACK_ALIGN == 0, "STACK_SIZE must be aligned size.");

    //スタック確保
    sub(esp, STACK_SIZE);

    const Address& stack_orig_esp_offset   = dword[esp + stack_ptr_orig_esp_offset];
    const Address& stack_ptr_dst           = dword[esp + stack_ptr_dst_offset];
    const Address& stack_ptr_src0          = dword[esp + stack_ptr_src0_offset];
    const Address& stack_ptr_src1          = dword[esp + stack_ptr_step_offset];
    const Address& stack_ptr_step          = dword[esp + stack_ptr_dst_block_fin_offset];
    const Address& stack_ptr_w             = dword[esp + stack_ptr_w_offset];
    const Address& stack_ptr_dst_block_fin = dword[esp + stack_ptr_dst_block_fin_offset];
    const Address& stack_ptr_dst_line      = dword[esp + stack_ptr_dst_line_offset];
    const Address& stack_ptr_src0_line     = dword[esp + stack_ptr_src0_line_offset];
    const Address& stack_ptr_src1_line     = dword[esp + stack_ptr_src1_line_offset];
    const Address& stack_ptr_tb_order      = dword[esp + stack_ptr_tb_order_offset];
    const Address& stack_ptr_x_start       = dword[esp + stack_ptr_x_start_offset];
    const Address& stack_ptr_stripe_count  = dword[esp + stack_ptr_stripe_count_offset];
    const Address& stack_ptr_stripe_mask   = dword[esp + stack_ptr_stripe_mask_offset];
    const Address& stack_ptr_mc_clip       = dword[esp + stack_ptr_mc_clip_offset];
    const Address& stack_ptr_mc_mask_y     = dword[esp + stack_ptr_mc_mask_y_offset];
    const Address& stack_ptr_mc_mask_x     = dword[esp + stack_ptr_mc_mask_x_offset];

    mov(stack_orig_esp_offset, eax); //and(esp, -32)の差分を保存
    mov(ecx, dword[ebp + 4 + 40]);   //int *stripe_count
    mov(stack_ptr_stripe_count, ecx);

    mov(eax, dword[ebp + 4 + 36]); //int tb_order
    mov(stack_ptr_tb_order, eax);

    mov(dword[esp + eax * 4 + stack_ptr_stripe_mask_offset], 0x50505050);
    inc(eax);
    and(eax, 1);
    mov(dword[esp + eax * 4 + stack_ptr_stripe_mask_offset], 0x60606060);
    mov(eax, dword[ebp + 4 + 16]); //int w
    mov(stack_ptr_w, eax);
    mov(ecx, dword[ebp + 4 + 44]); //void *mc_clip
    mov(stack_ptr_mc_clip, ecx);

    mov(edx, dword[ebp + 4 +  4]);     //BYTE *dst
    mov(ebx, dword[ebp + 4 + 32]);     //int x_fin
    add(ebx, edx);                     //dst_block_fin = dst + x_fin
    mov(stack_ptr_dst_block_fin, ebx);
    mov(esi, dword[ebp + 4 + 8]);      //void *src0
    mov(edi, dword[ebp + 4 + 12]);     //void *src1
    mov(eax, dword[ebp + 4 + 28]);     //int x_start
    mov(stack_ptr_x_start, eax);
    add(esi, eax); //src0 += x_start
    add(edi, eax); //src1 += x_start
    add(edx, eax); //dst += x_start

    //カウンタをクリア
    pxor(mm3, mm3);
    const int loop_x_inc = (avx512) ? 64 : 32;
    const int align = (avx512) ? 64 : 32;
    const int block_x_size = (param.si_w / (((param.si_w + BLOCK_SIZE-1) / BLOCK_SIZE)) + (align-1)) & (~(align-1));
    L("block_loop"); {
        //カウンタをクリア
        pxor(mm0, mm0);
 
        init_mc_mask(stack_ptr_mc_mask_x_offset, stack_ptr_w, stack_ptr_mc_clip);
        //定数をロード
        load_vec_const();

        mov(stack_ptr_src0, esi);
        mov(stack_ptr_src1, edi);
        mov(stack_ptr_dst, edx);
        mov(eax, block_x_size);
        mov(ebx, stack_ptr_dst_block_fin);
        sub(ebx, edx); //dst_block_fin - dst
        cmp(eax, ebx);
        cmovge(ebx, eax); //step = min(dst_block_fin - dst, block_size);
        mov(stack_ptr_step, ebx);
        mov(stack_ptr_src0_line, esi);
        mov(stack_ptr_src1_line, edi);
        lea(ecx, ptr[edx + param.si_w]);
        mov(stack_ptr_dst_line, ecx);
        lea(ebp, ptr[edx + ebx]);
        //loop pre
        xor(ecx, ecx);
        set_mc_y_mask(stack_ptr_mc_mask_y_offset);
        lea(ebx, dword[esp + stack_ptr_mc_mask_x_offset]);
        L("loop_x_1"); {
            loop(LOOP_PRE, stack_ptr_mc_mask_y, stack_ptr_stripe_mask_offset);
            add(ebx, loop_x_inc / 8);
            add(esi, loop_x_inc);
            add(edi, loop_x_inc);
            add(edx, loop_x_inc);
            cmp(edx, ebp);
            jb("loop_x_1");
        }
        pshufw(mm0, mm0, _MM_SHUFFLE(1, 0, 3, 2));
        inc(ecx);
        //loop main
        mov(esi, stack_ptr_src0_line);
        mov(edi, stack_ptr_src1_line);
        mov(edx, stack_ptr_dst_line);
        L("loop_main_y"); {
            set_mc_y_mask(stack_ptr_mc_mask_y_offset);

            lea(ebx, dword[esp + stack_ptr_mc_mask_x_offset]);
            //loop main
            mov(ebp, stack_ptr_step);
            add(ebp, edx);
            L("loop_x_2"); {
                loop(LOOP_MAIN, stack_ptr_mc_mask_y, stack_ptr_stripe_mask_offset);
                add(ebx, loop_x_inc / 8);
                add(esi, loop_x_inc);
                add(edi, loop_x_inc);
                add(edx, loop_x_inc);
                cmp(edx, ebp);
                jb("loop_x_2");
            }
            pshufw(mm0, mm0, _MM_SHUFFLE(1, 0, 3, 2));
            //loop_main_y loop to next
            mov(esi, stack_ptr_src0_line);
            mov(edi, stack_ptr_src1_line);
            mov(edx, stack_ptr_dst_line);
            add(esi, param.si_w);
            add(edi, param.si_w);
            add(edx, param.si_w);
            mov(stack_ptr_src0_line, esi);
            mov(stack_ptr_src1_line, edi);
            mov(stack_ptr_dst_line,  edx);
            inc(ecx);
            cmp(ecx, param.h-1);
            jnz("loop_main_y", T_NEAR);
        }

        //loop post
        set_mc_y_mask(stack_ptr_mc_mask_y_offset);
        lea(ebx, dword[esp + stack_ptr_mc_mask_x_offset]);
        mov(ebp, stack_ptr_step);
        add(ebp, edx);
        L("loop_x_3"); {
            loop(LOOP_POST, stack_ptr_mc_mask_y, stack_ptr_stripe_mask_offset);
            add(ebx, loop_x_inc / 8);
            add(esi, loop_x_inc);
            add(edi, loop_x_inc);
            add(edx, loop_x_inc);
            cmp(edx, ebp);
            jb("loop_x_3");
        }
        //block loop to next
        paddd(mm3, mm0);
        mov(esi, stack_ptr_src0);
        mov(edi, stack_ptr_src1);
        mov(edx, stack_ptr_dst);
        mov(eax, stack_ptr_x_start);
        add(esi, block_x_size);
        add(edi, block_x_size);
        add(edx, block_x_size);
        add(eax, block_x_size);
        mov(stack_ptr_x_start, eax);
        cmp(edx, stack_ptr_dst_block_fin);
        jb("block_loop", T_NEAR);
    }
    mov(eax, param.h);
    add(eax, stack_ptr_tb_order);
    and(eax, 1);
    mov(ecx, (size_t)pb_mshufmask);
    //qword[eax + 0] ->
    //qwordfeax + 4] ->
    //qword[eax + 8] ->
    //tb_order =0, (ih & 1) = 0 -> +4
    //tb_order =0, (ih & 1) = 1 -> +0
    //tb_order =1, (ih & 1) = 0 -> +0
    //tb_order =1, (ih & 1) = 1 -> +4

    pshufb(mm3, qword[ecx + eax * 4]);
    mov(eax, stack_ptr_stripe_count);
    paddd(mm3, qword[eax]);
    movq(qword[eax], mm3);

    //終了
    vzeroupper();
    emms();
    mov(eax, stack_orig_esp_offset);
    add(esp, STACK_SIZE);
    pop(ebx); pop(esi); pop(edi);
    add(esp, eax); //本来のespを復元
    pop(ebp);
    ret(44);
}

void AFSMergeScanXbyak::set_mc_y_mask(int stack_ptr_mc_mask_y_offset) {
    const int top = param.mc_scan_top;
    const int mc_scan_y_limit = (param.h - param.mc_scan_bottom - top) & ~1;
    mov(eax, ecx);
    sub(eax, param.mc_scan_top); //ih - top
    cmp(eax, mc_scan_y_limit);
    sbb(eax, eax); //((DWORD)(y -top) < (DWORD)y_limit) ? Oxffffffff : 0x00;
    mov(dword[esp + stack_ptr_mc_mask_y_offset], eax);
    if (avx512) {
        mov(dword[esp + stack_ptr_mc_mask_y_offset + 4], eax);
    }
}

void AFSMergeScanXbyak::init_mc_mask(const int stack_ptr_mc_mask_x_offset, const Xbyak::Address& stack_ptr_w, const Xbyak::Address& stack_ptr_mc_clip) {
    if (avx512) {
        init_mc_mask_avx512(stack_ptr_mc_mask_x_offset, stack_ptr_w, stack_ptr_mc_clip);
    } else {
        init_mc_mask_avx2(stack_ptr_mc_mask_x_offset, stack_ptr_w, stack_ptr_mc_clip);
    }
}

//eax ... x_start
//ebp ... 使用済み
void AFSMergeScanXbyak::init_mc_mask_avx2(const int stack_ptr_mc_mask_x_offset, const Xbyak::Address& stack_ptr_w, const Xbyak::Address& stack_ptr_mc_clip) {
    using namespace Xbyak;
    vmovdqa(ymm2, yword[pw_index]); //[i+ 0, i+ 8]
    vpcmpeqb(ymm7, ymm7, ymm7);
    movd(xmm3, eax);
    vpbroadcastw(ymm3, xmm3);
    vpaddw(ymm2, ymm2, ymm3);
    vpsrlw(ymm7, ymm7, 15);
    vpsllw(ymm7, ymm7, 4); //16
    vpaddw(ymm1, ymm2, ymm7); //[i+16, i+24]
    vpsllw(ymm7, ymm7, 1); //32
    vperm2i128(ymm0, ymm2, ymm1, (2<<4) + 0); //あとでvpacksswbするので [i+ 0, i+16]
    vperm2i128(ymm1, ymm2, ymm1, (3<<4) + 1); //あとでvpacksswbするので [i+ 8, i+24]

    mov(ebx, stack_ptr_mc_clip);
    xor(eax, eax);
    dec(eax);
    add(eax, dword[ebx + offsetof(AFS_SCAN_CLIP, left)]); //mc_clip->left
    movd(xmm2, eax);
    vpbroadcastw(ymm2, xmm2); //mc_clip->left - 1
    mov(eax, stack_ptr_w); //width
    sub(eax, dword[ebx + offsetof(AFS_SCAN_CLIP, right)]); //width - mc_clip->right
    movd(xmm3, eax);
    vpbroadcastw(ymm3, xmm3); //width - mc_clip->right
    xor(ebx, ebx);
    L("init_mc_mask_loop1"); {
        vpcmpgtw(ymm4, ymm0, ymm2); // ([i+ 0] > (mc_clip->left - 1)) = ([i+ 0] >= mc_clip->left)
        vpcmpgtw(ymm5, ymm3, ymm0); // mc_clip->right > [i+ 0]
        vpand(ymm4, ymm4, ymm5); // mc_clip->left <= [i+ 0] < mc_clip->right

        vpcmpgtw(ymm5, ymm1, ymm2); // ([i+16] > (mc_clip->left - 1)) = ([i+16] >= mc_clip->left)
        vpcmpgtw(ymm6, ymm3, ymm1); // mc_clip->right > [i+16]
        vpand(ymm5, ymm5, ymm6); // mc_clip->left <= [i+16] < mc_clip->right

        vpacksswb(ymm4, ymm4, ymm5);
        vpmovmskb(eax, ymm4);
        mov(dword[esp + ebx * 4 + stack_ptr_mc_mask_x_offset], eax);
        vpaddw(ymm0, ymm0, ymm7); //[i+ 0] += 32
        vpaddw(ymm1, ymm1, ymm7); //[i+16] += 32
        inc(ebx);
        cmp(ebx, BLOCK_SIZE / 32);
        static_assert(BLOCK_SIZE % 32 == 0, "BLOCK_SIZE should be mod32.");
        jb("init_mc_mask_loop1");
    }
}

//eax ... x_start
//ebp ... 使用済み
void AFSMergeScanXbyak::init_mc_mask_avx512(int stack_ptr_mc_mask_x_offset, const Xbyak::Address& stack_ptr_w, const Xbyak::Address& stack_ptr_mc_clip) {
    using namespace Xbyak;
    vmovdqa32(zmm0, zword[pw_index]); //[i+ 0, i+ 8, i+16, i+24]
    vpternlogd(zmm7, zmm7, zmm7, 0xff);
    vpbroadcastw(zmm3, ax);
    vpaddw(zmm0, zmm0, zmm3);
    vpsrlw(zmm7, zmm7, 15);
    vpsllw(zmm7, zmm7, 5); //32
    vpaddw(zmm1, zmm0, zmm7); //[i+32, i+40, i+48, i+56]
    vpsllw(zmm7, zmm7, 1); //64

    mov(ebx, stack_ptr_mc_clip);
    xor(eax, eax);
    dec(eax);
    add(eax, dword[ebx + offsetof(AFS_SCAN_CLIP, left)]); //mc_clip->left
    vpbroadcastw(zmm2, ax); //mc_clip->left - 1
    mov(eax, stack_ptr_w); //width
    sub(eax, dword[ebx + offsetof(AFS_SCAN_CLIP, right)]); //width - mc_clip->right
    vpbroadcastw(zmm3, ax); //width - mc_clip->right

    lea(ebx, ptr[esp + stack_ptr_mc_mask_x_offset]);
    mov(eax, BLOCK_SIZE / 64);
    static_assert(BLOCK_SIZE % 64 == 0, "BLOCK_SIZE should be mod64.");
    L("init_mc_mask_loop2"); {
        vpcmpgtw(k3, zmm0, zmm2); // ([i+ 0] > (mc_clip->left - 1)) = ([i+ 0] >= mc_clip->left)
        vpcmpgtw(k2, zmm3, zmm0); // mc_clip->right > [i+ 0]
        kandd(k2, k2, k3);        // mc_clip->left <= [i+ 0] < mc_clip->right
        kmovd(dword[ebx], k2);

        vpcmpgtw(k3, zmm1, zmm2); // ([i+32] > (mc_clip->left - 1)) = ([i+32] >= mc_clip->left)
        vpcmpgtw(k2, zmm3, zmm1); // mc_clip->right > [i+32]
        kandd(k2, k2, k3);        // mc_clip->left <= [i+32] < mc_clip->right
        kmovd(dword[ebx+4], k2);

        vpaddw(zmm0, zmm0, zmm7); //[i+ 0] += 64
        vpaddw(zmm1, zmm1, zmm7); //[i+32] += 64
        add(ebx, 8);
        dec(eax);
        jnz("init_mc_mask_loop2");
    }
}

void AFSMergeScanXbyak::load_vec_const() {
    mov(eax, (size_t)pd_mask_const);
    if (avx512) {
        vpbroadcastd(zmm7, dword[eax+0]); //0xf3f3f3f3
        vpbroadcastd(zmm6, dword[eax+4]); //0x44444444
        vpbroadcastd(zmm3, dword[eax+8]); //0x33333333
    } else {
        vpbroadcastd(ymm7, dword[eax+0]); //0xf3f3f3f3
        vpbroadcastd(ymm6, dword[eax+4]); //0x44444444
        vpbroadcastd(ymm3, dword[eax+8]); //0x33333333
    }
}

void AFSMergeScanXbyak::loop(LOOP_PHASE loop_phase, const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset) {
    if (avx512) {
        loop_avx512(loop_phase, stack_ptr_mc_mask_y, stack_ptr_stripe_mask_offset);
    } else {
        loop_avx2(loop_phase, stack_ptr_mc_mask_y, stack_ptr_stripe_mask_offset);
    }
}

//eax ... tmp
//ebx ... ptr_mc_mask_x
//ecx ... ih
//edx ... dst
//esi ... src0
//edi ... src1
//ebp ... dst_fin
//ymm3 ... ymm3_PbMask33
//ymm6 ... ymm6_PbHask44
//ymmZ ... ymm7_PbHaskf3
void AFSMergeScanXbyak::loop_avx2(LOOP_PHASE loop_phase, const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset) {
    const Xbyak::Ymm ymm7_PbMaskf3(ymm7);
    const Xbyak::Ymm ymm6_PbMask44(ymm6);
    const Xbyak::Ymm ymm3_PbMask33(ymm3);
    vmovdqa(ymm0, yword[esi + param.si_w]);
    vmovdqa(ymm2, yword[edi + param.si_w]);
    switch (loop_phase) {
    case LOOP_PRE:
        vpor(ymm4, ymm0, yword[esi + param.si_w * 2]);
        vpor(ymm5, ymm2, yword[edi + param.si_w * 2]);
        prefetcht0(ptr[esi + param.si_w * 3]);
        prefetcht0(ptr[edi + param.si_w * 3]);
        break;
    case LOOP_POST:
        vpor(ymm4, ymm0, yword[esi]);
        vpor(ymm5, ymm2, yword[edi]);
        break;
    case LOOP_MAIN:
    default:
        vmovdqa(ymm4, yword[esi]);
        vmovdqa(ymm5, yword[edi]);
        vpor(ymm4, ymm4, yword[esi + param.si_w * 2]);
        vpor(ymm5, ymm5, yword[edi + param.si_w * 2]);
        prefetcht0(ptr[esi + param.si_w * 3]);
        prefetcht0(ptr[edi + param.si_w * 3]);
        break;
    }
    vpor(ymm4, ymm4, ymm7_PbMaskf3);
    vpor(ymm5, ymm5, ymm7_PbMaskf3);
    vpand(ymm4, ymm4, ymm0);
    vpand(ymm5, ymm5, ymm2);
    vpand(ymm4, ymm4, ymm5);
    vpand(ymm4, ymm4, ymm6_PbMask44);
    vpandn(ymm5, ymm0, ymm3_PbMask33);
    vpor(ymm4, ymm4, ymm5);
    vmovdqa(yword[edx], ymm4);
    count_stripe_avx2(stack_ptr_mc_mask_y, stack_ptr_stripe_mask_offset);
}
//eax ... tmp
//ebx ... ptr_mc_mask_x
//ecx ... ih
//edx ... dst
//esi ... src0
//edi ... src1
//ebp ... dst_fin
//zmm3 ... zmm3_PbMask33
//zmm6 ... zmm6_PbMask44
//zmm7 ... zmm7_PbMaskf3
void AFSMergeScanXbyak::loop_avx512(LOOP_PHASE loop_phase, const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset) {
    Xbyak::Zmm zmm7_PbMaskf3(zmm7);
    Xbyak::Zmm zmm6_PbMask44(zmm6);
    Xbyak::Zmm zmm3_PbMask33(zmm3);
    switch (loop_phase) {
    case LOOP_PRE:
    case LOOP_POST:
        vmovdqa32(zmm0, zword[esi + param.si_w]);
        vmovdqa32(zmm2, zword[edi + param.si_w]);
        if (loop_phase == LOOP_PRE) {
            vpord(zmm4, zmm0, zword[esi + param.si_w * 2]);
            vpord(zmm5, zmm2, zword[edi + param.si_w * 2]);
            prefetcht0(ptr[esi + param.si_w * 3]);
            prefetcht0(ptr[edi + param.si_w * 3]);
        } else { //loop_phase == LOOP_POST
            vpord(zmm4, zmm0, zword[esi]);
            vpord(zmm5, zmm2, zword[edi]);
        }
        vpternlogd(zmm4, zmm7_PbMaskf3, zmm0, (TL_R0 | TL_R1) & TL_R2);
        vpternlogd(zmm5, zmm7_PbMaskf3, zmm2, (TL_R0 | TL_R1) & TL_R2);
        break;
    case LOOP_MAIN:
    default:
        vmovdqa32(zmm0, zword[esi + param.si_w]);
        vmovdqa32(zmm4, zword[esi]);
        vmovdqa32(zmm5, zword[edi]);
        vpord(zmm4, zmm4, zword[esi + param.si_w * 2]);
        vpord(zmm5, zmm5, zword[edi + param.si_w * 2]);
        vpternlogd(zmm4, zmm7_PbMaskf3, zmm0, (TL_R0 | TL_R1) & TL_R2);
        vpternlogd(zmm5, zmm7_PbMaskf3, zword[edi + param.si_w], (TL_R0 | TL_R1) & TL_R2);
        prefetcht0(ptr[esi + param.si_w * 3]);
        prefetcht0(ptr[edi + param.si_w * 3]);
        break;
    }
    vpternlogd(zmm4, zmm5, zmm6_PbMask44, TL_R0 & TL_R1 & TL_R2);
    vpternlogd(zmm4, zmm0, zmm3_PbMask33, TL_R0 | ((~TL_R1) & TL_R2));
    vmovdqa32(zword[edx], zmm4);
    count_stripe_avx512(stack_ptr_mc_mask_y, stack_ptr_stripe_mask_offset);
};

//eax ... tmp
//ebx ... ptr_mc_mask_x
//ecx ... ih
//edx ... dst
//esi ... src0
//edi ... src1
//ebp ... dst_fin
//ymm4 ... 対象
//ymm3 ... 定数
//ymm6 ... 定数
//ymm7 ... 定数
void AFSMergeScanXbyak::count_stripe_avx2(const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset) {
    mov(eax, ecx);
    and(eax, 1);
    vpbroadcastd(ymm1, yword[esp + eax * 4 + stack_ptr_stripe_mask_offset]);
    vpxor(ymm0, ymm0, ymm0);
    vpand(ymm4, ymm4, ymm1);
    vpcmpeqb(ymm0, ymm4, ymm0);
    vpmovmskb(eax, ymm0);
    and(eax, stack_ptr_mc_mask_y);
    and(eax, dword[ebx]);
    popcnt(eax, eax);
    movd(mm1, eax);
    paddd(mm0, mm1);
}

//eax ... tmp
//ebx ... ptr_mc_mask_x
//ecx ... ih
//edx ... dst
//esi ... src0
//edi ... src1
//ebp ... dst_fin
//ymm4 ... 対象
//ymm3 ... 定数
//ymm6 ... 定数
//ymm7 ... 定数
void AFSMergeScanXbyak::count_stripe_avx512(const Xbyak::Address& stack_ptr_mc_mask_y, int stack_ptr_stripe_mask_offset) {
    mov(eax, ecx);
    and(eax, 1);
    vpxord(zmm0, zmm0, zmm0);
    vpandd(zmm4, zmm4, ptr_b[esp + eax * 4 + stack_ptr_stripe_mask_offset]);
    vpcmpeqb(k1, zmm4, zmm0);
    kmovq(k2, ptr[ebx]);
    kmovq(k3, stack_ptr_mc_mask_y);
    kandq(k1, k1, k2);
    kandq(k1, k1, k3);

    kmovd(eax, k1);
    kshiftrq(k1, k1, 32);
    popcnt(eax, eax);
    movd(mm1, eax);
    paddd(mm0, mm1);

    kmovd(eax, k1);
    popcnt(eax, eax);
    movd(mm1, eax);
    paddd(mm0, mm1);
}
