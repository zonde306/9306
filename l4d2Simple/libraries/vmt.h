#pragma once
#include <string>
#include <vector>
#include <cassert>
#include <exception>

class CVMT
{
public:
	/*void* Hook(void* Instance, int Index, void* HookedFunc)
	{
	
		DWORD VirtualTable = *(DWORD*)Instance;

		DWORD VirtualFunction = VirtualTable + sizeof(DWORD) * Index;
		intptr_t OriginalFunction = *((DWORD*)VirtualFunction);

		DWORD OldProtect;
		VirtualProtect((LPVOID)VirtualFunction, sizeof(DWORD), PAGE_EXECUTE_READWRITE, &OldProtect);
		*((DWORD*)VirtualFunction) = (DWORD)HookedFunc;
		VirtualProtect((LPVOID)VirtualFunction, sizeof(DWORD), OldProtect, &OldProtect);

		return (void*)OriginalFunction;
	
	}*/

	template<typename Function>
	static Function getvfunc(PVOID Base, DWORD Index)
	{
		DWORD **VTablePointer = nullptr, *VTableFunctionBase = nullptr, dwAddress = NULL;
		
		try
		{
			VTablePointer = (PDWORD*)Base;
			VTableFunctionBase = *VTablePointer;
			dwAddress = VTableFunctionBase[Index];
		}
		catch(...)
		{
			Utils::log("%s (%d) 错误：获取 0x%X 的虚函数表失败 %d", __FILE__, __LINE__, Base, Index);
			Utils::log("VTablePointer = 0x%X", VTablePointer);
			Utils::log("VTableFunctionBase = 0x%X", VTableFunctionBase);
			Utils::log("dwAddress = 0x%X", dwAddress);
			
#ifdef _DEBUG
			throw std::exception("getvfunc 出现了一些问题");
#endif

			return nullptr;
		}

		return (Function)(dwAddress);
	}

	static void* GetFunction(void* Instance, int Index)
	{
		DWORD VirtualPointer = NULL;
		void* VirtualFunction = nullptr;

		try
		{
			VirtualPointer = (*(DWORD*)Instance) + sizeof(DWORD) * Index;
			VirtualFunction = (void*)*((DWORD*)VirtualPointer);
		}
		catch(...)
		{
			// Utils::log("%s (%d) 错误：获取 0x%X 的虚函数表失败 %d", __FILE__, __LINE__, Instance, Index);
			// Utils::log("VirtualPointer = 0x%X", VirtualPointer);
			// Utils::log("VirtualFunction = 0x%X", VirtualFunction);

#ifdef _DEBUG
			throw std::exception("getvfunc 出现了一些问题");
#endif

			return nullptr;
		}

		return VirtualFunction;
	}

};

extern CVMT VMT;
#define k_page_writeable (PAGE_READWRITE | PAGE_EXECUTE_READWRITE)
#define k_page_readable (k_page_writeable|PAGE_READONLY|PAGE_WRITECOPY|PAGE_EXECUTE_READ|PAGE_EXECUTE_WRITECOPY)
#define k_page_offlimits (PAGE_GUARD|PAGE_NOACCESS)
/*Credits to null for helping me with this.*/

class CVMTHookManager
{
public:
	CVMTHookManager()
	{}
	/*Deconstructor*/
	~CVMTHookManager()
	{
		this->HookTable(false);
	}

	CVMTHookManager(void* Interface)
	{
		this->Init(Interface);
	}

	/*Inits the function*/
	bool Init(void* Interface)
	{
		pOrgTable = *(void**)Interface;
		this->count = this->GetCount();
		pCopyTable = malloc(sizeof(void*)* count);
		memcpy(pCopyTable, pOrgTable, sizeof(void*) * count);
		pObject = (DWORD*)Interface;
		return true;
	}
	/*Hook/Unhook*/
	bool HookTable(bool hooked)
	{
		try
		{
			if (hooked)
			{
				*pObject = (DWORD)pCopyTable;
			}
			else
			{
				*pObject = (DWORD)pOrgTable;
			}
		}
		catch (std::exception e)
		{
			return false;
		}

		return true;
	}

	/*Hooks function*/
	void* HookFunction(int Index, void* hkFunction)
	{
		if (Index < this->count && Index >= 0)
		{
			((DWORD*)pCopyTable)[Index] = (DWORD)hkFunction;
			return (void*)((DWORD*)pOrgTable)[Index];
		}
		return NULL;
	}

	void* GetOriginalFunction(int Index)
	{
		if (Index < this->count && Index >= 0)
			return (void*)((DWORD*)pOrgTable)[Index];
		
		return NULL;
	}

	template<typename Fn>
	Fn* SetupHook(int Index, Fn* hkFunction)
	{
		if (Index < this->count && Index >= 0)
		{
			((DWORD*)pCopyTable)[Index] = (DWORD)hkFunction;
			return (Fn*)((DWORD*)pOrgTable)[Index];
		}

		return nullptr;
	}

	template<typename Fn>
	Fn* GetFunction(int Index)
	{
		if (Index < this->count && Index >= 0)
			return (Fn*)((DWORD*)pOrgTable)[Index];

		return nullptr;
	}

private:
	/*Returns if you can read the pointer*/
	bool CanReadPointer(void* table)
	{
		MEMORY_BASIC_INFORMATION mbi;
		if (table == nullptr) return false;
		if (!VirtualQuery(table, &mbi, sizeof(mbi))) return false;
		if (mbi.Protect & k_page_offlimits) return false;
		return (mbi.Protect & k_page_readable);
	}
	/*Gets VMT count*/
	int GetCount()
	{
		int index = 0;
		void** table = ((void**)pOrgTable);
		try
		{
			for (void* fn; (fn = table[index]) != nullptr; index++)
			{
				if (!this->CanReadPointer(fn)) break;
			}
		}
		catch(...)
		{
			// Utils::log("%s (%d) 错误：遍历虚函数表失败 0x%X 索引为 %d", __FILE__, __LINE__, (DWORD)table, index--);
		}
		
		return index;
	}
	int count;
	void* pCopyTable;
	DWORD* pObject;
	void* pOrgTable;
};