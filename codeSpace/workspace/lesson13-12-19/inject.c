#include <windows.h>
#include <stdio.h>

int main(int argc,char *argv[]){
     int pid = 0;
    HANDLE hproc = 0;
    int buf;
    int nRead, nWrite;
    char *s = "Hello.exe";
    int h,f;
    int rcode;
    DWORD
    char *code[]={0x68,0xCC,0xDD,0xAA,0xBB,
                  0xB8,0x77,0x4f,0xdc,0x55,
                  0xFF,0xD0,
                  0xC3} ;
    
    PBYTE pRemote = (PBYTE)0x004077b0;
    PBYTE pRemoteLLA;
    PBYTE pRemoteStr;
    
    h=f=0;
     if (argc < 2) {
        printf("Usage: %s pid\n", argv[0]);
        return -1;
    }
    pid = atoi(argv[1]);
    if (pid <= 0) {
        printf("[E]: pid must be positive (pid>0)!\n"); 
        return -2;
    }
    hproc = OpenProcess(PROCESS_QUERY_INFORMATION 
        | PROCESS_VM_READ
        | PROCESS_VM_WRITE
        | PROCESS_VM_OPERATION, 0, pid);
    
    //注入hello.exe ,这种方法无法面对动态基址的hello.exe 
    if (!ReadProcessMemory(hproc, 
        pRemote, &buf, 4, &nRead)) {
        printf("[E]: Read DWORD from remote process failed at 0x%08x!\n", pRemote);
    }
    else {
        printf("[I]: Read DWORD from remote process (%d) from 0x%08x --> 0x%08x \n", pid, pRemote, buf);
    }
    printf("nRead:%08x\n",nRead);

    if (!WriteProcessMemory(hproc, 
        pRemote, s, strlen(s)+1, &nWrite)) {
        printf("[E]: Write string to remote process failed at 0x%08x!\n", pRemote);
    } else {
        printf("[I]: Write string (size: %d) to remote process at 0x%08x.\n", nWrite, pRemote);
    }

    if (!CloseHandle(hproc)) {
        printf("[E]: Process (%d) cannot be closed !\n", pid);
        return 2;
    }else 
    {
    	printf("[I]: Process (%d) is closed. \n", pid);
    }
    
    
    pRemoteStr=pRemote;
    
    __asm{
          mov ebx,offset code
          mov eax,[pRemoteStr]
          mov [ebx+0x1],eax    
          }
    
    //获得本机LoadLibraryA地址 
    
    h=LoadLibraryA("kernel32.dll");
    f=GetProcAddress(h,"LoadLibraryA");
    pRemoteLLA=f;
    printf("LLA:0x%08x\n",pRemoteLLA);
    
    __asm{
          mov ebx,offset code
          mov eax,[pRemoteLLA]
          mov [ebx+0x5],eax    
          }
    
    rcode=(PBYTE)VirtualAllocEx
    
    
    
    
    return 0;
    
    }
