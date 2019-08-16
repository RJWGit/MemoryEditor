#include <iostream>
#include <windows.h>
#include <string>
#include <psapi.h>
#include <stdio.h>
#include <ctype.h>
#include <vector>

bool CompareFilenames(char file1[], char file2[]);
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam);
DWORD GetProcID(HWND hWnd);
std::vector<MODULEINFO> MainModule(HANDLE handle);
void DisplayMatches(std::vector<DWORD64> matches);
void UserTypeChoice(std::vector<DWORD64> &matches, HANDLE handle, std::vector<MODULEINFO> mod);


//Search for desired user-defined value
template <class myType>
void Search(myType search, std::vector<DWORD64> &matches, HANDLE handle, MEMORY_BASIC_INFORMATION info, DWORD64 regionoffset, LPVOID baseadress) {

	std::vector<myType> buffer;
	buffer.resize(info.RegionSize);

	if (0 != ReadProcessMemory(handle, (LPCVOID)((DWORD64)(baseadress)+regionoffset), &buffer[0], info.RegionSize, NULL)) {
		void *ptr = &buffer[0];
		
		//Search for potential matches and store address in 'matches' if value == search value
		for (int i = 0; i < info.RegionSize; i++) {
			if (*(myType*)ptr == search) {
				matches.push_back((DWORD64)(baseadress)+i + regionoffset);

			}
			//Increase pointer by 1 byte
			ptr = static_cast<char*>(ptr) + 1;
		}

	}

	else {
		DWORD error = GetLastError();
		std::cout << "ERROR WITH READPROCESSMEMORY, FUNCTION: SEARCH" << std::endl;
		std::cout << "ERROR CODE: " << error << std::endl;
		std::cin.get();
	}
}

//Initial scan of foreign process memory 
template <class myType>
void ScanMemory(myType find, std::vector<DWORD64> &matches, HANDLE handle, std::vector<MODULEINFO> mod) {

	matches.resize(0);
	DWORD64 regionoffset = 0;
	MEMORY_BASIC_INFORMATION info;
	std::vector<MODULEINFO>::iterator it;
	std::cout << "number of mods in process:" << mod.size() << std::endl;

	//Scan through all the modules possible foreign process memory module by module, then pass information to 'search' function
	for (it = mod.begin(); it != mod.end(); it++) {
		regionoffset = 0;

		//Iterate through the module's memory and check if pages have state of 'MEM_COMMIT'
		//Keep adding size of memory found to 'regionoffset' until it is greater than the sizeof the module
		while (regionoffset < it->SizeOfImage) {

			//Returns a block of memory with all identical blocks of memory, such as 'MEM_COMMIT'. 'sizeof(info)' will always return a block of memory with identical page states
			if (0 != VirtualQueryEx(handle, (LPCVOID)((DWORD64)(it->lpBaseOfDll) + regionoffset), &info, sizeof(info))) {

				if (info.State == MEM_COMMIT)
				{
					Search(find, matches, handle, info, regionoffset, it->lpBaseOfDll);
				}

				
				regionoffset += info.RegionSize;

			}

			else {
				DWORD error = GetLastError();
				std::cout << "ERROR WITH VIRTUAL QUERY EX" << std::endl;
				std::cout << "ERROR CODE: " << error << std::endl;
				std::cin.get();
			}
		}

	}

	std::cout << "Found: " << matches.size() << " possible matches\n";
}

//Should be only called after intial scan, compares the possible matches found against a new search and checks if old matches are equal to user new input value
template <class myType>
void NarrowDownList(myType search, std::vector<DWORD64> &matches, HANDLE handle){

	myType buffer;
	std::vector<DWORD64> tempvec;

	for (int i = 0; i < matches.size(); i++) {
		if (0 != ReadProcessMemory(handle, (LPCVOID)(matches[i]), &buffer, sizeof(buffer), NULL)) {

			if (buffer == search)
			{
				tempvec.push_back(matches[i]);
			}

		}
		else {
			DWORD error = GetLastError();
			std::cout << "ERROR WITH READPROCESSMEMORY, FUNCTION: NARROW DOWN LIST" << std::endl;
			std::cout << "ERROR CODE: " << error << std::endl;
			std::cin.get();
		}
	}

	if (tempvec.size() == 0) {
		std::cout << "No possible matches found\n";
	}

	else {
		matches = tempvec;
		std::cout << "Found: " << matches.size() << " possible matches\n";
	}
}

template <class myType>
void WriteToMemory(myType value, std::vector<DWORD64> &matches, HANDLE handle) {

	int choice = -1;
	DisplayMatches(matches);
	std::cout << "ENTER WHICH ADRESS FROM THE LIST INDEX, FROM 0 TO X\n";
	std::cin >> choice;
	if (choice == -1 || choice > matches.size() || choice < 0 || matches.size() == 0) {
		std::cout << "INVALID INPUT OR POSSIBLE MATCHES IS 0\n";
	}
	else {
		if (0 != WriteProcessMemory(handle, (LPVOID)matches[choice], &value, sizeof(myType), NULL)) {
			std::cout << "SUCESS\n";
		}

		else {
			DWORD error = GetLastError();
			std::cout << "ERROR WITH WRITEPROCESSMEMORY, FUNCTION: WRITETOMEMORY" << std::endl;
			std::cout << "ERROR CODE: " << error << std::endl;
			std::cin.get();
		}

	}

}

template <class myType>
bool Input(myType find, std::vector<DWORD64> &matches, HANDLE handle, std::vector<MODULEINFO> mod) {

	std::cout << "\n\n";
	int switchval;

	std::cout << "Enter 1 to start a new scan\n";
	std::cout << "Enter 2 to narrow down possible matches (only call this after scanning)\n";
	std::cout << "Enter 3 to exit\n";
	std::cout << "Enter 4 to print possible matches\n";
	std::cout << "Enter 5 to write to an adress\n";
	std::cout << "Enter 6 to change type of search\n";
	std::cout << "Enter 7 to print possible matches found\n";

	std::cin >> switchval;

	switch (switchval)
	{
		case 1:
			UserTypeChoice(matches, handle, mod);
			break;

		case 2:
			std::cout << "What value do you want search for?\n";
			std::cin >> find;
			NarrowDownList(find, matches, handle);
			break;

		case 3:
			return false;

		case 4:
			DisplayMatches(matches);
			break;

		case 5: {
			myType write = NULL;
			std::cout << "What value do you want to write to memory?\n";
			std::cin >> write;
			WriteToMemory(write, matches, handle);
			break;
		}

		case 6:
			UserTypeChoice(matches, handle, mod);
			return false;

		case 7:
			DisplayMatches(matches);
			break;

		default: {
			std::cout << "Invalid Input\n";
			break;
		}
	}

	std::cout << "\n\n";

}

//Global window name input by user, must remain global so 'CALLBACK' can access it
char windowName[255];

int main()
{
	std::cout << "Enter desired window name\n";
	std::cin.getline(windowName, 255);

	HWND hWnd = NULL;

	//Find HWND variable by searching by window's name
	EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&hWnd));

	if (hWnd == NULL) {
		std::cout << "Could not find handle \n";
		DWORD error = GetLastError();
		std::cout << error;
		std::cin.get();
		exit(EXIT_FAILURE);

	}


	else {

		DWORD id = GetProcID(hWnd);
		HANDLE handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, id);

		if (handle == NULL)
		{
			std::cout << "Could not find process ID or handle\n";
			std::cin.get();
			exit(EXIT_FAILURE);
		}

		else {
			std::vector<DWORD64> matches;
			std::vector<MODULEINFO> mod = MainModule(handle);

			UserTypeChoice(matches, handle, mod);
		}

	}
}
BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {

	//Max path files names in windows 256
	char name[255];

	GetWindowText(hwnd, name, sizeof(name));
	std::string find = name;

	//std::cout << find <<endl;

	if (find == windowName)
	{
		*(reinterpret_cast<HWND*>(lParam)) = hwnd;
		return false;
	}
	return true;
}
std::vector<MODULEINFO> MainModule(HANDLE handle) {

	std::vector<MODULEINFO> mods;

	//Enumerate Buffer
	char name[MAX_PATH];

	//Executable buffer
	char exeName[MAX_PATH];

	//Retrieves the name of the executable file for the specified process
	GetModuleFileNameExA(handle, NULL, exeName, sizeof(exeName));

	DWORD bytesNeeded;
	HMODULE arrHandles[10000];
	MODULEINFO modinfo;


	//Enumerate through all the modules in a given process
	if (EnumProcessModules(handle, arrHandles, sizeof(arrHandles), &bytesNeeded))
	{
		for (int i = 0; i < (sizeof(arrHandles) / bytesNeeded); i++)
		{
			//Retrieves the fully qualified path for the file containing the specified module.
			GetModuleFileNameExA(handle, arrHandles[i], name, sizeof(name));

			//std::cout << name << endl;

			//Retrieves information about the specified module in the MODULEINFO structure.
			if (0 != GetModuleInformation(handle, arrHandles[i], &modinfo, sizeof(modinfo))) {
				mods.push_back(modinfo);
			}
		}
	}

	//Return a vector of all found modules in found process
	if (mods.size() != 0) {
		return mods;
	}

	else {
		std::cout << "EnumProcessModules Failed" << std::endl;
		DWORD error = GetLastError();
		std::cout << error;
		std::cin.get();
		exit(EXIT_FAILURE);
	}
}
DWORD GetProcID(HWND hWnd) {

	DWORD id;
	GetWindowThreadProcessId(hWnd, &id);

	if (id == NULL)
	{
		std::cout << "Could not find process ID" << std::endl;
		std::cin.get();
		exit(EXIT_FAILURE);

	}

	std::cout << "Proc ID: " << id << std::endl;
	return id;

}
bool CompareFilenames(char file1[], char file2[]) {

	int i = 0;

	while (i <= 256) {
		if (toupper(file1[i]) != toupper(file2[i])) {
			return false;
		}

		i++;
	};

	return true;
}
void DisplayMatches(std::vector<DWORD64> matches) {
	for (int i = 0; i < matches.size(); i++) {
		std::cout << std::dec <<"Address: " << std::hex << matches[i] << std::dec << std::endl;
	}
}
void UserTypeChoice(std::vector<DWORD64> &matches, HANDLE handle, std::vector<MODULEINFO> mod) {
	int switchvalue = -1;

	std::cout << "Choose data type of what value you're searching for\n";
	std::cout << "1: int\n";
	std::cout << "2: short\n";
	std::cout << "3: float\n";
	std::cout << "4: int 64\n";

	std::cin >> switchvalue;
	switch (switchvalue)
	{
	case 1: {
		int find;
		std::cout << "Enter int\n";
		std::cin >> find;
		ScanMemory(find, matches, handle, mod);
		while (Input(find, matches, handle, mod));
		break;
	}
	case 2: {
		short find;
		std::cout << "Enter short\n";
		std::cin >> find;
		ScanMemory(find, matches, handle, mod);
		while (Input(find, matches, handle, mod));
		break;
	}
	case 3: {
		float find;
		std::cout << "Enter float\n";
		std::cin >> find;
		ScanMemory(find, matches, handle, mod);
		while (Input(find, matches, handle, mod));
		break;
	}
	case 4: {
		double find;
		std::cout << "Enter double\n";
		std::cin >> find;
		ScanMemory(find, matches, handle, mod);
		while (Input(find, matches, handle, mod));
		break;
	}
	case 8: {
		long int find;
		std::cout << "Enter 64 bit int\n";
		std::cin >> find;
		ScanMemory(find, matches, handle, mod);
		while (Input(find, matches, handle, mod));

	}

	//***POSSIBLE ADD SUPPORT FOR SEARCH FOR STRINGS/CHARS/CHAR[]***
	
	//case 5: {
	//	std::string find;
	//	std::cout << "Enter string\n";
	//	std::cin >> find;
	//	while (Input(find, matches, handle, mod));
	//	break;
	//}
	//case 6: {
	//	char find;
	//	std::cout << "Enter char\n";
	//	std::cin >> find;
	//	while (Input(find, matches, handle, mod));
	//	break;
	//}
	//case 7: {
	//
	//	char find[256000];
	//	std::cout << "Enter char array\n";
	//	std::cin >> find;
	//	while (Input(find, matches, handle, mod));
	//	break;
	//}


	default:
		std::cout << "INVALID INPUT\n";
		break;

	}
}
