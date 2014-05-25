#include <stdio.h>

static int rate, scale;

void write_timecode(FILE *fp, int frame, int q_jitter)
{
  fprintf(fp, "%.0f\n", scale * (1000.0 * frame + 250.0 * q_jitter) / rate);
}

int main(int argc, char** argv)
{
  char timecode_filename[4096], buf[256];
  FILE *fpr, *fpw_t;
  int frame, i, l;
  
  printf("AFS log to Synchronize Timecode ver1.2 by Aji\n");
  
  if(argc < 2 || argc > 3){
    printf("usage: %s <log filename> [<timecode filename>]\n", argv[0]);
    return -1;
  }
  
  /* setup filenames */
  
  if(argc < 3)
    sprintf(timecode_filename, "%s_sync.txt", argv[1]);
  else
    sprintf(timecode_filename, "%s", argv[2]);
  
  /* open files */
  
  if((fpr   = fopen(argv[1],             "rb")) == NULL) return -1;
  if((fpw_t = fopen(timecode_filename,   "wb")) == NULL) return -1;
  
  printf("afs7 log: %s\nsynctime: %s\n\n", argv[1], timecode_filename);
  
  /* read log header */
  
  l = fread(buf, sizeof(char), 64, fpr);
  if(l != 64 || buf[0] != 'a' || buf[1] != 'f' || buf[2] != 's' || buf[3] != '7')
    return -1;
  
  for(frame = 0, i = 16; i < 30; i++)
    frame = frame * 10 + ((buf[i] > '0' && buf[i] <= '9') ? buf[i] - '0' : 0);
  
  for(rate = 0,  i = 32; i < 46; i++)
    rate  = rate  * 10 + ((buf[i] > '0' && buf[i] <= '9') ? buf[i] - '0' : 0);
  
  for(scale = 0, i = 48; i < 62; i++)
    scale = scale * 10 + ((buf[i] > '0' && buf[i] <= '9') ? buf[i] - '0' : 0);
  if(scale == 0) return -1;
  
  printf("Source      %11.6f fps (%d/%d)\n      %10d frames\n\n",
         (double)rate/(double)scale, rate, scale, frame);
  
  /* process timecode */
  
  for(i = 0; i < frame; i++){
    l = fread(buf, sizeof(char), 9, fpr);
    if(i > 0)
      write_timecode(fpw_t, i, buf[0] == '1' ? -2 : 0);
    else
      write_timecode(fpw_t, i, 0);
  }
  
  printf(">>>>>>>> done.\n\n");
  
  return 0;
}

