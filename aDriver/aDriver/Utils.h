#pragma once
#include "cImports.h"


//Создаем структуру наших пакетов

struct Packet
{
	int opcode = 0; // Получаемая команда
	uint64_t source = 0; // 
	uint64_t destination = 0; // Адресс откуда или куда будет производиться запись

	uint32_t source_pid = 0; // id процесса откуда получаем данные
	uint32_t dest_pid = 0; // id процесса куда нужно их записать/считать

	uint32_t size = 0; // размер считываемой или записываемой памяти
};

enum opcodes // Перечисление наших кодов для управление драйвером
{
	OP_INVALID = 0,
	OP_READ = 1,
	OP_WRITE = 2,
	OP_BASE = 3,
	OP_DONE = 9
};


/*
 *Получаем базовый адресс процесса оп PID
 */
uint64_t handle_get_base_address(int pid); 

/*
 * Получаем значение Dword из реестра
 */
NTSTATUS getRegDword(IN ULONG  RelativeTo, IN PWSTR  Path, IN PWSTR ParameterName, IN OUT PULONG ParameterValue);

/*
 * Получаем значение Dword из реестра
 */
NTSTATUS getRegQword(IN ULONG  RelativeTo, IN PWSTR  Path, IN PWSTR ParameterName, IN OUT PULONGLONG ParameterValue);

/*
 * Устанавливаем прослушку на пакеты
 */
bool listen(int tpid, uint64_t packetAdress, const Packet& out, PEPROCESS destProcess);



/*
 * Универсальная функция для чтения и записи из пакета в требуемую память
 */
void handlememory_meme(const Packet& in);



/*
 * Функция устанавливает статус пакета
 */
void writeopcode(int tpid, uint32_t opcode, uint64_t packetAdress, PEPROCESS srcProcess);



/*
 * Функция для получения базового адреса
 */
void handle_base_packet(const Packet& in, int tpid, PEPROCESS srcProcess);



//Обнуление пакета
void wipepacket(Packet& in) noexcept;