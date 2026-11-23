#include <windows.h>
#include "share.h"

void _debug(char *text) {
  MessageBox(0, text, "debug window", MB_OK);
}

//------------------------- string ------------------------------

string string::replace(string sub, string dest) {
  int p = 0, c = 0, np, i = 0;
  PChar _buf;
  while( (p = posex(sub, p)) != -1 ) {
    c ++;
    p += sub.GetLen();
  }
  p = len + c*(dest.GetLen() - sub.GetLen()); 
  _buf = (PChar)malloc(p + 1);
  if(c == 0)
    strcpy(_buf, buf);
  else _buf[p] = 0;
  p = 0;
  while( (np = posex(sub, p)) != -1 ) {
    strncpy(&_buf[i], &buf[p], np - p);
    i += np - p;
    strncpy(&_buf[i], dest.c_str(), dest.GetLen());
    i += dest.GetLen();
      
    p = np + sub.GetLen();
  }
  string res;
  res.SetBuf(_buf);
  return res;
}

string string::remove(int Start, int Count) {
  string res = *this;
  if( (Start >= 0)&&(Start < len) ) {
    if(Count < 1)Count = 1;
    if(Count > len - Start) Count = len - Start;
    #ifndef UNICODE
      strcpy(&res.buf[Start], &res.buf[Start+Count]);
    #else
      wcscpy(&res.buf[Start], &res.buf[Start+Count]);
    #endif
    res.len -= Count;
  }
  return res;
}

string string::copy(int Start, int Count) {
  string Res;
  if( (Start >= 0)&&(Start < len)&& Count ) {
    if(Count < 1)Count = 1;
    
    if(Count > len - Start) Count = len - Start;
    PChar _buf = (PChar)malloc(Count+1);
    #ifndef UNICODE
      strncpy(_buf,&buf[Start],Count);
    #else
      wcsncpy(_buf,&buf[Start],Count);
    #endif
    _buf[Count] = 0;
    Res.SetBuf(_buf);
  }
  return Res;
}

string string::insert(int pos, string dest) {
  if( pos < 0 ) pos = 0;
  if( pos >= len ) pos = len-1;
  PChar _buf = (PChar)malloc(len + dest.GetLen() + 1);
  #ifndef UNICODE
    strncpy(_buf, buf, len-pos);
    strcpy( (PChar)((int)_buf + pos), dest.c_str());
    strcpy( (PChar)((int)_buf + pos + dest.GetLen()), &buf[pos]);
  #else
    wcsncpy(_buf, buf, len-pos);
    wcscpy( &_buf[pos], dest.c_str());
    wcscpy( &_buf[pos + dest.GetLen()], buf[pos]);
  #endif
  string res;
  res.SetBuf(_buf);

  return res;
}

int string::posex(string target, int Start) {
  PChar _buf = buf;
  if(Start < len )
    _buf = &buf[Start];
  else return -1;
  #ifdef UNICODE
    PChar Res = wcsstr(_buf, target.c_str());
  #else
    PChar Res = strstr(_buf, target.c_str());
  #endif
  if( Res )
    return Res - buf;
  else return -1;
}

//------------------------- Stream ------------------------------

void TStream::CopyFrom(TStream &Stream) {
  Stream.SetPosition(0);
  char buf[4096];
  while( Stream.GetPosition() < Stream.Size() )
    Write(buf, Stream.Read(buf, 4096));
}

//------------------------- TFileStream ------------------------------
/*
HANDLE CreateFile(
   LPCTSTR lpFileName,        // Указатель на имя файла (устройства)
   DWORD dwDesiredAccess,  //Параметры доступа
   DWORD dwShareMode,        //Разделяемый доступ
   LPSECURITY_ATTRIBUTES lpSecurityAttributes,    //безопасность
   DWORD dwCreationDistribution,// Описание
   DWORD dwFlagsAndAttributes,     // Атрибуты файла
   HANDLE hTemplateFile      // Файл шаблона
);
*/
TFileStream::TFileStream(string FileName, int Mode) {
  f = CreateFile(FileName.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
//  f = CreateFile(FileName.c_str(), GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
}
                     
int TFileStream::Read(void *buf,int Size) {
  DWORD rd;
  ReadFile(f, buf, Size, &rd, NULL);
  return rd;
}

int TFileStream::Write(void *buf,int Size) {
  DWORD wr;
  WriteFile(f, buf, Size, &wr, NULL);
  return wr;
}
                     
int TFileStream::GetPosition() {
  return SetFilePointer(f, 0, NULL, FILE_CURRENT);
}
