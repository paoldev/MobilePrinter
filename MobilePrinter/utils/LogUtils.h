#pragma once

enum class  LogType : int
{
	LogMin = 0,
	LogDebug = LogMin,
	LogVerbose,
	LogDefault,
	LogWarning,
	LogError,
	LogMax,
};

extern LogType s_minLogType;

#define LOG(format, ...) do { if (s_minLogType <= LogType::LogDefault) printf("%ls - " __FUNCTION__ " " ##format "\n", time_now().c_str(), ##__VA_ARGS__); } while(0)
#define LOG2(format, ...) do { if (s_minLogType <= LogType::LogDefault) printf(##format, ##__VA_ARGS__); } while(0)
#define VLOG(format, ...) do { if (s_minLogType <= LogType::LogVerbose) printf("%ls - " __FUNCTION__ " " ##format "\n", time_now().c_str(), ##__VA_ARGS__); } while(0)
#define VLOG2(format, ...) do { if (s_minLogType <= LogType::LogVerbose) printf(##format, ##__VA_ARGS__); } while(0)
#define DBGLOG(format, ...) do { if (s_minLogType <= LogType::LogDebug) printf("%ls - " __FUNCTION__ " " ##format "\n", time_now().c_str(), ##__VA_ARGS__); } while(0)
#define DBGLOG2(format, ...) do { if (s_minLogType <= LogType::LogDebug) printf(##format, ##__VA_ARGS__); } while(0)
#define WLOG(format, ...) do { if (s_minLogType <= LogType::LogWarning) printf("%ls - " __FUNCTION__ " " ##format "\n", time_now().c_str(), ##__VA_ARGS__); } while(0)
#define WLOG2(format, ...) do { if (s_minLogType <= LogType::LogWarning) printf(##format, ##__VA_ARGS__); } while(0)
#define ELOG(format, ...) do { if (s_minLogType <= LogType::LogError) printf("%ls - " __FUNCTION__ " " ##format "\n", time_now().c_str(), ##__VA_ARGS__); } while(0)
#define ELOG2(format, ...) do { if (s_minLogType <= LogType::LogError) printf(##format, ##__VA_ARGS__); } while(0)
