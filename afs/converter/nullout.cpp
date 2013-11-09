#include <stdio.h>
#include <windows.h>
#include "output.h"

OUTPUT_PLUGIN_TABLE output_plugin_table = {
  NULL,
  "NULL出力",
  "AVI (*.avi)\0*.avi\0WMV (*.wmv)\0*.wmv\0AllFile (*.*)\0*.*\0",
  "NULL出力プラグイン ver1.0 by Aji",
  NULL,//func_init,
  NULL,//func_exit
  func_output,
  NULL,//func_config,
  NULL,//func_config_get,
  NULL//func_config_set
};

EXTERN_C OUTPUT_PLUGIN_TABLE __declspec(dllexport) * __stdcall GetOutputPluginTable(void)
{
  return &output_plugin_table;
}

BOOL func_output(OUTPUT_INFO *oip)
{
  FILE *fp;
  int i;
  
  fp = fopen(oip->savefile, "rb");
  if(fp == NULL) fp = fopen(oip->savefile, "wb");
  if(fp != NULL) fclose(fp);
  
  for(i = 0; i < oip->n; i++){
    if(oip->func_is_abort()) return FALSE;
    
    oip->func_get_video_ex(i, NULL);
    
    if((i & 15) == 0){
      oip->func_rest_time_disp(i, oip->n);
      oip->func_update_preview();
    }
  }
  
  return TRUE;
}

