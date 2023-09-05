#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include "toast.h"
#include "wintoastlib.h"

#include <codecvt>

using namespace WinToastLib;

class ToastHandler : public IWinToastHandler
{
  public:
	void toastActivated() const
	{
		std::wcout << L"The user clicked in this toast" << std::endl;
		exit(0);
	}

	void toastActivated(int actionIndex) const
	{
		std::wcout << L"The user clicked on action #" << actionIndex << std::endl;
		exit(16 + actionIndex);
	}

	void toastDismissed(WinToastDismissalReason state) const
	{
		switch(state) {
		case UserCanceled:
			std::wcout << L"The user dismissed this toast" << std::endl;
			exit(0);
			break;
		case TimedOut:
			std::wcout << L"The toast has timed out" << std::endl;
			// exit(2);
			break;
		case ApplicationHidden:
			std::wcout << L"The application hid the toast using ToastNotifier.hide()" << std::endl;
			exit(3);
			break;
		default:
			std::wcout << L"Toast not activated" << std::endl;
			exit(4);
			break;
		}
	}

	void toastFailed() const
	{
		std::wcout << L"Error showing current toast" << std::endl;
		exit(5);
	}
};

toast_result_t toast_init(void)
{
	if(!WinToast::isCompatible()) {
		std::wcerr << L"Error, your system in not supported!" << std::endl;
		return TOAST_RESULT_SYSTEM_NOT_SUPPORTED;
	}

	std::wstring appName = L"GPU Speed Check";
	std::wstring companyName = appName + L" company";
	std::wstring subproduct = appName + L" subproduct";
	std::wstring version = L"v1.0.0";

	WinToast::instance()->setAppName(appName);
	WinToast::instance()->setAppUserModelId(
		WinToast::configureAUMI(companyName, appName, subproduct, version));

	if(!WinToast::instance()->initialize()) {
		std::wcerr << L"Error, your system in not compatible!" << std::endl;
		return TOAST_RESULT_INITIALIZATION_FAILURE;
	}

	return TOAST_RESULT_SUCCESS;
}

toast_result_t toast_show(const char *const attribution, const char *const first_line,
						  const char *const second_line, uint64_t expiration_ms)
{
	std::string _attribution((attribution) ? attribution : "");
	std::string _first_line((first_line) ? first_line : "");
	std::string _second_line((second_line) ? second_line : "");

	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;

	return toast_showW(converter.from_bytes(_attribution).c_str(),
					   converter.from_bytes(_first_line).c_str(),
					   converter.from_bytes(_second_line).c_str(), expiration_ms);
}

toast_result_t toast_showW(const wchar_t *const attribution, const wchar_t *const first_line,
						   const wchar_t *const second_line, uint64_t expiration_ms)
{
	std::wstring _attribution((attribution) ? attribution : L"");
	std::wstring _first_line((first_line) ? first_line : L"");
	std::wstring _second_line((second_line) ? second_line : L"");

	WinToastTemplate templ(WinToastTemplate::ImageAndText02);
	templ.setTextField(_first_line, WinToastTemplate::FirstLine);
	templ.setTextField(_second_line, WinToastTemplate::SecondLine);
	templ.setAttributionText(_attribution);

	templ.setExpiration(expiration_ms);
	templ.setDuration(WinToastTemplate::Duration::Long);

	templ.setAudioPath(WinToastTemplate::AudioSystemFile::Reminder);

	if(WinToast::instance()->showToast(templ, new ToastHandler()) < 0) {
		std::wcerr << L"Could not launch your toast notification!";
		return TOAST_RESULT_TOAST_FAILED;
	}

	return TOAST_RESULT_SUCCESS;
}
