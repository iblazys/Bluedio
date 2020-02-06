#pragma once
#include "imports.h"

namespace memory {

	void DumpHex(const void* data, size_t size) {
		char ascii[17];
		size_t i, j;
		ascii[16] = '\0';
		for (i = 0; i < size; ++i) {
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "%02X ", ((unsigned char*)data)[i]);
			if (((unsigned char*)data)[i] >= ' ' && ((unsigned char*)data)[i] <= '~') {
				ascii[i % 16] = ((unsigned char*)data)[i];
			}
			else {
				ascii[i % 16] = '.';
			}
			if ((i + 1) % 8 == 0 || i + 1 == size) {
				DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, " ");
				if ((i + 1) % 16 == 0) {
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "|  %s \n", ascii);
				}
				else if (i + 1 == size) {
					ascii[(i + 1) % 16] = '\0';
					if ((i + 1) % 16 <= 8) {
						DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, " ");
					}
					for (j = (i + 1) % 16; j < 16; ++j) {
						DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "   ");
					}
					DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "|  %s \n", ascii);
				}
			}
		}
	}

	PVOID get_system_module_base(const char* module_name) {

		ULONG bytes = 0;
		NTSTATUS status = ZwQuerySystemInformation(SystemModuleInformation, 0, bytes, &bytes);

		if (!bytes)
			return 0;


		PRTL_PROCESS_MODULES modules = (PRTL_PROCESS_MODULES)ExAllocatePoolWithTag(NonPagedPool, bytes, 0x454E4F45); // 'ENON'

		status = ZwQuerySystemInformation(SystemModuleInformation, modules, bytes, &bytes);

		if (!NT_SUCCESS(status))
			return 0;


		PRTL_PROCESS_MODULE_INFORMATION module = modules->Modules;
		PVOID module_base = 0, module_size = 0;

		for (ULONG i = 0; i < modules->NumberOfModules; i++)
		{

			if (strcmp((char*)module[i].FullPathName, module_name) == 0)
			{
				module_base = module[i].ImageBase;
				module_size = (PVOID)module[i].ImageSize;
				break;
			}
		}

		if (modules)
			ExFreePoolWithTag(modules, 0);

		if (module_base <= 0)
			return 0;

		return module_base;
	}

	PVOID get_system_module_export(const char* module_name, LPCSTR routine_name)
	{
		PVOID lpModule = memory::get_system_module_base(module_name);

		if (!lpModule)
			return NULL;

		return RtlFindExportedRoutineByName(lpModule, routine_name);
	}

	bool read_memory(void* address, void* buffer, size_t size) {
		if (!RtlCopyMemory(buffer, address, size)) {
			return false;
		}
		else
		{
			return true;
		}
	}

	bool write_memory(void* address, void* buffer, size_t size) {
		if (!RtlCopyMemory(address, buffer, size)) {
			return false;
		}
		else
		{
			return true;
		}
	}

	bool write_to_read_only_memory(void* address, void* buffer, size_t size) {

		PMDL Mdl = IoAllocateMdl(address, size, FALSE, FALSE, NULL);

		if (!Mdl)
			return false;

		// Locking and mapping memory with RW-rights:
		MmProbeAndLockPages(Mdl, KernelMode, IoReadAccess);
		PVOID Mapping = MmMapLockedPagesSpecifyCache(Mdl, KernelMode, MmNonCached, NULL, FALSE, NormalPagePriority);
		MmProtectMdlSystemAddress(Mdl, PAGE_READWRITE);

		// Write your buffer to mapping:
		write_memory(Mapping, buffer, size);

		// Resources freeing:
		MmUnmapLockedPages(Mapping, Mdl);
		MmUnlockPages(Mdl);
		IoFreeMdl(Mdl);

		return true;
	}

	bool call_kernel_function(void* kernel_function_address) {
		if (!kernel_function_address)
			return false;

		PVOID* dxgk_routine
			= reinterpret_cast<PVOID*>(memory::get_system_module_export("\\SystemRoot\\System32\\drivers\\dxgkrnl.sys", "NtOpenCompositionSurfaceSectionInfo"));

		if (!dxgk_routine) {
			return false;
		}

		/*
		just overwrite the first 15 bytes of any function in dxgkrnl.sys ez
		*/

		BYTE dxgk_original[] = { 0x4C, 0x8B, 0xDC, 0x49, 0x89, 0x5B, 0x18, 0x4D, 0x89, 0x4B, 0x20, 0x49, 0x89, 0x4B, 0x08 };

		BYTE shell_code_start[]
		{
			0x48, 0xB8 // mov rax, [xxx]
		};

		BYTE shell_code_end[]
		{
			0xFF, 0xE0, // jmp rax
			0xCC //
		};

		RtlSecureZeroMemory(&dxgk_original, sizeof(dxgk_original));
		memcpy((PVOID)((ULONG_PTR)dxgk_original), &shell_code_start, sizeof(shell_code_start));
		uintptr_t test_address = reinterpret_cast<uintptr_t>(kernel_function_address);
		memcpy((PVOID)((ULONG_PTR)dxgk_original + sizeof(shell_code_start)), &test_address, sizeof(void*));
		memcpy((PVOID)((ULONG_PTR)dxgk_original + sizeof(shell_code_start) + sizeof(void*)), &shell_code_end, sizeof(shell_code_end));


		write_to_read_only_memory(dxgk_routine, &dxgk_original, sizeof(dxgk_original));

		return true;
	}

	bool write_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size) {
		if (!address || !buffer || !size)
			return false;

		NTSTATUS Status = STATUS_SUCCESS;
		PEPROCESS process;
		PsLookupProcessByProcessId(pid, &process);

		KAPC_STATE state;
		KeStackAttachProcess((PKPROCESS)process, &state);

		MEMORY_BASIC_INFORMATION info;

		Status = ZwQueryVirtualMemory(ZwCurrentProcess(), (PVOID)address, MemoryBasicInformation, &info, sizeof(info), NULL);
		if (!NT_SUCCESS(Status)) {
			KeUnstackDetachProcess(&state);
			return false;
		}

		if (((uintptr_t)info.BaseAddress + info.RegionSize) < (address + size))
		{
			KeUnstackDetachProcess(&state);
			return false;
		}

		if (!(info.State & MEM_COMMIT) || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
		{
			KeUnstackDetachProcess(&state);
			return false;
		}

		if ((info.Protect & PAGE_EXECUTE_READWRITE) || (info.Protect & PAGE_EXECUTE_WRITECOPY) || (info.Protect & PAGE_READWRITE) || (info.Protect & PAGE_WRITECOPY))
		{
			RtlCopyMemory((void*)address, buffer, size);
		}
		KeUnstackDetachProcess(&state);
		return true;
	}

	NTSTATUS protect_virtual_memory(ULONG64 pid, PVOID address, ULONG size, ULONG protection, ULONG& protection_out)
	{
		if (!pid || !address || !size || !protection)
			return STATUS_INVALID_PARAMETER;

		NTSTATUS status = STATUS_SUCCESS;
		PEPROCESS target_process = nullptr;

		if (!NT_SUCCESS(PsLookupProcessByProcessId(reinterpret_cast<HANDLE>(pid), &target_process)))
		{
			return STATUS_NOT_FOUND;
		}

		//PVOID address = reinterpret_cast<PVOID>( memory_struct->address );
		//ULONG size = (ULONG)( memory_struct->size );
		//ULONG protection = memory_struct->protection;
		ULONG protection_old = 0;

		KAPC_STATE state;
		KeStackAttachProcess(target_process, &state);

		status = ZwProtectVirtualMemory(NtCurrentProcess(), &address, &size, protection, &protection_old);

		KeUnstackDetachProcess(&state);

		if (NT_SUCCESS(status))
			protection_out = protection_old;

		ObDereferenceObject(target_process);
		return status;
	}

	bool read_kernel_memory(HANDLE pid, uintptr_t address, void* buffer, SIZE_T size) {

		if (!address || !buffer || !size)
			return false;

		SIZE_T bytes = size;
		NTSTATUS status = STATUS_SUCCESS;
		PEPROCESS process;

		if (!NT_SUCCESS(PsLookupProcessByProcessId(pid, &process)))
		{
			status = STATUS_NOT_FOUND;
			return false;
		}
		
		//DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[KDriv] Attempting to read %d bytes from 0x%p", size, address);

		__try 
		{
			status = MmCopyVirtualMemory(process, (void*)address, (PEPROCESS)PsGetCurrentProcess(), buffer, size, KernelMode, &bytes);
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return false;
		}

		//DumpHex(buffer, size);

		if (!NT_SUCCESS(status))
			return false;
		else
			return true;
	}

	ULONG64 get_module_base_x64(PEPROCESS proc, UNICODE_STRING module_name) {
		PPEB pPeb = PsGetProcessPeb(proc);

		if (!pPeb) {
			return 0; // failed
		}

		KAPC_STATE state;

		KeStackAttachProcess(proc, &state);

		PPEB_LDR_DATA pLdr = (PPEB_LDR_DATA)pPeb->Ldr;

		if (!pLdr) {
			KeUnstackDetachProcess(&state);
			return 0; // failed
		}

		// loop the linked list
		for (PLIST_ENTRY list = (PLIST_ENTRY)pLdr->ModuleListLoadOrder.Flink;
			list != &pLdr->ModuleListLoadOrder; list = (PLIST_ENTRY)list->Flink) {
			PLDR_DATA_TABLE_ENTRY pEntry =
				CONTAINING_RECORD(list, LDR_DATA_TABLE_ENTRY, InLoadOrderModuleList);
			if (RtlCompareUnicodeString(&pEntry->BaseDllName, &module_name, TRUE) ==
				0) {
				ULONG64 baseAddr = (ULONG64)pEntry->DllBase;
				KeUnstackDetachProcess(&state);
				return baseAddr;
			}
		}
		KeUnstackDetachProcess(&state);

		return 0;
	}
}