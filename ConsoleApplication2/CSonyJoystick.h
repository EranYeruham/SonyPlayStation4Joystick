// =================================================================================================
// DESCRIPTION
//
// Author: Eran yeruham, Date: July 28, 2024
//
// Copyright � Globus 2024 All Rights Reserved
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

// =================================================================================================
// ================================= TYPES, CLASSES, STRUCTURES ====================================

struct JSDATA
{
	LONG leftX;
	LONG leftY;
	LONG leftZ;
	LONG rightX;
	LONG rightY;
	LONG rightZ;
	bool HandlePressed[13];
	int arrowValue;

	JSDATA() :
			leftX(0),
			leftY(0),
			leftZ(0),
			rightX(0),
			rightY(0),
			rightZ(0),
		    arrowValue(-1)
	{
		for (int i=0;i<13;i++)
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

