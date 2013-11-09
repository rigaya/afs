// 削除リスト インポート・エクスポートプラグイン
#include <windows.h>
#include <stdio.h>
#include "filter.h"

#define FILE_TXT "TextFile (*.txt)\0*.txt\0AllFile (*.*)\0*.*\0"

FILTER_DLL filter = {
  FILTER_FLAG_IMPORT|FILTER_FLAG_EXPORT|FILTER_FLAG_NO_CONFIG|FILTER_FLAG_ALWAYS_ACTIVE|FILTER_FLAG_PRIORITY_LOWEST|FILTER_FLAG_EX_INFORMATION,
  0,0,
  "削除リスト",
  NULL,NULL,NULL,
  NULL,NULL,
  NULL,NULL,NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  func_WndProc,
  NULL,NULL,
  NULL,
  NULL,
  "削除リスト インポート・エクスポート ver.1.3 by Aji",
  NULL,
  NULL,
};

EXTERN_C FILTER_DLL __declspec(dllexport) * __stdcall GetFilterTable(void)
{
  return &filter;
}

BOOL func_WndProc(HWND, UINT message, WPARAM, LPARAM, void *editp, FILTER *fp)
{
  TCHAR buf[MAX_PATH];
  FILE *fh;
  FRAME_STATUS fs;
  int *dst_frame, *index;
  int frame_n, frame_s, frame_e, delete_count, check, clip, i;
  
  if(!fp->exfunc->is_editing(editp)) return FALSE;
  
  if(message == WM_FILTER_IMPORT){
    if(!fp->exfunc->dlg_get_load_name(buf, FILE_TXT, NULL))
      return FALSE;
    
    if((fh = fopen(buf, "r")) == NULL)
      return FALSE;
    
    frame_n = fp->exfunc->get_frame_n(editp);
    if(frame_n <= 0) return FALSE;
    if((dst_frame = (int*)malloc(sizeof(int) * frame_n)) == NULL) return FALSE;
    for(i = 0; i < frame_n; i++) dst_frame[i] = i;
    
    while(fgets(buf, sizeof(buf), fh)){
      clip = check = i = 0;
      while(buf[i] == ' ') i++;
      frame_s = 0;
      if(buf[i] != '-' && buf[i] != '*'){
        for(frame_s = 0; buf[i] >= '0' && buf[i] <= '9'; i++){
          frame_s = frame_s * 10 + buf[i] - '0';
          check = 1;
        }
        while(buf[i] == ' ') i++;
      }
      if(buf[i] == '-' || buf[i] == '*'){
        if(buf[i++] == '*') clip = 1;
        while(buf[i] == ' ') i++;
        if(buf[i] >= '0' && buf[i] <= '9'){
          for(frame_e = 0; buf[i] >= '0' && buf[i] <= '9'; i++){
            frame_e = frame_e * 10 + buf[i] - '0';
            check = 1;
          }
        }else
          frame_e = frame_n - 1;
      }else
        frame_e = frame_s;
      if(buf[i] >= 32 && buf[i] != ' ' && buf[i] !=  '#'){
        MessageBox(NULL, "不正な削除リストです。", "削除リスト インポート", MB_OK|MB_ICONSTOP);
        free(dst_frame);
        fclose(fh);
        return FALSE;
      }
      if(!check) continue;
      
      if(frame_s > frame_e){
        MessageBox(NULL, "不正な削除フレーム範囲の指定があります。", "削除リスト インポート", MB_OK|MB_ICONSTOP);
        free(dst_frame);
        fclose(fh);
        return FALSE;
      }
      
      if(frame_s >= frame_n || frame_e >= frame_n){
        MessageBox(NULL, "削除フレーム指定が範囲外です。", "削除リスト インポート", MB_OK|MB_ICONSTOP);
        free(dst_frame);
        fclose(fh);
        return FALSE;
      }
      
      if(clip){
        delete_count = 0;
        for(i = 0; i < frame_s; i++)
          if(dst_frame[i] >= 0){
            dst_frame[i] = -1;
            delete_count++;
          }
        for(; i <= frame_e; i++)
          if(dst_frame[i] >= 0) dst_frame[i] -= delete_count;
        while(i < frame_n) dst_frame[i++] = -1;
      }else{
        delete_count = 0;
        for(i = frame_s; i <= frame_e; i++)
          if(dst_frame[i] >= 0){
            dst_frame[i] = -1;
            delete_count++;
          }
        if(delete_count)
          for(; i < frame_n; i++)
            if(dst_frame[i] >= 0) dst_frame[i] -= delete_count;
      }
    }
    fclose(fh);
    
    delete_count = 0;
    for(i = 0; i < frame_n; i++)
      if(dst_frame[i] == -1) delete_count++;
    if(delete_count == frame_n){
      MessageBox(NULL, "全フレームが削除指定されています。", "削除リスト インポート", MB_OK|MB_ICONSTOP);
      free(dst_frame);
      return FALSE;
    }
    
    if(delete_count > 0){
      for(i = 0; i < frame_n; i++)
        if(dst_frame[i] >= 0 && dst_frame[i] != i)
          fp->exfunc->copy_frame(editp, dst_frame[i], i);
      free(dst_frame);
      fp->exfunc->set_select_frame(editp, 0, frame_n - delete_count - 1);
      fp->exfunc->set_frame_n(editp, frame_n - delete_count);
      return TRUE;
    }
    free(dst_frame);
  }else if(message == WM_FILTER_EXPORT){
    frame_n = fp->exfunc->get_frame_n(editp);
    if(frame_n <= 0) return FALSE;
    if((index = (int*)malloc(sizeof(int) * frame_n)) == NULL) return FALSE;
    
    for(i = 0; i < frame_n; i++){
      if(!fp->exfunc->get_frame_status(editp, i, &fs)){
        free(index);
        return FALSE;
      }
      index[i] = fs.video;
      if(i > 0){
        if((index[i] >> 24) != (index[i-1] >> 24)){
          MessageBox(NULL, "複数のファイルが連結されています。", "削除リスト エクスポート", MB_OK|MB_ICONSTOP);
          free(index);
          return FALSE;
        }
        if((index[i] & 0xffffff) <= (index[i-1] & 0xffffff)){
          MessageBox(NULL, "フレーム番号が不連続です。", "削除リスト エクスポート", MB_OK|MB_ICONSTOP);
          free(index);
          return FALSE;
        }
      }
    }
    for(i = 0; i < frame_n; i++)
      index[i] &= 0xffffff;
    
    if(!fp->exfunc->dlg_get_load_name(buf, FILE_TXT, NULL))
      return FALSE;
    
    if((fh = fopen(buf, "w")) == NULL)
      return FALSE;
    
    wsprintf(buf, "%d * %d\n", index[0], index[frame_n - 1]);
    fputs(buf, fh);
    
    for(i = 1; i < frame_n; i++){
      if(index[i] == index[i-1] + 2){
        wsprintf(buf, "%d\n", index[i-1] + 1);
        fputs(buf, fh);
      }else if(index[i] > index[i-1] + 2){
        wsprintf(buf, "%d - %d\n", index[i-1] + 1, index[i] - 1);
        fputs(buf, fh);
      }
    }
    fclose(fh);
    free(index);
  }
  
  return FALSE;
}
