/*
The main executable file for crunchypack.
Takes a stencilpack and a handle to graphics window and builds the stencilpack object and codes into executable code that runs 
live analysis on the graphical region of the desktop currently occupied by the location and dimensions of the handle, forwarding the result to an interprogram cache

This is an active build. Changes are frequent, and with them documentation changes. 

NOTE: This program cannot enumerate all possible handle-to-windows on a target system. Use a program like Spy++ (Download from microsoft.) to locate the 
handle to the graphical window you want to capture. 


*/

#include "stdafx.h"
#include "screen_live_analysis.h"
#include <string>
#include <vector>
#include <memory>
#include <fstream>
#include <iostream>
#include <Windows.h>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <deque>
#include "Do_Object.h"
#include "Token.h"
#include "Token_Manager.h"
#include <queue>
#include "Buffer_Adapter.h"
#include "region_for_map.h"
#include <random>
#include <time.h>


#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const int GLOBAL_DEBUG_MODE = 1; //global debug mode. set 1 to on



//special vector escape argument numbers for the injector processes. Commands enclosed are NOT explicit, but escaped. 
const int DELAY_ESCAPE_CODE = -1;
const int MOUSE_ESCAPE_CODE = -2;
const int MOUSE_RUNTIME_INJECT_CODE = -3;
const int KEY_HOLD_PROXIMAL = -4; 
const int KEY_HOLD_RELEASE_HOLD = -5; 
const int IMAGE_SLICE_CAPTURE_CODE = -6; 


CWinApp theApp; // The one and only application object

void captureFromDesktop(Buffer_Adapter&, int);
int handle_window_move_check(const RECT& rcScreen, int previous_leftx, int previous_lefty, int previous_rightx, int previous_righty); 
void file_from_slice_atf(const Buffer_Adapter&, int client_startx, int client_starty, int xlength, int ylength);
int transform_client_to_screen(int client, const RECT rcScreen); 
int transform_client_y_to_buffer(int client, const RECT rcClient); 



int stencilpack_load(std::unordered_map<long int, int>&,
	const std::string&,
	int[2],
	std::string&,
	std::vector<std::string>&,
	std::vector<Do_Object*>&,
	std::string&,
	std::vector<std::vector<int>>&,
	std::vector<std::vector<int>>&,
	std::vector<std::vector<int>>&);

std::vector<std::vector<int>> parse_and_load_images(std::string& ); 

int bgr_table_lookup(std::unordered_map<long int, int>& rbg_hmap,
	std::deque<int>& hits_cache,
	int* hits,
	const std::vector<std::vector<int>>& blue_table,
	const std::vector<std::vector<int>>& green_table,
	const std::vector<std::vector<int>>& red_table,
	int blue, int green, int red); 


void symbolic_parser(std::unordered_map<long int, int>& rbg_hmap,
	std::vector<Do_Object*>& action_objects,
	const int overwatch_numeric_id,
	Token_Manager& tk_mgr,
	std::deque<Do_Object*>& unsorted_script_codes,
	const std::vector<Token*>& numerical_order,
	const std::vector<Token*>& execution_order,
	const Buffer_Adapter& screen,
	std::deque<int>& hits_cache,
	int* hits,
	const std::vector<std::vector<int>>& blue_table,
	const std::vector<std::vector<int>>& green_table,
	const std::vector<std::vector<int>>& red_table,
	bool overwatch_state); 

void pop_priority_queue(const Buffer_Adapter& screen_source, 
	std::deque<Do_Object*>& unsorted_script_codes,
	const std::vector<Do_Object*>& do_scripts,
	INPUT& ipt,
	HWND& hwnd,
	const std::unordered_map<std::string, std::vector<int>>& ksinject,
	std::unordered_map<std::string, region_for_map*> object_lookup_by_name);


int load_script_list(std::string& file, std::unordered_map<std::string, std::vector<int>>& ksinject);

std::vector<int> keystroke_vector_converter(std::string& cmdstr); 

void script_to_keystroke(const Buffer_Adapter& screen_source,
	INPUT& ipt,
	HWND& hwnd,
	const Do_Object* obj,
	const std::unordered_map<std::string, std::vector<int>>& ksinject,
	const std::unordered_map<std::string, region_for_map*>& string_to_region);

void load_map_with_regions(std::vector<std::string>& object_ident_sequences, std::unordered_map<std::string, region_for_map*>& object_lookup_by_name); 


void main_running_loop(std::unordered_map<long int, int>& rbg_hmap,
	std::vector<Do_Object*>& action_objects,
	const int overwatch_numeric_id,
	Token_Manager& tk_mgr,
	std::deque<Do_Object*>& unsorted_script_codes,
	const std::vector<Token*>& numerical_order,
	const std::vector<Token*>& execution_order,
	Buffer_Adapter& screen,
	std::deque<int>& hits_cache,
	int* hits,
	const std::vector<std::vector<int>>& blue_table,
	const std::vector<std::vector<int>>& green_table,
	const std::vector<std::vector<int>>& red_table,
	INPUT& ipt,
	HWND& hwnd,
	const std::unordered_map<std::string, std::vector<int>>& ksinject,
	std::unordered_map<std::string, region_for_map*> object_lookup_by_name
); 


int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

	BYTE* screen_source = nullptr;

    if (hModule != nullptr)
    {
        // initialize MFC and print and error on failure
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: change error code to suit your needs
            wprintf(L"Fatal Error: MFC initialization failed\n");
            nRetCode = 1;
        }
        else
        {
			//debugging purposes. 
			//change to your test build file location. 
			const std::string filepath =				"C:\\Users\\JamesH\\Pictures\\Screenshots\\Stencilpack_tnd_pages.txt";
			const std::string dbg_scriptpack_filepath = "C:\\Users\\JamesH\\Pictures\\Screenshots\\tnd_script.txt";

			/*
			Preallocation of variables to be defined with extracted stencilpack data. 
			*/
			//a size of stencil screen, line 1
			int stencilsize[2];
			//the execution sequence, line 2. 
			std::string execution_sequence;
			//as long as character 1 is not A, read object codes
			std::vector<std::string> identification_sequence;
			//As long as character is A, read in action codes & create do objects. 
			std::vector<Do_Object*> script_sequence;
			//When it is not A, read in the groups list
			std::string group_and_image_list;
			//until end of file, read the tab-delimited and build the RBG table. 
			std::vector<std::vector<int>> final_blue_table;
			final_blue_table.resize(256);
			std::vector<std::vector<int>> final_green_table;
			final_green_table.resize(256);
			std::vector<std::vector<int>> final_red_table;
			final_red_table.resize(256);

			std::unordered_map<long int, int> rbg_hmap; //builds the RBG table as a hash


			//if loadstatus nonzero, splash invalid file error and force to try again or abort 
			int loadstatus = 1;
			std::string stencil_filepath;
			while (loadstatus != 0) {
				std::cout << "Enter the full path to the stencilpack file: ";
				if (GLOBAL_DEBUG_MODE == 1) {
					stencil_filepath = filepath;
				}
				else {
					std::getline(std::cin, stencil_filepath);
				}
				loadstatus = stencilpack_load(rbg_hmap,
					stencil_filepath,
					stencilsize,
					execution_sequence,
					identification_sequence,
					script_sequence,
					group_and_image_list,
					final_blue_table,
					final_green_table,
					final_red_table);
				if (loadstatus != 0) {
					std::cout << "Could not find the file at the supplied path. \nBe sure to include the full file name and .txt extension. " << '\n';
				}
			}
			std::cout << "Pack load successful!" << '\n';

			//Build the fast lookup map of object identifier name to object dimensions and coordinates. 
			std::unordered_map<std::string, region_for_map*> object_lookup_by_name;
			load_map_with_regions(identification_sequence, object_lookup_by_name);


			//now get the script list file. 
			int scriptpack_load_status = 1;
			//the map of do script output strings to keystroke injections. 
			std::unordered_map<std::string, std::vector<int>> keystroke_injection; 
			std::string scriptpack_filepath;
			while (scriptpack_load_status != 0) {
				std::cout << "Enter the full path to the script builder file: ";
				if (GLOBAL_DEBUG_MODE == 1) {
					scriptpack_filepath = dbg_scriptpack_filepath;
				}
				else {
					std::getline(std::cin, scriptpack_filepath);
				}
				scriptpack_load_status = load_script_list(scriptpack_filepath, keystroke_injection);
				switch (scriptpack_load_status) {
					case 0:
						//successful loading code
						break; 
					case -1:
						std::cout << "Could not find the file at the supplied path. \nBe sure to include the full file name and .txt extension. " << '\n';
						break;
					case -2:
						std::cout << "The script file was found, but contained syntatical errors. Check the file again. " << '\n';
						break; 
					default:
						std::cout << "An unspecified error occured while reading the script list. " << '\n';
						break; 
				}
			}
			std::cout << "Script load successful!" << '\n'; 


			//SET UP VIRTUAL KEYBOARD
			INPUT ipt;
			ipt.type = INPUT_KEYBOARD;
			ipt.ki.wScan = 0; // hardware scan code for key
			ipt.ki.time = 0;
			ipt.ki.dwExtraInfo = 0;



			//prepare mock_items from a list of items a,a,grpID1, c,c,grpID1, b,b,grpID2
			//push back into group item number in sequence. 
			//std::string mock_init_string = "0,1,2,none,\nbluesquare256b.bmp,bluesquare256b.bmp,2,\tgreensquareb256.bmp,greensquareb256.bmp,3,\tredsquareb256.bmp,redsquareb256.bmp,3,\t\r\n"; 
			std::vector<std::vector<int>> mock_items = parse_and_load_images(group_and_image_list);
			//need total number of mock items in the items vector. 
			int itemcount = 0; 
			for (int i = 0; i < mock_items.size(); ++i) {
				itemcount += mock_items[i].size(); 
			}


			//get back to smart pointer once stablized.
			int* hits = new int[itemcount]; //allocate once, array for tracking which images got a hit at bgr
			for (int i = 0; i < itemcount; ++i) {
				//initialize array to 0. 
				hits[i] = 0;
			}
			
			//Cache the hit objects that will need to be reset after symbolic parse. 
			std::deque<int> hits_cache;

			Token_Manager tk_mgr(mock_items);
			std::string exe_order = execution_sequence;
			//add identification tokens to the manager
			for (int i = 0; i < identification_sequence.size(); ++i) {
				tk_mgr.add_token(identification_sequence[i]);
			}
			//build the finalized execution symbols. 
			tk_mgr.build_execution_order(exe_order);
			//access the map and get the name value for overwatch. 
			const int overwatch_numerical_id = tk_mgr.getOverwatchNumeric();

			//all the scripts that need to be executed for this pass of the execution. 
			//could have a vector of xy pairs but it would be unsorted. How to link deque to vector? 
			//could actually attatch callx cally parameters to do object. at symbolic parser build time. 
			std::deque<Do_Object*> unsorted_script_codes;

			/*
			Add a function that grabs, updates, and caches the origin of the rectangle associated with the handle. Notify on movement or changes.
			*/
			//use microsft spy++ to find the handle for the desired window
			//the handle CANNOT be minimized as spy++ is searching or it won't appear locatable. 
			std::wstring string_target_handle;
			std::cout << "Enter the handle to the window of the screen you want to capture.\n";
			if (GLOBAL_DEBUG_MODE == 1) {
				//string_target_handle = L"pseeditor";
				//string_target_handle = L"Notepad";
				string_target_handle = L"PPTFrameClass";
			}
			else {
				std::getline(std::wcin, string_target_handle);
			}
			LPCWSTR lpcwstr_target_handle = string_target_handle.c_str();
			HWND hwnd = FindWindow((lpcwstr_target_handle), NULL);    //the window can't be minimized. Pass handle-to-MC 
																	  //find the correct handle from spy++
			RECT rcClient; //1307 297 //scale script y down
			if (hwnd == NULL)
			{
				std::wcout << L"Could not locate window " << string_target_handle << std::endl;
			}
			else {
				std::wcout << L"Found such a window!";
			}



			//and screen interaaction or data requests MUST be handled by the adpater. 
			//create the containe rfor the screen object.
			Buffer_Adapter screen_buffer(hwnd); 
			screen_buffer.setStencilLength(stencilsize[0]); 
			screen_buffer.setStencilHeight(stencilsize[1]);


			//-------------------------------------------KEY INJECTOR SHORT CIRCUIT TEST
			/*

			Do_Object* d1 = new Do_Object(0, "a");
			Do_Object* d2 = new Do_Object(0, "b");
			d2->setx(1000);
			d2->sety(500);
			Do_Object* d3 = new Do_Object(0, "c");


			script_to_keystroke(screen_buffer, ipt, hwnd, d2, keystroke_injection, object_lookup_by_name);
			script_to_keystroke(screen_buffer, ipt, hwnd, d1, keystroke_injection, object_lookup_by_name);
			//script_to_keystroke(ipt, hw, d3, keystroke_injection);

			//Test image slice object.
			region_for_map* mp = new region_for_map(0, 200, 100, 100, "R0");
			object_lookup_by_name.emplace("R0", mp);

			*/
			//std::wstring hdl = L"Notepad";
			//LPCWSTR hdd = hdl.c_str();
			//HWND hw = FindWindow((hdd), NULL);


			//-------------------------------------------KEY INJECTOR SHORT CIRCUIT TEST END


			//Number of cycle runs. 
			for (int i = 0; i < 3; ++i) {
				main_running_loop(rbg_hmap,
					script_sequence,
					overwatch_numerical_id,
					tk_mgr,
					unsorted_script_codes,
					tk_mgr.access_numerical_order(),
					tk_mgr.access_execution_order(),
					screen_buffer,
					hits_cache,
					hits,
					final_blue_table,
					final_green_table,
					final_red_table,
					ipt,
					hwnd,
					keystroke_injection,
					object_lookup_by_name);
			}
        }
    }
    else
    {
        // TODO: change error code to suit your needs
        wprintf(L"Fatal Error: GetModuleHandle failed\n");
        nRetCode = 1;
    }

	system("pause"); 
    return nRetCode;
} 




/*
Gets the handle to the screen in memory and creates a bitmap source for the buffer.
DEBUG FEATURE: Turns buffer into a .bmp screenshot. 
*/
void  captureFromDesktop(Buffer_Adapter& screen_buffer, int identifier)
{

	if(GLOBAL_DEBUG_MODE == 1) {
		std::wstring file_prefix = L"captureqws";
		std::wstring edition = std::to_wstring(identifier);
		std::wstring ftype = L".bmp";
		std::wstring file_name = file_prefix + edition + ftype;
		LPCWSTR sw = file_name.c_str();


		// A file is created, this is where we will save the screen capture.
		HANDLE hFile = CreateFile(sw,
			GENERIC_WRITE,
			0,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL, NULL);

		// Add the size of the headers to the size of the bitmap to get the total file size
		DWORD dwSizeofDIB = screen_buffer.c_dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER); 

		DWORD dwBytesWritten = 0;
		WriteFile(hFile, (LPSTR)&screen_buffer.c_bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
		WriteFile(hFile, (LPSTR)&screen_buffer.c_bi_header, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
		WriteFile(hFile, (LPSTR)screen_buffer.source, screen_buffer.c_dwBmpSize, &dwBytesWritten, NULL);

		//Close the handle for the file that was created
		CloseHandle(hFile);
	}
}




/*
Test if revalidation of window needs to occur by checking if window origins or size changed. 
Return 0 if revalidation not required, 1 if required. 
*/
int handle_window_move_check(const RECT& rcScreen, int previous_leftx, int previous_lefty, int previous_rightx, int previous_righty) {
	//check the rect of hwnd. Does it match origin? If not, need to call a revalidate. 
	rcScreen.left; 

	//if all 4 match, return 0; No revalidation required. 
	//else return 1: revalidation reuqired. 
	if (rcScreen.left == previous_leftx &&
		rcScreen.top == previous_lefty &&
		rcScreen.right == previous_rightx &&
		rcScreen.bottom == previous_righty) {
		return 0; 
	}
	else {
		return 1; 
	}

	return 1; 
}






/*
GIVEN: Cartesian x/y and a length
Captures a slice of the buffer into a bitmap
*/
void file_from_slice_atf(const Buffer_Adapter& screen_source, int cartesian_startx, int cartesian_starty, int xlength, int ylength) {


	/*So...turns out 0,0 on the buffer is at the BOTTOM RIGHT, NOT THE TOP LEFT.

	Need to invert all coordinates from client. 
	*/

	//cpack assumes a top-left 0,0 coordinate system. This system is in a bottom left coordinate scheme. 

	/*
	NEED TO SCALE LENGTH AND HEIGHT AS WELL. 
	*/


	//calcluate the offset from 0 in the buffer to the desired index in the buffer. 
	//so need to translate the client x/y requirements into buffer offsets (where does this region start in source buffer?)
	int client_start_y = screen_source.cartesian_to_screen_y(cartesian_starty);
	int client_start_x = screen_source.cartesian_to_screen_x(cartesian_startx);
	int buffer_offset_to_start = (((client_start_y- ylength)*screen_source.rcClient.right) * 4 + client_start_x * 4);


	//allocate space for the bitmap to be copied to for the file. Need to try-catch. 
	BYTE* slice = new BYTE[(xlength * ylength)*4];
	for (int h = 0; h < (xlength*ylength); ++h) {
		slice[4 * h] = 0;
		slice[4 * h + 1] = 0;
		slice[4 * h + 2] = 255;
		slice[4 * h + 3] = 0;
	}


	//now we need to copy the data into the slice buffer. 
	//first line works. just need tyo get the jump right. 
	//0,0 is located at the bottom left. 
	const int jump_offset = ((screen_source.rcClient.right) - (xlength))*4; //the offset when jumping down a row. 

	int add_offset = 0; 
	int current_dot_counter = 0; 
	for (int h = 0; h < (xlength*ylength); ++h) {
		if (current_dot_counter == (xlength)) {
			add_offset += jump_offset;
			current_dot_counter = 0;
		}
		//possibly need a try-catch to autoreturn last index if try to exceed buffer bounds. 
		slice[4 * h] = screen_source.source[buffer_offset_to_start + 4 * h + 0 + add_offset];
		slice[4 * h + 1] = screen_source.source[buffer_offset_to_start + 4 * h + 1 + add_offset];
		slice[4 * h + 2] = screen_source.source[buffer_offset_to_start + 4 * h + 2 + add_offset];
		slice[4 * h + 3] = screen_source.source[buffer_offset_to_start + 4 * h + 3 + add_offset];
		++current_dot_counter; 
	}


	BITMAPINFOHEADER   bi;
	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = xlength; //length in pixels
	bi.biHeight = ylength;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;


	DWORD dwBmpSize = (((bi.biWidth) * bi.biBitCount + 31) / 32) * 4 * bi.biHeight;

	//randomized extension generator. 
	std::random_device rd;
	std::srand(rd());
	int random_extension = rand() % 1000000;
	std::wstring extension = std::to_wstring(random_extension); 

	std::wstring file_name = L"slice" + extension + L".bmp"; 

	LPCWSTR fname = file_name.c_str(); 

	HANDLE hFile = CreateFile(fname,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	BITMAPFILEHEADER   bmfHeader;
	bmfHeader.bfType = 0x4D42; //BM  
	bmfHeader.bfSize = dwSizeofDIB;
	//bfType must always be BM for Bitmaps 
	bmfHeader.bfReserved1 = 0;
	bmfHeader.bfReserved2 = 0;
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	DWORD dwBytesWritten = 0;
	WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
	WriteFile(hFile, (LPSTR)slice, dwBmpSize, &dwBytesWritten, NULL);

	//Unlock and Free the DIB from the heap

	//Close the handle for the file that was created
	CloseHandle(hFile);
	delete[] slice; 
}


/*
Takes the request for a coordinate in the client system and converts it into the correct screen coordinate

An incredibly annoying feature from stencilpack is the location of 0,0. 
Stencilpack origin is the upper-left. 
Windows graphical object origins are the lower left. 
This results in the need to mirror every stencilpack Y coordinate to find the equivalent windows Y
This annoyance is not so much a glitch but is noted as a low, but very very very nice to fix, priority issue. 

*/
int transform_client_to_screen(int client, const RECT rcScreen) {

	return (rcScreen.left + client); 
}


int transform_client_y_to_buffer(int client, const RECT rcClient) {
	//mirror y coordinate
	//recieve 0, turn into rcclient top. 
	//recieve max turn to 0
	int midpoint = rcClient.bottom / 2; 
	if (client > midpoint) {
		return (midpoint - (midpoint - client));
	}
	else {
		return (midpoint + (midpoint - client)); 
	}
}




int transform_plain_to_client_x(int scriptx, const RECT rcClient, int script_length) {
	//the script requests an x which it assumes is cartesian 0 at the size in file header. 
	//x does not require inversion but does require rescaling. 

	//1000/800 expected/actual scale down scri
	float adjust_x = (float)rcClient.right/ (float)script_length;
	int stretched_x = (float)scriptx*adjust_x; 

	return stretched_x; 
}

int transform_plain_to_client_y(int scripty, const RECT rcClient, int script_height) {
	//invert y then stretch the coordinate

	/* reversion
		int midpoint = rcClient.bottom / 2;
	int post_inversion = 0; 
	if (scripty > midpoint) {
		post_inversion = (midpoint - (midpoint - scripty));
	}
	else {
		post_inversion = (midpoint + (midpoint - scripty));
	}

	float adjust_y = (float)rcClient.bottom / (float)script_height;
	int stretched_y = (float)scripty*adjust_y;

	return stretched_y; 
	*/

	int midpoint = rcClient.bottom / 2;

	float adjust_y = (float)rcClient.bottom / (float)script_height;
	int stretched_y = (float)scripty*adjust_y;

	int post_inversion = 0;
	if (stretched_y > midpoint) {
		post_inversion = (midpoint - (stretched_y - midpoint));
	}
	else {
		post_inversion = (midpoint + (midpoint - stretched_y));
	}

	return post_inversion;
}




/*
MAIN RUNNING LOOP FUNCTION
Takes the current screen objects bitmap memory and loads and appropriate Do_Objects into the unsorted queue
The queue is then popped in order of script priority and the appropriate keystroke injection sequence is fired
Then the buffer is updated. 
*/
void main_running_loop(std::unordered_map<long int, int>& rbg_hmap,
	std::vector<Do_Object*>& action_objects,
	const int overwatch_numeric_id,
	Token_Manager& tk_mgr,
	std::deque<Do_Object*>& unsorted_script_codes,
	const std::vector<Token*>& numerical_order,
	const std::vector<Token*>& execution_order,
	Buffer_Adapter& screen,
	std::deque<int>& hits_cache,
	int* hits,
	const std::vector<std::vector<int>>& blue_table,
	const std::vector<std::vector<int>>& green_table,
	const std::vector<std::vector<int>>& red_table,
	INPUT& ipt,
	HWND& hwnd,
	const std::unordered_map<std::string, std::vector<int>>& ksinject,
	std::unordered_map<std::string, region_for_map*> object_lookup_by_name
) {

	symbolic_parser(rbg_hmap,
		action_objects,
		overwatch_numeric_id,
		tk_mgr,
		unsorted_script_codes,
		tk_mgr.access_numerical_order(),
		tk_mgr.access_execution_order(),
		screen,
		hits_cache,
		hits,
		blue_table,
		green_table,
		red_table,
		false);


	pop_priority_queue(screen, unsorted_script_codes, action_objects, ipt, hwnd, ksinject, object_lookup_by_name);

	/*
	Forcibly reset all condition flags of all objects to 0 after the queue is popped or GETS will re-queue themselves. 
	However, states now can no longer persist between frames.
	Should frames be stateless? Or does each point need a if NOT [0,0] accompanying it?
	For now, frames are stateless. This may change in the future. 
	*/

	//get access to token manager and flush all condition vectors. use execution order. 
	tk_mgr.reset_all_condition_flags(); 

	screen.update_buffer();
}











/*
Extracts the objects, images, pixels, and trigger codes from the stencilpack file. 
*/
int stencilpack_load(std::unordered_map<long int, int>& rbg_hmap,
	const std::string& filepath,
	int pack_dimensions[2],
	std::string& exe_seq,
	std::vector<std::string>& object_ident_sequences,
	std::vector<Do_Object*>& script_sequence,
	std::string& group_and_image_list,
	std::vector<std::vector<int>>& final_blue_table,
	std::vector<std::vector<int>>& final_green_table,
	std::vector<std::vector<int>>& final_red_table) {
	/*
	//from file, extract...
	//a size of stencil screen, line 1
	int stencilsize[2];
	//the execution sequence, line 2.
	std::string execution_sequence;
	//as long as character 1 is not A, read object codes
	std::vector<std::string> identification_sequence;
	//As long as character is A, read in action codes & create do objects.
	std::vector<Do_Object> script_sequence;
	//When it is not A, read in the groups list
	std::string group_list;
	//next line, read in image names
	std::string image_list;
	//until end of file, read the tab-delimited and build the RBG table.
	std::vector<std::vector<int>> final_blue_table;
	std::vector<std::vector<int>> final_green_table;
	std::vector<std::vector<int>> final_red_table;

	doesnt build with standalones. 
	*/

	std::ifstream stencil_file(filepath, std::ios::in);
	std::string line;
	std::string tmpline;
	int line_counter = 0;
	bool done_identifiers = false;
	bool done_actions = false;
	bool loading_hashmap = false; //if hashmap mode, set to true. 
	bool loading_hashmap_check_firstline = false;

	//matching strings for building Do_Objects. Delay is a special command. 
	const std::string search_do = "DO[";
	const std::string search_delay = "DELAY[";

	int table_builder_counter = 0; //tracks which rbg table line to insert on. 

	if (stencil_file) {
		while (std::getline(stencil_file, line)) {
			//what if the picture name or groups start with pora?
			/*
			switch the doneactions state
			*/
			switch (line_counter) {
			case 0: {
				//extract dimensions
				std::string substr;
				for (int i = 0; i < line.length() + 1; ++i) {
					if (line[i] == ' ' || i == line.length()) {
						if (line[i] == ' ') {
							pack_dimensions[0] = std::stoi(substr);
							substr.clear();
						}
						else {
							pack_dimensions[1] = std::stoi(substr);
							break;
						}
					}
					else {
						substr += line[i];
					}
				}
				++line_counter;
				break;
			}
			case 1:
				exe_seq = line;
				++line_counter;
				break;
			case 2:
				switch (line[0]) {
				case 'P':
					object_ident_sequences.push_back(line);
					break;
				case 'O':
					object_ident_sequences.push_back(line);
					break;
				case 'R':
					object_ident_sequences.push_back(line);
					break;
				case 'A': {
					//dealing with actual action object. Some fairly complex regex extraction goes here.  
					//DO[priority,alpha or numeric string] DELAY[priority, duration in ms]
					//could recieve a string DO[7,12] DELAY[4,3000}
					//priority is the highest priority of all tokens. 
					//really since vba interprets the sequence, don't need a custom delay. 
					//delay is speecial. actually halts updating within the program itself. 
					//no implementation for center, yet. 

					std::vector<size_t> do_positions; // holds all the positions that sub occurs within str
					size_t pos = line.find(search_do, 0);
					while (pos != std::string::npos)
					{
						do_positions.push_back(pos);
						pos = line.find(search_do, pos + 1);
					}

					std::vector<size_t> delay_positions; // holds all the positions that sub occurs within str
					size_t posb = line.find(search_delay, 0);
					while (posb != std::string::npos)
					{
						delay_positions.push_back(posb);
						posb = line.find(search_delay, posb + 1);
					}

					if (delay_positions.size() == 0) {
						delay_positions.push_back(9999999);
					}

					if (do_positions.size() == 0) {
						do_positions.push_back(9999999);
					}


					//merge the two vectors
					std::string command;
					int do_ext_ctr = 0;
					int do_next_ctr = 0;
					int delay_ext_ctr = 0;
					int delay_next_ctr = 0;
					int highpriority = 0;
					for (int m = 0; m < (do_positions.size() + delay_positions.size()); ++m) {
						if (do_positions[do_ext_ctr] < delay_positions[delay_ext_ctr]) {
							std::string subdata;
							for (int n = do_positions[do_ext_ctr] + 3; n < line.length(); ++n) {
								if (line[n] == ',' || line[n] == ']') {
									if (line[n] == ',') {
										int localpriority = std::stoi(subdata);
										if (localpriority > highpriority) {
											highpriority = localpriority;
										}
										subdata.clear();
									}
									else {
										if (subdata == "*") {
											//internal capture image capture character. 
											command += "#*#";
										}
										else {
											command += subdata;
										}
										break;
									}
								}
								else {
									subdata += line[n];
								}
							}
							++do_next_ctr;
							if (do_next_ctr < do_positions.size()) {
								++do_ext_ctr;
							}
							else {
								//exhausted all do options. make sure it doesn't re-enter. 
								do_positions[do_ext_ctr] = 9999999;
							}
						}
						else if (delay_positions[delay_ext_ctr] < do_positions[do_ext_ctr]) {
							//handling a delay
							std::string subdata;
							for (int n = delay_positions[delay_ext_ctr] + 6; n < line.length(); ++n) {
								if (line[n] == ',' || line[n] == ']') {
									if (line[n] == ',') {
										int localpriority = std::stoi(subdata);
										if (localpriority > highpriority) {
											highpriority = localpriority;
										}
										subdata.clear();
									}
									else {
										//insert special delay timer. don't read this to output, handle internally. 
										command += "#" + subdata + "#";
										break;
									}
								}
								else {
									subdata += line[n];
								}
							}
							++delay_next_ctr;
							if (delay_next_ctr < delay_positions.size()) {
								delay_ext_ctr = delay_positions[delay_next_ctr];
							}
							else {
								//exhausted all do options. make sure it doesn't re-enter. 
								delay_positions[delay_ext_ctr] = 9999999;
							}
						}
					}

					Do_Object* tmpdo = new Do_Object(highpriority, command);
					script_sequence.push_back(tmpdo);
					break;
				}
				default: {
					if (line[line.length() - 1] != ';') {
						//read into the next line..but it doesn't necessarity start with a. 
						tmpline = line;
						++line_counter;
						done_actions = true;
					}
					break;
				}
				}
				break;
			case 3:
				//combine the group and itemname line
				group_and_image_list = tmpline + '\n' + line + '\n';
				++line_counter;
				break;
			case 4:
				//rbg tables built here. 
				//0 1,2,	0,2,	0,1,

				if (loading_hashmap == true) {
					//load map procedure. 
					std::string temp;
					long int hvalue = 0;
					int image_id = 0;
					for (int s = 0; s < line.length() + 1; ++s) {
						if (line[s] == ' ' || s == line.length()) {
							if (s < line.length()) {
								hvalue = std::stoi(temp);
								temp.clear();
							}
							else if (s == line.length()) {
								image_id = std::stoi(temp);
								rbg_hmap[hvalue] = image_id; //I suspect this will result in a failure point. 
								break;
							}
						}
						else {
							temp += line[s];
						}
					}
				}
				else {
					//eval if loading hmap true. 
					//if so, due first digit and break
					//else, do normal load
					if (loading_hashmap_check_firstline == false) {
						//test if line contains a tab. if not, set loading to true and process hashmap bits
						int tabcount = 0;
						for (int s = 0; s < line.length(); ++s) {
							if (line[s] == '\t') {
								++tabcount;
								break;
							}
						}
						if (tabcount == 0) {
							loading_hashmap = true;
							std::string temp;
							long int hvalue = 0;
							int image_id = 0;
							for (int s = 0; s < line.length() + 1; ++s) {
								if (line[s] == ' ' || s == line.length()) {
									if (s < line.length()) {
										hvalue = std::stoi(temp);
										temp.clear();
									}
									else {
										image_id == std::stoi(temp);
										rbg_hmap[hvalue] = image_id;
										break;
									}
								}
								else {
									temp += line[s];
								}
							}
						}
						loading_hashmap_check_firstline = true;
					}


					//now try and load table. Don't if load_hmap mode set. 
					if (loading_hashmap == true) {
						//do nothing. alread processed first.
					}
					else {
						//Processing a table. 
						std::string substr;
						bool first_tab = true;
						for (int k = 0; k < line.length() + 1; ++k) {
							//space, tab, tab, newline. 
							if (k == line.length()) {
								std::string localsub;
								for (int n = 0; n < substr.length(); ++n) {
									if (substr[n] == ',') {
										final_red_table[table_builder_counter].push_back(std::stoi(localsub));
										localsub.clear();
									}
									else {
										localsub += substr[n];
									}
								}
								break;
							}
							if (line[k] == ' ' || line[k] == '\t' || line[k] == '\n' || line[k] == '\r') {
								switch (line[k]) {
								case ' ':
									substr.clear();
									break;
								case '\t':
									switch (first_tab) {
									case true: {
										if (substr.empty()) {
											//prevent blank tabs from getting casught in stoi
											first_tab = false;
											substr.clear();
											break;
										}
										//blue group
										std::string locsubstr;
										for (int n = 0; n < substr.length(); ++n) {
											if (substr[n] == ',') {
												final_blue_table[table_builder_counter].push_back(std::stoi(locsubstr));
												locsubstr.clear();
											}
											else {
												locsubstr += substr[n];
											}
										}
										substr.clear();
										first_tab = false;
										break;
									}
									case false: {
										if (substr.empty()) {
											substr.clear();
											break;
										}
										//green group
										std::string locsubstr;
										for (int n = 0; n < substr.length(); ++n) {
											if (substr[n] == ',') {
												final_green_table[table_builder_counter].push_back(std::stoi(locsubstr));
												locsubstr.clear();
											}
											else {
												locsubstr += substr[n];
											}
										}
										substr.clear();
										break;
									}
									}
									break;
								case '\r':
									//red group, need tyo run into \n
								case '\n': {
									//red table
									std::string locsubstr;
									for (int n = 0; n < substr.length(); ++n) {
										if (substr[n] == ',') {
											final_red_table[table_builder_counter].push_back(std::stoi(locsubstr));
											locsubstr.clear();
										}
										else {
											locsubstr += substr[n];
										}
									}
									break;
								}
								}
							}
							else {
								substr += line[k];
							}
						}
						++table_builder_counter;
					}
				}
				break;
			}
		}
		return 0;
	}
	else {
		return 1;
	}

	return 1;

}



/*
From the stencilpack supplied string, extract the names and groups of the images used by the watchobjects
*/
std::vector<std::vector<int>> parse_and_load_images(std::string& loadstring) {
	//table is written in BGR order
	int lcount = 0;
	std::string substr_groups;
	std::string substr_images;
	std::vector<std::vector<int>> img_by_group;
	int img_added_count = 0;
	int group_counts = 0;

	int local_comma = 0;
	std::string group_member_number;

	for (int i = 0; i < loadstring.length(); ++i) {
		if (loadstring[i] == '\r' || loadstring[i] == '\n') {
			if (loadstring[i] == '\r') {
				++i;
			}
			switch (lcount) {
			case 0:
				for (int c = 0; c < substr_groups.length(); ++c) {
					if (substr_groups[c] == ',') {
						++group_counts;
					}
					else {

					}
				}
				img_by_group.resize(group_counts);
				break;
			case 1:
				//extract image group from end of the image listing. Push back the COUNTER value onto the group. 
				//parse name string, everything between comma number 2 and 3 on each tab delimited string. 
				for (int d = 0; d < substr_images.length(); ++d) {
					if (substr_images[d] == ',') {
						++local_comma;
						switch (local_comma) {
						case 3:
							//extract the group ID from the last CSV in the tab delimited line
							int group_id = std::stoi(group_member_number);
							//push back the objects order-number into the apprpriate group. 
							img_by_group[group_id].push_back(img_added_count);
							++img_added_count;
							local_comma = 0;
							group_member_number.clear();
							break;
						}
					}
					else {
						if (local_comma == 2) {
							group_member_number += substr_images[d];
						}
					}
				}
				break;
			}
			++lcount;
		}
		else {
			switch (lcount) {
			case 0:
				substr_groups += loadstring[i];
				break;
			case 1:
				substr_images += loadstring[i];
				break;
			}
		}
	}
	return img_by_group;
}



/*
Using the object_ident_sequences vector as a source, create a map of string identifers to region_for_map structs for fast coordinate and dimension lookups, 
*/
void load_map_with_regions(std::vector<std::string>& object_ident_sequences, std::unordered_map<std::string, region_for_map*>& object_lookup_by_name) {
	for (int i = 0; i < object_ident_sequences.size(); ++i) {
		std::string temp = object_ident_sequences[i];
		
		std::string substr = "";
		int cord_info[4] = { 0,0,1,1 };
		int j = 0;
		int cord_iter = 0;
		
		while (temp[j] != '(') {
			substr += temp[j];
			++j;
		}
		++j;
		std::string subnum = "";
		while (temp[j] != ')') {
			if (temp[j] == ',') {
				cord_info[cord_iter] = std::stoi(subnum);
				++cord_iter; 
				subnum.clear(); 
			}
			else {
				subnum += temp[j];
			}
			++j;
		}
		cord_info[cord_iter] = std::stoi(subnum);

		//create the region with dimensions. 
		region_for_map* nr = new region_for_map(cord_info[0], cord_info[1], cord_info[2], cord_info[3], substr); 

		//add the item to the map for fast dimension access by name
		object_lookup_by_name.emplace(substr, nr); 
	}
}



/*
KEY COMPONENT
Takes all the extracted stencilpack data and executes a check against every watchobjects object trigger scripts.
If every trigger leading up to an action is true, that action will be added to the execution queue. 
A false condition in the statements leading up to an action will abort its enqueuement. 
The actions that were enqueue'd per this execution are executed and flushed by pop_priority_queue
*/
void symbolic_parser(std::unordered_map<long int, int>& rbg_hmap,
	std::vector<Do_Object*>& action_objects,
	const int overwatch_numeric_id,
	Token_Manager& tk_mgr,
	std::deque<Do_Object*>& unsorted_script_codes,
	const std::vector<Token*>& numerical_order,
	const std::vector<Token*>& execution_order,
	const Buffer_Adapter& screen,
	std::deque<int>& hits_cache,
	int* hits,
	const std::vector<std::vector<int>>& blue_table,
	const std::vector<std::vector<int>>& green_table,
	const std::vector<std::vector<int>>& red_table,
	bool overwatch_state) {
	if (overwatch_state == true) {
		//std::cout << "transject";
	}

	for (int i = 0; i < execution_order.size(); ++i) {
		//for each in execution order, access the symbolic vector and evaluate each line. If true up to a do/set, do that. If flase. slip to next set or end. 
		//the number for a get refers to the numerical ordering vector
		const std::vector<std::vector<int>>& current_symbols = execution_order[i]->access_symbols();
		std::cout << '\n' << execution_order[i]->toStringConditions() << '\n';
		//Current symbols for a single token. 

		bool set_not = false; //if set to true, invert the outcome of the next operation. 
		bool set_or = false;

		bool continue_on = true; //on false, skip to the next -5 or -6. Set skipping state to true. Once a non 5-6 is found with flag set, reset flags and do.
		bool skipping_state_flag = false;

		//implement a manager to handle OR cases with a continue-on being false
		bool or_forgiveness_state = false;

		for (int j = 0; j < current_symbols.size(); ++j) {
			//allows survival of a previous false if OR was last called. 
			if (continue_on == false && set_or == true) {
				continue_on = true;
				set_or = false;
				or_forgiveness_state = true;
			}
			if (continue_on == false && or_forgiveness_state == true) {
				continue_on = false;
				or_forgiveness_state = false;
				set_or = false;
			}

			//current line of symbols in token 
			switch (current_symbols[j][0]) {
			case -1: //IF check, Typical -1-numerical_ident-match_this
					 //if is at most composed of 4 symbols. IF-operation, NOT, matchID, OR

					 //header: if we hit a false, skip until we hit a DO/SET. Skip all DO/SETS. Resume on the next IF/GETIF
				if (continue_on == false) {
					if (skipping_state_flag == true) {
						continue_on = true;
						skipping_state_flag = false;
					}
					else {
						break;
					}
				}

				for (int k = 1; k < current_symbols[j].size(); ++k) {
					switch (current_symbols[j][k]) {
					case -2:
						//set NOT
						set_not = true;
						break;
					case -3:
						//set OR
						set_or = true;
						break;
					default:
						//must be a match ID
						switch (execution_order[i]->getName().at(0)) {
						case 'O': {
							if (overwatch_state == true) {
								//if in an overwatch state, run past this O and into P
							}
							else {
								//overwatch state is false, need to use deque as exeuction and recurse call. 
								//the object is a deque...function expects a vector. 
								//sets all the flags for the overwatch points
								symbolic_parser(rbg_hmap,
									action_objects,
									overwatch_numeric_id,
									tk_mgr,
									unsorted_script_codes,
									numerical_order,
									tk_mgr.get_overwatch_points(), //recurse with overwatch vector of points as the exeuction order. 
									screen,
									hits_cache,
									hits,
									blue_table,
									green_table,
									red_table,
									true);
								break;
							}
						}
						case 'P': {
							//get X, Y from the object. 
							int cartesianx = execution_order[i]->get_ulx();
							int cartesiany = execution_order[i]->get_uly();
							//call screen to get the current B G R and x,y
							//Need to translate screenx/y into 
							int cblue = screen.getColorAtChannel(cartesianx, cartesiany, 0);
							int cgreen = screen.getColorAtChannel(cartesianx, cartesiany, 1);
							int cred = screen.getColorAtChannel(cartesianx, cartesiany, 2);

							//call bgr table lookup to retrieve the match for this BGR
							int match = bgr_table_lookup(rbg_hmap, hits_cache, (hits), blue_table, green_table, red_table, cblue, cgreen, cred);

							//Verify if this is a match for the expected. 
							int valat = current_symbols[j][k];
							if (match == current_symbols[j][k]) {
								if (set_not == false) {
									continue_on = true;
								}
								else {
									continue_on = false;
								}
							}
							else {
								if (set_not == true) {
									continue_on = true;
								}
								else {
									continue_on = false;
								}
							}
							set_not = false;
							break;
						}
						case 'R': {
							/*
							Regions implement a lookup by scanning the whole region and aborting once the object is found anywhere within.
							*/
							//get X, Y from the object. 
							int screenux = execution_order[i]->get_ulx();
							int screenuy = execution_order[i]->get_uly();
							int screenlx = execution_order[i]->get_lrx();
							int screenly = execution_order[i]->get_lry();

							//do a test over the ENTIRE region. True and break on first true. False only if all false. 
							bool match_once = false;
							for (int n = screenux; n <= screenux + screenlx; ++n) {
								for (int o = screenuy; o <= screenuy + screenly; ++o) {
									int cblue = screen.getColorAtChannel(n, o, 0);
									int cgreen = screen.getColorAtChannel(n, o, 1);
									int cred = screen.getColorAtChannel(n, o, 2);

									int match = bgr_table_lookup(rbg_hmap, hits_cache, (hits), blue_table, green_table, red_table, cblue, cgreen, cred);
									int valat = current_symbols[j][k];
									if (match == current_symbols[j][k]) {
										if (set_not == false) {
											match_once = true;
											break;
										}
										else {
											match_once = false;
										}
									}
									else {
										if (set_not == true) {
											match_once = true;
											break;
										}
										else {
											match_once = false;
										}
									}
								}
								if (match_once == true) {
									break;
								}
							}

							//Verify if this is a match for the expected. 
							if (match_once == true) {
								continue_on = true;
							}
							else {
								continue_on = false;
							}

							set_not = false;
							break;
						}
								  //break;
						}
						break;
					}
				}
				break;
			case -4: //IF-GET
				if (continue_on == false) {
					if (skipping_state_flag == true) {
						continue_on = true;
						skipping_state_flag = false;
					}
					else {
						break;
					}
				}

				int foreign_id;
				int foreign_condition_index;
				int foreign_expected_value;
				for (int k = 1; k < current_symbols[j].size(); ++k) {
					switch (current_symbols[j][k]) {
					case -2:
						//set NOT
						set_not = true;
						break;
					case -3:
						//set OR
						set_or = true;
						break;
					default:
						//get numeric ID, condition index, check if value.
						//incriments k over the field. 
						//ISSUE: How to retrieve overwatch conditions? It refrences a point that is the first in the deque. 
						//check if foriegn_id is for overwatch. if so, intercept the at call and replace with a lookup to overwatch 
						foreign_id = current_symbols[j][k];
						foreign_condition_index = current_symbols[j][k + 1];
						foreign_expected_value = current_symbols[j][k + 2];
						k = k + 3;

						int foreign_condition_actual;
						if (foreign_id == overwatch_numeric_id) {
							foreign_condition_actual = tk_mgr.get_overwatch_condition(foreign_condition_index);
						}
						else {
							foreign_condition_actual = numerical_order.at(foreign_id)->get_condition_value(foreign_condition_index);
						}
						if (foreign_condition_actual == foreign_expected_value) {
							if (set_not == false) {
								continue_on = true;
							}
							else {
								continue_on = false;
							}
						}
						else {
							if (set_not == true) {
								continue_on = true;
							}
							else {
								continue_on = false;
							}
						}
						set_not = false;
						break;
					}
				}
				break;
			case -5: //DO
				if (continue_on == false) {
					skipping_state_flag = true;
					break;
				}
				//otherwise, add DO code to queue. 
				//Overwatch cannot implement a DO so not special concerns here. 
				//what if it implements a do multiple times?
				//maybe check action code against a bool overwatch specific array of set script codes. 

				//if p or r. p is direct, r needs to calculate mid. 
				if (execution_order[i]->getName().at(0) == 'P') {
					action_objects[current_symbols[j][1]]->setx(execution_order[i]->get_ulx());
					action_objects[current_symbols[j][1]]->sety(execution_order[i]->get_uly());

				}
				else {
					//R point, need the coordinate at the MIDDLE of the square.
					action_objects[current_symbols[j][1]]->setx((execution_order[i]->get_ulx() + execution_order[i]->get_lrx()) / 2);
					action_objects[current_symbols[j][1]]->sety((execution_order[i]->get_uly() + execution_order[i]->get_lry()) / 2);
				}
				unsorted_script_codes.push_back(action_objects[current_symbols[j][1]]);
				//code refers to which script code to include. 
				//Code 3 might be DO[abba] SLEEP[2000] DO[>] 
				//do object contains this string an a priority.
				//make a vector of TODO objects and push back onto deque the appropriate object, which has a full string and priority code. 
				//then sort todo by priority field. 
				break;
			case -6: //SET
				if (continue_on == false) {
					skipping_state_flag = true;
					break;
				}
				if (overwatch_state == true) {
					//set the OVERWATCH master condition array. e 
					//if[1.0] set condit 0,4(true) if[1.0] set condit[0,4](false) 
					//since its false, it doesn't modify the array anyway..
					//non-issue. 
					execution_order[i]->set_condition_value(current_symbols[j][1], current_symbols[j][2]);
					tk_mgr.set_overwatch_value(current_symbols[j][1], current_symbols[j][2]);
				}
				else {
					//index to set, value. 
					execution_order[i]->set_condition_value(current_symbols[j][1], current_symbols[j][2]);
				}
				break;
			}
		}
		std::cout << execution_order[i]->toStringConditions() << '\n';
	}
	if (overwatch_state == true) {
		std::cout << "exit";
	}
}








/*
Given a B.G.R pixel value extracted from a location at the provided handle, determine if the pixel belongs to an image in the image collection. 
If it does, return the image ID. If not, return -1. 
*/
int bgr_table_lookup(std::unordered_map<long int, int>& rbg_hmap,
	std::deque<int>& hits_cache,
	int* hits,
	const std::vector<std::vector<int>>& blue_table,
	const std::vector<std::vector<int>>& green_table,
	const std::vector<std::vector<int>>& red_table,
	int blue, int green, int red) {

	//if hashmap occupied, use hmap lookup scheme. 
	if (!rbg_hmap.empty()) {
		//table lookup: compute rolling hash of bgr and check table. return digit or -1 
		long int h1 = (blue * 257 + green) % 1000000007; //b-g	
		long int h2 = (h1 * 257 + red) % 1000000007; //bg-r	
		long int h3 = (h2 * 257 + blue) % 1000000007; //bgr-b	

		if (rbg_hmap.find(h3) != rbg_hmap.end()) {
			return rbg_hmap.find(h3)->second;
		}
		else {
			return -1;
		}

	}
	else {
		//allocate a number-of-images sized array. 
		//go through the 3 vectors with each piece of data, adding a hit to each index. 
		//on first index to hit 3, return that number. 

		//for each item in blue 255, +1 index of number. Store the number. if adding to an index would create 3, clear index and return number. 

		//keep a deque of indexes for the exit re-zeroization speedup. 

		//problem: full hit pushes back 0,0,0 on deque. Just test if zero
		// 2 3 4   4 5 7   0 1 4  deque. 

		//init table back to 0 on exit. 
		int current_hitleader = -1;
		for (int i = 0; i < blue_table[blue].size(); ++i) {
			++(hits[blue_table[blue][i]]);
			hits_cache.push_back(blue_table[blue][i]); //cache the hits for re-zeroization before pre exit.
			if ((hits[blue_table[blue][i]]) > current_hitleader) {
				current_hitleader = blue_table[blue][i];
			}
		}
		for (int i = 0; i < green_table[green].size(); ++i) {
			++(hits[green_table[green][i]]);
			hits_cache.push_back(green_table[green][i]);
			if ((hits[green_table[green][i]]) > current_hitleader) {
				current_hitleader = green_table[green][i];
			}
		}
		for (int i = 0; i < red_table[red].size(); ++i) {
			++(hits[red_table[red][i]]);
			hits_cache.push_back(red_table[red][i]);
			if ((hits[red_table[red][i]]) == 3) {
				//early abort on the first 3-point hit. Flush deque and reset array. 
				for (std::deque<int>::iterator it = hits_cache.begin(); it != hits_cache.end(); ++it) {
					if (hits[(*it)] != 0) {
						hits[(*it)] = 0;
					}
				}
				hits_cache.clear();
				return red_table[red][i];
			}
			if ((hits[red_table[red][i]]) > current_hitleader) {
				current_hitleader = red_table[red][i];
			}
		}

		//didn't get 3 hits. Returning no match
		for (std::deque<int>::iterator it = hits_cache.begin(); it != hits_cache.end(); ++it) {
			//flush the deque and clear the hits array
			if (hits[(*it)] != 0) {
				hits[(*it)] = 0;
			}
		}
		hits_cache.clear();
	}
	return -1; //default -1 matched nothing. 
}




/*
If all the conditions of an watchobjects trigger scripts are met, its action object is enqueue'd along with its priority.
Calling this function performs all the actions of objects whose conditions were met in order of their priority. 
*/
void pop_priority_queue(const Buffer_Adapter& screen_source, 
	std::deque<Do_Object*>& unsorted_script_codes,
	const std::vector<Do_Object*>& do_scripts,
	INPUT& ipt, 
	HWND& hwnd,
	const std::unordered_map<std::string, std::vector<int>>& ksinject,
	std::unordered_map<std::string, region_for_map*> object_lookup_by_name
) {

	struct compare_scripts {
		bool operator() (Do_Object* const & obj1, Do_Object* const & obj2) const
		{
			return obj1->getPriority() < obj2->getPriority();
		}
	};

	//convert deque to vector for prioirty queue
	std::vector<Do_Object*> scripts_enqueue;
	for (std::deque<Do_Object*>::const_iterator q = unsorted_script_codes.begin(); q != unsorted_script_codes.end(); ++q) {
		//make a vector of the correct script pbjects. 
		scripts_enqueue.push_back((*q));
	}
	unsorted_script_codes.clear(); //flush deque for this round. 

	std::priority_queue<Do_Object*, std::vector<Do_Object*>, compare_scripts> script_sorted_queue;
	//now use the unsorted to locate the do_scripts objects 
	for (int k = 0; k < scripts_enqueue.size(); ++k) {
		script_sorted_queue.push(scripts_enqueue[k]);
	}

	std::cout << "Script ordering: " << '\n';
	while (!script_sorted_queue.empty()) {
		//pop these to the intersystem cache
		std::cout << script_sorted_queue.top()->getDoStr() << '\n';
		script_to_keystroke(screen_source, ipt, hwnd, script_sorted_queue.top(), ksinject, object_lookup_by_name);
		script_sorted_queue.pop();
	}

	//flush all the current iteration codes. they have been executed. 
	unsorted_script_codes.clear(); 
}




/*
Loads the keystroke injection list
special character lsit
use capital letters for special characters
use #...# to specify a process hold in ms
Special characters like 

a E#50#F
b RP#30#RE%934 112%
c test

*/
int load_script_list(std::string& file_location, std::unordered_map<std::string, std::vector<int>>& ksinject) {
	//lcalled with the script file

	//try and open file, return -1 on fail & abourt
	std::ifstream file(file_location, std::ios::in);

	if (file.fail()) {
		return -1; 
	}
	else {
		try {
			std::string reader = ""; 
			while (!file.eof())
			{
				std::getline(file, reader);
				if (reader.length() >= 3) {
					int counter = 0;
					std::string key = "";
					while (reader[counter] != ' ' && counter < reader.length()) {
						key += reader[counter];
						++counter;
					}
					std::string ks_script = reader.substr(counter + 1, reader.length());
					std::vector<int> result = keystroke_vector_converter(ks_script);
					ksinject.emplace(key, result);
				}
			}
			file.close();
		}
		catch (const std::exception& e) {
			return -2;
		}
	}
	return 0; 
}


/*
Processes each individual string into a vector of keystroke arguments. 
*/
std::vector<int> keystroke_vector_converter(std::string& cmdstr) {
	/*
	Eventually compacted into a map that has 
	DO argument - array of keystroke values to execute. 
	a - 0x41,0x44,special negative value for delay/escape, raw value, negative again to resume, 0x55, 

	Need to add a special subimage snapshot function: d @ snapshot the subimage at coordinates 
	How to specify region coordinates if rectangle is not the caller?

	d @/R1/
	where d is output by the watchpoint and R1 is the call to the regions map to find the matching component. 
	Extract dimensions
	THEN a call to file slice is made. 

	Also need a HOLD CNTL and then key for tabbing between browser panels. 

	*/


	std::vector<int> args_array; 

	bool escaped = false; //entering special escaped sequences. escaped until find the matching brace. 
	bool delay_args = false; //the numeric values are literal used for a delay in MS. Implicit 30 MS delay on all.

	std::string escaped_substr = ""; //holds the temp numerical args for delays & clicks. 

	for (int i = 0; i < cmdstr.length(); ++i) {
		char assign = cmdstr[i];
		switch (assign) {
			case 'a':
			{
				int kcode = 0x41;
				args_array.push_back(kcode);
				break;
			}
			case 'b':
			{
				int kcode = 0x42;
				args_array.push_back(kcode);
				break;
			}
			case 'c':
			{
				int kcode = 0x43;
				args_array.push_back(kcode);
				break;
			}
			case 'd':
			{
				int kcode = 0x44;
				args_array.push_back(kcode);
				break;
			}
			case 'e':
			{
				int kcode = 0x45;
				args_array.push_back(kcode);
				break;
			}
			case 'f':
			{
				int kcode = 0x46;
				args_array.push_back(kcode);
				break;
			}
			case 'g':
			{
				int kcode = 0x47;
				args_array.push_back(kcode);
				break;
			}
			case 'h':
			{
				int kcode = 0x48;
				args_array.push_back(kcode);
				break;
			}
			case 'i':
			{
				int kcode = 0x49;
				args_array.push_back(kcode);
				break;
			}
			case 'j':
			{
				int kcode = 0x4A;
				args_array.push_back(kcode);
				break;
			}
			case 'k':
			{
				int kcode = 0x4B;
				args_array.push_back(kcode);
				break;
			}
			case 'l':
			{
				int kcode = 0x4C;
				args_array.push_back(kcode);
				break;
			}
			case 'm':
			{
				int kcode = 0x4D;
				args_array.push_back(kcode);
				break;
			}
			case 'n':
			{
				int kcode = 0x4E;
				args_array.push_back(kcode);
				break;
			}
			case 'o':
			{
				int kcode = 0x4F;
				args_array.push_back(kcode);
				break;
			}
			case 'p':
			{
				int kcode = 0x50;
				args_array.push_back(kcode);
				break;
			}
			case 'q':
			{
				int kcode = 0x51;
				args_array.push_back(kcode);
				break;
			}
			case 'r':
			{
				int kcode = 0x52;
				args_array.push_back(kcode);
				break;
			}
			case 's':
			{
				int kcode = 0x53;
				args_array.push_back(kcode);
				break;
			}
			case 't':
			{
				int kcode = 0x54;
				args_array.push_back(kcode);
				break;
			}
			case 'u':
			{
				int kcode = 0x55;
				args_array.push_back(kcode);
				break;
			}
			case 'v':
			{
				int kcode = 0x56;
				args_array.push_back(kcode);
				break;
			}
			case 'w':
			{
				int kcode = 0x57;
				args_array.push_back(kcode);
				break;
			}
			case 'x':
			{
				int kcode = 0x58;
				args_array.push_back(kcode);
				break;
			}
			case 'y':
			{
				int kcode = 0x59;
				args_array.push_back(kcode);
				break;
			}
			case 'z':
			{
				int kcode = 0x5A;
				args_array.push_back(kcode);
				break;
			}

			/*
			numbers can be literal keys OR parts of a escaped argument. 
			Escaped arguments are shunted to the special substring. 
			*/
			case '0': 
			{
				int kcode = 0x30;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '1': 
			{
				int kcode = 0x31;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '2': 
			{
				int kcode = 0x32;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '3':
			{
				int kcode = 0x33;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '4':
			{
				int kcode = 0x34;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '5':
			{
				int kcode = 0x35;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '6':
			{
				int kcode = 0x36;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '7':
			{
				int kcode = 0x37;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '8':
			{
				int kcode = 0x38;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}
			case '9':
			{
				int kcode = 0x39;
				if (escaped || delay_args) {
					escaped_substr.push_back(assign);
				}
				else {
					args_array.push_back(kcode);
				}
				break;
			}

				//special processing characters
				/*
				Delay, escape, arrows, mouse commands, numbers,
				*/
			case 'E':
				//enter
			{
				int kcode = 0x0D;
				args_array.push_back(kcode);
				break;
			}
			case 'L':
				//left mouse
			{
				int kcode = 0x01;
				args_array.push_back(kcode);
				break;
			}
			case 'R':
				//right mouse
			{
				int kcode = 0x02;
				args_array.push_back(kcode);
				break;
			}
			case 'S':
				//spacebar
			{
				int kcode = 0x20;
				args_array.push_back(kcode);
				break;
			}
			case '/':
				//escape number args
				if (!escaped) {
					args_array.push_back(MOUSE_ESCAPE_CODE); 
					escaped = true; 
				}
				else {
					args_array.push_back(std::stoi(escaped_substr));
					args_array.push_back(MOUSE_ESCAPE_CODE);
					escaped_substr.clear();
					escaped = false;
				}
				break;
			case ' ':
				//break in mouse arguments 
				args_array.push_back(std::stoi(escaped_substr));
				escaped_substr.clear(); 
				break;
			case '#':
				//delay of in milliseconds
				if (!delay_args) {
					args_array.push_back(DELAY_ESCAPE_CODE); 
					delay_args = true; 
				}
				else {
					args_array.push_back(std::stoi(escaped_substr)); 
					args_array.push_back(DELAY_ESCAPE_CODE); 
					escaped_substr.clear(); 
					delay_args = false; 
				}
				break;
			case 'T':
				//tab 
			{
				int kcode = 0x09;
				args_array.push_back(kcode);
				break;
			}
			case '<':
				//arrow left
			{
				int kcode = 0x25;
				args_array.push_back(kcode);
				break;
			}
			case '^':
				//arrow up
			{
				int kcode = 0x26;
				args_array.push_back(kcode);
				break;
			}
			case '>':
				//arrow right
			{
				int kcode = 0x27;
				args_array.push_back(kcode);
				break;
			}
			case '_':
				//arrow down
			{
				int kcode = 0x27;
				args_array.push_back(kcode);
				break;
			}
			case '*':
				//Need to inject a mouse click at the trigger location.
				//need to reserve two vector characters that will be resolved at runtime. 
				args_array.push_back(MOUSE_RUNTIME_INJECT_CODE);
				break;
			case '@':
				//IMAGE_SLICE_CAPTURE_CODE = -6;
				// d @R1@ 
				//used to call for IMAGE SLICE CAPTURE SNAPSHOT on R1 whose dimensions are extracted from snapshot.
				//note that this way any point cal call for image capture, nt just rectangles. 

				/*
				Problem is args is an integer array. Is the hmap locked at this point?
				If not, push back the integer value of the string that looks up the hmap value. 
				*/
				args_array.push_back(IMAGE_SLICE_CAPTURE_CODE);

				try {
					++i; 
					while (cmdstr[i] != '@' && i < cmdstr.length()) {
						args_array.push_back((int)cmdstr[i]);
						++i; 
					}
				}
				catch (...) {
					//Unmatched escape. Auto insert. 
				}

				args_array.push_back(IMAGE_SLICE_CAPTURE_CODE);
				break;
			case 'C': {
				//control key. 
				int kcode = 0x11;
				args_array.push_back(kcode);
				break;
			}
			case '+':
				//HOLD the next key until release is encountered. 
				args_array.push_back(KEY_HOLD_PROXIMAL);
				//engage held lock. 
				break; 
			case '%':
				// release the most recently held key. 
				args_array.push_back(KEY_HOLD_RELEASE_HOLD);
				break; 
		}

		/*
				if (!escaped && !delay_args) {
			ipt.ki.wVk = kcode; // virtual-key code for the key
			ipt.ki.dwFlags = 0; // 0 for key press
			SendInput(1, &ipt, sizeof(INPUT)); //send event

			//small delay to allow for chaining. 
			Sleep(30); 

			ipt.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
			SendInput(1, &ipt, sizeof(INPUT)); //send event. 
		}
		
		*/
	}

	return args_array;
}



/*
Pipeline locally to tessarect 
*/
void extract_ocr() {

}


/*
Offload images to an OCR cluster
*/
void export_to_ftp() {

}


/*
Takes the map of do-keystroke sequences and injects the sequences. 
*/
void script_to_keystroke(const Buffer_Adapter& screen_source, 
	INPUT& ipt, 
	HWND& hwnd, 
	const Do_Object* obj, 
	const std::unordered_map<std::string, std::vector<int>>& ksinject,
	const std::unordered_map<std::string, region_for_map*>& string_to_region) {
	//based on the pipeline argument, possibly injecting coordinates, perform the keystroke sequence.

	bool special_injector_state = false; 
	int special_injector_value = 0; 

	int inject_click_values[2]; 
	inject_click_values[0] = -1; 
	inject_click_values[1] = -1; 
	int inject_delay_value = 0; 

	int delayed_key_code = 0; 

	int a; 
		int b; 

	std::string extract_dimensions_key = ""; 
	region_for_map* extract_dimensions_for_cap = nullptr; 

	for (int i = 0; i < ksinject.at(obj->get_command()).size(); ++i) {
		//execute keystrokes. 
		if (ksinject.at(obj->get_command())[i] >= 0) {
			if (special_injector_state) {
				//handle special nonliteral sequences. redirect to temp holders. 
				switch (special_injector_value) {
					case DELAY_ESCAPE_CODE:
						Sleep(ksinject.at(obj->get_command())[i]); 
						break; 
					case MOUSE_ESCAPE_CODE:
						//the next two numbers are to be assigned to mouse holder
						if (inject_click_values[0] == -1) {
							inject_click_values[0] = ksinject.at(obj->get_command())[i]; 
						}
						else {
							inject_click_values[1] = ksinject.at(obj->get_command())[i];
						}
						break; 
					case MOUSE_RUNTIME_INJECT_CODE:
						//should never occure. handled in one step when encountered. 
						break; 
					case KEY_HOLD_PROXIMAL:
						//Set the key to be held and send this keystroke. 
						delayed_key_code = ksinject.at(obj->get_command())[i]; 
						ipt.ki.wVk = ksinject.at(obj->get_command())[i]; // virtual-key code for the key
						ipt.ki.dwFlags = 0; // 0 for key press
						SendInput(1, &ipt, sizeof(INPUT)); //send event
						special_injector_state = false; 
						break;
					case IMAGE_SLICE_CAPTURE_CODE:
						//need to build the lookup string. 
						extract_dimensions_key += (char)ksinject.at(obj->get_command())[i];
						break; 
				}
			}
			else {
				//bring the window to the front so keystrokes go to the active window of analysis
				SetForegroundWindow(hwnd);

				//if mouse, set position. 
				if (inject_click_values[0] != -1 && inject_click_values[1] != -1) {
					SetCursorPos(inject_click_values[0], inject_click_values[1]);
					inject_click_values[0] = -1; 
					inject_click_values[1] = -1; 
				}


				ipt.ki.wVk = ksinject.at(obj->get_command())[i]; // virtual-key code for the key
				ipt.ki.dwFlags = 0; // 0 for key press
				SendInput(1, &ipt, sizeof(INPUT)); //send event

				ipt.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
				SendInput(1, &ipt, sizeof(INPUT)); //send event.

				Sleep(30);  //small delay to allow for chaining.
			}
		}
		else {
			if (special_injector_state) {
				switch (special_injector_value) {
				case DELAY_ESCAPE_CODE:
					//do nothing. 
					break; 
				case MOUSE_ESCAPE_CODE:

					break; 
				case MOUSE_RUNTIME_INJECT_CODE:

					break;
				case IMAGE_SLICE_CAPTURE_CODE:
					//execute the image slice. 
					if (string_to_region.find(extract_dimensions_key) != string_to_region.end()) {
						extract_dimensions_for_cap = string_to_region.at(extract_dimensions_key);
						//call file from slice atf.  it will handle inversions and shifting.  
						//file_from_slice_atf(screen_source, int cartesian_startx, int cartesian_starty, int xlength, int ylength)
						file_from_slice_atf(screen_source, extract_dimensions_for_cap->x,
							extract_dimensions_for_cap->y,
							extract_dimensions_for_cap->length,
							extract_dimensions_for_cap->height);
					}
					break; 
				}


				special_injector_state = false; 
				special_injector_value = 0; 
				inject_click_values[0] = -1;
				inject_click_values[1] = -1;
				inject_delay_value = 0;
				extract_dimensions_for_cap = nullptr;
				extract_dimensions_key.clear(); 
			}
			else {
				switch (ksinject.at(obj->get_command())[i]) {
					case DELAY_ESCAPE_CODE:
						special_injector_value = DELAY_ESCAPE_CODE;
						special_injector_state = true;
						break;
					case MOUSE_ESCAPE_CODE:
						special_injector_value = MOUSE_ESCAPE_CODE;
						special_injector_state = true;
						break;
					case MOUSE_RUNTIME_INJECT_CODE:
						//should not set special injector. 
						inject_click_values[0] = screen_source.rcScreen.left + screen_source.stretch_x_only(obj->getx());
						inject_click_values[1] = screen_source.rcScreen.bottom - screen_source.rcClient.bottom + screen_source.stretch_y_only(obj->gety()); //should be around 400...
						break;
					case KEY_HOLD_PROXIMAL:
						//set lock & set key held code. The next key becomes the held key
						special_injector_value = KEY_HOLD_PROXIMAL; 
						special_injector_state = true;
						break; 
					case KEY_HOLD_RELEASE_HOLD:
						//release the key down key. injector lock was already released when delayed_key_code was set. 
						SetForegroundWindow(hwnd);
						ipt.ki.wVk = delayed_key_code; // virtual-key code for the key
						ipt.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
						SendInput(1, &ipt, sizeof(INPUT)); //send event.
						break; 
					case IMAGE_SLICE_CAPTURE_CODE:
						//enable escaped lock. 
						special_injector_state = true;
						special_injector_value = IMAGE_SLICE_CAPTURE_CODE;
						break; 
				}
			}
		}
	}
}