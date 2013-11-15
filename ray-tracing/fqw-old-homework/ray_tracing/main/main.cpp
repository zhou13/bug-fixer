/*
    Name:       Fanfan
    Propose:    Homework of Advanced Computer Graphics
    Date:       Nov 25, 2011
    Version:    Alpha 4.0 - Submit version
*/
#include <windows.h>
#include <vector>
#include <string>
#include <sstream>
#include "main.h"
#include "koala.h"
#include "basic.h"
using namespace std;

char szInputFile[999];
int WINDOW_POSITION_X;
int WINDOW_POSITION_Y;
int WINDOW_SIZE_X;
int WINDOW_SIZE_Y;
int PIXELS_TO_FLUSH;

vector<vector<int> > pixels;
Koala koala;
BOOL bQuited, bFinished;
LRESULT Compute(HWND hwnd) {
    bFinished = true;
    RECT rect;
    GetClientRect(hwnd, &rect);
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    
    #ifdef USE_TIMER
	   DWORD __start = GetTickCount();
	#endif	
	koala.Initalize(width, height, szInputFile);
    #ifdef USE_TIMER
        DWORD __init_time = GetTickCount()-__start;
        //printf("Initalize: cost time = %.4lf s\n", (double)(GetTickCount()-__start)/1000.0);fflush(stdout);
	#endif
    
    pixels.resize(width);
    for (int i = 0; i < width; ++ i)
        pixels[i].resize(height);
    
	vector<pair<int, int> > queue;
	for (int x = 0; x < width; ++ x)
	   for (int y = 0; y < height; ++ y)
			queue.push_back(make_pair(x, y));
	/*
	for (int i = 1; i < queue.size(); ++ i)
		swap(queue[i], queue[random_int(i)]);
	//*/
		
    #ifdef USE_TIMER
	   __start = GetTickCount();
	#endif	
	for (int i = 0; i < queue.size(); ++ i) {
        if ((i+1) % PIXELS_TO_FLUSH == 0)
            InvalidateRect(hwnd, &rect, false);
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            if (bQuited)
                return 0;
        }
		int curx = queue[i].first;
		int cury = queue[i].second;
		int color = koala.GetPixel(curx, cury);
		pixels[curx][cury] = color;
	}
    InvalidateRect(hwnd, &rect, false);
    
    char message[999]; 
    #ifdef USE_TIMER
        DWORD __calc_time = GetTickCount()-__start;
        sprintf(message, "绘制结束。\n\n初始化耗时：%.3lf sec.\n计算耗时：  %.3lf sec.", (double)__init_time/1000.0, (double)__calc_time/1000.0);
    #else
        sprintf(message, "绘制结束。");
        //printf("Total: cost time = %.4lf s\n", (double)(GetTickCount()-__start)/1000.0);fflush(stdout);
	#endif	
	MessageBox(hwnd, message, "完成", MB_OK);
	return 0;
}

//int buffer[MAX_BUFFER_SIZE];

LRESULT OnWindowPaint(HWND hwnd) {
    int width = pixels.size();
    int height = pixels[0].size();
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);
    int *buffer;
    if ((buffer = (int*)malloc(width * height * 4)) != NULL) {
        for (int i = 0; i < width; ++ i)
            for (int j = 0; j < height; ++ j)
                buffer[j * width + i] = pixels[i][j];
        HBITMAP hbm = CreateCompatibleBitmap(hdc, width, height);
        HDC hdcMemory = CreateCompatibleDC(hdc);
        SetBitmapBits(hbm, width * height * 4, buffer);
        SelectObject(hdcMemory, hbm);
        BitBlt(hdc, 0, 0, width, height, hdcMemory, 0, 0, SRCCOPY);
        DeleteDC(hdcMemory);
        DeleteObject(hbm);
        free(buffer);
    }
    EndPaint(hwnd, &ps);
}

LRESULT OnWindowCreate(HWND hwnd) {
    //printf("OnWindowCreate\n");fflush(stdout);
    bQuited = FALSE;
    bFinished = FALSE;
    return 0;
}

LRESULT CALLBACK WindowProcedure(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    //printf("WindowProcedure msg=%d wparam=%d\n", message, wParam);fflush(stdout);
    try {
		switch (message) {
            case WM_CREATE:
                return OnWindowCreate(hwnd);
                
            case WM_PAINT:
       // printf("WM_PAINT\n");fflush(stdout);
                if (!bFinished)
                    return Compute(hwnd);
                return OnWindowPaint(hwnd);
				
			case WM_DESTROY:
      //  printf("WM_DESTROY\n");fflush(stdout);
                bQuited = TRUE;
				PostQuitMessage(0);    
				break;
				
			default:                 
				return DefWindowProc(hwnd, message, wParam, lParam);
		}
    } catch (std::string error) {
        for (MSG msg; PeekMessage(&msg, NULL, 0, 0, PM_REMOVE); ) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        MessageBox(hwnd, error.c_str(), "ERROR!", MB_OK);
    }
    return 0;
}

void InitalizeParameters(LPSTR lpszArgument) {
    istringstream is(lpszArgument);
    string tmp;
    int tmp1, tmp2;
    
    strcpy(szInputFile, default_szInputFile);
    WINDOW_POSITION_X = DEFAULT_WINDOW_POSITION_X;
    WINDOW_POSITION_Y = DEFAULT_WINDOW_POSITION_Y;
    WINDOW_SIZE_X = DEFAULT_WINDOW_SIZE_X;
    WINDOW_SIZE_Y = DEFAULT_WINDOW_SIZE_Y;    
    int FLUSH_TIMES = DEFAULT_FLUSH_TIMES;
    if (is >> tmp)
        strcpy(szInputFile, tmp.c_str());
    if (is >> tmp1 >> tmp2)
        WINDOW_SIZE_X = tmp1, WINDOW_SIZE_Y = tmp2;
    if (is >> tmp1 >> tmp2)
        WINDOW_POSITION_X = tmp1, WINDOW_POSITION_Y = tmp2;
    if (is >> tmp1)
        FLUSH_TIMES = tmp1;
    PIXELS_TO_FLUSH = WINDOW_SIZE_X * WINDOW_SIZE_Y / FLUSH_TIMES;
}

int WINAPI WinMain(HINSTANCE hThisInstance,
                    HINSTANCE hPrevInstance,
                    LPSTR lpszArgument,
                    int nFunsterStil)

{
           //     freopen("__log.txt","w",stdout);fflush(stdout);
    InitalizeParameters(lpszArgument);
    
    HWND hwnd;            
    MSG messages;         
    WNDCLASSEX wincl;     

    wincl.hInstance = hThisInstance;
    wincl.lpszClassName = szClassName;
    wincl.lpfnWndProc = WindowProcedure;     
    wincl.style = CS_DBLCLKS;              
    wincl.cbSize = sizeof(WNDCLASSEX);

    wincl.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hIconSm = LoadIcon(NULL, IDI_APPLICATION);
    wincl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wincl.lpszMenuName = NULL;              
    wincl.cbClsExtra = 0;                  
    wincl.cbWndExtra = 0;                 
    wincl.hbrBackground = (HBRUSH)COLOR_BACKGROUND;

    if (!RegisterClassEx(&wincl))
        return 0;
    hwnd = CreateWindowEx(
           0,                   // Extended possibilites for variation
           szClassName,         // Classname 
           szTitleText,         // Title Text
           WS_CAPTION | WS_POPUPWINDOW | WS_VISIBLE, // default window 
           WINDOW_POSITION_X,   // Windows decides the position 
           WINDOW_POSITION_Y,   // where the window ends up on the screen 
           WINDOW_SIZE_X,       // The programs width
           WINDOW_SIZE_Y,       // and height in pixels 
           HWND_DESKTOP,        // The window is a child-window to desktop
           NULL,                // No menu
           hThisInstance,       // Program Instance handler 
           NULL                 // No Window Creation data
    );
    ShowWindow(hwnd, nFunsterStil);

    while (GetMessage(&messages, NULL, 0, 0)) {
        TranslateMessage(&messages);
        DispatchMessage(&messages);
    }
    return messages.wParam;
}
