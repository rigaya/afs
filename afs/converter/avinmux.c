#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <vfw.h>

#define BUFSIZE 4096

static PAVIFILE filer, filew;
static PAVISTREAM streamr, streamw;
static AVISTREAMINFO info;
static char filename[BUFSIZE], aviname[BUFSIZE], buf[BUFSIZE];

static void error_exit(LPTSTR m)
{
  MessageBox(NULL, m, "AVI Null-Frame Multiplexer", MB_OK|MB_ICONSTOP);
  
  exit(-1);
}

int main(int argc, char** argv)
{
  int *sp;
  FILE *fp;
  void *format, *data = NULL;
  int actual, i, c;
  int src, dst, key, data_size;
  DWORD rate, scale, length;
  LONG format_size, l;
  
  printf("AVI Null-Frame Multiplexer ver1.1 by Aji\n");
  
  if(argc < 2 || argc > 3){
    printf("usage: %s <input filename> [<output filename>]\n", argv[0]);
    error_exit("入力ファイルを指定してください。");
  }
  
  /* setup filenames */
  
  if(argc < 3)
    sprintf(filename, "%s.avi", argv[1]);
  else
    sprintf(filename, "%s", argv[2]);
  
  /* read mux file */
  
  if((fp = fopen(argv[1], "rb")) == NULL) error_exit("ファイルが開けません。");
  if(fgets(aviname, BUFSIZE, fp) == NULL) error_exit("ファイルの読み込みに失敗しました。");
  i = 0;
  while(aviname[i] != 0) i++;
  if(i > 0) if(aviname[i-1] < 0x20) aviname[--i] = 0;
  if(i > 0) if(aviname[i-1] < 0x20) aviname[--i] = 0;
  if(fgets(buf, BUFSIZE, fp) == NULL) error_exit("ファイルの読み込みに失敗しました。");
  actual = atoi(buf);
  if(fgets(buf, BUFSIZE, fp) == NULL) error_exit("ファイルの読み込みに失敗しました。");
  rate = atoi(buf);
  if(fgets(buf, BUFSIZE, fp) == NULL) error_exit("ファイルの読み込みに失敗しました。");
  scale = atoi(buf);
  
  if((sp = malloc((actual+1) * sizeof(int))) == NULL) error_exit("メモリの確保に失敗しました。");
  
  length = actual;
  for(i = 0; i <= actual; i++){
    if(fgets(buf, BUFSIZE, fp) == NULL) error_exit("ファイルの読み込みに失敗しました。");
    sp[i] = atoi(buf);
    length += sp[i];
  }
  fclose(fp);
  
  printf("input : %s\nframe = %d\nrate  = %d\nscale = %d\n", aviname, length, rate, scale);
  
  /* open avi */
  
  AVIFileInit();
  if(AVIFileOpen(&filer, aviname, OF_READ, NULL)) error_exit("AVIファイルを開けません。");
  if(AVIFileGetStream(filer, &streamr, streamtypeVIDEO, 0)) error_exit("ビデオストリームを取得できません。");
  
  /* read info */
  
  if(AVIStreamInfo(streamr, &info, sizeof(AVISTREAMINFO))) error_exit("ストリーム情報を取得できません。");
  
  if(AVIStreamReadFormat(streamr, 0, NULL, &format_size)) error_exit("フォーマット情報のサイズを取得できません。");
  if((format = malloc(format_size)) == NULL) return -1;
  if(AVIStreamReadFormat(streamr, 0, format, &format_size)) error_exit("フォーマット情報を取得できません。");
  
  /* modify stream info */
  
  info.dwRate = rate;
  info.dwScale = scale;
  info.dwLength = length;
  
  /* create avi */
  
  printf("output: %s\n", filename);
  
  if(AVIFileOpen(&filew, filename, OF_CREATE|OF_WRITE|OF_SHARE_EXCLUSIVE, NULL)) error_exit("AVIファイルを作成できません。");
  if(AVIFileCreateStream(filew, &streamw, &info)) error_exit("ビデオストリームを作成できません。");
  if(AVIStreamSetFormat(streamw, 0, format, format_size)) error_exit("フォーマット情報を記録できません。");
  
  /* mux stream */
  
  data_size = 0x100000;
  if((data = malloc(data_size)) == NULL) error_exit("バッファメモリの確保に失敗しました。");
  
  for(i = 0, src = 0, dst = 0; i < actual; i++){
    for(c = 0; c < sp[i]; c++)
      AVIStreamWrite(streamw, dst++, 1, NULL, 0, 0, NULL, NULL);
    if(AVIStreamRead(streamr, src, 1, NULL, 0, &l, NULL)) error_exit("データサイズの取得に失敗しました。");
    while(l == 0){
      src++;
      if(AVIStreamRead(streamr, src, 1, NULL, 0, &l, NULL)) error_exit("データサイズの取得に失敗しました。");
    }
    if(data_size < l){
      free(data);
      if((data = malloc(l)) == NULL) error_exit("バッファメモリの確保に失敗しました。");
      data_size = l;
    }
    if(AVIStreamRead(streamr, src, 1, data, data_size, &l, NULL)) error_exit("データの読み込みに失敗しました。");
    key = (AVIStreamIsKeyFrame(streamr, src));
    src++;
    if(AVIStreamWrite(streamw, dst++, 1, data, l,  key ? AVIIF_KEYFRAME : 0, NULL, NULL)) error_exit("データの書き込みに失敗しました。");
  }
  
  for(c = 0; c < sp[actual]; c++)
    AVIStreamWrite(streamw, dst++, 1, NULL, 0, 0, NULL, NULL);
  
  /* close avi */
  
  AVIStreamRelease(streamw);
  AVIFileRelease(filew);
  AVIStreamRelease(streamr);
  AVIFileRelease(filer);
  AVIFileExit();
  
  free(sp);
  
  printf(">>>>>> done.\n");
  
  return 0;
}

