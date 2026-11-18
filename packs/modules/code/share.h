#ifndef _HI_STRING_H_
#define _HI_STRING_H_

#include <windows.h>

extern void _debug(char *text);

typedef char* PChar; 

inline void *GetMemory(int size) {
  return VirtualAlloc(NULL,size,MEM_RESERVE,PAGE_READWRITE);
}

inline void FreeMemory(void *p) {
  VirtualFree(p,0,MEM_RELEASE);
}

inline char *ExtractFilePath( char *path ) {
  char drive[_MAX_DRIVE];
  char dir[_MAX_DIR];
  char fname[_MAX_FNAME];
  char ext[_MAX_EXT];
  char *c;
  _splitpath( path, drive, dir, fname, ext );
  //c = (char *) strcat (drive, dir);
  c = (char *) dir;
  return c;
}

class string {
  private:
    PChar buf;
    int len;
  public:
    string() {
      len = 0;
      buf = NULL;
    }
    string(const string &str) {
      len = str.len;
      buf = (PChar)malloc(len+1);
      strcpy(buf, str.buf);
    }
    string(PChar text) {
      len = strlen(text);
      buf = (PChar)malloc(len+1);
      strcpy(buf, text);
    }
    string(int i) {
      char bf[32];
      itoa(i, bf, 10);
      len = strlen(bf);
      buf = (PChar)malloc(len+1);
      strcpy(buf, bf);
    }
    string(float f) {
      char bf[32];
      int i = (int)f;
      itoa(i, bf, 10);
      len = strlen(bf);
      buf = (PChar)malloc(len+1);
      strcpy(buf, bf);
    }
    ~string() {              
      if(buf) {
        free(buf);
      }
    }
    PChar c_str() {
      return buf;
    }
    void SetBuf(PChar bf) {
      if(buf) free(buf);
      buf = bf;
      len = strlen(bf);
    }    
    int GetLen() { return len; }
    string operator+ (string s) {
      PChar result = (PChar)malloc(len + s.GetLen() + 1);
      if(buf)
        strcpy(result, buf);
      strcpy(result + len, s.c_str());
      return string(result);
    }
    operator PChar(void) {
      return buf;
    }
    string replace(string sub, string dest);
    string remove(int Start, int Count);
    string copy(int Start, int Count);
    string insert(int pos, string dest);
    int posex(string target, int Start);
};

//------------------------- CvtTypes ------------------------------

#define str_to_int(s) atoi((s).c_str())
#define int_to_str(i) string(i)
#define str_to_real(s) atof((s).c_str())
#define real_to_str(f) string(f)
#define real_to_int(f) (int)(f)

//------------------------- Stream ------------------------------

class TStream
{
  protected:
  public:
    virtual int Read(void *buf,int Size) = 0;
    virtual int Write(void *buf,int Size) = 0;
    virtual int Size() = 0;
    virtual int GetPosition() = 0;
    virtual void SetPosition(int Value) = 0;
    virtual void CopyFrom(TStream &Stream);
};

//------------------------- TFileStream ------------------------------

class TFileStream:public TStream
{
  private:
    HANDLE f;
  protected:
  public:
    TFileStream(string FileName, int Mode);
    ~TFileStream(){ CloseHandle(f); }
    int Read(void *buf,int Size);
    int Write(void *buf,int Size);
    int Size(){ return GetFileSize(f, NULL); }
    int GetPosition();
    void SetPosition(int Value){ SetFilePointer(f, Value, NULL, FILE_BEGIN); }
};

#endif
