#include "main.h"
#include "resource.h"
#include "gui/login/login_form.h"
#include "gui/main/main_form.h"
#include "app_dump.h"
#include "base/util/at_exit.h"
#include "shared/xml_util.h"
#include "base/util/string_number_conversions.h"
#include "callback/chatroom_callback.h"
#include "module/config/config_manager.h"

void MainThread::Init()
{
	nbase::ThreadManager::RegisterThread(kThreadUI);
	PreMessageLoop();

	std::wstring theme_dir = QPath::GetAppPath();
	ui::GlobalManager::Startup(theme_dir + L"resources\\", ui::CreateControlCallback(), false);

	nim_ui::UserConfig::GetInstance()->SetDefaultIcon(IDI_ICON);

	std::wstring app_crash = QCommand::Get(kCmdAppCrash);
	if( app_crash.empty() )
	{	
		nim_ui::WindowsManager::SingletonShow<LoginForm>(LoginForm::kClassName);
	}
	else
	{
		std::wstring content(L"程序崩溃了，崩溃日志：");
		content.append(app_crash);

		MsgboxCallback cb = nbase::Bind(&MainThread::OnMsgBoxCallback, this, std::placeholders::_1);
		ShowMsgBox(NULL, content, cb, L"提示", L"打开", L"取消");
	}
}

void MainThread::Cleanup()
{
	ui::GlobalManager::Shutdown();

	PostMessageLoop();
	SetThreadWasQuitProperly(true);
	nbase::ThreadManager::UnregisterThread();

	nim_chatroom::ChatRoom::Cleanup();
}

void MainThread::PreMessageLoop()
{
	misc_thread_.reset( new MiscThread(kThreadGlobalMisc, "Global Misc Thread") );
	misc_thread_->Start();

	screen_capture_thread_.reset(new MiscThread(kThreadScreenCapture, "screen capture"));
	screen_capture_thread_->Start();

	db_thread_.reset( new DBThread(kThreadDatabase, "Database Thread") );
	db_thread_->Start();
}

void MainThread::PostMessageLoop()
{
	misc_thread_->Stop();
	misc_thread_.reset(NULL);

	screen_capture_thread_->Stop();
	screen_capture_thread_.reset(NULL);

	db_thread_->Stop();
	db_thread_.reset(NULL);
}

void MainThread::OnMsgBoxCallback( MsgBoxRet ret )
{
	if(ret == MB_YES)
	{
		std::wstring dir = QPath::GetNimAppDataDir();
		QCommand::AppStartWidthCommand(dir, L"");
	}

	nim_ui::WindowsManager::SingletonShow<LoginForm>(LoginForm::kClassName);
}

/**
* 全局函数，初始化云信。包括读取应用服务器地址，载入云信sdk，初始化安装目录和用户目录，注册收到推送时执行的回调函数。
* @return void 无返回值
*/
static void InitNim()
{
	// 读取本地配置文件并预设一些默认值，如果配置文件中读取不到，则使用这些默认值
	const auto& config_manager = nim_conf::ConfigManager::GetInstance();
	config_manager->LoadConfig(nbase::UTF16ToUTF8(QPath::GetAppPath() + L"nim_server.conf"));
	config_manager->SetDefaultValue(nim_conf::kAppKey, "1ee5a51b7d008254cd73b1d4369a9494");
	config_manager->SetDefaultValue(nim_conf::kWebBoardAddress, "http://apptest.netease.im/webdemo/testWB/webview.html");

	nim::SDKConfig config;
	// SDK 能力参数（必填）目前只支持最多32个字符的加密密钥！建议使用32个字符
	config.database_encrypt_key_ = "Netease";
	// 载入云信sdk，初始化安装目录和用户目录，并设置 SDK 回调线程
	bool ret = nim::Client::Init(config_manager->GetValue(nim_conf::kAppKey), "NIM_EDU", "", config);
	assert(ret);
	nim::Client::SetCallbackFunction([](const StdClosure& task) {
		nbase::ThreadManager::PostTask(kThreadUI, task);
	});
	// 初始化聊天室组件，并设置 SDK 回调线程
	ret = nim_chatroom::ChatRoom::Init("");
	assert(ret);
#ifdef CPPWRAPPER_DLL
	nim_chatroom::ChatRoom::SetCallbackFunction([](const StdClosure& task) {
		nbase::ThreadManager::PostTask(ThreadId::kThreadUI, task);
	});
#endif

	nim_ui::InitManager::GetInstance()->InitUiKit();
	nim_chatroom::ChatroomCallback::InitChatroomCallback();
}

int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPWSTR lpszCmdLine, int nCmdShow)
{
	nbase::AtExitManager at_manager;

	CComModule _Module;
	_Module.Init(NULL, hInst);

	_wsetlocale(LC_ALL, L"chs");

#ifdef _DEBUG
	AllocConsole();
	FILE* fp = NULL;
	freopen_s(&fp, "CONOUT$", "w+t", stdout);
	wprintf_s(L"Command:\n%s\n\n", lpszCmdLine);
#endif

	srand( (unsigned int) time(NULL) );

	::SetUnhandledExceptionFilter(MyUnhandledExceptionFilter);

	QCommand::ParseCommand(lpszCmdLine);

	HRESULT hr = ::OleInitialize(NULL);
	if( FAILED(hr) )
		return 0;

	InitNim(); // 初始化云信和UI组件

	{
		MainThread thread; // 创建主线程
		thread.RunOnCurrentThreadWithLoop(nbase::MessageLoop::kUIMessageLoop); // 执行主线程循环
	}
	QLOG_APP(L"exit ui loop");

	// 程序结束之前，清理云信sdk和UI组件
	nim_ui::InitManager::GetInstance()->CleanupUiKit();

	QLOG_APP(L"app exit");

	// 是否重新运行程序
	std::wstring restart = QCommand::Get(kCmdRestart);
	if( !restart.empty() )
	{
		std::wstring cmd;
		std::wstring acc = QCommand::Get(kCmdAccount);
		if( !acc.empty() )
			cmd.append( nbase::StringPrintf(L" /%s %s ", kCmdAccount.c_str(), acc.c_str()) );
		std::wstring exit_why = QCommand::Get(kCmdExitWhy);
		if( !exit_why.empty() )
			cmd.append( nbase::StringPrintf(L" /%s %s ", kCmdExitWhy.c_str(), exit_why.c_str()) );
		QCommand::RestartApp(cmd);
	}

	_Module.Term();
	::OleUninitialize();

	return 0;
}

