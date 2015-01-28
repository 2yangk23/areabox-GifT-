// Rectangle.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "dllmain.cpp"

HHOOK mouseHook;
HWND hWnd, bg, frame;
BOOL drawing, selecting, recording, finished, rMode, initialized;
int sX, sY, nX = -5, nY, box[4];


VOID MouseLeftPressed(int, int);
VOID MouseLeftReleased();
VOID MouseMoved(int, int);
VOID setFrame(int);
VOID orderedRect(int, int, int, int);
DWORD swapBytes(DWORD);

LRESULT CALLBACK MouseEventHandle(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode >= 0)
    {
        PMSLLHOOKSTRUCT lp = reinterpret_cast<PMSLLHOOKSTRUCT>(lParam);

        switch (wParam)
        {
            case WM_LBUTTONDOWN:
                MouseLeftPressed(lp->pt.x, lp->pt.y);
                break; //returning 1 prevents any clicks from registering
 
            case WM_LBUTTONUP:
                MouseLeftReleased();
                break;

            case WM_MOUSEMOVE:
                MouseMoved(lp->pt.x, lp->pt.y);
                break;
        }
    }

    return CallNextHookEx(mouseHook, nCode, wParam, lParam);
}

LRESULT CALLBACK WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

VOID MouseLeftPressed(int x, int y)
{
    if(selecting)
    {
        sX = x;
        sY = y;
        drawing = TRUE;
    }
}

VOID MouseLeftReleased()
{
    drawing = FALSE;
    
    if(nX != -5)
    {
        box[0] = sX < nX ? sX : nX;
        box[1] = sY < nY ? sY : nY;
        box[2] = abs(nX - sX);
        box[2] = box[2] % 2 == 0 ? box[2] : box[2] + 1;
        box[3] = abs(nY - sY);
        box[3] = box[3] % 2 == 0 ? box[3] : box[3] + 1;

        selecting = FALSE;
        ShowWindow(bg, SW_HIDE);
        SetWindowPos(hWnd, HWND_TOPMOST, -999, 0, 0, 0, SWP_HIDEWINDOW);

        if(rMode)
        {
            setFrame(4);
            nX = -5;
            recording = TRUE;
        }
        else
        {
            finished = true;
            PostMessage(hWnd, 0, 0, 0); //need to post another message to cause main loop to process
        }
    }
}

VOID MouseMoved(int x, int y) //When mouse is moved, get new coordinates and draw new rectangle
{
    if(drawing)
    {
        nX = x;
        nY = y;
        orderedRect(sX, sY, nX, nY);
    }
}

VOID orderedRect(int aX, int aY, int bX, int bY)
{
    int l = aX < bX ? aX : bX;
    int t = aY < bY ? aY : bY;
    int w = abs(aX - bX);
    int h = abs(aY - bY);

    SetWindowPos(hWnd, HWND_TOPMOST, l, t, w, h, SWP_SHOWWINDOW);
}

VOID setFrame(int thick) //Sets a frame around the selection
{
    RECT win;
    HRGN combined = CreateRectRgn(0, 0, 0, 0);

    SetWindowPos(frame, HWND_TOPMOST, box[0] - thick, box[1] - thick, box[2] + 2 * thick, box[3] + 2 * thick, SWP_HIDEWINDOW);
    GetWindowRect(frame, &win);

    HRGN outer = CreateRectRgn(0, 0, win.right - win.left, win.bottom - win.top);
    HRGN inner = CreateRectRgn(thick, thick, win.right - win.left - thick, win.bottom - win.top - thick);
    CombineRgn(combined, outer, inner, RGN_XOR);

    SetWindowRgn(frame, combined, TRUE);
    DeleteObject(combined);
    DeleteObject(outer);
    DeleteObject(inner);

    ShowWindow(frame, SW_SHOW);
}

DWORD swapBytes(DWORD value) //Flips the RGB bytes in color because it's reversed, not sure why
{
    DWORD flip;
    flip = value & 0xFF;
    flip = (flip << 8) | ((value >> 8) & 0xFF);
    flip = (flip << 8) | ((value >> 16) & 0xFF);

    return flip;
}

extern "C"
{
    DECLDIR VOID initialize()
    {
        if(!initialized)
        {
            const WCHAR* const sWin = L"select";
            WNDCLASSEX wndclass = {sizeof(WNDCLASSEX), CS_DBLCLKS, WindowProcedure, 0, 0, GetModuleHandle(0), LoadIcon(0, IDI_APPLICATION),
                                    LoadCursor(0, IDC_CROSS), HBRUSH(COLOR_WINDOW+1), 0, sWin, LoadIcon(0, IDI_APPLICATION)};
            if(RegisterClassEx(&wndclass))
            {
                hWnd = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, sWin, L"", WS_OVERLAPPEDWINDOW, -999, 0, 0, 0, 0, 0, GetModuleHandle(0), 0);
                bg = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, sWin, L"", WS_OVERLAPPEDWINDOW, -999, 0, 0, 0, 0, 0, GetModuleHandle(0), 0);

                LONG lStyle = GetWindowLong(hWnd, GWL_STYLE) & ~(WS_CAPTION | WS_THICKFRAME | WS_MINIMIZE | WS_MAXIMIZE | WS_SYSMENU);
                LONG lExStyle = GetWindowLong(hWnd, GWL_EXSTYLE) & ~(WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE | WS_EX_STATICEDGE);

                SetWindowLong(hWnd, GWL_STYLE, lStyle);
                SetWindowLong(hWnd, GWL_EXSTYLE, lExStyle);
                SetWindowLong(bg, GWL_STYLE, lStyle);
                SetWindowLong(bg, GWL_EXSTYLE, lExStyle);

                SetLayeredWindowAttributes(bg, 0, 1, LWA_ALPHA);
        
                /*rMode = FALSE;
                if(false && finKey != 0)
                {
                    rMode = TRUE;
                    frame = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_LAYERED, sWin, L"", WS_OVERLAPPEDWINDOW, -999, 0, 0, 0, 0, 0, GetModuleHandle(0), 0);
                    SetWindowLong(frame, GWL_STYLE, lStyle);
                    SetWindowLong(frame, GWL_EXSTYLE, lExStyle);
                    SetLayeredWindowAttributes(frame, 0, 255, LWA_ALPHA);
                    RegisterHotKey(NULL, 2, finMod | MOD_NOREPEAT, finKey);
                    SetWindowPos(frame, HWND_TOPMOST, -999, 0, 0, 0, SWP_SHOWWINDOW);
                }*/
            }
            initialized = TRUE;
        }
    }

    DECLDIR VOID deinitialize()
    {
        if(initialized)
        {
            DestroyWindow(hWnd);
            DestroyWindow(bg);
            DestroyWindow(frame);
            initialized = FALSE;
        }
    }

    DECLDIR BOOL setColor(DWORD color, BYTE alpha)
    {
        SetLayeredWindowAttributes(hWnd, 0, alpha, LWA_ALPHA);
        HBRUSH brush = CreateSolidBrush(swapBytes(color));
        if(SetClassLongPtr(hWnd, GCLP_HBRBACKGROUND, (LONG)brush) == 0) {
            return FALSE;
        }
        return TRUE;
    }

    DECLDIR VOID setFinish(UINT finMod, UINT finKey)
    {
        RegisterHotKey(NULL, 2, finMod | MOD_NOREPEAT, finKey);
    }

    DECLDIR VOID moveFrame(int thick) //Sets a frame around the selection
    {
        RECT win;
        HRGN combined = CreateRectRgn(0, 0, 0, 0);

        SetWindowPos(frame, HWND_TOPMOST, box[0] - thick, box[1] - thick, box[2] + 2 * thick, box[3] + 2 * thick, SWP_HIDEWINDOW);
        GetWindowRect(frame, &win);

        HRGN outer = CreateRectRgn(0, 0, win.right - win.left, win.bottom - win.top);
        HRGN inner = CreateRectRgn(thick, thick, win.right - win.left - thick, win.bottom - win.top - thick);
        CombineRgn(combined, outer, inner, RGN_XOR);

        SetWindowRgn(frame, combined, TRUE);
        DeleteObject(combined);
        DeleteObject(outer);
        DeleteObject(inner);

        ShowWindow(frame, SW_SHOW);
    }

    DECLDIR int* getSelection()
    {
        MSG msg;
        mouseHook = SetWindowsHookEx(WH_MOUSE_LL, MouseEventHandle, GetModuleHandle(NULL), 0 );
        if (!mouseHook) printf("err: %d\n", GetLastError());

        RegisterHotKey(NULL, 1, MOD_NOREPEAT, VK_ESCAPE);

        selecting = TRUE;
        finished = FALSE;
        SetWindowPos(bg, HWND_TOPMOST, 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN), SWP_SHOWWINDOW);
        
        if(hWnd)
        {
            while(GetMessage(&msg, NULL, 0, 0) && !finished)
            {
                if (msg.message == WM_HOTKEY)
                {
                    switch(msg.wParam)
                    {
                        case 1:
                            if(selecting || drawing || recording)
                            {
                                nX = -5;
                                SetWindowPos(hWnd, HWND_TOPMOST, -999, 0, 0, 0, SWP_HIDEWINDOW);
                                ShowWindow(frame, SW_HIDE);
                                ShowWindow(bg, SW_HIDE);
                                selecting = FALSE;
                                drawing = FALSE;
                                recording = FALSE;
                                box[0] = 0;
                                box[1] = 0;
                                box[2] = 0;
                                box[3] = 0;
                                //printf("Selection canceled...\n");
                                finished = TRUE;
                            }
                            break;

                        case 2:
                            if(recording)
                            {
                                ShowWindow(frame, SW_HIDE);
                                recording = FALSE;
                                printf("Recording finished...\n");
                                finished = TRUE;
                            }
                            break;
                    }
                }
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }
        }
        UnhookWindowsHookEx(mouseHook);
        UnregisterHotKey(NULL, 1);

        //printf("(%d, %d, %d, %d)", box[0], box[1], box[2], box[3]);
        return box;
    }
}