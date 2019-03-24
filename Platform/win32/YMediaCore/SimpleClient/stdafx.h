// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>

// C RunTime Header Files
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>


// TODO: reference additional headers your program requires here
#include "../DuiLib/UIlib.h"
using namespace DuiLib;

#include "resource.h"
#include <functional>
#include <string>


enum USEREVENT
{
	EV_VIDEO = WM_USER + 10,
	EV_DUR,
	EV_CUR,
	EV_PERCENT,

};

struct EV_VIDEO_STRUCT
{
	void *data;
	int w;
	int h;
};