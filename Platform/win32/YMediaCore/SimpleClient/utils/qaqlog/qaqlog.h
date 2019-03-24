//spdlog��־��װ
//spdlog�μ�:https://github.com/gabime/spdlog
//��װ��Ҫ�Էּ����п���,����ȫ�ֵȼ�,err(����),critical(����)
//ȫ�ֵȼ�,��������������־�ļ�
//err��critical���ļ���С����������־�ļ�
//Ҳ�ɸ��ӿ���̨��־
//ʹ��ʾ��:
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


//ƽ̨�괦��
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


//����ͷ�ļ�
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
#include <string.h> //֧��c
#include <time.h>
#include <sys/stat.h>
#endif



namespace lg
{

    //��־�ȼ�
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
        //����̨��־id
        std::string log_console_id_ = "console";
        //����������־id
        std::string log_daily_file_id_ = "daily";
        //������־id
        std::string log_error_id_ = "error";
        //���ش�����־id
        std::string log_critical_id_ = "critical";
        //Ӧ������
        std::string app_name_ = "app";
        //Ӧ���ļ���
        std::string app_dir_;
        //д�ļ���־����
        int flush_interval_ = 0;
    private:

    };

    //��ʼ����ʶ
    extern QAQLogData log_data_;
#endif
    namespace
    {
        //��ȡӦ�����ƺ���־Ŀ¼
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
            // �����м�Ŀ¼
            for (i = 0; i < iLen; i++)
            {
                if (pszDir[i] == '\\' || pszDir[i] == '/')
                {
                    pszDir[i] = '\0';
                    //���������,����
                    iRet = _access(pszDir, 0);
                    if (iRet != 0)
                    {
                        iRet = _mkdir(pszDir);
                        if (iRet != 0)
                        {
                            return -1;
                        }
                    }
                    //֧��linux,������\����/
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
                //��־�ļ���:log\yyyymm
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
            char szapipath[MAX_PATH];//��D:\Documents\Downloads\TEST.exe��
            memset(szapipath, 0, MAX_PATH);
            GetModuleFileNameA(NULL, szapipath, MAX_PATH);
            //��ȡӦ�ó�������
            char szExe[MAX_PATH] = "";//��TEST.exe��
            char *pbuf = NULL;
            char* szLine = strtok_s(szapipath, "\\", &pbuf);
            while (NULL != szLine)
            {
                strcpy_s(szExe, szLine);
                szLine = strtok_s(NULL, "\\", &pbuf);
            }
            //ɾ��.exe
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
            // �����м�Ŀ¼
            for (i = 0; i < iLen; i++)
            {
                if (pszDir[i] == '\\' || pszDir[i] == '/')
                {
                    if (i == 0)
                    {
                        continue;
                    }
                    pszDir[i] = '\0';
                    //���������,����
                    iRet = access(pszDir, 0);
                    if (iRet != 0)
                    {
                        iRet = mkdir(pszDir, 777);
                        if (iRet != 0)
                        {
                            return -1;
                        }
                    }
                    //֧��linux,������\����/
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
                //��־�ļ���:log\yyyymm
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

    //���õȼ�,�����ð���������ļ���־�ȼ�
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

    //���ӿ���̨��־,�ɲμ�spdlog�趨���⹦��
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

    //���Ӵ����ļ���־,�ɲμ�spdlog�趨���⹦��
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

    //�������ش����ļ���־,�ɲμ�spdlog�趨���⹦��
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

    //���Ӱ���������ļ���־,�ɲμ�spdlog�趨���⹦��
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


    //��ʼ��
    //app_name,Ӧ������,����д�����ݲ�ͬ����ϵͳ�Զ���ȡӦ������
    //log_dir,��־����Ŀ¼,����д��������Ӧ��"/log/yyyyMM/"Ŀ¼
    //level,��־�ȼ�,����дĬ��ʹ��info�ȼ�,����֮��ʹ��SetLevel���ı�
    //b_daily_file,ÿ�����������ļ���־����,�����ʼ��Ϊfalse,����֮��ʹ��AddDailyLogger������
    //b_err_file,�����ļ���־����,�����ʼ��Ϊfalse,����֮��ʹ��AddErrLogger������
    //b_critical_file,���ش����ļ���־����,�����ʼ��Ϊfalse,����֮��ʹ��AddCriticalLogger������
    //b_console,����̨��־����,�����ʼ��Ϊfalse,����֮��ʹ��AddConsoleLogger������
    //flush_interval,д�ļ���־����
    inline void Initialize(const std::string& app_name = ""
                           , const std::string& log_dir = ""
                           //Ĭ����־�ȼ�
                           , QAQLOG_LEVEL level = info
                           //Ĭ����־����
                           , bool b_daily_file = true
                           //������־���� level:err
                           , bool b_err_file = true
                           //���ش�����־���� level:critical
                           , bool b_critical_file = false
                           //����̨��־���� level��Ĭ����־һ��
                           , bool b_console = false
                           //д��־�ļ�����,Ĭ��3��,����Ϊ0����д��־
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
        //��־������������
        //warning: only use if all your loggers are thread safe!
        log_data_.flush_interval_ = flush_interval;
        if (log_data_.flush_interval_ > 0)
        {
            spdlog::flush_every(std::chrono::seconds(log_data_.flush_interval_));
        }

        //�ļ���־
        if (b_daily_file)
        {
            AddDailyLogger(level);
        }

        //������־
        if (b_err_file)
        {
            AddErrLogger();
        }

        //���ش�����־
        if (b_critical_file)
        {
            AddCriticalLogger();
        }

        //����̨
        if (b_console)
        {
            AddConsoleLogger(level);
        }
#endif
    }

    //VS֧�ֲ������ݱ���,��֧��spdlog�ĸ�ʽ��־
    //����:lg::Info("Value1={},Value2={}", 10,"���");//�����־:Value1=10,Value2="���"
    //��ϸ�μ�:https://github.com/gabime/spdlog
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


    //��...����ɸ�ʽ����std::string
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

    //��װC��ͳ��δ�ӡ��ʽ������־
    //�μ�printfʹ�÷�ʽ
    //��lg::InfoF("This is an %s.","apple");//�����־:This is an apple.
    //��gcc�޷�����ͨ����δ���,�ݲ���ʹ�������װ�ĸ�ʽ��־�ӿ�,��linuxҪʹ��spdlog�ĸ�ʽ��־,�μ�����
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

    //ע����־,�Ὣ������־flush(��־Ĭ����3��д�ļ��ӳ�),�����ڳ���ر�ʱ����
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
    //��ȥextern������Ҫ��cpp��ʵ��
    __declspec(selectany) QAQLogData log_data_;
#endif
}

#endif // _INCLUDED_QAQLOG_
#if defined(_MSC_VER)
#pragma warning(pop)
#endif