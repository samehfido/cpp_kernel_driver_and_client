#include <ntddk.h>
#include "cImports.h"
#include "Utils.h"

//#define ABSOLUTE(wait) (wait)
//#define RELATIVE(wait) (-(wait))
//#define NANOSECONDS(nanos) \
//(((signed __int64)(nanos)) / 100L)
//#define MICROSECONDS(micros) \
//(((signed __int64)(micros)) * NANOSECONDS(1000L))
//#define MILLISECONDS(milli) \
//(((signed __int64)(milli)) * MICROSECONDS(1000L))
//#define SECONDS(seconds) \
//(((signed __int64)(seconds)) * MILLISECONDS(1000L))


uint64_t offset1 = 0x0, offset2 = 0x0, offset3 = 0x0;// Получаем смещение для работы коммуникации (там где лежит наш пакет P с опкодами и данными) под 3 параметра
KTIMER timer1, timer2, timer3; //Время выполнения ядра

//Открываем наш поток
void NTAPI thread1(void*)
{
	//инициализирует расширенный объект таймера ядра.
	KeInitializeTimerEx(&timer1, TIMER_TYPE::SynchronizationTimer); // 1ms
	const LARGE_INTEGER due = { 0 };
	/*
	 *  KeSetTimerEx устанавливает абсолютный или относительный интервал, с которым объект таймера должен быть установлен
	 *  в сигнальное состояние, дополнительно предоставляет подпрограмму CustomTimerDpc, которая будет выполняться по
	 *  истечении этого интервала, и при необходимости предоставляет повторяющийся интервал для таймера.
	 */
	KeSetTimerEx(&timer1, due, 1, nullptr);

	unsigned long client_pid = 0; // ID процесса clientV2
	bool b_gotBaseFlag = false; // Флаг получения базового адреса процесса
	uint64_t base = 0; // Сам базовый адресс процесса
	Packet p{}; // Создаем переменную p для коммуникации

	PEPROCESS destProcess = PsGetCurrentProcess(); //Возвращает указатель на процесс текущего потока.

	//DbgPrintEx(0, 0, "Thread #1 START");
	
	while (true)
	{
		//Подпрограмма переводит текущий поток в состояние ожидания, пока данный объект диспетчера не будет
		//установлен в сигнальное состояние или (необязательно), пока время ожидания не истечет.
		//Если * Timeout = 0, процедура возвращается без ожидания.
		KeWaitForSingleObject(&timer1, Executive, KernelMode, FALSE, 0);

		//Получаем значение из реестра по ключу pid  и заполняем pid переменную
		getRegDword(0, L"\\Registry\\Machine\\SOFTWARE\\WowAesdt", L"pid", &client_pid);
		//Получаем значение из реестра по ключу offset1 и заполняем offset1 переменную
		getRegQword(0, L"\\Registry\\Machine\\SOFTWARE\\WowAesdt", L"offset1", &offset1);


		//отправляем строку в отладчик ядра, если указанные вами условия выполнены.
		//DbgPrintEx(0, 0, "THREAD1 ->pid - %i, offset1 - 0x%p", client_pid, offset1);

		//Проверка выставлен ли PID и OFFSET клиентом
		if (client_pid > 0 && offset1)
		{
			if (!b_gotBaseFlag) //Если мы получили данные от клиента, выставляем флаг в TRUE
			{
				b_gotBaseFlag = true; //Устанавливаем флаг в положительное значение

				base = handle_get_base_address(client_pid); //Получаем базовый адресс

				//отправляем строку в отладчик ядра, если указанные вами условия выполнены.
				//DbgPrintEx(0, 0, "found um client - pid: %i - base: 0x%p - offset1: 0x%p\n", client_pid, base, offset1);
			}

			if (base) // Если найдена база, то ...
			{
				if (listen(client_pid, base + offset1, p, destProcess)) //читаем пакет в переменную p из клиента
				{
					if (p.opcode == OP_READ)
					{
						//DbgPrintEx(0, 0, "THREAD1 ->opcode READ: %i\n", p.opcode);
						handlememory_meme(p); //читаем из памяти
						writeopcode(client_pid, OP_DONE, base + offset1, destProcess);
					}

					if (p.opcode == OP_WRITE)
					{
						//DbgPrintEx(0, 0, "THREAD1 ->opcode WRITE: %i\n", p.opcode);
						handlememory_meme(p); //записываем в память
						writeopcode(client_pid, OP_DONE, base + offset1, destProcess);
					}

					if (p.opcode == OP_BASE)
					{
						//DbgPrintEx(0, 0, "THREAD1 ->opcode BASE: %i\n", p.opcode);
						//DbgPrintEx(0, 0, "THREAD1 ->pid: %i, dest: 0x%p\n", p.dest_pid, p.destination);
						handle_base_packet(p, client_pid, destProcess); //Получение базового адреса
						writeopcode(client_pid, OP_DONE, base + offset1, destProcess);
					}
				}
			}
		}
		else
		{
			b_gotBaseFlag = false;
		}
	}
	KeCancelTimer(&timer1);
}

void NTAPI thread2(void*)
{
	//инициализирует расширенный объект таймера ядра.
	KeInitializeTimerEx(&timer2, TIMER_TYPE::SynchronizationTimer); // 1ms
	const LARGE_INTEGER due = { 0 };
	/*
	 *  KeSetTimerEx устанавливает абсолютный или относительный интервал, с которым объект таймера должен быть установлен
	 *  в сигнальное состояние, дополнительно предоставляет подпрограмму CustomTimerDpc, которая будет выполняться по
	 *  истечении этого интервала, и при необходимости предоставляет повторяющийся интервал для таймера.
	 */
	KeSetTimerEx(&timer2, due, 1, nullptr);

	unsigned long client_pid = 0; // ID процесса clientV2
	bool b_gotBaseFlag = false; // Флаг получения базового адреса процесса
	uint64_t base = 0; // Сам базовый адресс процесса
	Packet p{}; // Создаем переменную p для коммуникации	

	PEPROCESS destProcess = PsGetCurrentProcess(); //Возвращает указатель на процесс текущего потока.

	//DbgPrintEx(0, 0, "Thread #2 START");
	
	while (true)
	{
		//Подпрограмма переводит текущий поток в состояние ожидания, пока данный объект диспетчера не будет
		//установлен в сигнальное состояние или (необязательно), пока время ожидания не истечет.
		//Если * Timeout = 0, процедура возвращается без ожидания.
		KeWaitForSingleObject(&timer2, Executive, KernelMode, FALSE, 0);

		//Получаем значение из реестра по ключу pid  и заполняем pid переменную
		getRegDword(0, L"\\Registry\\Machine\\SOFTWARE\\WowAesdt", L"pid", &client_pid);
		//Получаем значение из реестра по ключу offset1 и заполняем offset1 переменную
		getRegQword(0, L"\\Registry\\Machine\\SOFTWARE\\WowAesdt", L"offset2", &offset2);


		//отправляем строку в отладчик ядра, если указанные вами условия выполнены.
		//DbgPrintEx(0, 0, "THREAD2 ->pid - %i, offset2 - 0x%p", client_pid, offset2);

		//Проверка выставлен ли PID и OFFSET клиентом
		if (client_pid > 0 && offset2)
		{
			if (!b_gotBaseFlag) //Если мы получили данные от клиента, выставляем флаг в TRUE
			{
				b_gotBaseFlag = true; //Устанавливаем флаг в положительное значение

				base = handle_get_base_address(client_pid); //Получаем базовый адресс

				//отправляем строку в отладчик ядра, если указанные вами условия выполнены.
				//DbgPrintEx(0, 0, "found um client - pid: %i - base: 0x%p - offset1: 0x%p\n", client_pid, base, offset2);
			}

			if (base) // Если найдена база, то ...
			{
				if (listen(client_pid, base + offset2, p, destProcess)) //читаем пакет в переменную p из клиента
				{
					if (p.opcode == OP_READ)
					{
						//DbgPrintEx(0, 0, "THREAD2 ->opcode READ: %i\n", p.opcode);
						handlememory_meme(p); //читаем из памяти
						writeopcode(client_pid, OP_DONE, base + offset2, destProcess);
					}

					if (p.opcode == OP_WRITE)
					{
						//DbgPrintEx(0, 0, "THREAD2 ->opcode WRITE: %i\n", p.opcode);
						handlememory_meme(p); //записываем в память
						writeopcode(client_pid, OP_DONE, base + offset2, destProcess);
					}

					if (p.opcode == OP_BASE)
					{
						//DbgPrintEx(0, 0, "THREAD2 ->opcode BASE: %i\n", p.opcode);
						//DbgPrintEx(0, 0, "THREAD2 ->pid: %i, dest: 0x%p\n", p.dest_pid, p.destination);
						handle_base_packet(p, client_pid, destProcess); //Получение базового адреса
						writeopcode(client_pid, OP_DONE, base + offset2, destProcess);
					}
				}
			}
		}
		else
		{
			b_gotBaseFlag = false;
		}
	}
	KeCancelTimer(&timer2);
}

void NTAPI thread3(void*)
{
	//инициализирует расширенный объект таймера ядра.
	KeInitializeTimerEx(&timer3, TIMER_TYPE::SynchronizationTimer); // 1ms
	const LARGE_INTEGER due = { 0 };
	/*
	 *  KeSetTimerEx устанавливает абсолютный или относительный интервал, с которым объект таймера должен быть установлен
	 *  в сигнальное состояние, дополнительно предоставляет подпрограмму CustomTimerDpc, которая будет выполняться по
	 *  истечении этого интервала, и при необходимости предоставляет повторяющийся интервал для таймера.
	 */
	KeSetTimerEx(&timer3, due, 1, nullptr);

	unsigned long client_pid = 0; // ID процесса clientV2
	bool b_gotBaseFlag = false; // Флаг получения базового адреса процесса
	uint64_t base = 0; // Сам базовый адресс процесса
	Packet p{}; // Создаем переменную p для коммуникации	

	PEPROCESS destProcess = PsGetCurrentProcess(); //Возвращает указатель на процесс текущего потока.

	//DbgPrintEx(0, 0, "Thread #3 START");

	while (true)
	{
		//Подпрограмма переводит текущий поток в состояние ожидания, пока данный объект диспетчера не будет
		//установлен в сигнальное состояние или (необязательно), пока время ожидания не истечет.
		//Если * Timeout = 0, процедура возвращается без ожидания.
		KeWaitForSingleObject(&timer3, Executive, KernelMode, FALSE, 0);

		//Получаем значение из реестра по ключу pid  и заполняем pid переменную
		getRegDword(0, L"\\Registry\\Machine\\SOFTWARE\\WowAesdt", L"pid", &client_pid);
		//Получаем значение из реестра по ключу offset1 и заполняем offset1 переменную
		getRegQword(0, L"\\Registry\\Machine\\SOFTWARE\\WowAesdt", L"offset3", &offset3);


		//отправляем строку в отладчик ядра, если указанные вами условия выполнены.
		//DbgPrintEx(0, 0, "THREAD3 ->pid - %i, offset3 - 0x%p", client_pid, offset3);

		//Проверка выставлен ли PID и OFFSET клиентом
		if (client_pid > 0 && offset3)
		{
			if (!b_gotBaseFlag) //Если мы получили данные от клиента, выставляем флаг в TRUE
			{
				b_gotBaseFlag = true; //Устанавливаем флаг в положительное значение

				base = handle_get_base_address(client_pid); //Получаем базовый адресс

				//отправляем строку в отладчик ядра, если указанные вами условия выполнены.
				//DbgPrintEx(0, 0, "found um client - pid: %i - base: 0x%p - offset1: 0x%p\n", client_pid, base, offset3);
			}

			if (base) // Если найдена база, то ...
			{
				if (listen(client_pid, base + offset3, p, destProcess)) //читаем пакет в переменную p из клиента
				{
					if (p.opcode == OP_READ)
					{
						//DbgPrintEx(0, 0, "THREAD2 ->opcode READ: %i\n", p.opcode);
						handlememory_meme(p); //читаем из памяти
						writeopcode(client_pid, OP_DONE, base + offset3, destProcess);
					}

					if (p.opcode == OP_WRITE)
					{
						//DbgPrintEx(0, 0, "THREAD2 ->opcode WRITE: %i\n", p.opcode);
						handlememory_meme(p); //записываем в память
						writeopcode(client_pid, OP_DONE, base + offset3, destProcess);
					}

					if (p.opcode == OP_BASE)
					{
						//DbgPrintEx(0, 0, "THREAD2 ->opcode BASE: %i\n", p.opcode);
						//DbgPrintEx(0, 0, "THREAD2 ->pid: %i, dest: 0x%p\n", p.dest_pid, p.destination);
						handle_base_packet(p, client_pid, destProcess); //Получение базового адреса
						writeopcode(client_pid, OP_DONE, base + offset3, destProcess);
					}
				}
			}
		}
		else
		{
			b_gotBaseFlag = false;
		}
	}
	KeCancelTimer(&timer3);
}
/*
 * Точка входа и создание драйвера
 */
extern "C" NTSTATUS DriverEntry(PDRIVER_OBJECT driver_object, PUNICODE_STRING registry_path)
{
	// These are invalid for mapped drivers.
	UNREFERENCED_PARAMETER(driver_object);
	UNREFERENCED_PARAMETER(registry_path);


	//Подпрограмма входит в защищенную область, которая отключает всю доставку APC в режиме ядра в текущий поток.
	/*
	 * Асинхронный вызов процедуры (APC) - это функция, которая выполняется асинхронно в контексте определенного потока.
	 * Когда APC ставится в очередь в поток, система выдает программное прерывание.
	 * Сгенерированный системой APC называется APC в режиме ядра. APC, сгенерированный приложением,
	 * называется APC пользовательского режима.
	 */
	KeEnterGuardedRegion();

	//Выделяет пул памяти указанного типа и возвращает указатель на выделенный блок.
	PWORK_QUEUE_ITEM WorkItem1 = (PWORK_QUEUE_ITEM)ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
	if (!WorkItem1) return STATUS_UNSUCCESSFUL;

	//Инициализирует элемент рабочей очереди с предоставленным вызывающим контекстом и подпрограммой обратного вызова,
	//которая ставится в очередь для выполнения, когда системный рабочий поток получает контроль.
	//Попросту говоря генерирует новый поток
	ExInitializeWorkItem(WorkItem1, thread1, WorkItem1);

	//вставляет данный рабочий элемент в очередь, из которой системный рабочий поток удаляет элемент и
	//передает управление подпрограмме, предоставленной вызывающей стороной для ExInitializeWorkItem
	ExQueueWorkItem(WorkItem1, DelayedWorkQueue);


	//2й поток
	PWORK_QUEUE_ITEM WorkItem2 = (PWORK_QUEUE_ITEM)ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
	if (!WorkItem2) return STATUS_UNSUCCESSFUL;
	ExInitializeWorkItem(WorkItem2, thread2, WorkItem2);
	ExQueueWorkItem(WorkItem2, DelayedWorkQueue);
	//3й поток
	PWORK_QUEUE_ITEM WorkItem3 = (PWORK_QUEUE_ITEM)ExAllocatePool(NonPagedPool, sizeof(WORK_QUEUE_ITEM));
	if (!WorkItem3) return STATUS_UNSUCCESSFUL;
	ExInitializeWorkItem(WorkItem3, thread3, WorkItem3);
	ExQueueWorkItem(WorkItem3, DelayedWorkQueue);

	
	//KeLeaveGuardedRegion - покидаем данный регион
	KeLeaveGuardedRegion();

	//Остается открытым только поток
	return STATUS_SUCCESS;
}


