#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../../CGTShare.h"

int buildGetParamsProc(TBuildParams *params) {
  params->flags = CGMP_RUN;
  return(CG_SUCCESS);
}

int buildMakePrj(TBuildMakePrjRec *params) {
  FILE *f = fopen(params->prjFilename, "w");
  fwrite(params->result, strlen((const char *)params->result), 1, f);
  fclose(f);
  return CG_SUCCESS;
}

int buildCompliteProc(TBuildCompliteRec *params) {
  return CG_SUCCESS;
}

int buildRunProc(TBuildRunRec *params) {
/*
  STARTUPINFO si7;
  PROCESS_INFORMATION pi7;
  ZeroMemory(&si7, sizeof(STARTUPINFO));
  si7.cb = sizeof(STARTUPINFO);
  si7.dwFlags = STARTF_USESHOWWINDOW;
  si7.wShowWindow = SW_SHOWDEFAULT;
  CreateProcess(NULL, (string("explorer.exe ") + string(params.FileName)).c_str(), NULL, NULL, false, CREATE_DEFAULT_ERROR_MODE, NULL, NULL, &si7, &pi7);
  while(WaitForSingleObject(pi7.hProcess, -1) == WAIT_TIMEOUT) {
  }
*/
    // Construct the command to open the HTML file
    char command[9999];
    #ifdef _WIN32
        // For Windows
        snprintf(command, sizeof(command), "start %s", params->FileName);
    #else
        // For Unix/Linux/Mac
        snprintf(command, sizeof(command), "xdg-open %s", params->FileName);
    #endif
  return system(command) == 0 ? CG_SUCCESS : CG_NOT_FOUND;
}

int main() {
	return 0;
}
// made by "HiAsm 4.4 build 185"

