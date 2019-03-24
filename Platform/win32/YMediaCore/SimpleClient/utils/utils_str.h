#pragma once
#include <string>
#include <list>
#include <sstream>
namespace utils
{
    namespace str
    {
        //宽转窄字符串
        std::string ws2s(const std::wstring& ws);
        //窄转宽字符串
        std::wstring s2ws(const std::string& s);

        //通用转窄字符串:适用于int,float,double,long等数字
        template <class T>
        std::string ToStdString(T data)
        {
            std::stringstream stream;
            stream << data;
            std::string result(stream.str());
            return result;
        }
        //const std::wstring& >> std::string
        template <> std::string ToStdString(const std::wstring& ws);

        //std::wstring >> std::string
        template <> std::string ToStdString(std::wstring ws);

        //const wchar_t* >> std::string
        template <> std::string ToStdString(const wchar_t* cwc);

        //bool >> std::string
        template <> std::string ToStdString(bool b);

        //通用转宽字符串:适用于int,float,double,long等数字
        template <class T>
        std::wstring ToStdWString(T data)
        {
            std::wstringstream wstream;
            wstream << data;
            std::wstring result(wstream.str());
            return result;
        }
        template <> std::wstring ToStdWString(const std::string& str);
        template <> std::wstring ToStdWString(std::string str);

		bool IsTextUTF8(const std::string& rstr);
		std::string ToUTF8(const std::wstring& wstr);
		std::string ToUTF8A(const std::string& str);
		std::wstring FromUTF8(const std::string& utf8_str);
		std::string FromUTF8A(const std::string& utf8_str);
        //UTF8字符串转GB2312
        std::string UTF8ToGB2312(const std::string& utf8_string);
        //GB2312字符串转UTF8
        std::string GB2312ToUTF8(const std::string& gb2312_string);

        //分割字符串 "a,b"=>以,分割=>[a],[b]
        void SplitStr(const std::string& s, const std::string& delim, std::list<std::string>* ret);
        //分割宽字符串 "a,b"=>以,分割=>[a],[b]
        void SplitStr(const std::wstring& s, const std::wstring& delim, std::list<std::wstring>* ret);
        //分割字符串为 key value形式
        void SplitStrKeyVal(const std::string& s, const std::string& delim, std::list<std::string>* ret);
        //替换字符串 "abcdef"=>替换def为sss=>"abcsss"
        void ReplaceStrDistinct(std::string& str, const std::string& old_value, const std::string& new_value);
        //替换宽字符串 "abcdef"=>替换def为sss=>"abcsss"
        void ReplaceStrDistinct(std::wstring& str, const std::wstring& old_value, const std::wstring& new_value);

        //std::string 字母小写化
        std::string StrToLower(const std::string& str);
        //std::wstring 字母小写化
        std::wstring StrToLower(const std::wstring& str);

        //std::string 字母大写化
        std::string StrToUpper(const std::string& str);
        //std::wstring 字母大写化
        std::wstring StrToUpper(const std::wstring& str);

        template <class T>
        T ConvertTo(const std::string& str, const T& def_val)
        {
            throw std::exception("not support type.");
        }
        template<> int ConvertTo(const std::string& str, const int& def_val);
        template<> long ConvertTo(const std::string& str, const long& def_val);
        template<> unsigned long ConvertTo(const std::string& str, const unsigned long& def_val);
        template<> double ConvertTo(const std::string& str, const double& def_val);
        template<> float ConvertTo(const std::string& str, const float& def_val);
        template<> bool ConvertTo(const std::string& str, const bool& def_val);

        //AES加密
        //ori_str:原始字符串
        //key:加密key,固定16位
        //iv:加密iv,固定16位
        std::string CodeAes(const std::string& ori_str, const std::string& key, const std::string& iv);
        //AES解密
        //code_str:加密字符串
        //key:加密key,固定16位
        //iv:加密iv,固定16位
        //out_str:输出字符串
        //返回是否成功,仍需要外部对解密字符串进行再次验证
        bool DecodeAes(const std::string& code_str, const std::string& key, const std::string& iv, std::string& out_str);
        //base64加密
        std::string CodeBase64(const std::string& ori_str);
        //base64解密
        std::string DecodeBase64(const std::string& code_str);
        //获取格式化的日期
        std::string GetFormatDateTime(const std::string& format = "%X");
        //去掉路径字符串最后斜杠和内容
        std::wstring GetPathDir(const std::wstring& path);
        std::string GetPathDir(const std::string& path);
    }
}