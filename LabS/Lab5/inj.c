#include <windows.h>
#include <stdio.h>
DWORD dwSizeOfCode;
PBYTE pCode, pOrignCode;

__declspec(naked) void code_start()
{
    __asm {
// ---------------------------------------------------------
// type : DWORD GetBaseKernel32()
				//call _GetBaseKernel32 //
_GetBaseKernel32:
				//pop ebx
        push    ebp
        mov     ebp, esp
        push    esi
        push    edi
        xor     ecx, ecx                    // ECX = 0
        mov     esi, fs:[0x30]              // ESI = &(PEB) ([FS:0x30])
        mov     esi, [esi + 0x0c]           // ESI = PEB->Ldr (LDR_DATA)
        mov     esi, [esi + 0x1c]           // ESI = PEB->Ldr.InInitOrder
_next_module:
        mov     eax, [esi + 0x08]           // EBP = InInitOrder[X].base_address
        mov     edi, [esi + 0x20]           // EBP = InInitOrder[X].module_name (unicode) �����ַָ���λ�ô����һ��unicode������ַ������ַ������ݾ���dll���ļ���
        mov     esi, [esi]                  // ESI = InInitOrder[X].flink (next module) 
        cmp     [edi + 12*2], cx            // modulename[12] == 0 ? һ��unicode�ĳ���Ϊ2���ֽڣ�����*2��Ȼ��kernel32.dll��12*2���ֽڳ�����ô��25���ֽڱ�Ϊ0�����ַ�����\0 �Һ��������8��AB��Ҳ����˵�ַ������ȶ��˾���25��λ�þ����ַ������˾���AB ��������\0

        jne     _next_module                 // No: try next module.
        pop     edi
        pop     esi
        mov     esp, ebp
        pop     ebp
      // ret
      	pop 	ebx//��ԭ���̵���һ��ָ���ַeip����ebx���൱��ȥ��jmpָ���ret
      	push  eax
      	push 	ebx
        
        
        // type : DWORD GetGetProcAddrBase(DWORD base)
_GetGetProcAddrBase:
        push    ebp
        mov     ebp, esp
        push    edx
        push    ebx
        push    edi
        push    esi
        mov     ebx, [ebp+8] //�˴���8Ӧ���ǲ���������4�Ƿ��ص�ַ
        //mov     ebx, [ebp+4]
        mov     eax, [ebx + 0x3c] // edi = BaseAddr, eax = pNtHeader(RVA)
        mov     edx, [ebx + eax + 0x78]
        add     edx, ebx          // edx = Export Table (RVA)+BaseAddr
        mov     ecx, [edx + 0x18] // ecx = NumberOfNames
        mov     edi, [edx + 0x20] //
        add     edi, ebx          // ebx = AddressOfNames
_search:
        dec     ecx
        mov     esi, [edi + ecx*4]//ecx--,�Ӻ���ǰ����name�ı�����һ�����Ծ���name�ַ�����RVA
        add     esi, ebx
        mov     eax, 0x50746547 // "PteG" �ȿ�ǰ�ĸ��ֽ��ǲ���GetP
        cmp     [esi], eax
        jne     _search //���ǵĻ�����������һ��name��Ԫ
        mov     eax, 0x41636f72 //"Acor" �ǵĻ��ٿ����ĸ���Ԫ�ǲ���rocA
        cmp     [esi+4], eax
        jne     _search //���ǵĻ�����������һ��name��Ԫ
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
    pCode= VirtualAlloc(NULL, dwSizeOfCode, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
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
    return dwSizeOfCode;
} 

int main(int argc ,char* argv[]){
		DWORD pid = 0;
		DWORD sizeOfInjectedCode;
		DWORD addrGetProcAddress;
		int TID,num,old;
		PBYTE rcode;
		DWORD hproc,hthread;
		
    // Ϊpid��ֵΪhello.exe�Ľ���ID
    if (argc < 2) {
        printf("Usage: %s pid\n", argv[0]);
        return -1;
    }
    pid = atoi(argv[1]);
		if (pid <= 0) {
        	printf("[E]: pid must be positive (pid>0)!\n"); 
        	return -2;
		}
		sizeOfInjectedCode=make_code();//��������벢�������µ����򣬷��ش��볤��
    hproc = OpenProcess(
          PROCESS_CREATE_THREAD  | PROCESS_QUERY_INFORMATION
        | PROCESS_VM_OPERATION   | PROCESS_VM_WRITE 
        | PROCESS_VM_READ, FALSE, pid);//����Զ�̽��̾��
        
    rcode=(PBYTE)VirtualAllocEx(hproc,0,sizeOfInjectedCode,MEM_COMMIT,PAGE_EXECUTE_READWRITE);//open the space in remote process
		if(!WriteProcessMemory(hproc, rcode, pCode, sizeOfInjectedCode, &num)){//д�����
			printf("[E]Write pCode Failed\n");
			}else 
			{
			printf("[I]Write pCode Success\n");
			}
		hthread = CreateRemoteThread(hproc,NULL, 0, (LPTHREAD_START_ROUTINE)rcode,0, 0 , &TID);//�����߳�
		WaitForSingleObject(hthread, 0xffffffff);//�첽�ȴ�
		if(!GetExitCodeThread(hthread, &addrGetProcAddress)){//��ȡ����ֵ
  		printf("[E]Get exitCode Failed\n");
  		}else 
  		{
  		printf("[I]Get exitCode Success\n");
  		}
		printf("The address of GetProcAddress is 0x%08x\n",addrGetProcAddress);//��ӡ
		
	}  