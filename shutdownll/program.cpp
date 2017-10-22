#include <string>
#include <iostream>
#include <algorithm>
#include <Windows.h>

using namespace std;

enum shutdown_action
{
	shutdown_no_reboot,
	shutdown_reboot,
	shutdown_power_off
};

using NtShutdownSystem_t = NTSTATUS(NTAPI *)(shutdown_action);

void enable_privilege(string privilege_name)
{
	HANDLE token_handle;
	if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &token_handle))
		throw runtime_error("OpenProcessToken failed: " + to_string(GetLastError()));

	LUID luid;
	if (!LookupPrivilegeValue(nullptr, privilege_name.c_str(), &luid))
	{
		CloseHandle(token_handle);
		throw runtime_error("LookupPrivilegeValue failed: " + to_string(GetLastError()));
	}

	TOKEN_PRIVILEGES tp;
	tp.PrivilegeCount = 1;
	tp.Privileges[0].Luid = luid;
	tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
	if (!AdjustTokenPrivileges(token_handle, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
	{
		CloseHandle(token_handle);
		throw runtime_error("AdjustTokenPrivilege failed: " + to_string(GetLastError()));
	}
}

int main(int argc, char* argv[])
{
	shutdown_action action;
	try
	{
		if (argc == 1)
			action = shutdown_power_off;
		else if (argc == 2)
		{
			auto arg = string(argv[1]);
			transform(arg.begin(), arg.end(), arg.begin(), towlower);
			if (arg == "-s" || arg == "/s")
				action = shutdown_power_off;
			else if (arg == "-r" || arg == "/r")
				action = shutdown_reboot;
			else
				throw invalid_argument("Invalid argument: " + arg);
		}
		else
			throw invalid_argument("Invalid argument.");

		enable_privilege(SE_SHUTDOWN_NAME);
		const auto NtShutdownSystem = reinterpret_cast<NtShutdownSystem_t>(GetProcAddress(
			GetModuleHandle("ntdll.dll"), "NtShutdownSystem"));
		NtShutdownSystem(action);
	}
	catch (exception e)
	{
		cout << e.what() << endl;
	}

	return 0;
}
