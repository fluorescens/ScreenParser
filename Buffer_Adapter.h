#pragma once

//contains the pointer to main buffer. Handed normal cartesian points and replies with buffer data. Handles all internal conversions
class Buffer_Adapter
{
public:
	Buffer_Adapter(HWND);
	int getColorAtChannel(int cartesianx, int cartesiany, int channel) const;
	int cartesian_to_screen_x(int scriptx) const; 
	int cartesian_to_screen_y(int scriptx) const; 
	void update_buffer(); //determins if lightweight or full revalidate and buffer  
	void setStencilLength(int); 
	void setStencilHeight(int); 
	BYTE* getRawBuffer() const; 


	//window components
	HWND hwnd_to_source;
	BYTE* source; //pointer to source array
	HDC c_hdcMemDC;
	HDC c_hdcScreen;
	HBITMAP c_hbmScreen;
	HDC c_hdcWindow;
	BITMAP c_bmpScreen;
	BITMAPINFOHEADER c_bi_header;
	BITMAPFILEHEADER c_bmfHeader; 
	DWORD c_dwBmpSize; 

	//rectangle screen components. 
	RECT rcClient;

	~Buffer_Adapter();
private:
	void release_handles(); //releases the handles associated with the window
	int revalidate_properties(); 
	void update_properties_and_buffer(); //updates the window properties and buffer
	void update_buffer_no_change(); //lightweight update to buffer if no resize has occured.

	RECT rcScreen; //used in revalidation testing only. 

	int stencil_length;
	int stencil_height; 


	int cache_screen_top; //cache screen values to detect motion of screen resize. 
	int cache_screen_bottom;
	int cache_screen_left;
	int cache_screen_right;
};

