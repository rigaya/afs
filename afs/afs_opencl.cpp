#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Shlwapi.h>
#include <vector>
#include <string>
#include <algorithm>
#include "afs.h"
#pragma comment(lib, "opencl.lib")
#pragma comment(lib, "shlwapi.lib")

#include "afs_opencl.cl"

static const char *BUILD_ERR_FILE = "afs_opencl_build_error.txt";

using std::vector;

#define ICEILDIV(x, div) (((x) + (div) - 1) / (div))
#define ICEIL(x, div) (ICEILDIV((x), (div)) * (div))

static inline int get_gcd(int a, int b) {
    int c;
    while ((c = a % b) != 0)
        a = b, b = c;
    return b;
}

static inline const char *strichr(const char *str, int c) {
    c = tolower(c);
    for (; *str; str++)
        if (c == tolower(*str))
            return str;
    return nullptr;
}

static inline const char *stristr(const char *str, const char *substr) {
    size_t len = 0;
    if (substr && (len = strlen(substr)) != 0)
        for (; (str = strichr(str, substr[0])) != nullptr; str++)
            if (_strnicmp(str, substr, len) == 0)
                return str;
    return nullptr;
}

void afs_opencl_release_buffer(AFS_CONTEXT *afs) {
    if (!afs->opencl.ctx) {
        return;
    }
    for (int i = 0; i < _countof(afs->opencl.source_buf); i++) {
        //afs_opencl_source_buffer_unmap()を呼ぶとnullにされるので、その前に取り出しておく
        void *ptr = afs->source_array[i].map;
        afs_opencl_source_buffer_unmap(afs, i, true);
        for (int j = 0; j < 2; j++) {
            if (afs->opencl.source_img[i][j]) {
                clReleaseMemObject(afs->opencl.source_img[i][j]);
            }
            if (afs->opencl.source_buf[i][j]) {
                clReleaseMemObject(afs->opencl.source_buf[i][j]);
            }
        }
        //取り出しておいたポインタを戻す
        //これはafs.cppのfree_source_cache()のほうで解放される
        afs->source_array[i].map = ptr;
    }
    for (int i = 0; i < _countof(afs->opencl.scan_mem); i++) {
        //afs_opencl_scan_buffer_unmap()を呼ぶとnullにされるので、その前に取り出しておく
        unsigned char *ptr = afs->scan_array[i].map;
        afs_opencl_scan_buffer_unmap(afs, i, true);
        if (afs->opencl.scan_mem[i]) {
            clReleaseMemObject(afs->opencl.scan_mem[i]);
        }
        afs->scan_array[i].status = 0;
        //取り出しておいたポインタを戻す
        //これはafs.cppのfree_analyze_cache()のほうで解放される
        afs->scan_array[i].map = ptr;
    }
    for (int i = 0; i < _countof(afs->opencl.motion_count_temp); i++) {
        //afs_opencl_count_motion_temp_unmap()を呼ぶとnullにされるので、その前に取り出しておく
        void *ptr = afs->opencl.motion_count_temp_map[i];
        afs_opencl_count_motion_temp_unmap(afs, i, true);
        if (afs->opencl.motion_count_temp[i]) {
            clReleaseMemObject(afs->opencl.motion_count_temp[i]);
        }
        //SVM Modeなら、clSVMFree()による開放も必要
        if (afs->afs_mode & AFS_MODE_OPENCL_SVMF) {
            clSVMFree(afs->opencl.ctx, ptr);
        }
    }
    for (int i = 0; i < _countof(afs->opencl.stripe_mem); i++) {
        //afs_opencl_stripe_buffer_unmap()を呼ぶとnullにされるので、その前に取り出しておく
        unsigned char *ptr = afs->stripe_array[i].map;
        afs_opencl_stripe_buffer_unmap(afs, i, true);
        if (afs->opencl.stripe_mem[i]) {
            clReleaseMemObject(afs->opencl.stripe_mem[i]);
        }
        afs->stripe_array[i].status = 0;
        //取り出しておいたポインタを戻す
        //これはafs.cppのfree_analyze_cache()のほうで解放される
        afs->stripe_array[i].map = ptr;
    }
    clFinish(afs->opencl.queue);
    memset(afs->opencl.source_img, 0, sizeof(afs->opencl.source_img));
    memset(afs->opencl.source_buf, 0, sizeof(afs->opencl.source_buf));
    memset(afs->opencl.scan_mem,   0, sizeof(afs->opencl.scan_mem));
    memset(afs->opencl.stripe_mem, 0, sizeof(afs->opencl.stripe_mem));
    memset(afs->opencl.motion_count_temp,     0, sizeof(afs->opencl.motion_count_temp));
    memset(afs->opencl.motion_count_temp_map, 0, sizeof(afs->opencl.motion_count_temp_map));
}

void afs_opencl_close(AFS_CONTEXT *afs) {
    afs_opencl_release_buffer(afs);
    for (int i = 0; i < _countof(afs->opencl.program_analyze); i++) {
        if (afs->opencl.kernel_analyze[i]) clReleaseKernel(afs->opencl.kernel_analyze[i]);
        if (afs->opencl.program_analyze[i]) clReleaseProgram(afs->opencl.program_analyze[i]);
    }
    if (afs->opencl.queue) clReleaseCommandQueue(afs->opencl.queue);
    if (afs->opencl.ctx) clReleaseContext(afs->opencl.ctx);
    int device_check = afs->opencl.device_check;
    memset(&afs->opencl, 0, sizeof(afs->opencl));
    afs->opencl.device_check = device_check;
}

cl_int afs_opencl_check_dll() {
    HMODULE hdll = LoadLibrary("opencl.dll");
    if (!hdll) {
        return CL_INVALID_VALUE;
    }
    FreeLibrary(hdll);
    return CL_SUCCESS;
}

static bool afs_opencl_check_vendor(const char *str, const char *vendor_name) {
    if (nullptr != stristr(str, vendor_name))
        return true;
    return false;
}

static cl_int afs_opencl_get_device(const char *vendor_name, cl_int device_type, AFS_OPENCL *cl_data, int major = 0, int minor = 0) {
    cl_uint size = 0;
    cl_int ret = CL_SUCCESS;

    if (CL_SUCCESS != (ret = clGetPlatformIDs(0, nullptr, &size))) {
        return ret;
    }

    vector<cl_platform_id> platform_list(size);
    if (CL_SUCCESS != (ret = clGetPlatformIDs(size, &platform_list[0], &size))) {
        return ret;
    }

    auto checkPlatformForVendor = [vendor_name](cl_platform_id platform_id) {
        char buf[1024] = { 0 };
        return (CL_SUCCESS == clGetPlatformInfo(platform_id, CL_PLATFORM_VENDOR, _countof(buf), buf, nullptr)
            && afs_opencl_check_vendor(buf, vendor_name));
    };

    for (auto platform : platform_list) {
        if (checkPlatformForVendor(platform)) {
            if (CL_SUCCESS != (ret = clGetDeviceIDs(platform, device_type, 0, nullptr, &size))) {
                continue;
            }
            if (size == 0) {
                continue;
            }
            vector<cl_device_id> device_list(size);
            if (CL_SUCCESS != (ret = clGetDeviceIDs(platform, device_type, size, &device_list[0], &size))) {
                return ret;
            }
            for (auto device : device_list) {
                //cl_device_svm_capabilities svm_cap = { 0 };
                //clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES, sizeof(svm_cap), &svm_cap, nullptr);
                //std::string str;
                //if (svm_cap & CL_DEVICE_SVM_COARSE_GRAIN_BUFFER) str += "CL_DEVICE_SVM_COARSE_GRAIN_BUFFER ";
                //if (svm_cap & CL_DEVICE_SVM_FINE_GRAIN_BUFFER)   str += "CL_DEVICE_SVM_FINE_GRAIN_BUFFER ";
                //if (svm_cap & CL_DEVICE_SVM_FINE_GRAIN_SYSTEM)   str += "CL_DEVICE_SVM_FINE_GRAIN_SYSTEM ";
                //if (svm_cap & CL_DEVICE_SVM_ATOMICS)             str += "CL_DEVICE_SVM_ATOMICS ";
                char buf[1024] = { 0 };
                if (CL_SUCCESS == (ret = clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(buf), buf, nullptr))) {
                    int dev_major, dev_minor;
                    if (2 == sscanf_s(buf, "OpenCL %d.%d", &dev_major, &dev_minor)
                        && ((dev_major > major) || (dev_major == major && dev_minor >= minor))) {
                        cl_data->platform = platform;
                        cl_data->device = device_list[0];
                        if (dev_major >= 2) {
                            cl_device_svm_capabilities svm_cap = { 0 };
                            clGetDeviceInfo(device, CL_DEVICE_SVM_CAPABILITIES, sizeof(svm_cap), &svm_cap, nullptr);
                            cl_data->bSVMAvail = (svm_cap & CL_DEVICE_SVM_FINE_GRAIN_BUFFER) != 0;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    return (ret != CL_SUCCESS) ? ret : ((cl_data->platform != nullptr) ? 0 : -1);
}

static cl_int afs_opencl_create_kernel(AFS_OPENCL *cl_data, cl_program& program, cl_kernel& kernel, const char *source, size_t sourceSize, const char *buildOptions, const char *kernelName) {
    cl_int ret = CL_SUCCESS;
    program = clCreateProgramWithSource(cl_data->ctx, 1, &source, &sourceSize, &ret);
    if (CL_SUCCESS != ret)
        return ret;

    if (CL_SUCCESS != (ret = clBuildProgram(program, 1, &cl_data->device, buildOptions, nullptr, nullptr))) {
        std::vector<char> buffer(16 * 1024, '\0');
        size_t length = 0;
        clGetProgramBuildInfo(program, cl_data->device, CL_PROGRAM_BUILD_LOG, buffer.size(), buffer.data(), &length);
        FILE *fp = nullptr;
        if (!fopen_s(&fp, BUILD_ERR_FILE, "a")) {
            fprintf(fp, "%s\n", buffer.data());
            fclose(fp);
        }
        return ret;
    }
    kernel = clCreateKernel(program, kernelName, &ret);
    if (CL_SUCCESS != ret) {
        return ret;
    }
    return ret;
}

static cl_int afs_opencl_create_kernel(AFS_OPENCL *cl_data) {
    cl_int ret = CL_SUCCESS;
    if (cl_data->ctx) {
        return ret;
    }
    cl_data->ctx = clCreateContext(0, 1, &cl_data->device, nullptr, nullptr, &ret);
    if (CL_SUCCESS != ret)
        return ret;
#if 0
    cl_queue_properties qprop[] = {
        CL_QUEUE_PROPERTIES, (cl_command_queue_properties)CL_QUEUE_PROFILING_ENABLE | CL_QUEUE_ON_DEVICE | CL_QUEUE_ON_DEVICE_DEFAULT, 0
    };
    cl_data->queue = clCreateCommandQueueWithProperties(cl_data->ctx, cl_data->device, qprop, &ret);
#else
    cl_data->queue = clCreateCommandQueueWithProperties(cl_data->ctx, cl_data->device, nullptr, &ret);
#endif
    if (CL_SUCCESS != ret)
        return ret;

    if (PathFileExists(BUILD_ERR_FILE)) {
        DeleteFile(BUILD_ERR_FILE);
    }
    std::string sBuildOptionBase = "";
    if (cl_data->bSVMAvail) {
        sBuildOptionBase += " -cl-std=CL2.0";
    }
    //OpenCLのカーネル用のコードはリソース埋め込みにしているので、それを呼び出し
    HRSRC hResource = nullptr;
    HGLOBAL hResourceData = nullptr;
    const char *clSourceFile = nullptr;
    size_t resourceSize = 0;
    if (   nullptr == (hResource = FindResource(cl_data->hModuleDLL, "CLDATA", "KERNEL_DATA"))
        || nullptr == (hResourceData = LoadResource(cl_data->hModuleDLL, hResource))
        || nullptr == (clSourceFile = (const char *)LockResource(hResourceData))
        || 0       == (resourceSize = SizeofResource(cl_data->hModuleDLL, hResource))) {
        return 1;
    }
    for (int i = 0; i < _countof(cl_data->program_analyze); i++) {
        std::string sBuildOptions = sBuildOptionBase + " -D PREFER_SHORT4=1";
        sBuildOptions += (i) ? " -D TB_ORDER=1" : " -D TB_ORDER=0";
        ret = afs_opencl_create_kernel(cl_data, cl_data->program_analyze[i], cl_data->kernel_analyze[i], clSourceFile, resourceSize, sBuildOptions.c_str(), "afs_analyze_12_nv16_kernel");
        if (CL_SUCCESS != ret)
            return ret;
    }
    return ret;
}

int afs_opencl_open_device(AFS_CONTEXT *afs, HMODULE hModuleDLL) {
#if ENABLE_OPENCL
    if (afs->opencl.device_check == AFS_OPENCL_DEVICE_CHECK_FAIL) {
        return 1;
    }
    afs_opencl_close(afs);
    cl_int ret = CL_SUCCESS;
    if (   CL_SUCCESS != (ret = afs_opencl_check_dll())
        || CL_SUCCESS != (ret = afs_opencl_get_device("Intel", CL_DEVICE_TYPE_GPU, &afs->opencl, 1, 2))) {
        afs->opencl.device_check = AFS_OPENCL_DEVICE_CHECK_FAIL;
        afs_opencl_close(afs);
        return 1;
    }
    if (hModuleDLL) {
        afs->opencl.hModuleDLL = hModuleDLL;
    }
    return 0;
#else
    return 1;
#endif
}

int afs_opencl_init(AFS_CONTEXT *afs) {
    cl_int ret = CL_SUCCESS;
    if (afs->opencl.device == nullptr) {
        if (CL_SUCCESS != (ret = afs_opencl_open_device(afs, NULL))) {
            return 1;
        }
    }
    if (CL_SUCCESS != (ret = afs_opencl_create_kernel(&afs->opencl))) {
        afs_opencl_close(afs);
        return 1;
    }
    return 0;
}

cl_int afs_opencl_queue_finish(AFS_CONTEXT *afs) {
    return clFinish(afs->opencl.queue);
}

cl_int afs_opencl_source_buffer_map(AFS_CONTEXT *afs, int i) {
    cl_int ret = CL_SUCCESS;
    if (afs->source_array[i].map != nullptr) {
        return ret;
    }
#if PREFER_IMAGE
    size_t origin[3] = { 0, 0, 0 };
    size_t region[3] = { (size_t)afs->opencl.source_w, (size_t)afs->opencl.source_h, 1 };
    size_t image_row_pitch = 0;
    size_t image_slice_picth = 0;
    afs->source_array[i].map = clEnqueueMapImage(afs->opencl.queue, afs->opencl.source_img[i][0], CL_FALSE, CL_MAP_WRITE | CL_MAP_READ,
        origin, region, &image_row_pitch, &image_slice_picth, 0, NULL, NULL, &ret);
    if (ret != CL_SUCCESS) {
        return ret;
    }
    clEnqueueMapImage(afs->opencl.queue, afs->opencl.source_img[i][1], CL_FALSE, CL_MAP_WRITE | CL_MAP_READ,
        origin, region, &image_row_pitch, &image_slice_picth, 0, NULL, NULL, &ret);
    afs->source_w = image_row_pitch;
#else
    afs->source_array[i].map = clEnqueueMapBuffer(afs->opencl.queue, afs->opencl.source_buf[i][0], CL_FALSE, CL_MAP_WRITE | CL_MAP_READ,
        0, afs->opencl.source_w * afs->opencl.source_h * 2, 0, NULL, NULL, &ret);
    if (ret != CL_SUCCESS) {
        return ret;
    }
    afs->source_w = afs->opencl.source_w;
#endif
    return ret;
}

cl_int afs_opencl_source_buffer_unmap(AFS_CONTEXT *afs, int i, bool force) {
    if (afs->source_array[i].map == nullptr) {
        return CL_SUCCESS;
    }
    if (!force && (afs->afs_mode & AFS_MODE_OPENCL_SVMF)) {
        return CL_SUCCESS;
    }
    char *ptrY = (char *)(afs->source_array[i].map);
#if PREFER_IMAGE
    char *ptrC = ptrY + afs->source_w * afs->source_h;
    cl_int ret = clEnqueueUnmapMemObject(afs->opencl.queue, afs->opencl.source_img[i][0], ptrY, 0, NULL, NULL);
    if (ret == CL_SUCCESS) {
        afs->source_array[i].map = nullptr;
    }
    ret = clEnqueueUnmapMemObject(afs->opencl.queue, afs->opencl.source_img[i][1], ptrC, 0, NULL, NULL);
#else
    cl_int ret = clEnqueueUnmapMemObject(afs->opencl.queue, afs->opencl.source_buf[i][0], ptrY, 0, NULL, NULL);
    if (ret == CL_SUCCESS) {
        afs->source_array[i].map = nullptr;
    }
#endif
    return ret;
}

int afs_opencl_source_buffer_index(AFS_CONTEXT *afs, void *p0) {
    for (int i = 0; i < _countof(afs->source_array); i++) {
        if (afs->source_array[i].map == p0) {
            return i;
        }
    }
    return -1;
}

int afs_opencl_source_buffer_pitch(AFS_CONTEXT *afs, int w, int h, int *pitch, int *baseAddressAlign) {
    const int widthint32 = (w + 3) >> 2;
    int image2dAlign = 0;
    cl_int ret = clGetDeviceInfo(afs->opencl.device, CL_DEVICE_IMAGE_PITCH_ALIGNMENT, sizeof(cl_uint), &image2dAlign, nullptr);
    if (ret != CL_SUCCESS) {
        return 1;
    }
    image2dAlign = std::max(image2dAlign, 64);

    int baseAlign;
    ret = clGetDeviceInfo(afs->opencl.device, CL_DEVICE_IMAGE_BASE_ADDRESS_ALIGNMENT, sizeof(cl_uint), &baseAlign, nullptr);
    baseAlign = std::max(baseAlign, 4096);
    int gcd = get_gcd(h, baseAlign);

    image2dAlign = std::max(image2dAlign, baseAlign / gcd);

    *pitch = ICEIL(widthint32 * 4, image2dAlign);
    *baseAddressAlign = baseAlign;

    return 0;
}

int afs_opencl_create_source_buffer(AFS_CONTEXT *afs, int w, int h) {
    if (!afs->opencl.ctx) {
        return 1;
    }
#if PREFER_IMAGE
    cl_image_format format;
    format.image_channel_order = CL_RGBA;
    format.image_channel_data_type = CL_UNSIGNED_INT8;
    const int width_uchar4 = (w + 3) >> 2;
    cl_image_desc img_desc;
    img_desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    img_desc.image_width = width_uchar4;
    img_desc.image_height = h;
    img_desc.image_depth = 0;
    img_desc.image_array_size = 0;
    img_desc.image_row_pitch = afs->source_w;
    img_desc.image_slice_pitch = 0;
    img_desc.num_mip_levels = 0;
    img_desc.num_samples = 0;
    img_desc.buffer = 0;
    img_desc.mem_object = 0;
    afs->opencl.source_w = width_uchar4;
    afs->opencl.source_h = h;

    const int imageSizeBytes = afs->source_w * h;
#else
    afs->opencl.source_w = w;
    afs->opencl.source_h = h;
    const int imageSizeBytes = w * h * 2;
#endif

    for (int i = 0; i < _countof(afs->source_array); i++) {
        cl_int ret = CL_SUCCESS;
#if PREFER_IMAGE
        char *ptrY = (char *)(afs->source_array[i].map);
        char *ptrC = ptrY + img_desc.image_row_pitch * h;
        char *ptr[2] = { ptrY, ptrC };
        for (int j = 0; j < 2; j++) {
            //一度CL_MEM_USE_HOST_PTRでバッファを作ってから、cl_image_descのmem_objectに指定することで、ZeroCopyなimage2dが可能になる
            //clCreateImageのhost_ptrに直接ポインタを渡し、CL_MEM_USE_HOST_PTRを指定してもZeroCopyにはならないことに注意
            afs->opencl.source_buf[i][j] = clCreateBuffer(afs->opencl.ctx, CL_MEM_USE_HOST_PTR, imageSizeBytes, ptr[j], &ret);
            if (ret != CL_SUCCESS) {
                return 1;
            }
            img_desc.mem_object = afs->opencl.source_buf[i][j];
            afs->opencl.source_img[i][j] = clCreateImage(afs->opencl.ctx, CL_MEM_READ_ONLY, &format, &img_desc, nullptr, &ret);
            if (ret != CL_SUCCESS) {
                return 1;
            }
        }
#else
        afs->opencl.source_buf[i][0] = clCreateBuffer(afs->opencl.ctx, CL_MEM_USE_HOST_PTR, imageSizeBytes, afs->source_array[i].map, &ret);
        if (ret != CL_SUCCESS) {
            return 1;
        }
#endif
        //afs->source_array[i].mapはafs_opencl_source_buffer_map()で再取得するため、ここではnullptrにしておく
        afs->source_array[i].map = nullptr;
        afs_opencl_source_buffer_map(afs, i);
        afs->source_array[i].status = 0;
    }
    //キューの完了を待つ: 重要!!
    clFinish(afs->opencl.queue);
    return 0;
}

cl_int afs_opencl_count_motion_temp_map(AFS_CONTEXT *afs, int i) {
    cl_int ret = CL_SUCCESS;
    if (afs->opencl.motion_count_temp_map[i] != nullptr) {
        return ret;
    }
    afs->opencl.motion_count_temp_map[i] = (unsigned short *)clEnqueueMapBuffer(afs->opencl.queue, afs->opencl.motion_count_temp[i], CL_FALSE, CL_MAP_WRITE | CL_MAP_READ,
        0, afs->opencl.motion_count_temp_max, 0, NULL, NULL, &ret);
    return ret;
}

cl_int afs_opencl_count_motion_temp_unmap(AFS_CONTEXT *afs, int i, bool force) {
    if (afs->opencl.motion_count_temp_map[i] == nullptr) {
        return CL_SUCCESS;
    }
    if (!force && (afs->afs_mode & AFS_MODE_OPENCL_SVMF)) {
        return CL_SUCCESS;
    }
    cl_int ret = clEnqueueUnmapMemObject(afs->opencl.queue, afs->opencl.motion_count_temp[i], afs->opencl.motion_count_temp_map[i], 0, NULL, NULL);
    if (ret == CL_SUCCESS) {
        afs->opencl.motion_count_temp_map[i] = nullptr;
    }
    return ret;
}

cl_int afs_opencl_scan_buffer_map(AFS_CONTEXT *afs, int i) {
    cl_int ret = CL_SUCCESS;
    if (afs->scan_array[i].map != nullptr) {
        return ret;
    }
    const int size = (afs->opencl.scan_w * afs->opencl.scan_h + 63) & ~63;
    afs->scan_array[i].map = (unsigned char *)clEnqueueMapBuffer(afs->opencl.queue, afs->opencl.scan_mem[i], CL_FALSE, CL_MAP_WRITE | CL_MAP_READ,
        0, size, 0, NULL, NULL, &ret);
    return ret;
}

cl_int afs_opencl_scan_buffer_unmap(AFS_CONTEXT *afs, int i, bool force) {
    if (afs->scan_array[i].map == nullptr) {
        return CL_SUCCESS;
    }
    if (!force && (afs->afs_mode & AFS_MODE_OPENCL_SVMF)) {
        return CL_SUCCESS;
    }
    cl_int ret = clEnqueueUnmapMemObject(afs->opencl.queue, afs->opencl.scan_mem[i], afs->scan_array[i].map, 0, NULL, NULL);
    if (ret == CL_SUCCESS) {
        afs->scan_array[i].map = nullptr;
    }
    return ret;
}

cl_int afs_opencl_stripe_buffer_map(AFS_CONTEXT *afs, int i) {
    cl_int ret = CL_SUCCESS;
    if (afs->stripe_array[i].map != nullptr) {
        return ret;
    }
    const int size = (afs->opencl.scan_w * afs->opencl.scan_h + 63) & ~63;
    afs->stripe_array[i].map = (unsigned char *)clEnqueueMapBuffer(afs->opencl.queue, afs->opencl.stripe_mem[i], CL_FALSE, CL_MAP_WRITE | CL_MAP_READ,
        0, size, 0, NULL, NULL, &ret);
    return ret;
}

cl_int afs_opencl_stripe_buffer_unmap(AFS_CONTEXT *afs, int i, bool force) {
    if (afs->stripe_array[i].map == nullptr) {
        return CL_SUCCESS;
    }
    if (!force && (afs->afs_mode & AFS_MODE_OPENCL_SVMF)) {
        return CL_SUCCESS;
    }
    cl_int ret = clEnqueueUnmapMemObject(afs->opencl.queue, afs->opencl.stripe_mem[i], afs->stripe_array[i].map, 0, NULL, NULL);
    if (ret == CL_SUCCESS) {
        afs->stripe_array[i].map = nullptr;
    }
    return ret;
}

int afs_opencl_create_scan_buffer(AFS_CONTEXT *afs, int si_w, int h) {
    const int size = (si_w * h + 63) & ~63;
    afs->opencl.scan_w = si_w;
    afs->opencl.scan_h = h;
    const int global_block_count = ICEILDIV(ICEILDIV((size_t)si_w, 4), BLOCK_INT_X) * ICEILDIV(ICEILDIV((size_t)h, BLOCK_LOOP_Y), BLOCK_Y);
    afs->opencl.motion_count_temp_max = (global_block_count + 63) & ~63;

    for (int i = 0; i < _countof(afs->scan_array); i++) {
        cl_int ret = CL_SUCCESS;
        afs->opencl.scan_mem[i] = clCreateBuffer(afs->opencl.ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, size, afs->scan_array[i].map, &ret);
        if (ret != CL_SUCCESS) {
            return 1;
        }
        //afs->scan_array[i].mapはafs_opencl_scan_buffer_map()で再取得するため、ここではnullptrにしておく
        afs->scan_array[i].map = nullptr;
        afs_opencl_scan_buffer_map(afs, i);
        afs->scan_array[i].status = 0;

        //motion_count用のバッファも作成
        if (afs->afs_mode & AFS_MODE_OPENCL_SVMF) {
            afs->opencl.motion_count_temp_map[i] = (uint16_t *)clSVMAlloc(afs->opencl.ctx, CL_MEM_WRITE_ONLY | CL_MEM_SVM_FINE_GRAIN_BUFFER, afs->opencl.motion_count_temp_max, 0);
            if (afs->opencl.motion_count_temp_map[i] == nullptr) {
                return 1;
            }
            afs->opencl.motion_count_temp[i] = clCreateBuffer(afs->opencl.ctx, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, afs->opencl.motion_count_temp_max, afs->opencl.motion_count_temp_map[i], &ret);
            //afs->opencl.motion_count_temp_mapはafs_opencl_count_motion_temp_map()で再取得するため、ここではnullptrにしておく
            afs->opencl.motion_count_temp_map[i] = nullptr;
        } else {
            afs->opencl.motion_count_temp[i] = clCreateBuffer(afs->opencl.ctx, CL_MEM_WRITE_ONLY | CL_MEM_ALLOC_HOST_PTR, afs->opencl.motion_count_temp_max, nullptr, &ret);
        }
        afs_opencl_count_motion_temp_map(afs, i);
    }
    for (int i = 0; i < _countof(afs->stripe_array); i++) {
        cl_int ret = CL_SUCCESS;
        afs->opencl.stripe_mem[i] = clCreateBuffer(afs->opencl.ctx, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, size, afs->stripe_array[i].map, &ret);
        if (ret != CL_SUCCESS) {
            return 1;
        }
        //afs->scan_array[i].mapはafs_opencl_stripe_buffer_map()で再取得するため、ここではnullptrにしておく
        afs->stripe_array[i].map = nullptr;
        afs_opencl_stripe_buffer_map(afs, i);
        afs->stripe_array[i].status = 0;
    }
    //キューの完了を待つ: 重要!!
    clFinish(afs->opencl.queue);
    return 0;
}

int afs_opencl_scan_buffer_index(AFS_CONTEXT *afs, void *p0) {
    for (int i = 0; i < _countof(afs->scan_array); i++) {
        if (afs->scan_array[i].map == p0) {
            return i;
        }
    }
    return -1;
}

int afs_opencl_analyze_12_nv16(AFS_CONTEXT *afs, int dst_idx, int p0_idx, int p1_idx, int tb_order, int width, int si_pitch, int h, int max_h,
    int thre_shift, int thre_deint, int thre_Ymotion, int thre_Cmotion, const void *_scan_clip, int *global_block_count) {
#define CLAMP(x, low, high) (((x) > (high))? (high) : ((x) < (low))? (low) : (x))
    uint8_t thre_shift_yuy2   = CLAMP((thre_shift  *219 + 383)>>12, 0, 127);
    uint8_t thre_deint_yuy2   = CLAMP((thre_deint  *219 + 383)>>12, 0, 127);
    uint8_t thre_Ymotion_yuy2 = CLAMP((thre_Ymotion*219 + 383)>>12, 0, 127);
    uint8_t thre_Cmotion_yuy2 = CLAMP((thre_Cmotion*  7 +  66)>> 7, 0, 127);
#undef CLAMP
    AFS_SCAN_CLIP *scan_clip = (AFS_SCAN_CLIP *)_scan_clip;
    uint32_t scan_left   = scan_clip->left;
    uint32_t scan_width  = width - scan_clip->left - scan_clip->right;
    uint32_t scan_top    = scan_clip->top;
    uint32_t scan_height = h - scan_clip->top - scan_clip->bottom;

/*
__global int *restrict ptr_dst,
__global int *restrict ptr_count,
__read_only image2d_t img_p0y,
__read_only image2d_t img_p0c,
__read_only image2d_t img_p1y,
__read_only image2d_t img_p1c,
int tb_order, int width_int, int si_pitch_int, int h,
uchar thre_Ymotion, uchar thre_Cmotion, uchar thre_deint, uchar thre_shift,
uint scan_left, uint scan_width, uint scan_top, uint scan_height)
*/
    const int width_int = width / sizeof(int);
    const int si_pitch_int = afs->opencl.scan_w / sizeof(int);
    const int source_w_int = afs->source_w / sizeof(int);
    const int frame_size_int = source_w_int * afs->opencl.source_h;
    // Set kernel arguments
    cl_int ret = CL_SUCCESS;
    cl_kernel kernel_analyze = afs->opencl.kernel_analyze[tb_order != 0];
    if (   CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  0, sizeof(cl_mem),   &afs->opencl.scan_mem[dst_idx]))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  1, sizeof(cl_mem),   &afs->opencl.motion_count_temp[dst_idx]))
#if PREFER_IMAGE
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  2, sizeof(cl_mem),   &afs->opencl.source_img[p0_idx][0]))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  3, sizeof(cl_mem),   &afs->opencl.source_img[p0_idx][1]))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  4, sizeof(cl_mem),   &afs->opencl.source_img[p1_idx][0]))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  5, sizeof(cl_mem),   &afs->opencl.source_img[p1_idx][1]))
#else
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  2, sizeof(cl_mem),   &afs->opencl.source_buf[p0_idx][0]))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  3, sizeof(cl_mem),   &afs->opencl.source_buf[p1_idx][0]))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  4, sizeof(int),      &source_w_int))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  5, sizeof(int),      &frame_size_int))
#endif
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  6, sizeof(int),      &width_int))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  7, sizeof(int),      &si_pitch_int))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  8, sizeof(int),      &h))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze,  9, sizeof(uint8_t),  &thre_Ymotion_yuy2))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze, 10, sizeof(uint8_t),  &thre_Cmotion_yuy2))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze, 11, sizeof(uint8_t),  &thre_deint_yuy2))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze, 12, sizeof(uint8_t),  &thre_shift_yuy2))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze, 13, sizeof(uint32_t), &scan_left))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze, 14, sizeof(uint32_t), &scan_width))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze, 15, sizeof(uint32_t), &scan_top))
        || CL_SUCCESS != (ret = clSetKernelArg(kernel_analyze, 16, sizeof(uint32_t), &scan_height))) {
        return 1;
    }
    size_t global[3] = { ICEIL(ICEILDIV((size_t)width, 4), BLOCK_INT_X), ICEIL(ICEILDIV((size_t)h, BLOCK_LOOP_Y), BLOCK_Y), 1 };
    size_t local[3]  = { BLOCK_INT_X, BLOCK_Y, 1 };
    size_t local_size_max;
    if (CL_SUCCESS != (ret = clGetKernelWorkGroupInfo(kernel_analyze, afs->opencl.device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), (void *)&local_size_max, NULL))) {
        return 1;
    }

    if (CL_SUCCESS != (ret = clEnqueueNDRangeKernel(afs->opencl.queue, kernel_analyze, 2, NULL, global, local, 0, NULL, NULL))) {
        return 1;
    }
    *global_block_count = ICEILDIV(ICEILDIV((size_t)width, 4), BLOCK_INT_X) * ICEILDIV(ICEILDIV((size_t)h, BLOCK_LOOP_Y), BLOCK_Y);
    return 0;
}
