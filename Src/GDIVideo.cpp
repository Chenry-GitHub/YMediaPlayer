#include "GDIVideo.h"
#if PLATFORM_IS_WIN32


GDIVideo::GDIVideo()
	:handle_(false)
{

}


GDIVideo::~GDIVideo()
{
}

void GDIVideo::SetDisplay(void *handle)
{
	handle_ = (HWND)handle;
}

void GDIVideo::PlayThread()
{
	while (false == is_stop_)
	{
		
		seek_func_();

		if (!IsPlaying())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
			continue;
		}

		char *data;
		int width;
		int height;
		double pts;
		if (!data_func_(&data, &width, &height, &pts))
		{
			continue;
		}

		clock_ = pts;
		
		if (sync_func_())
		{
			if (!user_display_func_(data, width, height))
			{
				ShowRGBToWnd(handle_, (BYTE*)data, width, height);
			}
		}
	}
}

void GDIVideo::ShowRGBToWnd(HWND hWnd, BYTE* data, int width, int height)
{
	if (data == NULL)
		return;

	static BITMAPINFO *bitMapinfo = NULL;
	static bool First = TRUE;

	if (First)
	{
		BYTE * m_bitBuffer = new BYTE[40 + 4 * 256];//¿ª±ÙÒ»¸öÄÚ´æÇøÓò  

		if (m_bitBuffer == NULL)
		{
			return;
		}
		First = FALSE;
		memset(m_bitBuffer, 0, 40 + 4 * 256);
		bitMapinfo = (BITMAPINFO *)m_bitBuffer;
		bitMapinfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
		bitMapinfo->bmiHeader.biPlanes = 1;
		for (int i = 0; i < 256; i++)
		{ //ÑÕÉ«µÄÈ¡Öµ·¶Î§ (0-255)  
			bitMapinfo->bmiColors[i].rgbBlue = bitMapinfo->bmiColors[i].rgbGreen = bitMapinfo->bmiColors[i].rgbRed = (BYTE)i;
		}
	}
	bitMapinfo->bmiHeader.biHeight = -height;
	bitMapinfo->bmiHeader.biWidth = width;
	bitMapinfo->bmiHeader.biBitCount = 3 * 8;
	RECT drect;
	GetClientRect(hWnd, &drect);    //pWndÖ¸ÏòCWndÀàµÄÒ»¸öÖ¸Õë   
	HDC hDC = GetDC(hWnd);     //HDCÊÇWindowsµÄÒ»ÖÖÊý¾ÝÀàÐÍ£¬ÊÇÉè±¸ÃèÊö¾ä±ú£»  
	SetStretchBltMode(hDC, COLORONCOLOR);
	StretchDIBits(hDC,
		0,
		0,
		drect.right,   //ÏÔÊ¾´°¿Ú¿í¶È  
		drect.bottom,  //ÏÔÊ¾´°¿Ú¸ß¶È  
		0,
		0,
		width,      //Í¼Ïñ¿í¶È  
		height,      //Í¼Ïñ¸ß¶È  
		data,
		bitMapinfo,
		DIB_RGB_COLORS,
		SRCCOPY
	);
	ReleaseDC(hWnd, hDC);
}
#endif