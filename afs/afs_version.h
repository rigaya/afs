#pragma once
#ifndef __AFS_VERSION_H__
#define __AFS_VERSION_H__

#ifdef DEBUG
#define VER_DEBUG   VS_FF_DEBUG
#define VER_PRIVATE VS_FF_PRIVATEBUILD
#else
#define VER_DEBUG   0
#define VER_PRIVATE 0
#endif

#define VER_FILEVERSION              7,5,22,0
#define VER_STR_FILEVERSION          "7.5a+22"

#define AFS_FILENAME "自動フィールドシフト 高速化版"

#define VER_STR_COMMENTS         ""
#define VER_STR_COMPANYNAME      ""
#define VER_STR_FILEDESCRIPTION  AFS_FILENAME
#define VER_STR_INTERNALNAME     AFS_FILENAME
#define VER_STR_ORIGINALFILENAME "afs.auf"
#define VER_STR_LEGALCOPYRIGHT   "afs.auf by rigaya"
#define VER_STR_PRODUCTNAME      AFS_FILENAME
#define VER_PRODUCTVERSION       VER_FILEVERSION
#define VER_STR_PRODUCTVERSION   VER_STR_FILEVERSION

#endif //__AFS_VERSION_H__
