#include "memory.h"
#include "cleaner.h"
#include "imports.h"

NTSTATUS hook_handler(PVOID called_param)
{
	COPY_MEMORY* m = (COPY_MEMORY*)called_param;	

	if ( m->change_protection )
	{
		memory::protect_virtual_memory( m->pid, (PVOID)m->address, m->size, m->protection, m->protection_old );
	}

	if (m->ReqBase != FALSE) {

		DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[KDriv] Requesting Base Address for ProcessID: %lu", m->pid);

		ANSI_STRING AS;
		UNICODE_STRING ModuleNAme;
		  
		RtlInitAnsiString(&AS, m->moduleName);
		RtlAnsiStringToUnicodeString(&ModuleNAme, &AS, TRUE);

		PEPROCESS process;
		if (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)m->pid, &process)))
			return 0;

		ULONG64 Base_Address64 = NULL;
		Base_Address64 = memory::get_module_base_x64(process, ModuleNAme);
		
		m->BaseAddress = Base_Address64;
		RtlFreeUnicodeString(&ModuleNAme);
	}

	if (m->write != FALSE) {
		PVOID kernelBuff = ExAllocatePool(NonPagedPool, m->size);

		if (!kernelBuff) 
			return STATUS_UNSUCCESSFUL;
		
			
		if (!memcpy(kernelBuff, m->bufferAddress, m->size))
			return STATUS_UNSUCCESSFUL;
		
		PEPROCESS Process;
		PsLookupProcessByProcessId((HANDLE)m->pid, &Process);
		memory::write_kernel_memory((HANDLE)m->pid, m->address, kernelBuff, m->size);
		ExFreePool(kernelBuff);
	}

	if(m->Read != FALSE)
	{
		void* ReadOutput = NULL;

		if (!memory::read_kernel_memory((HANDLE)m->pid, m->address, &ReadOutput, m->size))
		{
			DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[KDriv] Read failed!", m->pid);
			return STATUS_UNSUCCESSFUL;
		}

		m->Output = ReadOutput;
	}

	if (m->ClearPIDCache != FALSE) {

		if (cleaner::clean_piddbcachetalbe() != FALSE) {
			m->PIDCacheCleared = TRUE;
		}
		else
		{
			m->PIDCacheCleared = FALSE;
		}
	}

	if (m->ReadString != FALSE) {
		PVOID kernelBuffer = ExAllocatePoolWithTag(NonPagedPool, m->size, 0x454E4F45); // ENON

		if (!kernelBuffer)
			return STATUS_UNSUCCESSFUL;

		if (!memcpy(kernelBuffer, m->bufferAddress, m->size))
			return STATUS_UNSUCCESSFUL;

		if (!memory::read_kernel_memory((HANDLE)m->pid, m->address, kernelBuffer, m->size))
			return STATUS_UNSUCCESSFUL;

		RtlZeroMemory(m->bufferAddress, m->size);

		if (!memcpy(m->bufferAddress, kernelBuffer, m->size))
			return STATUS_UNSUCCESSFUL;
		
		ExFreePool(kernelBuffer);
	}

	if (m->WriteString != FALSE) {
		PVOID kernelBuffer1 = ExAllocatePool(NonPagedPool, m->size);

		if (!kernelBuffer1)
			return STATUS_UNSUCCESSFUL;


		if (!memcpy(kernelBuffer1, m->bufferAddress, m->size))
			return STATUS_UNSUCCESSFUL;

		PEPROCESS Process;
		PsLookupProcessByProcessId((HANDLE)m->pid, &Process);
		memory::write_kernel_memory((HANDLE)m->pid, m->address, kernelBuffer1, m->size);

		ExFreePool(kernelBuffer1);
	}

	return STATUS_SUCCESS;
}

extern "C" NTSTATUS DriverEntry(
	PDRIVER_OBJECT  driver_object,
	PUNICODE_STRING registry_path)
{
	cleaner::clean_piddbcachetalbe();
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[KDriv] PiDDBCacheTable Cleared");

	// These are invalid for mapped drivers.
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(registry_path);

	memory::call_kernel_function(&hook_handler);
	
	DbgPrintEx(DPFLTR_IHVDRIVER_ID, DPFLTR_ERROR_LEVEL, "[KDriv] DriverEntry OK");

	return STATUS_SUCCESS;
}