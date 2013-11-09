#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <vfw.h>

#define BUFSIZE 4096

static PAVIFILE filer, filew;
static PAVISTREAM streamr, streamw;
static AVISTREAMINFO info;
static char filename[BUFSIZE], aviname[BUFSIZE], buf[BUFSIZE];
static char usage[] = "usage: avitmux <timecode filename> <input filename> <output filename> [#rate #scale]\n(default #rate/#scale = 120000/1001)";

static void error_exit(LPTSTR m)
{
  puts(m);
  exit(-1);
}

int main(int argc, char** argv)
{
  int *sp;
  FILE *fp;
  void *format, *data = NULL;
  int actual, i, c;
  int src, dst, key, data_size;
  DWORD rate, scale, length, pos;
  LONG format_size, l;
  
  printf("AVI Timecode Multiplexer ver1.0 by Aji\n");
  
  if(argc != 4 && argc != 6) error_exit(usage);
  
  rate = 120000, scale = 1001;
  if(argc == 6){
    rate  = atoi(argv[4]);
    scale = atoi(argv[5]);
  }
  
  if(rate <= 0 || scale <= 0) error_exit(usage);
  if(rate/scale >= 1000) error_exit(usage);
  
  /* read timecode file */
  
  if((fp = fopen(argv[1], "rb")) == NULL) error_exit("ファイルが開けません。");
  
  actual = 0;
  while(fgets(buf, BUFSIZE, fp) != NULL)
    if(buf[0] >= '0' && buf[0] <= '9') actual++;
  
  if((sp = malloc((actual+1) * sizeof(int))) == NULL) error_exit("メモリの確保に失敗しました。");
  
  fseek(fp, 0L, SEEK_SET);
  
  length = i = 0;
  while(fgets(buf, BUFSIZE, fp) != NULL){
    if(buf[0] >= '0' && buf[0] <= '9'){
      pos = (int)(atof(buf) * 0.001 * (double)rate / (double)scale + 0.5);
      if(pos <= length){
        sp[i++] = 0;
        length++;
      }else{
        sp[i++] = pos - length;
        length = pos + 1;
      }
    }
  }
  length -= sp[0];
  sp[0] = 0;
  sp[actual] = 0;
  
  close(fp);
  
  /* open avi */
  
  printf("input : %s\nframe = %d\nrate  = %d\nscale = %d\nfps   = %.2f\n", argv[1], length, rate, scale, (double)rate/(double)scale);
  
  AVIFileInit();
  if(AVIFileOpen(&filer, argv[2], OF_READ, NULL)) error_exit("AVIファイルを開けません。");
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
  
  printf("output: %s\n", argv[3]);
  
  if(AVIFileOpen(&filew, argv[3], OF_CREATE|OF_WRITE|OF_SHARE_EXCLUSIVE, NULL)) error_exit("AVIファイルを作成できません。");
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

