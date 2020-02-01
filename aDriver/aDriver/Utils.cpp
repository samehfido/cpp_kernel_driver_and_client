#include "Utils.h"

extern uint64_t offset1;

//Получаем базовый адресс процесса оп PID
uint64_t handle_get_base_address(int pid)
{
	//Создаем переменную process с нулевым указателем
	PEPROCESS process = nullptr;
	//Получаем status (ответ) от функции и если все удачно передаем в процесс
	//ссылочный указатель на структуру EPROCESS процесса process.
	NTSTATUS status = PsLookupProcessByProcessId(HANDLE(pid), &process);

	//Если не получили выходим
	if (!NT_SUCCESS(status))
	{
		return 0;
	}

	//Получаем базовый адресс процесса
	const auto base_address = uint64_t(PsGetProcessSectionBaseAddress(process));
	ObDereferenceObject(process); //Уничтожаем ссылку на процесс

	return base_address; //Возвращаем базовый адресс
}


/*
 * Получаем значение Dword из реестра
 */
NTSTATUS getRegDword(IN ULONG  RelativeTo, IN PWSTR  Path, IN PWSTR ParameterName, IN OUT PULONG ParameterValue)
{
	//предоставляет информацию о значении реестра
	RTL_QUERY_REGISTRY_TABLE  paramTable[2];

	//Если не были переданы параметры - возвращает ошибку
	if ((NULL == Path) || (NULL == ParameterName) || (NULL == ParameterValue)) {
		return STATUS_INVALID_PARAMETER;
	}

	//заполняет блок памяти нулями, учитывая указатель на блок и длину в байтах для заполнения.
	RtlZeroMemory(paramTable, sizeof(paramTable));

	
	ULONG defaultValue = *ParameterValue;

	paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
	paramTable[0].Name = ParameterName;
	paramTable[0].EntryContext = ParameterValue;
	paramTable[0].DefaultType = REG_DWORD;
	paramTable[0].DefaultData = &defaultValue;
	paramTable[0].DefaultLength = sizeof(ULONG); //sizeof(ULONGLONG);

	//позволяет вызывающей стороне запрашивать несколько значений из поддерева реестра с помощью одного вызова.
	NTSTATUS status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
		Path,
		&paramTable[0],
		NULL,
		NULL);

	
	if (status != STATUS_SUCCESS) {
		//DbgPrintEx(0, 0, "FAILED w/status=%x\n", status);
	}

	return status;
}

/*
 * Получаем значение Qword из реестра
 */
NTSTATUS getRegQword(IN ULONG  RelativeTo, IN PWSTR  Path, IN PWSTR ParameterName, IN OUT PULONGLONG ParameterValue)
{
	RTL_QUERY_REGISTRY_TABLE  paramTable[2]; 

	if ((NULL == Path) || (NULL == ParameterName) || (NULL == ParameterValue)) {
		return STATUS_INVALID_PARAMETER;
	}

	RtlZeroMemory(paramTable, sizeof(paramTable));

	ULONGLONG defaultValue = *ParameterValue;

	paramTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
	paramTable[0].Name = ParameterName;
	paramTable[0].EntryContext = ParameterValue;
	paramTable[0].DefaultType = REG_QWORD;
	paramTable[0].DefaultData = &defaultValue;
	paramTable[0].DefaultLength = sizeof(ULONGLONG);

	NTSTATUS status = RtlQueryRegistryValues(RTL_REGISTRY_ABSOLUTE,
		Path,
		&paramTable[0],
		NULL,
		NULL);

	if (status != STATUS_SUCCESS) {
		//DbgPrintEx(0, 0, "FAILED w/status=%x\n", status);
	}

	return status;
}



/*
 * Устанавливаем прослушку на пакеты
 */
bool listen(int tpid, uint64_t packetAdress, const Packet& out, PEPROCESS destProcess)
{
	PEPROCESS srcProcess = nullptr;

	//Получаем status (ответ) от функции и если все удачно передаем в процесс
	//ссылочный указатель на структуру EPROCESS процесса process.
	if (!NT_SUCCESS(PsLookupProcessByProcessId(HANDLE(tpid), &srcProcess)))
	{
		return 0;
	}

	SIZE_T retsz = 0;

	//Получаем данные из нашего пакета в out.
	/*
	 *	srcProcess - процесс найденный по ID (клиентский) // ОТКУДА МЫ СЧИТЫВАЕ - ПРОЦЕСС
	 *	(void*)(base + offset1) - источник пакета (наш клиент) // ОТКУДА МЫ СЧИТЫВАЕ - ОБЛАСТЬ ПАМЯТИ
	 *	destProcess - целевой процесс - наш драйвер // ОТКУДА МЫ ЗАПИСЫВАЕМ - ПРОЦЕСС
	 *	(void*)&out - куда будет считываться наш пакет // ОТКУДА МЫ ЗАПИСЫВАЕМ - ОБЛАСТЬ ПАМЯТИ
	 *	sizeof(Packet) - размер считвающейся памяти // РАЗМЕР БУФФЕРА
	 *	KernelMode - режим ядра	// РЕЖИМ
	 *	&retsz - размер // ВОЗВРАЩАЕМЫЙ РАЗМЕР
	 */
	NTSTATUS ret = MmCopyVirtualMemory(srcProcess, (void*)(packetAdress), destProcess, (void*)&out, sizeof(Packet), KernelMode, &retsz);

	//ObDereferenceObject(destProcess);
	//Очищаем указатель
	ObDereferenceObject(srcProcess);

	if (ret == STATUS_SUCCESS)
		return 1;
	else
		return 0;
}


/*
 * Универсальная функция для чтения и записи из пакета в требуемую память
 */
void handlememory_meme(const Packet& in)
{
	PEPROCESS destProcess = nullptr;
	PEPROCESS srcProcess = nullptr;

    //Получаем указатели на процесс по dest_pid
	if (!NT_SUCCESS(PsLookupProcessByProcessId(HANDLE(in.dest_pid), &destProcess)))
		return;
	//Получаем указатели на процесс по source_pid
	if (!NT_SUCCESS(PsLookupProcessByProcessId(HANDLE(in.source_pid), &srcProcess)))
		return;

	SIZE_T retsz = 0;

	//DbgPrintEx(0, 0, ">handling memory operation..\n");
	//DbgPrintEx(0, 0, "Source pid: %i,@: 0x%p\n",in.source_pid,in.source);
	//DbgPrintEx(0, 0, "Dest   pid: %i,@: 0x%p\n", in.dest_pid, in.destination);
	//DbgPrintEx(0, 0, "size of data: %i\n",in.size);

	MmCopyVirtualMemory(srcProcess, (void*)in.source, destProcess, (void*)in.destination, in.size, KernelMode, &retsz);

	//if (status == STATUS_SUCCESS)
		//DbgPrintEx(0, 0, ">success\n");
	//else
		//DbgPrintEx(0, 0, ">failed\n");

	ObDereferenceObject(destProcess);
	ObDereferenceObject(srcProcess);
}


/*
 * Функция устанавливает статус пакета
 */
void writeopcode(int tpid, uint32_t opcode, uint64_t packetAdress, PEPROCESS srcProcess)
{
	PEPROCESS destProcess = nullptr;
	//PEPROCESS srcProcess = PsGetCurrentProcess();

	if (!NT_SUCCESS(PsLookupProcessByProcessId(HANDLE(tpid), &destProcess)))
	{
		return;
	}

	SIZE_T retsz = 0;

	MmCopyVirtualMemory(srcProcess, (void*)&opcode, destProcess, (void*)(packetAdress), sizeof(uint32_t), KernelMode, &retsz);

	ObDereferenceObject(destProcess);
	//ObDereferenceObject(srcProcess);
}


/*
 * Функция для получения базового адреса
 */
void handle_base_packet(const Packet& in, int tpid, PEPROCESS srcProcess)
{
	static uint64_t base = handle_get_base_address(in.dest_pid);

	//DbgPrintEx(0, 0, "->got base: 0x%p\n", base);

	PEPROCESS destProcess = nullptr;
	//PEPROCESS srcProcess = PsGetCurrentProcess();

	if (!NT_SUCCESS(PsLookupProcessByProcessId(HANDLE(tpid), &destProcess)))
	{
		return;
	}

	SIZE_T retsz = 0;

	MmCopyVirtualMemory(srcProcess, (void*)&base, destProcess, (void*)in.destination, sizeof(uint64_t), KernelMode, &retsz);

	ObDereferenceObject(destProcess);
	//ObDereferenceObject(srcProcess);
}

//Обнуление пакета
void wipepacket(Packet& in) noexcept
{
	in.destination = 0;
	in.dest_pid = 0;
	in.opcode = OP_INVALID;
	in.size = 0;
	in.source = 0;
	in.source_pid = 0;
}