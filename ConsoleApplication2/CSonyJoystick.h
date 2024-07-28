// =================================================================================================
// DESCRIPTION
//
// Author: Eran yeruham, Date: July 28, 2024
//
// Copyright © Globus 2024 All Rights Reserved
// -------------------------------------------------------------------------------------------------
 
#pragma once
#pragma comment(lib, "hid.lib")

// =================================================================================================
// ======================================== INCLUDED FILES =========================================

#include <windows.h>
#include <functional>
#include <tchar.h>


// =================================================================================================
// ==================================== FORWARD DECLARATIONS =======================================

// =================================================================================================
// ====================================== CONSTANTS, ENUMS =========================================

const int BUTTONS_NUM = 12;

// =================================================================================================
// ================================= TYPES, CLASSES, STRUCTURES ====================================

struct JSDATA
{
	LONG leftX;
	LONG leftY;
	LONG rightX;
	LONG rightY;
	LONG L2;
	LONG R2;
	bool HandlePressed[BUTTONS_NUM];
	int arrowValue;

	JSDATA() :
			leftX(0),
			leftY(0),
			rightX(0),
			rightY(0),
			L2(0),
			R2(0),
		    arrowValue(-1)
	{
		for (int i=0;i< BUTTONS_NUM;i++)
		{
			HandlePressed[i] = false;
		}
	}
};


class CSonyJoystick
{
public:
	CSonyJoystick(std::function<void(JSDATA)> callBackUpdate);
   ~CSonyJoystick();

   bool ProcessRawInput(HRAWINPUT hRawInput);
   void DrawTextOnDC(HDC hdc);
   void ShowDataWindow();

private:
   HWND InitDummyWindow();
   bool RegisterInputDevices(HWND hwnd);
   

   std::function<void(JSDATA)> m_CallbackUpdater;
   JSDATA m_jsData;
   HWND m_hwnd;


 

};

