#include <stdio.h>
#define AFS_CLIENT_NO_SHARE
#include "afs_client.h"

static int rate, scale;

void write_timecode(FILE *fp, int frame, int q_jitter)
{
  fprintf(fp, "%.0f\n", scale * (1000.0 * frame + 250.0 * q_jitter) / rate);
}

void write_deleteframe(FILE *fp, int frame)
{
  fprintf(fp, "%d\n", frame);
}

int main(int argc, char** argv)
{
  char timecode_filename[4096], deletelist_filename[4096], buf[256];
  FILE *fpr, *fpw_t, *fpw_d;
  unsigned char status;
  int frame, delete_count, i, l;
  
  printf("AFS log to Timecode/Delete-list ver1.3 by Aji\n");
  
  if(argc < 2 || argc > 4){
    printf("usage: %s <log filename> [<delete list filename> [<timecode filename>]]\n", argv[0]);
    return -1;
  }
  
  /* setup filenames */
  
  if(argc < 3)
    sprintf(deletelist_filename, "%s_del.txt", argv[1]);
  else
    sprintf(deletelist_filename, "%s", argv[2]);
  
  if(argc < 4)
    sprintf(timecode_filename, "%s_tcode.txt", argv[1]);
  else
    sprintf(timecode_filename, "%s", argv[3]);
  
  /* open files */
  
  if((fpr   = fopen(argv[1],             "rb")) == NULL) return -1;
  if((fpw_t = fopen(timecode_filename,   "wb")) == NULL) return -1;
  if((fpw_d = fopen(deletelist_filename, "wb")) == NULL) return -1;
  
  printf("afs7 log: %s\ntimecode: %s\ndel.list: %s\n\n",
         argv[1], timecode_filename, deletelist_filename);
  
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
  
  delete_count = 0;
  fprintf(fpw_t, "# timecode format v2\n");
  
  for(i = 0; i < frame; i++){
    l = fread(buf, sizeof(char), 9, fpr);
    status = AFS_STATUS_DEFAULT |
             ((buf[0] == '1') ? AFS_FLAG_SHIFT0     : 0) |
             ((buf[1] == '1') ? AFS_FLAG_SHIFT1     : 0) |
             ((buf[2] == '1') ? AFS_FLAG_SHIFT2     : 0) |
             ((buf[3] == '1') ? AFS_FLAG_SHIFT3     : 0) |
             ((buf[4] == '1') ? AFS_FLAG_FRAME_DROP : 0) |
             ((buf[5] == '1') ? AFS_FLAG_SMOOTHING  : 0) |
             ((buf[6] == '1') ? AFS_FLAG_FORCE24    : 0);
    
    if(i > 0){
      int drop = afs_set_status(status, 0);
      if(drop){
        write_deleteframe(fpw_d, i);
        afs_drop();
        delete_count++;
      }else
        write_timecode(fpw_t, i, afs_get_jitter());
    }else
      write_timecode(fpw_t, i, afs_init(status, 0));
  }
  
  printf(">>>>>>>>\n\nDelete%10d frames\nOuput Avg.  %11.6f fps\n",
         delete_count, ((double)rate*(frame-delete_count))/((double)scale*frame));
  
  return 0;
}

