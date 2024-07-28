
// =================================================================================================
// DESCRIPTION
//
// Author: Eran yeruham, Date: July 28, 2024
//
// Copyright © Globus 2024 All Rights Reserved
// -------------------------------------------------------------------------------------------------

// =================================================================================================
// ======================================== INCLUDED FILES =========================================

#include "CSonyJoystick.h"
#include <windows.h>
#include <iostream>
#include <hidusage.h>
#include <hidsdi.h>
#include <hidpi.h>

// =================================================================================================
// ========================================= NAMESPACES ============================================

// =================================================================================================
// =========================================== MACROS ==============================================

// =================================================================================================
// ====================================== CONSTANTS, ENUMS =========================================

// =================================================================================================
// ================================ TYPES, CLASSES, STRUCTURES =====================================

// =================================================================================================
// ================================== STATIC MEMBER VARIABLES ======================================

HDC hdcMem;
HBITMAP hbmMem, hbmOld;

CSonyJoystick* m_thisClass;


// =================================================================================================
// ===================================== GLOBAL VARIABLES ==========================================

// =================================================================================================
// ===================================== FUNCTION PROTOTYPES =======================================

// =================================================================================================
// ===================================== FUNCTION DEFINITIONS ======================================

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg)
    {
    case WM_ERASEBKGND:
        return 1; // Prevent flickering by not erasing the background

        case WM_INPUT:
            m_thisClass->ProcessRawInput((HRAWINPUT)lParam);
            InvalidateRect(hwnd, NULL, TRUE); // Request to redraw the window
            break;
        
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        
        case WM_SIZE:
        {
            // Resize the memory DC and bitmap when the window is resized
            if (hdcMem) 
            {
                HDC hdc = GetDC(hwnd);
                RECT rect;
                GetClientRect(hwnd, &rect);

                HBITMAP hbmNew = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
                SelectObject(hdcMem, hbmNew);
                DeleteObject(hbmMem);

                hbmMem = hbmNew;
                ReleaseDC(hwnd, hdc);
            }
        }
        break;
        
        case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            RECT rect;
            GetClientRect(hwnd, &rect);

            // Create memory DC and bitmap if they don't exist
            if (!hdcMem) 
            {
                hdcMem = CreateCompatibleDC(hdc);
                hbmMem = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
                hbmOld = (HBITMAP)SelectObject(hdcMem, hbmMem);
            }

            // Clear the memory DC
            FillRect(hdcMem, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));

            // Draw on the memory DC
            m_thisClass->DrawTextOnDC(hdcMem);

            // Copy the memory DC to the screen
            BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);

            EndPaint(hwnd, &ps);
            break;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}


// =================================================================================================
// CSonyJoystick
//
// Author: Eran Yeruham, Date: 28 July 2024
// -------------------------------------------------------------------------------------------------
CSonyJoystick::CSonyJoystick(std::function<void(JSDATA)> callBackUpdate)
{
    m_thisClass = this;
    m_hwnd = InitDummyWindow();
    RegisterInputDevices(m_hwnd);

    m_CallbackUpdater = callBackUpdate;
}

// =================================================================================================
// ~CSonyJoystick
//
// Author: Eran Yeruham, Date: 28 July 2024
// -------------------------------------------------------------------------------------------------
CSonyJoystick::~CSonyJoystick()
{
    if (hdcMem) 
    {
        SelectObject(hdcMem, hbmOld);
        DeleteDC(hdcMem);
        DeleteObject(hbmMem);
    }
}

// =================================================================================================
// ProcessRawInput
//
// Author: Eran Yeruham, Date: 28 July 2024
// -------------------------------------------------------------------------------------------------
bool CSonyJoystick::ProcessRawInput(HRAWINPUT hRawInput)
{
    UINT dwSize;
    GetRawInputData(hRawInput, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

    LPBYTE lpb = new BYTE[dwSize];
    if (lpb == NULL) 
    {
        return false;
    }

    if (GetRawInputData(hRawInput, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize) 
    {
        delete[] lpb;
        return false;
    }

    RAWINPUT* raw = (RAWINPUT*)lpb;
    if (raw->header.dwType == RIM_TYPEHID) {
        UINT cbSize = 0;

        // Query the required buffer size for the preparsed data
        if (GetRawInputDeviceInfo(raw->header.hDevice, RIDI_PREPARSEDDATA, NULL, &cbSize) != 0) 
        {
            delete[] lpb;
            return false;
        }

        // Allocate the buffer for the preparsed data
        PHIDP_PREPARSED_DATA pPreparsedData = (PHIDP_PREPARSED_DATA)new BYTE[cbSize];
        if (pPreparsedData == NULL) {
            delete[] lpb;
            return false;
        }

        // Get the preparsed data
        if (GetRawInputDeviceInfo(raw->header.hDevice, RIDI_PREPARSEDDATA, pPreparsedData, &cbSize) == -1) 
        {
            delete[] lpb;
            delete[] pPreparsedData;
            return false;
        }

        HIDP_CAPS caps;
        if (HidP_GetCaps(pPreparsedData, &caps) == HIDP_STATUS_SUCCESS)
        {
            USHORT numValues = caps.NumberInputValueCaps;
            USHORT numButtons = caps.NumberInputButtonCaps;


            HIDP_VALUE_CAPS* valueCaps = new HIDP_VALUE_CAPS[numValues];
            HIDP_BUTTON_CAPS* buttonCaps = new HIDP_BUTTON_CAPS[numButtons];


            if (HidP_GetValueCaps(HidP_Input, valueCaps, &numValues, pPreparsedData) != HIDP_STATUS_SUCCESS) 
            {
                delete[] valueCaps;
                delete[] buttonCaps;
                delete[] pPreparsedData;
                delete[] lpb;
                return false;
            }

            if (HidP_GetButtonCaps(HidP_Input, buttonCaps, &numButtons, pPreparsedData) != HIDP_STATUS_SUCCESS)
            {
                delete[] valueCaps;
                delete[] buttonCaps;
                delete[] pPreparsedData;
                delete[] lpb;
                return false;
            }

            for (USHORT i = 0; i < numValues - 1; i++)
            {
                ULONG value;
                if (valueCaps[i].Range.UsageMin == HID_USAGE_GENERIC_X)
                {
                    if (HidP_GetUsageValue(HidP_Input, valueCaps[i].UsagePage, 0, HID_USAGE_GENERIC_X, &value, pPreparsedData, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS) {
                        m_jsData.leftX = value;
                    }
                    else
                    {
                        int err = GetLastError();
                    }
                }
                else if (valueCaps[i].Range.UsageMin == HID_USAGE_GENERIC_Y)
                {
                    if (HidP_GetUsageValue(HidP_Input, valueCaps[i].UsagePage, 0, HID_USAGE_GENERIC_Y, &value, pPreparsedData, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS) {
                        m_jsData.leftY = value;
                    }
                    else
                    {
                    }
                }
                else if (valueCaps[i].Range.UsageMin == HID_USAGE_GENERIC_Z)
                {
                    if (HidP_GetUsageValue(HidP_Input, valueCaps[i].UsagePage, 0, HID_USAGE_GENERIC_Z, &value, pPreparsedData, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS) 
                    {
                        m_jsData.leftZ = value;
                    }
                    else
                    {
                    }
                }
                else if (valueCaps[i].Range.UsageMin == HID_USAGE_GENERIC_RX)
                {
                    if (HidP_GetUsageValue(HidP_Input, valueCaps[i].UsagePage, 0, HID_USAGE_GENERIC_RX, &value, pPreparsedData, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS) 
                    {
                        m_jsData.rightX = value;
                    }
                    else 
                    {
                    }
                }
                else if (valueCaps[i].Range.UsageMin == HID_USAGE_GENERIC_RY)
                {
                    if (HidP_GetUsageValue(HidP_Input, valueCaps[i].UsagePage, 0, HID_USAGE_GENERIC_RY, &value, pPreparsedData, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS) 
                    {
                        m_jsData.rightY = value;
                    }
                    else 
                    {
                    }
                }
                else if (valueCaps[i].Range.UsageMin == HID_USAGE_GENERIC_RZ)
                {
                    if (HidP_GetUsageValue(HidP_Input, valueCaps[i].UsagePage, 0, HID_USAGE_GENERIC_RZ, &value, pPreparsedData, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS) {
                        m_jsData.rightZ = value;
                    }
                    else 
                    {
                    }
                }
                else if (valueCaps[i].Range.UsageMin == HID_USAGE_GENERIC_HATSWITCH)
                {
                    if (HidP_GetUsageValue(HidP_Input, valueCaps[i].UsagePage, 0, HID_USAGE_GENERIC_HATSWITCH, &value, pPreparsedData, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS) {
                        m_jsData.arrowValue = value;
                    }
                }
            }

            for (ULONG j = 0; j < 13; j++)
            {
                m_jsData.leftHandlePressed[j] = false;

            }
            for (USHORT i = 0; i < numButtons; i++)
            {
                if (buttonCaps[i].UsagePage == HID_USAGE_PAGE_BUTTON)
                {
                    ULONG buttonUsageMin = buttonCaps[i].Range.UsageMin;
                    ULONG buttonUsageMax = buttonCaps[i].Range.UsageMax;
                    USHORT numUsages = USHORT(buttonUsageMax - buttonUsageMin + 1);

                    USAGE* usages = new USAGE[numUsages];
                    ULONG usageLength = numUsages;

                    if (HidP_GetUsages(HidP_Input, buttonCaps[i].UsagePage, 0, usages, &usageLength, pPreparsedData, (PCHAR)raw->data.hid.bRawData, raw->data.hid.dwSizeHid) == HIDP_STATUS_SUCCESS)
                    {
                        for (ULONG j = 0; j < usageLength; j++)
                        {
                            if (usages[j])
                            { 
                                m_jsData.leftHandlePressed[*usages - 1] = true;
                                break;
                            }
                        }
                    }

                    delete[] usages;
                }
            }
            delete[] valueCaps;
        }

        delete[] pPreparsedData;
    }

    delete[] lpb;

    m_CallbackUpdater(m_jsData);

    return true;
}

// =================================================================================================
// InitDummyWindow
//
// Author: Eran Yeruham, Date: 28 July 2024
// -------------------------------------------------------------------------------------------------
HWND CSonyJoystick::InitDummyWindow()
{
    HINSTANCE hInstance = GetModuleHandle(NULL);
    WNDCLASSEX wc;
    HWND hwnd;

    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = 0;
    wc.lpfnWndProc = WindowProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = L"RawInputExample";
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassEx(&wc))
    {
        return 0;
    }

    hwnd = CreateWindowEx(
        WS_EX_CLIENTEDGE,
        L"RawInputExample",
        L"Raw Input Example",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 640, 480,
        NULL, NULL, hInstance, NULL);

    if (hwnd == NULL)
    {
        return 0;
    }

    return hwnd;
}

// =================================================================================================
// RegisterInputDevices
//
// Author: Eran Yeruham, Date: 28 July 2024
// -------------------------------------------------------------------------------------------------
bool CSonyJoystick::RegisterInputDevices(HWND hwnd)
{
    RAWINPUTDEVICE rid[1];
    rid[0].usUsagePage = 0x01; // Generic Desktop Controls
    rid[0].usUsage = 0x05;     // Game Pad
    rid[0].dwFlags = RIDEV_INPUTSINK;
    rid[0].hwndTarget = hwnd;

    if (RegisterRawInputDevices(rid, 1, sizeof(rid[0])) == FALSE) 
    {
        return false;
    }
    
    return true;
}

// =================================================================================================
// DrawTextOnDC
//
// Author: Eran Yeruham, Date: 28 July 2024
// -------------------------------------------------------------------------------------------------
void CSonyJoystick::DrawTextOnDC(HDC hdc)
{
    // Create a string with the joystick state
    TCHAR buffer[256];
    _stprintf_s(buffer, _T("Left X: %ld Left Y: %ld Left Z: %ld\n"), m_jsData.leftX, m_jsData.leftY, m_jsData.leftZ);
    TextOut(hdc, 10, 10, buffer, (int)_tcslen(buffer));

    _stprintf_s(buffer, _T("Right X: %ld Right Y: %ld Right Z: %ld\n"), m_jsData.rightX, m_jsData.rightY, m_jsData.rightZ);
    TextOut(hdc, 10, 30, buffer, (int)_tcslen(buffer));

    _stprintf_s(buffer, _T("Arrow: %d\n"), m_jsData.arrowValue);
    TextOut(hdc, 10, 50, buffer, (int)_tcslen(buffer));

    for (int i=0; i<13; i++)
    {
        _stprintf_s(buffer, _T("Button Pressed: %s\n"), m_jsData.leftHandlePressed[i] ? _T("Yes") : _T("No"));
        TextOut(hdc, 10, 90 + i*20, buffer, (int)_tcslen(buffer));
    }
}

// =================================================================================================
// ShowDataWindow
//
// Author: Eran Yeruham, Date: 28 July 2024
// -------------------------------------------------------------------------------------------------
void CSonyJoystick::ShowDataWindow()
{
    ShowWindow(m_hwnd, SW_SHOW);
}