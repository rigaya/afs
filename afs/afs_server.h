#ifndef AFS_SERVER_H
#define AFS_SERVER_H

#define AFS_FLAG_SHIFT0      0x01
#define AFS_FLAG_SHIFT1      0x02
#define AFS_FLAG_SHIFT2      0x04
#define AFS_FLAG_SHIFT3      0x08
#define AFS_FLAG_FRAME_DROP  0x10
#define AFS_FLAG_SMOOTHING   0x20
#define AFS_FLAG_FORCE24     0x40
#define AFS_FLAG_ERROR       0x80
#define AFS_MASK_SHIFT0      0xfe
#define AFS_MASK_SHIFT1      0xfd
#define AFS_MASK_SHIFT2      0xfb
#define AFS_MASK_SHIFT3      0xf7
#define AFS_MASK_FRAME_DROP  0xef
#define AFS_MASK_SMOOTHING   0xdf
#define AFS_MASK_FORCE24     0xbf
#define AFS_MASK_ERROR       0x7f

#define AFS_STATUS_DEFAULT   0

#define AFS_SHARE_SIZE       0x0018
#define AFS_OFFSET_SHARE_N   0x0000
#define AFS_OFFSET_SHARE_ERR 0x0004
#define AFS_OFFSET_FRAME_N   0x0008
#define AFS_OFFSET_STARTFRM  0x000C
#define AFS_OFFSET_STATUSPTR 0x0010

static HANDLE AFS_hMemMap;
static unsigned char *AFS_MemView = NULL;
static int AFS_my_alloc = 0;
static int AFS_start_frame = 0;
#define afs_header(x) (*(int*)(AFS_MemView+(x)))
#define afs_headerp(x) (*(unsigned char**)(AFS_MemView+(x)))

static BOOL afs_check_share(void)
{
  static char MemFile[256] = "afs7_";
  DWORD pid, temp;
  int mem_exist, i;
  
  if(AFS_MemView != NULL)
    return TRUE;
  
  if(MemFile[5] == 0){
    pid = GetCurrentProcessId();
    for(i = 5; pid >= 10;){
      for(temp = 10; pid / 10 >= temp;) temp *= 10;
      MemFile[i++] = '0' + (char)(pid / temp);
      pid %= temp;
    }
    MemFile[i++] = '0' + (char)pid;
    MemFile[i] = 0;
  }
  
  AFS_hMemMap = CreateFileMapping((HANDLE)0xffffffff,NULL, PAGE_READWRITE, 0, AFS_SHARE_SIZE, MemFile);
  mem_exist = (GetLastError() == ERROR_ALREADY_EXISTS);
  if(AFS_hMemMap == NULL) return FALSE;
  AFS_MemView = (unsigned char*)MapViewOfFile(AFS_hMemMap, FILE_MAP_ALL_ACCESS, 0, 0, AFS_SHARE_SIZE);
  if(AFS_MemView == NULL){
    CloseHandle(AFS_hMemMap);
    return FALSE;
  }
  if(!mem_exist){
    afs_header(AFS_OFFSET_SHARE_N) = 0;
    afs_header(AFS_OFFSET_SHARE_ERR) = 0;
    afs_header(AFS_OFFSET_FRAME_N) = 0;
    afs_header(AFS_OFFSET_STARTFRM) = 0;
    afs_headerp(AFS_OFFSET_STATUSPTR) = NULL;
  }
  afs_header(AFS_OFFSET_SHARE_N)++;
  
  return TRUE;
}

static BOOL afs_alloc_cache(int frame_n)
{
  afs_headerp(AFS_OFFSET_STATUSPTR) =
    (unsigned char*)calloc(frame_n, sizeof(unsigned char));
  if(afs_headerp(AFS_OFFSET_STATUSPTR) == NULL) return FALSE;
  AFS_my_alloc = 1;
  afs_header(AFS_OFFSET_SHARE_ERR) = 0;
  afs_header(AFS_OFFSET_FRAME_N) = frame_n;
  afs_header(AFS_OFFSET_STARTFRM) = 0;
  
  return TRUE;
}

static void afs_free_cache(void)
{
  if(AFS_my_alloc){
    free(afs_headerp(AFS_OFFSET_STATUSPTR));
    AFS_my_alloc = 0;
    afs_header(AFS_OFFSET_FRAME_N) = 0;
    afs_header(AFS_OFFSET_STARTFRM) = 0;
    afs_headerp(AFS_OFFSET_STATUSPTR) = NULL;
  }
  afs_header(AFS_OFFSET_SHARE_ERR) = 0;
  return;
}

static void afs_release_share(void)
{
  afs_free_cache();
  if(AFS_MemView != NULL){
    afs_header(AFS_OFFSET_SHARE_N)--;
    if(afs_header(AFS_OFFSET_SHARE_N) == 0)
      if(afs_headerp(AFS_OFFSET_STATUSPTR) != NULL)
    UnmapViewOfFile(AFS_MemView);
    CloseHandle(AFS_hMemMap);
    AFS_MemView = NULL;
  }
  
  return;
}

static BOOL afs_is_ready(void)
{
  if(!afs_check_share()) return FALSE;
  if(afs_headerp(AFS_OFFSET_STATUSPTR) != NULL && !AFS_my_alloc){
    afs_header(AFS_OFFSET_SHARE_ERR) = 1;
    return FALSE;
  }
  return TRUE;
}

static void afs_set_start_frame(int frame)
{
  if(AFS_my_alloc) afs_header(AFS_OFFSET_STARTFRM) = frame;
  return;
}

static unsigned char afs_get_status(int frame_n, int frame)
{
  unsigned char *statusp;
  
  if(frame < 0 || frame >= frame_n)
    return AFS_STATUS_DEFAULT;
  
  if(AFS_MemView == NULL)
    if(!afs_check_share())
      return AFS_STATUS_DEFAULT;
  
  if(afs_headerp(AFS_OFFSET_STATUSPTR) != NULL && afs_header(AFS_OFFSET_FRAME_N) != frame_n)
    afs_free_cache();
  
  if(afs_headerp(AFS_OFFSET_STATUSPTR) == NULL)
    if(!afs_alloc_cache(frame_n))
      return AFS_STATUS_DEFAULT;
  statusp = afs_headerp(AFS_OFFSET_STATUSPTR);
  
  return statusp[frame];
}

static void afs_set(int frame_n, int frame, unsigned char status)
{
  unsigned char *statusp;
  
  if(frame < 0 || frame >= frame_n)
    return;
  
  if(AFS_MemView == NULL)
    if(!afs_check_share())
      return;
  
  if(afs_headerp(AFS_OFFSET_STATUSPTR) != NULL && afs_header(AFS_OFFSET_FRAME_N) != frame_n)
    afs_free_cache();
  
  if(afs_headerp(AFS_OFFSET_STATUSPTR) == NULL)
    if(!afs_alloc_cache(frame_n))
      return;
  statusp = afs_headerp(AFS_OFFSET_STATUSPTR);
  
  statusp[frame] = status;
  return;
}

#endif /* AFS_SERVER_H */
