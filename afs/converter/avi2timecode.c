#include <stdio.h>
#include <windows.h>
#include <vfw.h>

static PAVIFILE file;
static PAVISTREAM stream;
static AVISTREAMINFO info;

static void error_exit(LPTSTR m)
{
  MessageBox(NULL, m, "AVI to timecode", MB_OK|MB_ICONSTOP);
  
  exit(-1);
}

int main(int argc, char** argv)
{
  char filename[4096];
  int *sp;
  FILE *fp;
  int total, actual, i, c;
  LONG l;
  
  printf("AVI to timecode ver1.1 by Aji\n");
  
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
  fprintf(fp, "# timecode format v2\n");
  for(i = 0; i < total; i++){
    if(sp[i] > 0){
      fprintf(fp, "%.0f\n", 1000.0 * (double)i * (double)info.dwScale / (double)info.dwRate);
    }
  }
  fclose(fp);
  
  free(sp);
  
  return 0;
}



