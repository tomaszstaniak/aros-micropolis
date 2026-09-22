#include <map>
#include <string>
#include <cstring>
#include <cassert>
#include <cstdio>
#include "launch-directory.h"
static BPTR cwd=1,next=10;static std::map<BPTR,std::string> locks;
static bool programMissing=false,nameFail=false;
BPTR Lock(const char *s,int){if(programMissing&&std::string(s)=="PROGDIR:")return 0;auto n=next++;locks[n]=std::string(s)=="PROGDIR:"?"SYS:Games/Micropolis":std::string(s).find(':')!=std::string::npos?s:"Work:Caller/"+std::string(s);return n;}
void UnLock(BPTR n){assert(locks.erase(n)==1);}
int NameFromLock(BPTR n,char *p,long size){if(nameFail||locks[n].size()+1>(unsigned long)size)return 0;strcpy(p,locks[n].c_str());return 1;}
BPTR CurrentDir(BPTR n){auto old=cwd;cwd=n;return old;}
int AddPart(char *p,const char *name,unsigned long size){std::string s(p);if(!s.empty()&&s.back()!=':'&&s.back()!='/')s+='/';s+=name;if(s.size()+1>size)return 0;strcpy(p,s.c_str());return 1;}
int main(){
 std::string error;char a0[]="Micropolis",a1[]="my city.cty",a2[]="Assets:tiles.bmp",a3[]="sprites";char *a[]={a0,a1,a2,a3};
 {LaunchDirectory d;assert(d.prepare(4,a,error));assert(d.arguments[0]=="Work:Caller/my city.cty");assert(d.arguments[1]=="Assets:tiles.bmp");assert(d.arguments[2]=="Work:Caller/sprites");assert(cwd!=1);}
 assert(cwd==1&&locks.empty());
 // Workbench: tool icon alone, then a city project icon (drawer lock + name).
 WBArg wbArgs[2]={{0,a0},{0,nullptr}};WBStartup wb{1,wbArgs};
 {LaunchDirectory d;assert(d.prepare(0,reinterpret_cast<char **>(&wb),error));assert(d.arguments.empty());}
 assert(cwd==1&&locks.empty());
 BPTR drawer=next++;locks[drawer]="Work:Cities";char project[]="Harbor.cty";
 wbArgs[1]={drawer,project};wb.sm_NumArgs=2;
 {LaunchDirectory d;assert(d.prepare(0,reinterpret_cast<char **>(&wb),error));assert(d.arguments.size()==1&&d.arguments[0]=="Work:Cities/Harbor.cty");}
 assert(cwd==1&&locks.size()==1);
 wbArgs[1].wa_Lock=0;error.clear();
 {LaunchDirectory d;assert(!d.prepare(0,reinterpret_cast<char **>(&wb),error));assert(!error.empty());}
 locks.erase(drawer);
 assert(cwd==1&&locks.empty());
 nameFail=true;{LaunchDirectory d;assert(!d.prepare(2,a,error));}nameFail=false;
 assert(cwd==1&&locks.empty()&&!error.empty());
 programMissing=true;{LaunchDirectory d;assert(!d.prepare(1,a,error));}programMissing=false;
 assert(cwd==1&&locks.empty());
 puts("PASS CLI relative/volume paths, WB tool and project-icon arguments, failure cleanup, CWD restoration");
}
