#define CUR_PLATFORM PLATFORM_WIN32
////////////////////change up there to change build platform///////////////////////

#define PLATFORM_WIN32			0x01
#define PLATFORM_ANDROID		0x02
#define PLATFORM_IOS					0x03

#define PLATFORM_IS_WIN32  (CUR_PLATFORM == PLATFORM_WIN32)
#define PLATFORM_IS_ANDROID  (CUR_PLATFORM == PLATFORM_ANDROID)
#define PLATFORM_IS_IOS  (CUR_PLATFORM == PLATFORM_IOS)
