#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

#include "pcie_speed.h"
#include <stdint.h>

	typedef enum
	{
		TOAST_RESULT_SUCCESS = 0,
		TOAST_RESULT_TOAST_CLICKED,				  // user clicked on the toast
		TOAST_RESULT_TOAST_DISMISSED,			  // user dismissed the toast
		TOAST_RESULT_TOAST_TIMEOUT,				  // toast timed out
		TOAST_RESULT_TOAST_HIDED,				  // application hid the toast
		TOAST_RESULT_TOAST_NOT_ACTIVATED,		  // toast was not activated
		TOAST_RESULT_TOAST_FAILED,				  // toast failed
		TOAST_RESULT_SYSTEM_NOT_SUPPORTED,		  // system does not support toasts
		TOAST_RESULT_UNHANDLED_OPTION,			  // unhandled option
		TOAST_RESULT_MULTIPLE_TEXT_NOT_SUPPORTED, // multiple texts were provided
		TOAST_RESULT_INITIALIZATION_FAILURE, // toast notification manager initialization failure
		TOAST_RESULT_TOAST_NOT_LAUNCHED		 // toast could not be launched
	} toast_result_t;

	toast_result_t toast_init(void);

	toast_result_t toast_show(const char *const attribution, const char *const first_line,
							  const char *const second_line, uint64_t expiration_ms);

	toast_result_t toast_showW(const wchar_t *const attribution, const wchar_t *const first_line,
							   const wchar_t *const second_line, uint64_t expiration_ms);

#ifdef __cplusplus
}
#endif
