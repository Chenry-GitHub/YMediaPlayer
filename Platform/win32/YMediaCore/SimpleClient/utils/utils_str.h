#pragma once
#include <string>
#include <list>
#include <sstream>
namespace utils
{
    namespace str
    {
        //��תխ�ַ���
        std::string ws2s(const std::wstring& ws);
        //խת���ַ���
        std::wstring s2ws(const std::string& s);

        //ͨ��תխ�ַ���:������int,float,double,long������
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

        //ͨ��ת���ַ���:������int,float,double,long������
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
        //UTF8�ַ���תGB2312
        std::string UTF8ToGB2312(const std::string& utf8_string);
        //GB2312�ַ���תUTF8
        std::string GB2312ToUTF8(const std::string& gb2312_string);

        //�ָ��ַ��� "a,b"=>��,�ָ�=>[a],[b]
        void SplitStr(const std::string& s, const std::string& delim, std::list<std::string>* ret);
        //�ָ���ַ��� "a,b"=>��,�ָ�=>[a],[b]
        void SplitStr(const std::wstring& s, const std::wstring& delim, std::list<std::wstring>* ret);
        //�ָ��ַ���Ϊ key value��ʽ
        void SplitStrKeyVal(const std::string& s, const std::string& delim, std::list<std::string>* ret);
        //�滻�ַ��� "abcdef"=>�滻defΪsss=>"abcsss"
        void ReplaceStrDistinct(std::string& str, const std::string& old_value, const std::string& new_value);
        //�滻���ַ��� "abcdef"=>�滻defΪsss=>"abcsss"
        void ReplaceStrDistinct(std::wstring& str, const std::wstring& old_value, const std::wstring& new_value);

        //std::string ��ĸСд��
        std::string StrToLower(const std::string& str);
        //std::wstring ��ĸСд��
        std::wstring StrToLower(const std::wstring& str);

        //std::string ��ĸ��д��
        std::string StrToUpper(const std::string& str);
        //std::wstring ��ĸ��д��
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

        //AES����
        //ori_str:ԭʼ�ַ���
        //key:����key,�̶�16λ
        //iv:����iv,�̶�16λ
        std::string CodeAes(const std::string& ori_str, const std::string& key, const std::string& iv);
        //AES����
        //code_str:�����ַ���
        //key:����key,�̶�16λ
        //iv:����iv,�̶�16λ
        //out_str:����ַ���
        //�����Ƿ�ɹ�,����Ҫ�ⲿ�Խ����ַ��������ٴ���֤
        bool DecodeAes(const std::string& code_str, const std::string& key, const std::string& iv, std::string& out_str);
        //base64����
        std::string CodeBase64(const std::string& ori_str);
        //base64����
        std::string DecodeBase64(const std::string& code_str);
        //��ȡ��ʽ��������
        std::string GetFormatDateTime(const std::string& format = "%X");
        //ȥ��·���ַ������б�ܺ�����
        std::wstring GetPathDir(const std::wstring& path);
        std::string GetPathDir(const std::string& path);
    }
}