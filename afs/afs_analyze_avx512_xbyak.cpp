#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include "afs.h"
#if AFS_USE_XBYAK
#include <immintrin.h>
#include "afs_analyze_xbyak.h"


static const _declspec(align(64)) BYTE pb_thre_count[64]       = { 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03, 0x02, 0x03 };
static const _declspec(align(64)) BYTE pw_thre_count2[64]      = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
static const _declspec(align(64)) BYTE pw_thre_count1[64]      = { 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00, 0x03, 0x00 };
static const _declspec(align(64)) BYTE pw_mask_2stripe_0[64]   = { 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00, 0x01, 0x00 };
static const _declspec(align(64)) BYTE pw_mask_2stripe_1[64]   = { 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00, 0x02, 0x00 };
static const _declspec(align(64)) BYTE pw_mask_1stripe_0[64]   = { 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00, 0x10, 0x00 };
static const _declspec(align(64)) BYTE pw_mask_1stripe_1[64]   = { 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00, 0x20, 0x00 };
static const _declspec(align(64)) BYTE pw_mask_12stripe_0_lsft4[64]  = { 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01, 0x10, 0x01 };
static const _declspec(align(64)) BYTE pw_mask_12stripe_1_lsft4[64]  = { 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02, 0x20, 0x02 };
static const _declspec(align(64)) BYTE pw_mask_2motion_0[64]   = { 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04, 0x00, 0x04 };
static const _declspec(align(64)) BYTE pw_mask_1motion_0[64]   = { 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x40 };
static const _declspec(align(64)) BYTE pw_mask_12motion_0[64]  = { 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44, 0x00, 0x44 };

static const _declspec(align(64)) BYTE pb_mask_1stripe_01[64]  = { 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30 };
static const _declspec(align(64)) BYTE pb_mask_12stripe_01[64] = { 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 };
static const _declspec(align(64)) BYTE pb_mask_12motion_01[64] = { 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc };
static const _declspec(align(64)) BYTE pb_mask_12motion_stripe_01[64] = { 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0xcc, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33 };
static const _declspec(align(64)) BYTE pw_mask_12stripe_01[64] = { 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00, 0x33, 0x00 };
static const _declspec(align(64)) BYTE pw_mask_12motion_01[64] = { 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00, 0xcc, 0x00 };
static const _declspec(align(64)) BYTE pw_mask_lowbyte[64]     = { 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00 };
static const _declspec(align(64)) BYTE pw_mask_highbyte[64]    = { 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff, 0x00, 0xff };
static const _declspec(align(64)) BYTE pw_mask_lowshort[64]    = { 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00 };

static const _declspec(align(64)) BYTE pb_mask_2motion_0[64]   = { 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04 };
static const _declspec(align(64)) BYTE pb_mask_1motion_0[64]   = { 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40 };

static _declspec(align(64)) USHORT pw_thre_shift[32]    = { 0 };
static _declspec(align(64)) USHORT pw_thre_deint[32]    = { 0 };
static _declspec(align(64)) USHORT pw_thre_motion[32]   = { 0 };

static const _declspec(align(64)) USHORT pw_index[32] = {
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
};
static const _declspec(align(64)) BYTE pb_index[64] = {
     0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31,
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63
};

alignas(64) static const uint16_t PACK_YC48_SHUFFLE_AVX512[32] = {
     0,  3,  6,  9, 12, 15, 18, 21, 24, 27, 30, 33, 36, 39, 42, 45,
    48, 51, 54, 57, 60, 63,  2,  5,  8, 11, 14, 17, 20, 23, 26, 29
};

static const BYTE pb_mshufmask[] = { 4, 5, 6, 7, 0, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3 };

void __stdcall afs_analyze_set_threshold_avx512(int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion) {
    __m512i z0, z1;
    z0 = _mm512_set1_epi16((short)thre_shift);
    _mm512_store_si512((__m512i *)pw_thre_shift, z0);

    z1 = _mm512_set1_epi16((short)thre_deint);
    _mm512_store_si512((__m512i *)pw_thre_deint, z1);

    for (int i = 0; i < 64; i++) {
        pw_thre_motion[i] = (USHORT)((i % 3 == 0) ? thre_Ymotion : thre_Cmotion);
    }
}

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

AFSAnalyzeXbyakAVX512::AFSAnalyzeXbyakAVX512(
    int tb_order, int step, int si_pitch, int h, int max_h, int mc_scan_top, int mc_scan_bottom,
    size_t size, void *userPtr) : AFSAnalyzeXbyak(size, userPtr) {
    using namespace Xbyak;
    const int step6 = step * 6;
    const int STACK_ALIGN = 64;
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
    sub(eax, esp); //and(esp, -STACK_ALIGN)の差分

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
    const int stack_ptr_tmp16pix_offset      = 64; //size = 128
    const int stack_ptr_buffer_offset        = 192; //size = BUFFER_SIZE
    const int stack_ptr_buffer2_offset       = stack_ptr_buffer_offset  + BUFFER_SIZE;        //size = BUFFER_SIZE
    const int stack_ptr_mc_mask_offset       = stack_ptr_buffer2_offset + BLOCK_SIZE_YCP * 8; //size = BLOCK_SIZE_YCP
    const int stack_ptr_pw_thre_motion_offset = stack_ptr_mc_mask_offset + BLOCK_SIZE_YCP;  //size = BLOCK_SIZE_YCP
    const int stack_fin  = stack_ptr_pw_thre_motion_offset + 64 + 4 + 4;
    const int STACK_SIZE = ((stack_fin + (STACK_ALIGN-1) + STACK_PUSH) & ~(STACK_ALIGN-1)) - STACK_PUSH;
    static_assert(STACK_SIZE >= stack_fin, "STACK_SIZE is too small.");
    static_assert((STACK_SIZE + STACK_PUSH) % 64 == 0, "STACK_SIZE must be mod32.");
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
    const Address& stack_ptr_tmp16pix       = zword[esp + stack_ptr_tmp16pix_offset];      //size = 64
    const Address& stack_ptr_buffer         = zword[esp + stack_ptr_buffer_offset];        //size = BUFFER_SIZE
    const Address& stack_ptr_buffer2        = zword[esp + stack_ptr_buffer2_offset];       //size = BLOCK_SIZE_YCP * 8
    const Address& stack_ptr_mc_mask        = byte[esp + stack_ptr_mc_mask_offset];        //size = BLOCK_SIZE_YCP
    const Address& stack_ptr_pw_thre_motion = zword[esp + stack_ptr_pw_thre_motion_offset];  //size = 32
    
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
    lea(edx, ptr[ebp + 4]);
    mov(stack_ptr_h_start_plus4, edx);

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

    mov(ecx, 0xffc00000);
    kmovd(k7, ecx); //0xffc00000
    sar(ecx, 1);
    kmovd(k6, ecx); //0xffe00000
    mov(ecx, 0xaaaaaaaa);
    kmovd(k5, ecx); //0xaaaaaaaa
    kunpckdq(k5, k5, k5); //0xaaaaaaaaaaaaaaaa

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

void AFSAnalyzeXbyakAVX512::copy_pw_thre_motion_to_stack(const Xbyak::Address& stack_ptr_pw_thre_motion) {
    vmovdqa32(zmm0, zword[pw_thre_motion]);
    vmovdqa32(stack_ptr_pw_thre_motion, zmm0);
}

//edi ... dst
//ebx ... 変更せず
//esi ... 変更せず
//edx ... ih
//
//ecx ... ptr_mc_clip -> tmp
//eax ... tmp
//zmm0-zmm7 ... 使用
void AFSAnalyzeXbyakAVX512::init_mc_mask(const int stack_ptr_mc_mask_offset) {
    using namespace Xbyak;
    vmovdqa32(zmm2, zword[pw_index]); //[i+ 0, i+ 8, i+16, i+24]
    vpternlogd(zmm7, zmm7, zmm7, 0xff);
    vpsrlw(zmm7, zmm7, 15);
    vpsllw(zmm7, zmm7, 5); //32
    vpaddw(zmm1, zmm2, zmm7); //[i+32, i+40, i+48, i+56]
    vpsllw(zmm7, zmm7, 1); //64
    vshufi64x2(zmm0, zmm2, zmm1, _MM_SHUFFLE(2, 0, 2, 0)); //あとでvpacksswbするので [i+ 0, i+16, i+32, i+48]
    vshufi64x2(zmm1, zmm2, zmm1, _MM_SHUFFLE(3, 1, 3, 1)); //あとでvpacksswbするので [i+ 8, i+24, i+40, i+56]

    xor(eax, eax);
    dec(eax); //-1
    add(eax, dword[ecx + offsetof(AFS_SCAN_CLIP, left)]); //mc_clip->left -1
    vpbroadcastw(zmm2, ax); //mc_clip->left - 1
    mov(ecx, dword[ecx + offsetof(AFS_SCAN_CLIP, right)]); //mc_clip->right
    mov(eax, ebx); //width
    sub(eax, ecx); //width - mc_clip->right
    vpbroadcastw(zmm3, ax); //width - mc_clip->right

    lea(eax, ptr[esp + stack_ptr_mc_mask_offset]);
    mov(ecx, BLOCK_SIZE_YCP / 64);
    static_assert(BLOCK_SIZE_YCP % 64 == 0, "BLOCK_SIZE_YCP should be mod64.");
    L("init_mc_mask_loop1"); {
        vpcmpgtw(k3, zmm0, zmm2); // ([i+ 0] > (mc_clip->left - 1)) = ([i+ 0] >= mc_clip->left)
        vpcmpgtw(k2, zmm3, zmm0); // mc_clip->right > [i+ 0]
        kandd(k2, k2, k3);    // mc_clip->left <= [i+ 0] < mc_clip->right
        vpmovm2w(zmm4, k2);

        vpcmpgtw(k3, zmm1, zmm2); // ([i+8] > (mc_clip->left - 1)) = ([i+16] >= mc_clip->left)
        vpcmpgtw(k2, zmm3, zmm1); // mc_clip->right > [i+8]
        kandd(k2, k2, k3);    // mc_clip->left <= [i+8] < mc_clip->right
        vpmovm2w(zmm5, k2);

        vpacksswb(zmm4, zmm4, zmm5);
        vmovdqa32(zword[eax], zmm4);

        vpaddw(zmm0, zmm0, zmm7); //[i+ 0] += 64 
        vpaddw(zmm1, zmm1, zmm7); //[i+ 8] += 64 
        add(eax, 64);
        dec(ecx);
        jnz("init_mc_mask_loop1");
    }
}

//ebx ... width
//edx ... ih
//esi ... p0
//edi ... p1
//
//zmm6 ... pw_thre_shift
//zmm7 ... pw_mask_12motion_0
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
void AFSAnalyzeXbyakAVX512::afs_analyze_loop1(
    const Xbyak::Address& stack_ptr_p0, const Xbyak::Address& stack_ptr_p1,
    const Xbyak::Address& stack_ptr_pw_thre_motion,
    int step6, int stack_ptr_buffer2_offset) {
    lea(ecx, ptr[esp + stack_ptr_buffer2_offset]); //buf2ptr
    lea(ebp, ptr[ecx + ebx/* width */]); //buf2ptr_fin
    vmovdqa32(zmm7, zword[pw_mask_2motion_0]);
    L("afs_analyze_loop1"); {
        //zmm2のpe_thre_motionは、afs_shrink_infoの呼び出しで破棄されるのでここでロード
        vmovdqa32(zmm2, stack_ptr_pw_thre_motion); //broadcast不可
        //zmm6のpw_thre_shiftは、afs_shrink_infoの呼び出しで破棄されるのでここでロード
        vmovdqa32(zmm6, zword[pw_thre_shift]);
        afs_analyze_loop_1_internal(zmm5, zmm2, zmm7, zmm6, step6, false,  0);
        afs_analyze_loop_1_internal(zmm4, zmm2, zmm7, zmm6, step6, false, 32);
        afs_analyze_loop_1_internal(zmm3, zmm2, zmm7, zmm6, step6, true,  64);
        afs_shrink_info(true, ecx, zmm5, zmm4, zmm3);
        add(ecx, 32);
        cmp(ecx, ebp);
        jb("afs_analyze_loop1");
    }
}

//zmm3 - zmm7
//esi ... p0
//edi ... p1
//eax ... width
//ecx ... outer loop counter
//ebp ... outer loop range
//edx ... ih
//
//zmm0 - zmm1
//k2, k3 ... tmp
void AFSAnalyzeXbyakAVX512::afs_analyze_loop_1_internal(
    const Xbyak::Zmm& zmm_out, /*zmm3 - zmm5*/
    const Xbyak::Zmm& zmm2_pw_thre_motion,
    const Xbyak::Zmm& zmm7_pw_mask_2motion_0,
    const Xbyak::Zmm& zmm6_pw_thre_shift,
    int step6, bool third_call, int offset) {
    vmovdqu16(zmm0, zword[esi + offset]);
    vpsubw(zmm0, zmm0, zword[edi + offset]);
    prefetcht0(ptr[esi + step6 + offset]);
    prefetcht0(ptr[edi + step6 + offset]);
    vpabsw(zmm0, zmm0);
    vpcmpgtw(k3, zmm2_pw_thre_motion, zmm0);
    vpcmpgtw(k2, zmm6_pw_thre_shift, zmm0);
    if (!third_call) {
        //最後の呼び出し(third_call = true)以外では、
        //次の呼び出しのため、zmm2_pw_thre_motionをシャッフル
        vpermq(zmm2_pw_thre_motion, zmm2_pw_thre_motion, _MM_SHUFFLE(2,1,3,2));
    }
    vmovdqu16(zmm_out | k3 | T_z, zmm7_pw_mask_2motion_0); //0x0400
    vpsllw(zmm0    | k2 | T_z, zmm7_pw_mask_2motion_0, 4); //0x0400 -> 0x4000
    vpord(zmm_out, zmm_out, zmm0);
}

// ----------------------------------
//loop1 = true
//zmm6 - zmm7 ... 使用済み
//zmm3 - zmm5 ... 引数
//esi ... p0
//edi ... p1
//eax ... tmp
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
//edx ... ih
//zmm0 - zmm2 ... 一時変数
// ----------------------------------
//loop1 = false
// ----------------------------------
void AFSAnalyzeXbyakAVX512::afs_shrink_info(
    bool loop1,
    const Xbyak::Reg32& ecx, /*buf2_out*/
    const Xbyak::Zmm& zmm5, const Xbyak::Zmm& zmm4, const Xbyak::Zmm& zmm3) {
    vmovdqa32(zmm0, zword[PACK_YC48_SHUFFLE_AVX512]);
    vpxord(zmm6, zmm6, zmm6); //依存関係を明示的に切る
    vpternlogd(zmm6, zmm6, zmm6, 0xff);

    vmovdqa32(zmm1, zmm0);
    vpermi2w(zmm1, zmm5, zmm4);
    vpermw(zmm1 | k7, zmm0, zmm3);
    vpsubw(zmm0, zmm0, zmm6);

    vmovdqa32(zmm2, zmm0);
    vpermi2w(zmm2, zmm5, zmm4);
    vpermw(zmm2 | k6, zmm0, zmm3);
    vpsubw(zmm0, zmm0, zmm6);

    vmovdqa32(zmm6, zmm0);
    vpermi2w(zmm6, zmm5, zmm4);
    vpermw(zmm6 | k6, zmm0, zmm3);

    vmovdqa32(zmm0, zmm1);
    vpternlogd(zmm0, zmm2, zmm6, TL_R0 & TL_R1 & TL_R2);
    vpternlogd(zmm1, zmm2, zmm6, TL_R0 | TL_R1 | TL_R2);

    vpsraw(zmm0, zmm0, 8);
    vpmovwb(ymm0, zmm0);
    vpmovwb(ymm1, zmm1);
    vpandd(ymm0, ymm0, yword[pb_mask_12motion_01]);
    vpandd(ymm1, ymm1, yword[pb_mask_12stripe_01]);
    vpord(ymm0, ymm0, ymm1);
    if (loop1) {
        vmovdqa(yword[ecx], ymm0);
    } else {
        mov(eax, edx);
        and(eax, 7);
        shl(eax, BLOCK_SIZE_YCP_LOG2);
        add(eax, ecx);
        vmovdqa(yword[eax], ymm0);

        lea(eax, ptr[edx+1]);
        and(eax, 7);
        shl(eax, BLOCK_SIZE_YCP_LOG2);
        add(eax, ecx);
        vmovdqa(yword[eax], ymm1);
    }
}

//afs_analyze_loop2_w1 ループ内
//eax  ... tmp
//esi  ... p0
//edi  ... p1
//ebx  ... bufptr
//ecx  ... buf2ptr
//ebp  ... buf2ptr_fin
//zmm5 ... pw_thre_motion
//zmm6 ... pw_thre_shift
//zmm7 ... pb_thre_count
//
//afs_analyze_loop2_w2 ループ内
//edx  ... ih
//eax  ... tmp
//ebx  ... dst
//ecx  ... buf2ptr
//ebp  ... buf2ptr_fin
//esi  ... tmp
//edi  ... mc_mask_ptr
//zmm4 ... pb_mask_12stripe_01
//zmm5 ... pb_mask_1stripe_01
//zmm6 ... pw_thre_shift
//zmm7 ... pb_thre_count
void AFSAnalyzeXbyakAVX512::afs_analyze_loop2(int step6, int si_pitch,
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
    vmovdqa32(zmm7, zword[pb_thre_count]);
    vmovdqa32(zmm6, zword[pw_thre_shift]);
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
            vmovdqa32(zmm5, stack_ptr_pw_thre_motion); //broadcast不可
            //zmm6のpw_thre_shiftは、afs_shrink_infoの呼び出しで破棄されるのでここでロード
            vmovdqa32(zmm6, zword[pw_thre_shift]);
            vmovdqa32(zmm4, zword[pw_thre_deint]);
            call("afs_analyze_loop2_1_internal");
            vmovdqa32(zword[esp + stack_ptr_tmp16pix_offset + 0], zmm3);

            add(esi, 64);
            add(edi, 64);
            sub(ebx, -128);
            vpermq(zmm5, zmm5, _MM_SHUFFLE(2, 1, 3, 2));
            call("afs_analyze_loop2_1_internal");
            vmovdqa32(zword[esp + stack_ptr_tmp16pix_offset + 64], zmm3);

            add(esi, 64);
            add(edi, 64);
            sub(ebx, -128);
            vpermq(zmm5, zmm5, _MM_SHUFFLE(2, 1, 3, 2));
            call("afs_analyze_loop2_1_internal");

            add(esi, 64);
            add(edi, 64);
            sub(ebx, -128);

            vmovdqa32(zmm5, zword[esp + stack_ptr_tmp16pix_offset +  0]);
            vmovdqa32(zmm4, zword[esp + stack_ptr_tmp16pix_offset + 64]);
            afs_shrink_info(false, ecx, zmm5, zmm4, zmm3);
            add(ecx, 32);
            cmp(ecx, ebp);
            jb("afs_analyze_loop2_w1", T_NEAR);
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
            vmovdqa32(zmm4, zword[pb_mask_12stripe_01]);
            vmovdqa32(zmm5, zword[pb_mask_1stripe_01]);
            L("afs_analyze_loop2_w2"); {
                afs_analyze_loop2_2_internal(stack_ptr_mc_mask_offset);
                add(ebx, 64);
                add(edi, 64);
                add(ecx, 64);
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
//zmm5 ... pw_thre_motion
//zmm6 ... pw_thre_shift
//zmm7 ... pb_thre_count
//mm0  ... counter
//mm7  ... p1-p0
//
//zmm3 ... out
//m6   ... tmp
void AFSAnalyzeXbyakAVX512::afs_analyze_loop2_1_internal(int step6, int si_pitch, int h) {
    using namespace Xbyak;
    Zmm zmm7_pb_thre_count(zmm7);
    Zmm zmm6_pw_thre_shift(zmm6);
    Zmm zmm5_pw_thre_motion(zmm5);
    Zmm zmm4_pw_thre_deint(zmm4);
    align();
    L("afs_analyze_loop2_1_internal");
    //analyze motion
    vmovdqa32(zmm0, zword[esi + step6]);
    vmovdqa32(zmm1, zmm0);
    vpsubw(zmm0, zmm0, zword[edi + step6]);
    vpabsw(zmm0, zmm0);
    vpcmpgtw(k3, zmm5_pw_thre_motion, zmm0); //0x0400
    vpcmpgtw(k2, zmm6_pw_thre_shift, zmm0);  //0x4000
    vmovdqa32(zmm2, zword[pw_mask_2motion_0]);
    vmovdqu16(zmm3 | k3 | T_z, zmm2); //0x0400
    vpsllw(zmm2 | k2 | T_z, zmm2, 4); //0x0400 -> 0x4000
    vpord(zmm3, zmm3, zmm2);

    prefetcht0(ptr[esi + step6 * 2]);
    prefetcht0(ptr[edi + step6 * 2]);

    //analyze non-shift
    vmovdqa32(zmm2, zword[esi]);
    vpsubw(zmm2, zmm2, zmm1);
    vpabsw(zmm0, zmm2);
    vpcmpeqw(k2, zmm2, zmm0);
    vpcmpgtw(k1, zmm0, zmm4_pw_thre_deint);
    vpcmpgtw(k3, zmm0, zmm6_pw_thre_shift);
    vpmovm2w(zmm1, k1);
    vpmovm2w(zmm0, k3);
    vmovdqu8(zmm1 | k5, zmm0); //zmm0を上位バイトに

    vmovdqa32(zmm0, zword[ebx]);
    vpabsw(zmm2, zmm0);       //絶対値がcountの値
    vpmovw2m(k1, zmm0);       //符号を取り出し
    //vpsraw(zmm0, zmm0, 15);   //符号付きとしてシフトし、0xffffか0x0000を作る
    kxord(k1, k2, k1);
    //vpxord(zmm0, zmm2, zmm0);

    vpandd(zmm2, zmm2, zmm1);
    vmovdqu16(zmm2 | k1 | T_z, zmm2);

    vpsubsb(zmm0, zmm2, zmm1);
    //vpandd(zmm0, zmm4, zmm0);
    //vpsubsb(zmm0, zmm0, zmm1);

    vmovdqa32(zmm2, zmm0);
    vpxord(zmm1, zmm1, zmm1);
    vpsubw(zmm2 | k2, zmm1, zmm2); //フラグが負(0xffff)なら、カウントを負に
    //vpandnd(zmm4, zmm2, zmm0);  //フラグが負(0xffff)なら、カウントを0に
    //vpsignw(zmm2, zmm0, zmm2); //フラグが負(0xffff)なら、カウントを負に
    //vpord(zmm2, zmm2, zmm4);    //フラグが0だった場合の加算
    vmovdqa32(zword[ebx], zmm2);

    vpcmpgtb(k1, zmm0, zmm7_pb_thre_count);
    vmovdqu8(zmm0 | k1 | T_z, zword[pw_mask_12stripe_0_lsft4]);
    vpsrlw(zmm0, zmm0, 4);
    vpord(zmm3, zmm3, zmm0);

    //analyze shift
    movd(eax, mm7); //p1-p0 or 0
    vmovdqa32(zmm2, zword[esi + eax]); //p0 あるいは p1
    pshufw(mm6, mm7, _MM_SHUFFLE(1, 0, 3, 2));
    movd(eax, mm6); //さきほどの逆
    vpsubw(zmm2, zmm2, zword[esi + eax + step6]);
    vpabsw(zmm0, zmm2);
    vpcmpeqw(k2, zmm2, zmm0);
    vpcmpgtw(k1, zmm0, zmm4_pw_thre_deint);
    vpcmpgtw(k3, zmm0, zmm6_pw_thre_shift);
    vpmovm2w(zmm1, k1);
    vpmovm2w(zmm0, k3);
    vmovdqu8(zmm1 | k5, zmm0); //zmm0を上位バイトに

    vmovdqa32(zmm0, zword[ebx+64]);
    vpabsw(zmm2, zmm0);      //絶対値がcountの値
    vpmovw2m(k1, zmm0);       //符号を取り出し
    //vpsraw(zmm0, zmm0, 15);  //符号付きとしてシフトし、0xffffか0x0000を作る
    kxord(k1, k2, k1);
    //vpxord(zmm0, zmm2, zmm0);

    vpandd(zmm2, zmm2, zmm1);
    vmovdqu16(zmm2 | k1 | T_z, zmm2);

    vpsubsb(zmm0, zmm2, zmm1);
    //vpandd(zmm0, zmm4, zmm0);
    //vpsubsb(zmm0, zmm0, zmm1);

    vmovdqa32(zmm2, zmm0);
    vpxord(zmm1, zmm1, zmm1);
    vpsubw(zmm2 | k2, zmm1, zmm2); //フラグが負(0xffff)なら、カウントを負に
    //vpandnd(zmm4, zmm2, zmm0);  //フラグが負(0xffff)なら、カウントを0に
    //vpsignw(zmm2, zmm0, zmm2); //フラグが負(0xffff)なら、カウントを負に
    //vpord(zmm2, zmm2, zmm4);    //フラグが0だった場合の加算
    vmovdqa32(zword[ebx+64], zmm2);

    vpcmpgtb(k1, zmm0, zmm7_pb_thre_count);
    vmovdqu8(zmm0 | k1 | T_z, zword[pw_mask_12stripe_1_lsft4]);
    vpsrlw(zmm0, zmm0, 4);
    vpord(zmm3, zmm3, zmm0);
    ret();
}

//edx . . ih
//esi . . tmp
//edi . . mc_mask_ptr
//eax . . tmp
//ebx . . dst
//ecx . . buf2ptr
//ebp . . buf2ptr_fin
//zmm4 . . pb_mask_12stripe_01
//zmm5 . . pb_mask_1stripe_01
//zmm6 . . pw_thre_shift
//zmm7 . . pb_thre_count
void AFSAnalyzeXbyakAVX512::afs_analyze_loop2_2_internal(int stack_ptr_mc_mask_offset) {
    Xbyak::Zmm zmm4_pb_mask_12stripe_01(zmm4);
    Xbyak::Zmm zmm5_pb_mask_1stripe_01(zmm5);

    lea(eax, ptr[edx+5]); //edx-3
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vmovdqa32(zmm0, zword[ecx + eax]);

    lea(esi, ptr[edx+6]); //edx-2
    and(esi, 7);
    shl(esi, BLOCK_SIZE_YCP_LOG2);
    vpord(zmm0, zmm0, zword[ecx + esi]);

    lea(eax, ptr[edx+1]); //edx+1
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vpandd(zmm1, zmm5_pb_mask_1stripe_01, zword[ecx + eax]);

    lea(esi, ptr[edx+7]); //edx-1
    and(esi, 7);
    shl(esi, BLOCK_SIZE_YCP_LOG2);
    //vpord(zmm0, zmm0, zword[ecx + esi]);
    //vpandd(zmm0, zmm0, zmm4_pb_mask_12stripe_01);
    vpternlogd(zmm0, zmm4_pb_mask_12stripe_01, zword[ecx + esi], (TL_R0 | TL_R2) & TL_R1);

    lea(eax, ptr[edx+4]); //edx-4
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    //vpord(zmm1, zmm1, zword[ecx + eax]);
    //vpord(zmm0, zmm0, zmm1);
    vpternlogd(zmm0, zmm1, zword[ecx + eax], TL_R0 | TL_R1 | TL_R2);
    vmovdqu8(zword[ebx], zmm0);

    afs_analyze_count_motion(stack_ptr_mc_mask_offset);
}

//edx ... ih
//eax ... tmp
//esi ... tmp
//edi ... mc_mask_ptr
//ebx ... dst
//ecx ... buf2ptr
//ebp ... buf2ptr_fin
//zmm0 ... 対象
//zmm4 ... pb_mask_12stripe_01
//zmm5 ... pb_mask_1stripe_01
//zmm6 ... pw_thre_shift
//zmm7 ... pb_thre_count
void AFSAnalyzeXbyakAVX512::afs_analyze_count_motion(int stack_ptr_mc_mask_offset) {
    const int top = param.mc_scan_top;
    const int mc_scan_y_limit = (param.h - param.mc_scan_bottom - top) & ~1;
    //vpmovmskbは最上位ビットの取り出しであるから、最上位ビットさえ意識すればよい
    //ここで行いたいのは、「0x40ビットが立っていないこと」であるから、
    //左シフトで最上位ビットに移動させたのち、andnotで反転させながらmc_maskとの論理積をとればよい
    vpsllw(zmm0, zmm0, 1);
    vpandnd(zmm0, zmm0, zword[edi]);

    vpmovmskb(esi, ymm0);
    vextracti32x8(ymm0, zmm0, 1);
    vpmovmskb(eax, ymm0);
    popcnt(esi, esi);
    popcnt(eax, eax);
    add(esi, eax);

    mov(eax, edx);
    sub(eax, top+4); //ih - 4 - top
    cmp(eax, mc_scan_y_limit);
    mov(eax, 0); //フラグ変化なし
    cmovb(eax, esi); //((DW0RD)(y -top) < (DW0RD)y_limit) ? count(esi) : 0;
    movd(mm1, eax);
    paddd(mm0, mm1);
}

//ebp ... ih
//zmm4 ... pb_mask_12stripe_01
//zmm5 ... pb_mask_1stripe_01 -> zero
//zmm6 ... pw_thre_shift
//zmm7 ... pb_thre_count
//
//esi ... tmp
//edi ... mc_nask_ptr
//eax ... tmp
//ebx ... dst
//ecx ... buf2ptr
//edx ... buf2ptr_fin
void AFSAnalyzeXbyakAVX512::afs_analyze_loop3(int step6, int si_pitch,
    const Xbyak::Address& stack_ptr_dst,
    const Xbyak::Address& stack_ptr_width,
    const Xbyak::Address& stack_ptr_h_fin_l3,
    const int stack_ptr_tmp16pix_offset,
    const int stack_ptr_buffer_offset,
    const int stack_ptr_buffer2_offset,
    const int stack_ptr_mc_mask_offset) {
    vpxord(zmm5, zmm5, zmm5);
    vmovdqa32(zmm4, zword[pb_mask_12stripe_01]);

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
            add(ebx, 64);
            add(edi, 64);
            add(ecx, 64);
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
//zmm4 ... pb_mask_12stripe_01
//zmm5 ... zero
//zmm6 ... pw_thre_shi ft
//zmm7 ... pb_thre_count
void AFSAnalyzeXbyakAVX512::afs_analyze_loop3_internal(int stack_ptr_mc_mask_offset) {
    Xbyak::Zmm zmm4_pb_mask_12stripe_01(zmm4);
    Xbyak::Zmm zmm5_zero(zmm5);

    lea(eax, ptr[edx + 5]); //edx-3
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vmovdqa32(zmm0, zword[ecx + eax]);

    lea(esi, ptr[edx + 6]); //edx-2
    and(esi, 7);
    shl(esi, BLOCK_SIZE_YCP_LOG2);
    vpord(zmm0, zmm0, zword[ecx + esi]);

    lea(eax, ptr[edx + 7]); //edx-1
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    //vpord(zmm0, zmm0, zword[ecx + eax]);
    //vpandd(zmm0, zmm0, zmm4_pb_mask_12stripe_01);
    vpternlogd(zmm0, zmm4_pb_mask_12stripe_01, zword[ecx + eax], (TL_R0 | TL_R2) & TL_R1);

    lea(eax, ptr[edx +5]); //edx-4
    and(eax, 7);
    shl(eax, BLOCK_SIZE_YCP_LOG2);
    vpord(zmm0, zmm0, zword[ecx + eax]);

    vmovdqu8(zword[ebx], zmm0);
    mov(esi, edx); //edx
    and(esi, 7);
    shl(esi, BLOCK_SIZE_YCP_LOG2);
    vmovdqa32(zword[ecx + esi], zmm5_zero);

    afs_analyze_count_motion(stack_ptr_mc_mask_offset);
};
#endif //#if AFS_USE_XBYAK
