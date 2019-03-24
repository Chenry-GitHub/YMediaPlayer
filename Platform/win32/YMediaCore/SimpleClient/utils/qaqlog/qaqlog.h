//spdlog日志封装
//spdlog参见:https://github.com/gabime/spdlog
//封装主要对分级进行控制,包括全局等级,err(错误),critical(严重)
//全局等级,按天周期生成日志文件
//err和critical按文件大小滚动生成日志文件
//也可附加控制台日志
//使用示例:
//lg::Initialize("MyApp");
//lg::AddConsoleLogger(lg::trace);
//lg::Info("This is a line log."); //[2018-10-01 14:43:08.962] [MyApp_console] [info] This is a line log.
//lg::Shutdown();
#if defined(_MSC_VER)
#pragma once
#pragma warning(push)
#pragma warning(disable:4996)
#endif

#ifndef _INCLUDED_QAQLOG_
#define _INCLUDED_QAQLOG_


//平台宏处理
#ifndef _QAQLOG_PLATFORM_DEFINED_
#if defined(_WIN32) || defined(__WIN32__) || defined(__WINDOWS__)
#ifndef _QAQLOG_WIN32_
#define _QAQLOG_WIN32_
#endif

//#ifndef SPDLOG_WCHAR_TO_UTF8_SUPPORT
//#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
//#endif

#else
#ifndef _QAQLOG_POSIX_
#define _QAQLOG_POSIX_
#endif
#endif
#define _QAQLOG_PLATFORM_DEFINED_
#endif


//公用头文件
#include <sstream>
#ifdef ENABLE_LOG
#include "spdlog/spdlog.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/sinks/rotating_file_sink.h"
#include "spdlog/sinks/daily_file_sink.h"
#endif


#if defined(_QAQLOG_WIN32_)
#include <iomanip> //std::put_time
#include <shlwapi.h> //PathIsDirectory
#include <direct.h> //_mkdir
#include <corecrt_io.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h> //支持c
#include <time.h>
#include <sys/stat.h>
#endif



namespace lg
{

    //日志等级
    enum QAQLOG_LEVEL
    {
        trace = 0,
        debug = 1,
        info = 2,
        warn = 3,
        err = 4,
        critical = 5,
        off = 6
    };

#ifdef ENABLE_LOG
    class QAQLogData
    {
    public:
        QAQLogData(): initialized_(false) {}
        bool initialized_;
        //控制台日志id
        std::string log_console_id_ = "console";
        //按天周期日志id
        std::string log_daily_file_id_ = "daily";
        //错误日志id
        std::string log_error_id_ = "error";
        //严重错误日志id
        std::string log_critical_id_ = "critical";
        //应用名称
        std::string app_name_ = "app";
        //应用文件夹
        std::string app_dir_;
        //写文件日志周期
        int flush_interval_ = 0;
    private:

    };

    //初始化标识
    extern QAQLogData log_data_;
#endif
    namespace
    {
        //获取应用名称和日志目录
#if defined(_QAQLOG_WIN32_)

        std::string ws2s(const std::wstring& ws)
        {
            std::string curLocale = setlocale(LC_ALL, NULL);        // curLocale = "C";
            setlocale(LC_ALL, "chs");
            const wchar_t* _Source = ws.c_str();
            size_t _Dsize = 2 * ws.size() + 1;
            char *_Dest = new char[_Dsize];
            memset(_Dest, 0, _Dsize);
            size_t i;
            wcstombs_s(&i, _Dest, _Dsize, _Source, _TRUNCATE);
            std::string result = _Dest;
            delete[]_Dest;
            setlocale(LC_ALL, curLocale.c_str());
            return result;
        }

        std::wstring s2ws(const std::string& s)
        {
            std::wstring wszStr;
            int nLength = MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, NULL, NULL);
            wszStr.resize(nLength);
            LPWSTR lpwszStr = new wchar_t[nLength];
            MultiByteToWideChar(CP_ACP, 0, s.c_str(), -1, lpwszStr, nLength);
            wszStr = lpwszStr;
            delete[] lpwszStr;
            return wszStr;
        }

        std::wstring GetInstancePathW()
        {
            std::wstring sInstancePath;
            WCHAR tszModule[4096] = { 0 };
			WCHAR  tszLongModule[4096] = { 0 };
            ::GetModuleFileNameW(NULL, tszModule, 4096);
            ::GetLongPathNameW(tszModule, tszLongModule, 4096);
            sInstancePath = tszLongModule;
            int pos = sInstancePath.rfind(TEXT('\\'));
            if (pos >= 0)
            {
                sInstancePath = sInstancePath.erase(pos, sInstancePath.length() - pos);
            }
            return sInstancePath;
        }
        std::string GetInstancePathA()
        {
            return ws2s(GetInstancePathW());
        }

        std::string GetFormatDateTime(const std::string& format)
        {
            time_t tit = time(0);
            std::tm tm;
            localtime_s(&tm, &tit);
            std::ostringstream oss;
            oss << std::put_time(&tm, format.c_str());
            std::string str = oss.str();
            return str;
        }

        bool DirExsit(const std::string& dir)
        {
            return FILE_ATTRIBUTE_DIRECTORY == PathIsDirectoryA(dir.c_str());
        }

        int CreatDir(const std::string& dir)
        {
            char *pszDir, pDir[MAX_PATH];
            int i = 0, iRet, iLen;
            strcpy_s(pDir, dir.c_str());
            if (NULL == pDir)
            {
                return 0;
            }
            pszDir = _strdup(pDir);
            iLen = strlen(pszDir);
            // 创建中间目录
            for (i = 0; i < iLen; i++)
            {
                if (pszDir[i] == '\\' || pszDir[i] == '/')
                {
                    pszDir[i] = '\0';
                    //如果不存在,创建
                    iRet = _access(pszDir, 0);
                    if (iRet != 0)
                    {
                        iRet = _mkdir(pszDir);
                        if (iRet != 0)
                        {
                            return -1;
                        }
                    }
                    //支持linux,将所有\换成/
                    pszDir[i] = '/';
                }
            }
            iRet = _mkdir(pszDir);
            free(pszDir);
            return iRet;
        }

        std::string GetLogDir(const std::string& dir)
        {
            std::string log_dir = dir;
            if (log_dir.empty())
            {
                //日志文件夹:log\yyyymm
                log_dir = GetInstancePathA() +
                          "/log/" + GetFormatDateTime("%Y%m");
            }

            if (!DirExsit(log_dir))
            {
                CreatDir(log_dir);
            }
            return log_dir;
        }
        std::string GetAppName()
        {
            char szapipath[MAX_PATH];//（D:\Documents\Downloads\TEST.exe）
            memset(szapipath, 0, MAX_PATH);
            GetModuleFileNameA(NULL, szapipath, MAX_PATH);
            //获取应用程序名称
            char szExe[MAX_PATH] = "";//（TEST.exe）
            char *pbuf = NULL;
            char* szLine = strtok_s(szapipath, "\\", &pbuf);
            while (NULL != szLine)
            {
                strcpy_s(szExe, szLine);
                szLine = strtok_s(NULL, "\\", &pbuf);
            }
            //删除.exe
            strncpy_s(szapipath, szExe, strlen(szExe) - 4);
            return std::string(szapipath);
        }
#else

        std::string GetAppPath()
        {
            char path[BUFSIZ] = { 0 };
            int rval = readlink("/proc/self/exe", path, sizeof(path) - 1);
            if (rval == -1)
            {
                std::cout << "readlink error" << std::endl;
                return std::string();
            }
            return std::string(path);
        }
        std::string GetInstancePath()
        {
            std::string ipath = GetAppPath();
            if (ipath.empty())
            {
                return ipath;
            }
            std::size_t pos = ipath.rfind('/');
            if (pos != std::string::npos)
            {
                ipath = ipath.substr(0, pos);
            }
            return ipath;
        }

        std::string GetFormatDateTime(const std::string& format)
        {
            time_t tit = time(0);
            char str_time[BUFSIZ] = { 0 };
            strftime(str_time, BUFSIZ, format.c_str(), localtime(&tit));
            return std::string(str_time);
        }

        bool DirExsit(const std::string& dir)
        {
            return 0 == access(dir.c_str(), R_OK | W_OK);
        }

        int CreatDir(const std::string& pDir)
        {
            int i = 0;
            int iRet;
            int iLen;
            char* pszDir;
            if (pDir.empty())
            {
                return 0;
            }
            pszDir = strdup(pDir.c_str());
            iLen = strlen(pszDir);
            // 创建中间目录
            for (i = 0; i < iLen; i++)
            {
                if (pszDir[i] == '\\' || pszDir[i] == '/')
                {
                    if (i == 0)
                    {
                        continue;
                    }
                    pszDir[i] = '\0';
                    //如果不存在,创建
                    iRet = access(pszDir, 0);
                    if (iRet != 0)
                    {
                        iRet = mkdir(pszDir, 777);
                        if (iRet != 0)
                        {
                            return -1;
                        }
                    }
                    //支持linux,将所有\换成/
                    pszDir[i] = '/';
                }
            }
            iRet = mkdir(pszDir, 777);
            free(pszDir);
            return iRet;
        }


        std::string GetAppName()
        {
            std::string ipath = GetAppPath();
            if (ipath.empty())
            {
                return ipath;
            }
            return std::string(strrchr(ipath.c_str(), '/') + 1);
        }

        std::string GetLogDir(const std::string& dir)
        {
            std::string log_dir = dir;
            if (log_dir.empty())
            {
                //日志文件夹:log\yyyymm
                log_dir = GetInstancePath() +
                          "/log/" + GetFormatDateTime("%Y%m");
            }

            if (!DirExsit(log_dir))
            {
                if (0 != CreatDir(log_dir))
                {
                    std::cout << "CreateDir Error:" << log_dir.c_str() << std::endl;
                }
            }
            return log_dir;
        }
#endif
    }

    //设置等级,仅设置按天的周期文件日志等级
    inline void SetLevel(QAQLOG_LEVEL level)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        if (auto logger = spdlog::get(log_data_.log_daily_file_id_))
        {
            logger->set_level((spdlog::level::level_enum)level);
        }
#endif
    }

    //附加控制台日志,可参见spdlog设定特殊功能
    inline void AddConsoleLogger(QAQLOG_LEVEL level)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        if (auto logger = spdlog::get(log_data_.log_console_id_))
        {
            return;
        }
#if defined(_QAQLOG_WIN32_)
        AllocConsole();
        freopen("CONOUT$", "w+t", stdout);
#endif
        auto logger_console = spdlog::stdout_color_mt(log_data_.log_console_id_);
        logger_console->set_level((spdlog::level::level_enum)level);
#endif
    }

    //附加错误文件日志,可参见spdlog设定特殊功能
    inline void AddErrLogger()
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        if (auto logger = spdlog::get(log_data_.log_error_id_))
        {
            return;
        }
        auto logger_error = spdlog::rotating_logger_mt(log_data_.log_error_id_, log_data_.app_dir_ + "/" + log_data_.app_name_ + "_error.log", 1048576 * 5, 10);
        logger_error->set_level(spdlog::level::err);
#endif
    }

    //附加严重错误文件日志,可参见spdlog设定特殊功能
    inline void AddCriticalLogger()
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        if (auto logger = spdlog::get(log_data_.log_critical_id_))
        {
            return;
        }
        auto logger_critical = spdlog::rotating_logger_mt(log_data_.log_critical_id_, log_data_.app_dir_ + "/" + log_data_.app_name_ + "_critical.log", 1048576 * 5, 10);
        logger_critical->set_level(spdlog::level::critical);
#endif
    }

    //附加按天的周期文件日志,可参见spdlog设定特殊功能
    inline void AddDailyLogger(QAQLOG_LEVEL level)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        if (auto logger = spdlog::get(log_data_.log_daily_file_id_))
        {
            return;
        }
        auto logger_file = spdlog::daily_logger_mt(log_data_.log_daily_file_id_, log_data_.app_dir_ + "/" + log_data_.app_name_ + ".log", 23, 59);
        logger_file->set_level((spdlog::level::level_enum)level);
#endif
    }


    //初始化
    //app_name,应用名称,不填写将根据不同操作系统自动获取应用名称
    //log_dir,日志保存目录,不填写将生成在应用"/log/yyyyMM/"目录
    //level,日志等级,不填写默认使用info等级,可在之后使用SetLevel来改变
    //b_daily_file,每天周期生成文件日志开关,如果初始化为false,可在之后使用AddDailyLogger来附加
    //b_err_file,错误文件日志开关,如果初始化为false,可在之后使用AddErrLogger来附加
    //b_critical_file,严重错误文件日志开关,如果初始化为false,可在之后使用AddCriticalLogger来附加
    //b_console,控制台日志开关,如果初始化为false,可在之后使用AddConsoleLogger来附加
    //flush_interval,写文件日志周期
    inline void Initialize(const std::string& app_name = ""
                           , const std::string& log_dir = ""
                           //默认日志等级
                           , QAQLOG_LEVEL level = info
                           //默认日志开关
                           , bool b_daily_file = true
                           //错误日志开关 level:err
                           , bool b_err_file = true
                           //严重错误日志开关 level:critical
                           , bool b_critical_file = false
                           //控制台日志开关 level和默认日志一致
                           , bool b_console = false
                           //写日志文件周期,默认3秒,设置为0即刻写日志
                           , int flush_interval = 3)
    {
#ifdef ENABLE_LOG
        if (log_data_.initialized_)
        {
            return;
        }
        if (!b_daily_file && !b_console)
        {
            return;
        }
        log_data_.app_name_ = app_name;
        if (log_data_.app_name_.empty())
        {
            log_data_.app_name_ = GetAppName();
        }
        log_data_.app_dir_ = GetLogDir(log_dir);

        log_data_.log_daily_file_id_ = log_data_.app_name_;
        log_data_.log_console_id_ = log_data_.app_name_ + "_console";
        log_data_.log_error_id_ = log_data_.app_name_ + "_error";
        log_data_.log_critical_id_ = log_data_.app_name_ + "_critical";

        log_data_.initialized_ = true;
        //日志缓存周期设置
        //warning: only use if all your loggers are thread safe!
        log_data_.flush_interval_ = flush_interval;
        if (log_data_.flush_interval_ > 0)
        {
            spdlog::flush_every(std::chrono::seconds(log_data_.flush_interval_));
        }

        //文件日志
        if (b_daily_file)
        {
            AddDailyLogger(level);
        }

        //错误日志
        if (b_err_file)
        {
            AddErrLogger();
        }

        //严重错误日志
        if (b_critical_file)
        {
            AddCriticalLogger();
        }

        //控制台
        if (b_console)
        {
            AddConsoleLogger(level);
        }
#endif
    }

    //VS支持参数传递编译,以支持spdlog的格式日志
    //例如:lg::Info("Value1={},Value2={}", 10,"你好");//输出日志:Value1=10,Value2="你好"
    //详细参见:https://github.com/gabime/spdlog
#if defined(_MSC_VER)
    template<typename... Args>
    inline void Write(QAQLOG_LEVEL level, const char* fmt, const Args& ...args)
    {
#ifdef ENABLE_LOG
		if (!log_data_.initialized_)
		{
			return;
		}
		spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l)
		{
			spdlog::level::level_enum lv = (spdlog::level::level_enum)level;
			if (l->should_log(lv))
			{
				l->log(lv, fmt, args...);
				if (0 <= log_data_.flush_interval_)
				{
					l->flush();
				}
			}
		});
#endif

    }

    template<typename... Args>
    inline void Write(QAQLOG_LEVEL level, const char *msg)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l)
        {
            spdlog::level::level_enum lv = (spdlog::level::level_enum)level;
            if (l->should_log(lv))
            {
                l->log(lv, msg);
                if (0 <= log_data_.flush_interval_)
                {
                    l->flush();
                }
            }
        });
#endif
    }

    template<typename... Args>
    inline void Trace(const std::string& fmt, const Args& ...args)
    {
        Write(QAQLOG_LEVEL::trace, fmt.c_str(), args...);
    }

    template<typename... Args>
    inline void Debug(const std::string& fmt, const Args& ...args)
    {
        Write(QAQLOG_LEVEL::debug, fmt.c_str(), args...);
    }

    template<typename... Args>
    inline void Info(const std::string& fmt, const Args& ...args)
    {
        Write(QAQLOG_LEVEL::info, fmt.c_str(), args...);
    }

    template<typename... Args>
    inline void Warn(const std::string& fmt, const Args& ...args)
    {
        Write(QAQLOG_LEVEL::warn, fmt.c_str(), args...);
    }

    template<typename... Args>
    inline void Err(const std::string& fmt, const Args& ...args)
    {
        Write(QAQLOG_LEVEL::err, fmt.c_str(), args...);
    }

    template<typename... Args>
    inline void Critical(const std::string& fmt, const Args& ...args)
    {
        Write(QAQLOG_LEVEL::critical, fmt.c_str(), args...);
    }
#endif

    template<typename T>
    inline void Write(QAQLOG_LEVEL level, const T& msg)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l)
        {
            spdlog::level::level_enum lv = (spdlog::level::level_enum)level;
            if (l->should_log(lv))
            {
                l->log(lv, msg);
                if (0 <= log_data_.flush_interval_)
                {
                    l->flush();
                }
            }
        });
#endif
    }

    template<typename T>
    inline void Trace(const T& msg)
    {
        Write(QAQLOG_LEVEL::trace, msg);
    }

    template<typename T>
    inline void Debug(const T& msg)
    {
        Write(QAQLOG_LEVEL::debug, msg);
    }

    template<typename T>
    inline void Info(const T& msg)
    {
        Write(QAQLOG_LEVEL::info, msg);
    }

    template<typename T>
    inline void Warn(const T& msg)
    {
        Write(QAQLOG_LEVEL::warn, msg);
    }

    template<typename T>
    inline void Err(const T& msg)
    {
        Write(QAQLOG_LEVEL::err, msg);
    }

    template<typename T>
    inline void Critical(const T& msg)
    {
        Write(QAQLOG_LEVEL::critical, msg);
    }


    //将...构造成格式化的std::string
#ifndef LOG_GET_FORMAT_MSG
#define LOG_GET_FORMAT_MSG \
	va_list vlist; \
	char buffer[2048] = { 0 };\
	va_start(vlist, fmt); \
	vsprintf(buffer, fmt, vlist);\
	va_end(vlist);\
	std::string msg = std::string(buffer);\
	if (msg.empty())return;
#endif

    //安装C传统变参打印方式处理日志
    //参见printf使用方式
    //如lg::InfoF("This is an %s.","apple");//输出日志:This is an apple.
    //因gcc无法编译通过变参传递,暂不能使用上面封装的格式日志接口,若linux要使用spdlog的格式日志,参见如下
    //spdlog::apply_all([&](std::shared_ptr<spdlog::logger> l) {
    // auto lv = apdlog::level::info;
    //if (l->should_log(lv)) {
    //	l->log(lv, "This is an {}.","apple");
    //	if (0 <= flush_interval_)l->flush();
    //}
    //	});
    inline void WriteF(QAQLOG_LEVEL level, const char* fmt, ...)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        LOG_GET_FORMAT_MSG;
        Write(level, msg);
#endif
    }

    inline void TraceF(const char* fmt, ...)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        LOG_GET_FORMAT_MSG;
        Write(QAQLOG_LEVEL::trace, msg);
#endif
    }

    inline void DebugF(const char* fmt, ...)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        LOG_GET_FORMAT_MSG;
        Write(QAQLOG_LEVEL::debug, msg);
#endif
    }

    inline void InfoF(const char* fmt, ...)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        LOG_GET_FORMAT_MSG;
        Write(QAQLOG_LEVEL::info, msg);
#endif
    }

    inline void WarnF(const char* fmt, ...)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        LOG_GET_FORMAT_MSG;
        Write(QAQLOG_LEVEL::warn, msg);
#endif
    }

    inline void ErrF(const char* fmt, ...)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        LOG_GET_FORMAT_MSG;
        Write(QAQLOG_LEVEL::err, msg);
#endif
    }

    inline void CriticalF(const char* fmt, ...)
    {
#ifdef ENABLE_LOG
        if (!log_data_.initialized_)
        {
            return;
        }
        LOG_GET_FORMAT_MSG;
        Write(QAQLOG_LEVEL::critical, msg);
#endif
    }

    //注销日志,会将所有日志flush(日志默认有3秒写文件延迟),建议在程序关闭时调用
    inline void Shutdown()
    {
#ifdef ENABLE_LOG
        spdlog::shutdown();
        log_data_.initialized_ = false;
#endif
    }

#ifdef ENABLE_LOG
#ifndef _QAQLOG_WIN32_
#define __declspec_selectany __attribute__((weak))
#define __declspec_whatever __attribute__((whatever))
#define __declspec(x) __declspec_##x
#endif
    //免去extern变量需要在cpp中实现
    __declspec(selectany) QAQLogData log_data_;
#endif
}

#endif // _INCLUDED_QAQLOG_
#if defined(_MSC_VER)
#pragma warning(pop)
#endif