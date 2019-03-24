#pragma once

class MainWindow;

class MainWindowCtl
{
public:
	MainWindowCtl();
	~MainWindowCtl();

	void SetCloseFunction(std::function<bool ()>);

	void SetFinalFunction(std::function <void()> destory_func);

	void SetClickedSliderFunction(std::function <void(float)> func);

	void CreateMainWindow();

	void Close();

	std::string GetPlayUrl();

	//invoke by player
	void SetVideoImage(void* data, int w, int h);
	void SetDuration(int dur);
	void SetCurPos(int cur);
	void SetBufferPercent(float percent);
	//

	std::function<void()> btn_clicked_paly_media_func_=nullptr;

	MainWindow *main_win_;
};

