#include "stdafx.h"
#include "Buffer_Adapter.h"


Buffer_Adapter::Buffer_Adapter(HWND h_to_src_window)
{
	hwnd_to_source = h_to_src_window; 
	//initial setup of object. 
	HDC hdcScreen;
	HDC hdcWindow;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;


	// Retrieve the handle to a display device context for the client 
	// area of the window. 
	hdcScreen = GetDC(NULL); //device context of destination
	hdcWindow = GetDC(h_to_src_window); //device context of source


	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcWindow);



	if (!hdcMemDC)
	{
		std::wcout << L"Fatal Error: CreateCompatibleDC has failed. Program aborted." << '\n'; 
		MessageBox(h_to_src_window, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
		//goto done;
	}

	// set up rectangles and iniital cacheing values
	GetClientRect(h_to_src_window, &rcClient);
	GetWindowRect(h_to_src_window, &rcScreen);
	cache_screen_top = rcScreen.top; //set initial cache values
	cache_screen_bottom = rcScreen.bottom;
	cache_screen_left = rcScreen.left;
	cache_screen_right = rcScreen.right;



	// Create a compatible bitmap from the Window DC
	hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right, rcClient.bottom);
	if (!hbmScreen)
	{
		MessageBox(h_to_src_window, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
		//goto done;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmScreen);

	// Bit block transfer into our compatible memory DC.
	if (!BitBlt(hdcMemDC,
		0, 0, //destination rectangle start
		rcClient.right, rcClient.bottom, //w and l of destination
		hdcWindow, //source handle
		0, 0, //source start
		SRCCOPY)) //raster data for color transforming
	{
		MessageBox(h_to_src_window, L"BitBlt has failed", L"Failed", MB_OK);
		//goto done;
	}


	// Get the BITMAP from the HBITMAP
	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpScreen.bmWidth;
	bi.biHeight = bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM  

	//creates the buffer that holds the screencapture. 
	HANDLE hbuffer_bmap = new BYTE[dwBmpSize];


	GetDIBits(hdcWindow, hbmScreen, 0,
		(UINT)bmpScreen.bmHeight,
		hbuffer_bmap,
		(BITMAPINFO *)&bi, DIB_RGB_COLORS);

	//initialize default dimensions
	stencil_length = 1;
	stencil_height = 1;

	c_hdcScreen = hdcScreen; 
	c_hdcWindow = hdcWindow;
	c_hdcMemDC = hdcMemDC;
	c_hbmScreen = hbmScreen;
	c_bi_header = bi;
	c_bmpScreen = bmpScreen;
	c_dwBmpSize = dwBmpSize; 
	c_bmfHeader = bmfHeader; 
	source = (BYTE*)hbuffer_bmap; 
}

int Buffer_Adapter::getColorAtChannel(int cartesianx, int cartesiany, int channel) const 
{
	//turn pixel into a screen x and y.
	//get the buffer location start
	//add the bgr offset
	//return the value; 
	int screenx = cartesian_to_screen_x(cartesianx);
	int screeny = cartesian_to_screen_y(cartesiany); 
	int buffer_start = (((screeny)*rcClient.right) * 4 + screenx * 4);
	switch (channel) {
	case 0:
		return source[buffer_start + 0];
	case 1:
		return source[buffer_start + 1];
	case 2:
		return source[buffer_start + 2];
	default:
		return source[buffer_start + 0];
	}

	return source[buffer_start + 0];
}

int Buffer_Adapter::cartesian_to_screen_x(int scriptx) const
{
	//no inversion for x, just scale. 
	float adjust_x = (float)rcClient.right / (float)stencil_length;
	int stretched_x = (float)scriptx*adjust_x;

	return stretched_x;
}

int Buffer_Adapter::cartesian_to_screen_y(int scripty) const 
{
	//scale and mirror
	int midpoint = rcClient.bottom / 2;
	int post_inversion = 0;
	if (scripty > midpoint) {
		post_inversion = (midpoint - (scripty - midpoint));
	}
	else {
		post_inversion = (midpoint + (midpoint - scripty));
	}

	float adjust_y = (float)rcClient.bottom / (float)stencil_height;
	int stretched_y = (float)post_inversion*adjust_y;

	return stretched_y;
}

void Buffer_Adapter::update_buffer()
{
	//check validation, then call full or partial rescan. 
	int validation = revalidate_properties(); 
	if (validation == 0) {
		//same place, do fast update. 
		update_buffer_no_change(); 
	}
	else {
		//rectangles or locations have moved. Need to test for resize and realloc buffer. 
		//release all old objects and reacquire.  
		update_properties_and_buffer(); 
	}
}

int Buffer_Adapter::revalidate_properties()
{
	//checks if screen dimensions match cached. If not, full revalidation required. 
	//only tests for movement/resizes. 
	if (cache_screen_top != rcScreen.top) {
		cache_screen_top = rcScreen.top; //cache screen values to detect motion of screen resize. 
		cache_screen_bottom = rcScreen.bottom;
		cache_screen_left = rcScreen.left;
		cache_screen_right = rcScreen.right;
		return 1; 
	}
	if (cache_screen_bottom != rcScreen.bottom) {
		cache_screen_top = rcScreen.top; //cache screen values to detect motion of screen resize. 
		cache_screen_bottom = rcScreen.bottom;
		cache_screen_left = rcScreen.left;
		cache_screen_right = rcScreen.right;
		return 1;
	}
	if (cache_screen_left != rcScreen.left) {
		cache_screen_top = rcScreen.top; //cache screen values to detect motion of screen resize. 
		cache_screen_bottom = rcScreen.bottom;
		cache_screen_left = rcScreen.left;
		cache_screen_right = rcScreen.right;
		return 1;
	}
	if (cache_screen_right != rcScreen.right) {
		cache_screen_top = rcScreen.top; //cache screen values to detect motion of screen resize. 
		cache_screen_bottom = rcScreen.bottom;
		cache_screen_left = rcScreen.left;
		cache_screen_right = rcScreen.right;
		return 1;
	}
	return 0;
}

void Buffer_Adapter::update_properties_and_buffer()
{
	//full revalidation 
	HDC hdcScreen;
	HDC hdcWindow;
	HDC hdcMemDC = NULL;
	HBITMAP hbmScreen = NULL;
	BITMAP bmpScreen;


	// Retrieve the handle to a display device context for the client 
	// area of the window. 
	hdcScreen = GetDC(NULL); //device context of destination
	hdcWindow = GetDC(hwnd_to_source); //device context of source


	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcWindow);


	if (!hdcMemDC)
	{
		MessageBox(hwnd_to_source, L"CreateCompatibleDC has failed", L"Failed", MB_OK);
		//goto done;
	}

	// Get the client area for size calculation
	GetClientRect(hwnd_to_source, &rcClient);
	GetWindowRect(hwnd_to_source, &rcScreen);



	// Create a compatible bitmap from the Window DC
	hbmScreen = CreateCompatibleBitmap(hdcWindow, rcClient.right, rcClient.bottom);
	if (!hbmScreen)
	{
		MessageBox(hwnd_to_source, L"CreateCompatibleBitmap Failed", L"Failed", MB_OK);
		//goto done;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmScreen);

	// Bit block transfer into our compatible memory DC.
	if (!BitBlt(hdcMemDC,
		0, 0, //destination rectangle start
		rcClient.right, rcClient.bottom, //w and l of destination
		hdcWindow, //source handle
		0, 0, //source start
		SRCCOPY)) //raster data for color transforming
	{
		MessageBox(hwnd_to_source, L"BitBlt has failed", L"Failed", MB_OK);
		//goto done;
	}


	// Get the BITMAP from the HBITMAP
	GetObject(hbmScreen, sizeof(BITMAP), &bmpScreen);

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpScreen.bmWidth;
	bi.biHeight = bmpScreen.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD dwBmpSize = ((bmpScreen.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpScreen.bmHeight;

	//release the old source and 
	delete[] source; 
	HANDLE hbuffer_bmap = new BYTE[dwBmpSize];



	GetDIBits(hdcWindow, hbmScreen, 0,
		(UINT)bmpScreen.bmHeight,
		hbuffer_bmap,
		(BITMAPINFO *)&bi, DIB_RGB_COLORS);


	c_hdcScreen = hdcScreen;
	c_hdcWindow = hdcWindow;
	c_hdcMemDC = hdcMemDC;
	c_hbmScreen = hbmScreen;
	c_bi_header = bi;
	c_bmpScreen = bmpScreen;
	source = (BYTE*)hbuffer_bmap;
}

void Buffer_Adapter::update_buffer_no_change()
{
	//possibly swap to CreateDIBSection in a later version. 

	//lighetst posibble buffer update method. Just copy into existing source. 

	//transfer memory from the source window HANDLE into the destination HANDLE
	if (!BitBlt(c_hdcMemDC, //destination of transfer
		0, 0, //destination rectangle start
		rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, //w and l of destination
		c_hdcWindow, //source of transfer
		0, 0, //source start
		SRCCOPY)) //raster data for color transforming
	{
		MessageBox(hwnd_to_source, L"BitBlt has failed", L"Failed", MB_OK);
		//goto done
	}

	// Get the BITMAP from the HBITMAP HANDLE
	GetObject(c_hbmScreen, sizeof(BITMAP), &c_bmpScreen);

	//source buffer is now updated with new image data. 
	GetDIBits(c_hdcWindow, c_hbmScreen, 0,
		(UINT)c_bmpScreen.bmHeight,
		source,
		(BITMAPINFO *)&c_bi_header, DIB_RGB_COLORS);
}

void Buffer_Adapter::release_handles()
{
	//release all the handles and clear the source. 
	DeleteObject(c_hbmScreen);
	DeleteObject(c_hdcMemDC);
	ReleaseDC(NULL, c_hdcScreen);
	ReleaseDC(hwnd_to_source, c_hdcWindow);
	delete[] source; 
	source = nullptr; 
}

void Buffer_Adapter::setStencilLength(int x)
{
	stencil_length = x; 
}

void Buffer_Adapter::setStencilHeight(int y)
{
	stencil_height = y; 
}

BYTE * Buffer_Adapter::getRawBuffer() const
{
	return source;
}

Buffer_Adapter::~Buffer_Adapter()
{
	release_handles(); 
}
