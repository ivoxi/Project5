#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <string>
#include <stdio.h>
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <ctime>
#include <tchar.h>
#include <Shlwapi.h>

// Глобальные переменные
HINSTANCE hInst;
LPCWSTR szTitle = L"WinAPI App";
LPCWSTR szWindowClass = L"WinAPIClass";
HWND hWnd;
HANDLE hMapFile;
LPVOID pData;
LPCTSTR szMapFileName = TEXT("Local\\MySharedMemory");
int N = 10;
constexpr UINT syncMsg = WM_USER + 1;

COLORREF gridColor = RGB(255, 0, 0);
COLORREF circleColor = RGB(255, 255, 0);
COLORREF crossColor = RGB(255, 255, 0);
COLORREF backgroundColor = RGB(0, 0, 255);
int windowWidth = 320;
int windowHeight = 240;

void DrawGrid(HDC hdc, int width, int height, int cellSizeX, int cellSizeY)
{
	HPEN hPen = CreatePen(PS_SOLID, 1, gridColor);
	HGDIOBJ hOldPen = SelectObject(hdc, hPen);

	for (int i = 0; i <= N; i++) {
		int x = i * cellSizeX;
		int y = i * cellSizeY;
		MoveToEx(hdc, x, 0, nullptr);
		LineTo(hdc, x, height);
		MoveToEx(hdc, 0, y, nullptr);
		LineTo(hdc, width, y);
	}

	SelectObject(hdc, hOldPen);
	DeleteObject(hPen);
}

void DrawCircle(HDC hdc, int x, int y, int cellSize) {
	int circleSize = cellSize / 2;
	HBRUSH hCircleBrush = CreateSolidBrush(circleColor);
	HGDIOBJ oldBrush = SelectObject(hdc, hCircleBrush);

	Ellipse(hdc, x + (cellSize - circleSize) / 2, y + (cellSize - circleSize) / 2,
		x + (cellSize + circleSize) / 2, y + (cellSize + circleSize) / 2);

	SelectObject(hdc, oldBrush);
	DeleteObject(hCircleBrush);
}

void DrawCross(HDC hdc, int x, int y, int cellSize) {
	int crossSize = cellSize / 2;
	HPEN hCrossPen = CreatePen(PS_SOLID, 2, crossColor);
	HGDIOBJ oldPen = SelectObject(hdc, hCrossPen);

	MoveToEx(hdc, x + (cellSize - crossSize) / 2, y + (cellSize - crossSize) / 2, nullptr);
	LineTo(hdc, x + (cellSize + crossSize) / 2, y + (cellSize + crossSize) / 2);
	MoveToEx(hdc, x + (cellSize - crossSize) / 2, y + (cellSize + crossSize) / 2, nullptr);
	LineTo(hdc, x + (cellSize + crossSize) / 2, y + (cellSize - crossSize) / 2);

	SelectObject(hdc, oldPen);
	DeleteObject(hCrossPen);
}

struct Config {
	COLORREF gridColor;
	COLORREF circleColor;
	COLORREF crossColor;
	COLORREF backgroundColor;
	int windowWidth;
	int windowHeight;
};

char loadMethod[10] = "";
Config config = { 0 };
const TCHAR* CONFIG_FILE_PATH = TEXT("C:\\Users\\nevyg\\OneDrive\\Рабочий стол\\lab\\config.ini");

void UpdateUI() {
	// Обновление вашего интерфейса
	// Например, перерисовка или другие действия

	// Чтение конфигурации из общей памяти и применение её
	memcpy(&config, pData, sizeof(Config));

	InvalidateRect(hWnd, nullptr, TRUE);
}

bool LoadConfig() {
	if (strcmp(loadMethod, "map") == 0) {
		HANDLE hFile = CreateFile(CONFIG_FILE_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) return false;

		HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
		if (hMapping == NULL) {
			CloseHandle(hFile);
			return false;
		}

		LPVOID pView = MapViewOfFile(hMapping, FILE_MAP_READ, 0, 0, 0);
		if (pView == NULL) {
			CloseHandle(hMapping);
			CloseHandle(hFile);
			return false;
		}

		CopyMemory(&config, pView, sizeof(Config));
		UnmapViewOfFile(pView);
		CloseHandle(hMapping);
		CloseHandle(hFile);
	}
	else if (strcmp(loadMethod, "stdio") == 0) {
		FILE* file = _tfopen(CONFIG_FILE_PATH, _T("rb"));
		if (!file) return false;

		size_t readCount = fread(&config, sizeof(Config), 1, file);
		fclose(file);

		if (readCount != 1) return false;
	}
	else if (strcmp(loadMethod, "fstream") == 0) {
		std::ifstream file(CONFIG_FILE_PATH, std::ios::binary);
		if (!file.is_open()) return false;

		file.read(reinterpret_cast<char*>(&config), sizeof(Config));
		if (!file) {
			file.close();
			return false;
		}
		file.close();
	}
	else if (strcmp(loadMethod, "winapi") == 0) {
		HANDLE hFile = CreateFile(CONFIG_FILE_PATH, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) return false;

		DWORD bytesRead;
		BOOL readResult = ReadFile(hFile, &config, sizeof(Config), &bytesRead, NULL);
		CloseHandle(hFile);

		if (!readResult || bytesRead != sizeof(Config)) return false;
	}
	else {
		return false;
	}

	return true;
}

void SaveConfig() {
	if (strcmp(loadMethod, "map") == 0) {
		HANDLE hFile = CreateFile(CONFIG_FILE_PATH, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE) return;

		HANDLE hMapping = CreateFileMapping(hFile, NULL, PAGE_READWRITE, 0, sizeof(Config), NULL);
		if (hMapping == NULL) {
			CloseHandle(hFile);
			return;
		}

		LPVOID pView = MapViewOfFile(hMapping, FILE_MAP_WRITE, 0, 0, sizeof(Config));
		if (pView == NULL) {
			CloseHandle(hMapping);
			CloseHandle(hFile);
			return;
		}

		CopyMemory(pView, &config, sizeof(Config));
		UnmapViewOfFile(pView);
		CloseHandle(hMapping);
		CloseHandle(hFile);
	}
	else if (strcmp(loadMethod, "stdio") == 0) {
		FILE* file = _tfopen(CONFIG_FILE_PATH, _T("wb"));
		if (file) {
			fwrite(&config, sizeof(Config), 1, file);
			fclose(file);
		}
	}
	else if (strcmp(loadMethod, "fstream") == 0) {
		std::ofstream file(CONFIG_FILE_PATH, std::ios::binary);
		if (file.is_open()) {
			file.write(reinterpret_cast<const char*>(&config), sizeof(Config));
			file.close();
		}
	}
	else if (strcmp(loadMethod, "winapi") == 0) {
		HANDLE hFile = CreateFile(CONFIG_FILE_PATH, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			DWORD bytesWritten;
			WriteFile(hFile, &config, sizeof(Config), &bytesWritten, NULL);
			CloseHandle(hFile);
		}
	}
}

void SetDefaultConfigValues() {
	config.gridColor = RGB(255, 0, 0);
	config.circleColor = RGB(255, 255, 0);
	config.crossColor = RGB(255, 255, 0);
	config.backgroundColor = RGB(0, 0, 255);
	config.windowWidth = 320;
	config.windowHeight = 240;
}

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {

	/*if (lpCmdLine[0] == '\0') {
		MessageBox(NULL, TEXT("Аргументы командной строки не предоставлены."), TEXT("Предупреждение"), MB_OK);
	}*/

	int argc;
	LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc);

	// Установка метода загрузки и сохранения по умолчанию
	strcpy_s(loadMethod, "winapi");

	if (argc > 1) {
		// Если первый аргумент командной строки - это "map", "stdio", "fstream" или "winapi"
		wcstombs(loadMethod, argv[1], sizeof(loadMethod) - 1);
		loadMethod[sizeof(loadMethod) - 1] = '\0'; // Убедимся, что строка всегда заканчивается нуль-терминатором

		TCHAR message[256];
		if (strcmp(loadMethod, "map") == 0 ||
			strcmp(loadMethod, "stdio") == 0 ||
			strcmp(loadMethod, "fstream") == 0 ||
			strcmp(loadMethod, "winapi") == 0) {
			_stprintf_s(message, TEXT("Введите метод: %hs"), loadMethod);
			MessageBox(NULL, message, TEXT("Информация"), MB_OK);
		}
		else {
			_stprintf_s(message, TEXT("Вы ввели неподдерживаемый метод: %hs"), loadMethod);
			MessageBox(NULL, message, TEXT("Ошибка!"), MB_OK);
			strcpy_s(loadMethod, "winapi");
		}
	}

	// Попытка загрузить конфигурацию
	if (!LoadConfig()) {
		SetDefaultConfigValues();
	}

	windowWidth = config.windowWidth;
	windowHeight = config.windowHeight;
	gridColor = config.gridColor;
	circleColor = config.circleColor;
	crossColor = config.crossColor;
	backgroundColor = config.backgroundColor;

	// Создаем и регистрируем класс окна
	WNDCLASSEX wcex = {
		sizeof(WNDCLASSEX),        // Размер структуры WNDCLASSEX
		CS_HREDRAW | CS_VREDRAW,   // Стиль класса окна: перерисовка при изменении размеров горизонтально и вертикально
		WndProc,                   // Указатель на функцию обработки сообщений окна (WndProc)
		0, 0,                      // Дополнительные параметры
		GetModuleHandle(NULL),     // Дескриптор текущего модуля
		NULL,                      // Иконка окна
		NULL,                      // Курсор окна
		NULL,                      // Фон окна (HBRUSH)
		NULL,                      // Меню окна
		szWindowClass,             // Имя класса окна
		NULL                       // Маленькая иконка окна
	};
	RegisterClassEx(&wcex);

	hInst = hInstance;  // Сохраняем дескриптор текущего экземпляра приложения


	hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, 0, windowWidth, windowHeight, NULL, NULL, hInstance, NULL);

	hMapFile = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(Config), szMapFileName);
	if (hMapFile == NULL) {
		MessageBox(NULL, TEXT("Could not create file mapping object"), TEXT("Error"), MB_OK | MB_ICONERROR);
		return 1;
	}

	pData = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(Config));
	if (pData == NULL) {
		CloseHandle(hMapFile);
		MessageBox(NULL, TEXT("Could not map view of file"), TEXT("Error"), MB_OK | MB_ICONERROR);
		return 1;
	}

	RECT rect;
	rect.left = rect.top = 0;
	rect.right = windowWidth;
	rect.bottom = windowHeight;
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, 0);

	int adjustedWidth = rect.right - rect.left;
	int adjustedHeight = rect.bottom - rect.top;

	SetWindowPos(hWnd, NULL, 0, 0, adjustedWidth, adjustedHeight, SWP_NOMOVE | SWP_NOZORDER);


	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	MSG msg;
	while (GetMessage(&msg, nullptr, 0, 0)) {
		if (msg.message == syncMsg) {
			// Обработка сообщения синхронизации
			// Дополнительные действия по обновлению интерфейса или другие действия
			// Например, вызов функции обновления интерфейса
			UpdateUI();
		}
		else {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}


	printf("Saving configuration:\nGrid Color: 0x%06X\nCircle Color: 0x%06X\nCross Color: 0x%06X\nBackground Color: 0x%06X\nWindow Width: %d\nWindow Height: %d\n",
		config.gridColor,
		config.circleColor,
		config.crossColor,
		config.backgroundColor,
		config.windowWidth,
		config.windowHeight);
	SaveConfig();
	LocalFree(argv);

	return (int)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;

	// Объявляем переменные здесь
	int clickX = 0;
	int clickY = 0;
	int x = 0;
	int y = 0;

	switch (message) {

	case WM_SIZE:
		windowWidth = LOWORD(lParam);
		windowHeight = HIWORD(lParam);
		config.windowWidth = LOWORD(lParam);
		config.windowHeight = HIWORD(lParam);
		InvalidateRect(hWnd, nullptr, TRUE);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			int cellSizeX = rect.right / N;
			int cellSizeY = rect.bottom / N;
			x = 0;
			y = 0;

			HBRUSH hbr = CreateSolidBrush(config.backgroundColor);
			FillRect(hdc, &ps.rcPaint, hbr);
			DeleteObject(hbr);

			DrawGrid(hdc, rect.right, rect.bottom, cellSizeX, cellSizeY);
		}
		EndPaint(hWnd, &ps);
		break;

	case WM_LBUTTONDOWN:
		hdc = GetDC(hWnd);
		clickX = LOWORD(lParam);
		clickY = HIWORD(lParam);
		{
			RECT rect;
			GetClientRect(hWnd, &rect);
			int cellSizeX = rect.right / N;
			int cellSizeY = rect.bottom / N;
			DrawCircle(hdc, (clickX / cellSizeX) * cellSizeX, (clickY / cellSizeY) * cellSizeY, cellSizeX);
		}
		ReleaseDC(hWnd, hdc);

		// Отправка сообщения другим экземплярам
		PostMessage(HWND_BROADCAST, syncMsg, NULL, NULL);
		break;
	case syncMsg:
		// Обновите ваш интерфейс или выполните другие действия, связанные с синхронизацией
		InvalidateRect(hWnd, nullptr, TRUE);
		break;

	case WM_RBUTTONDOWN:
	{
		hdc = GetDC(hWnd);
		clickX = LOWORD(lParam);
		clickY = HIWORD(lParam);
		RECT rect;
		GetClientRect(hWnd, &rect);
		int cellSizeX = rect.right / N;
		int cellSizeY = rect.bottom / N;
		DrawCross(hdc, (clickX / cellSizeX) * cellSizeX, (clickY / cellSizeY) * cellSizeY, cellSizeX);
		ReleaseDC(hWnd, hdc);
		PostMessage(HWND_BROADCAST, syncMsg, NULL, NULL);
	}
	break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE || (wParam == 81 && GetKeyState(VK_CONTROL) < 0)) {
			PostQuitMessage(0);
		}
		else if (wParam == 13) {  // Нажата клавиша ENTER
			srand(static_cast<unsigned>(time(nullptr)));
			config.backgroundColor = RGB(rand() % 256, rand() % 256, rand() % 256);
			InvalidateRect(hWnd, nullptr, TRUE);
			SaveConfig();
		}
		else if (wParam == 67 && GetKeyState(VK_SHIFT) < 0) {  // Нажата клавиша C с зажатым Shift
			if (GetFileAttributes(TEXT("C:\\Windows\\notepad.exe")) != INVALID_FILE_ATTRIBUTES) { // проверка на то, установлен ли блокнот у пользователя
				system("notepad.exe");
			}
			else {
				MessageBox(hWnd, TEXT("Notepad.exe не найден на вашем компьютере!"), TEXT("Ошибка"), MB_ICONERROR | MB_OK);
			}
		}
		break;


	case WM_MOUSEWHEEL:
	{
		short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);  // Получаем дистанцию прокрутки колеса
		BYTE red = GetRValue(config.gridColor);
		BYTE green = GetGValue(config.gridColor);
		BYTE blue = GetBValue(config.gridColor);

		if (zDelta > 0)  // Прокрутка вперед
		{
			red = (red + 5) % 256;
			green = (green + 5) % 256;
			blue = (blue + 5) % 256;
		}
		else  // Прокрутка назад
		{
			red = (red - 5) % 256;
			green = (green - 5) % 256;
			blue = (blue - 5) % 256;
		}

		gridColor = RGB(red, green, blue);
		config.gridColor = RGB(red, green, blue);

		InvalidateRect(hWnd, nullptr, TRUE);  // Перерисовать окно для применения нового цвета
		SaveConfig();
	}
	break;

	case WM_ERASEBKGND:
	{
		HBRUSH hbr = CreateSolidBrush(backgroundColor);
		FillRect((HDC)wParam, &ps.rcPaint, hbr);
		DeleteObject(hbr);
		return (LRESULT)1;
	}
	break;

	case WM_DESTROY:
		SaveConfig();
		PostQuitMessage(0);
		break;

	case WM_CLOSE:
		// Закрытие окна
		SaveConfig();
		return DefWindowProc(hWnd, message, wParam, lParam);
		break;
	default: {
		return DefWindowProc(hWnd, message, wParam, lParam);
		if (message == syncMsg) InvalidateRect(hWnd, NULL, TRUE);
		break;
	}
		   return 0;
	}
}
