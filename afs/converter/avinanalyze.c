#include <stdio.h>
#include <windows.h>
#include <vfw.h>

static PAVIFILE file;
static PAVISTREAM stream;
static AVISTREAMINFO info;

static void error_exit(LPTSTR m)
{
  MessageBox(NULL, m, "AVI Null-Frame Analyzer", MB_OK|MB_ICONSTOP);
  
  exit(-1);
}

int main(int argc, char** argv)
{
  char filename[4096];
  int *sp;
  FILE *fp;
  int total, actual, i, c;
  LONG l;
  
  printf("AVI Null-Frame Analyzer ver1.1 by Aji\n");
  
  if(argc < 2 || argc > 3){
    printf("usage: %s <input filename> [<output filename>]\n", argv[0]);
    error_exit("入力ファイルを指定してください。");
  }
  
  /* setup filenames */
  
  if(argc < 3)
    sprintf(filename, "%s.txt", argv[1]);
  else
    sprintf(filename, "%s", argv[2]);
  
  /* open avi */
  
  AVIFileInit();
  if(AVIFileOpen(&file, argv[1], OF_READ, NULL)) error_exit("AVIファイルが開けません。");
  if(AVIFileGetStream(file, &stream, streamtypeVIDEO, 0)) error_exit("ビデオストリームを取得できません。");
  
  /* read stream info */
  
  if(AVIStreamInfo(stream, &info, sizeof(AVISTREAMINFO))) error_exit("ストリーム情報を取得できません。");
  
  /* analyze stream */
  
  total = info.dwLength;
  actual = 0;
  if((sp = malloc(total * sizeof(int))) == NULL) error_exit("メモリの確保に失敗しました。");
  
  for(i = 0; i < total; i++){
    AVIStreamRead(stream, i, 1, NULL, 0, &l, NULL);
    sp[i] = l;
    if(l > 0) actual++;
  }
  
  /* close avi */
  
  AVIStreamRelease(stream);
  AVIFileRelease(file);
  AVIFileExit();
  
  /* write result */
  
  if((fp = fopen(filename, "wb")) == NULL) error_exit("ファイルの作成に失敗しました。");
  fprintf(fp, "%s\n%d\n%d\n%d\n", argv[1], actual, info.dwRate, info.dwScale);
  for(i = 0, c = 0; i < total; i++){
    if(sp[i] > 0){
      fprintf(fp, "%d\n", c);
      c = 0;
    }else
      c++;
  }
  fprintf(fp, "%d\n", c);
  fclose(fp);
  
  free(sp);
  
  return 0;
}



