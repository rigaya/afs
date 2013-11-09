#include <stdio.h>
#include <windows.h>
#include <vfw.h>

#define BUFSIZE 4096

static PAVIFILE filer, filew;
static PAVISTREAM streamr, streamw;
static AVISTREAMINFO info;
static char filename[BUFSIZE], buf[BUFSIZE];

static void error_exit(LPTSTR m)
{
  MessageBox(NULL, m, "AVI Quarter FPS converter", MB_OK|MB_ICONSTOP);
  
  exit(-1);
}

int main(int argc, char** argv)
{
  int *sp, *dp;
  int total, new_total, actual, i, j, c;
  void *format, *data = NULL;
  int src, dst, key, data_size;
  DWORD rate, scale, length;
  LONG format_size, l;
  
  printf("AVI Quarter FPS converter ver1.1 by Aji\n");
  
  if(argc < 2 || argc > 3){
    printf("usage: %s <input filename> [<output filename>]\n", argv[0]);
    error_exit("入力ファイルを指定してください。");
  }
  
  /* setup filenames */
  
  if(argc < 3){
    i = sprintf(filename, "%s", argv[1]);
    if(i > 3){
      if(filename[i - 4] == '.' &&
         (filename[i - 3] == 'A' || filename[i - 3] == 'a') &&
         (filename[i - 2] == 'V' || filename[i - 2] == 'v') &&
         (filename[i - 1] == 'I' || filename[i - 1] == 'i'))
      sprintf(filename + i - 4, ".qfps%s", argv[1] + i - 4);
    }
  }else
    sprintf(filename, "%s", argv[2]);
  
  /* open avi */
  
  AVIFileInit();
  
  printf("input:  %s\n", argv[1]);
  
  if(AVIFileOpen(&filer, argv[1], OF_READ, NULL)) error_exit("AVIファイルを開けません。");
  if(AVIFileGetStream(filer, &streamr, streamtypeVIDEO, 0)) error_exit("ビデオストリームを取得できません。");
  
  /* read stream info */
  
  if(AVIStreamInfo(streamr, &info, sizeof(AVISTREAMINFO))) error_exit("ストリーム情報を取得できません。");
  
  /* analyze stream */
  
  total = info.dwLength;
  actual = 0;
  if((sp = malloc(total * sizeof(int))) == NULL) error_exit("メモリの確保に失敗しました。");
  
  for(i = 0; i < total; i++){
    AVIStreamRead(streamr, i, 1, NULL, 0, &l, NULL);
    sp[i] = l;
    if(l > 0) actual++;
  }
  
  if((dp = malloc((actual+1) * sizeof(int))) == NULL) error_exit("メモリの確保に失敗しました。");
  
  for(i = 0, j = 0, c = 0; i < total; i++){
    if(sp[i] > 0){
      dp[j++] = c;
      c = 0;
    }else
      c++;
  }
  dp[j] = c;
  
  total = dp[0];
  new_total = 0;
  dp[0] = 0;
  for(i = 0; i < actual; i++){
    total += dp[i+1] + 1;
    new_total++;
    if(((total + 3)>>2) > new_total)
      dp[i+1] = ((total + 3)>>2) - new_total;
    else
      dp[i+1] = 0;
    new_total += dp[i+1];
  }
  
  /* report convert stat. */
  
  rate = info.dwRate;
  scale = info.dwScale;
  if((rate & 3) == 0)
    rate >>= 2;
  else if((rate & 3) == 2)
    rate >>= 1, scale <<= 1;
  else
    scale <<= 2;
  
  printf("\n%.3fps (%d/%d) -> %.3fps (%d/%d)\n",
         (double)info.dwRate/(double)info.dwScale, info.dwRate, info.dwScale,
         (double)rate/(double)scale, rate, scale);
  printf("%dframes (%.3fsec) -> %dframes (%.3fsec)\n\n",
         info.dwLength, (double)info.dwLength*(double)info.dwScale/(double)info.dwRate,
         new_total, (double)new_total*(double)scale/(double)rate);
  
  /* read info */
  
  if(AVIStreamReadFormat(streamr, 0, NULL, &format_size)) error_exit("フォーマット情報のサイズを取得できません。");
  if((format = malloc(format_size)) == NULL) return -1;
  if(AVIStreamReadFormat(streamr, 0, format, &format_size)) error_exit("フォーマット情報を取得できません。");
  
  /* modify stream info */
  
  info.dwRate = rate;
  info.dwScale = scale;
  info.dwLength = new_total;
  
  /* create avi */
  
  printf("output: %s\n", filename);
  
  if(AVIFileOpen(&filew, filename, OF_CREATE|OF_WRITE|OF_SHARE_EXCLUSIVE, NULL)) error_exit("AVIファイルを作成できません。");
  if(AVIFileCreateStream(filew, &streamw, &info)) error_exit("ビデオストリームを作成できません。");
  if(AVIStreamSetFormat(streamw, 0, format, format_size)) error_exit("フォーマット情報を記録できません。");
  
  /* mux stream */
  
  data_size = 0x100000;
  if((data = malloc(data_size)) == NULL) error_exit("バッファメモリの確保に失敗しました。");
  
  for(i = 0, src = 0, dst = 0; i < actual; i++){
    for(c = 0; c < dp[i]; c++)
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
  
  for(c = 0; c < dp[actual]; c++)
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


