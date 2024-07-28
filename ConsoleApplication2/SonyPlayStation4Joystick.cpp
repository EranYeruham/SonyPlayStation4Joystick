#include <windows.h>
#include <iostream>
#include "CSonyJoystick.h"




void callBackUpdate(JSDATA jsData)
{

}

CSonyJoystick m_sony(callBackUpdate);

int main() 
{
    m_sony.ShowDataWindow();

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
