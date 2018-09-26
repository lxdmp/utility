#ifndef _LOG_MACRO_H_
#define _LOG_MACRO_H_

#include <log4cplus/loggingmacros.h>
#include <log4cplus/configurator.h>

void initRootLogger(const char *exe_path);

#define LOG_FATAL(...)     \
    LOG4CPLUS_FATAL_FMT(log4cplus::Logger::getRoot(), __VA_ARGS__)


#define LOG_ERROR(...)     \
    LOG4CPLUS_ERROR_FMT(log4cplus::Logger::getRoot(), __VA_ARGS__)


#define LOG_WARN(...)     \
    LOG4CPLUS_WARN_FMT(log4cplus::Logger::getRoot(), __VA_ARGS__)


#define LOG_INFO(...)     \
    LOG4CPLUS_INFO_FMT(log4cplus::Logger::getRoot(), __VA_ARGS__)

#define LOG_DEBUG(...)     \
    LOG4CPLUS_DEBUG_FMT(log4cplus::Logger::getRoot(), __VA_ARGS__)


#define LOG_TRACE(...)     \
    LOG4CPLUS_TRACE_FMT(log4cplus::Logger::getRoot(), __VA_ARGS__)


#define LOG_TRACE_ENTER() \
    LOG_TRACE("Enter %s",  __FUNCTION__)

#define LOG_TRACE_LEAVE() \
    LOG_TRACE("Leave %s",  __FUNCTION__)

#endif

