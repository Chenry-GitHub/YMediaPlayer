#pragma once

class MainWindow;
class MainWindowControler
{
public:
	MainWindowControler();
	~MainWindowControler();

	HWND GetWindowWnd();

	void CreateMainWindow();

	void CloseMainWindow();

	void ShowMainWindow(bool show =true);

	void SetCloseFunction(std::function <bool()> close_func);

	void SetFinalFunction(std::function <void ()> destory_func);

	void SetUpdateText(const std::wstring &update_text);

	void SetDownloadPercent(int percent);

	void SetDownloadCurSize(int size);

	void SetDownloadTotalSize(int size);

	int GetCurrentSize()
	{
		return cur_size_;
	}

	int GetTotalSize()
	{
		return total_size_;
	}


	MainWindow * main_win_;

	int cur_size_;
	int total_size_;
};

