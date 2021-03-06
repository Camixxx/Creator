﻿#include <weh.h>
#include <QApplication>
#include <QDesktopWidget>
#include <QMainWindow>
#include <QLibrary>
#include "mainwindow.h"
#include <stdint.h>
#include "quazip/JlCompress.h"
#include "codewindow.h"
#include "qmacopenfileapplication.h"
#include <QStyle>
#include <QStyleFactory>

extern CodeWindow *codeedit;
GLOBALSTRUCTURES_INIT();

//register error handling in Windows
#ifdef WIN32
#include <Windows.h>
#include <DbgHelp.h>
#pragma comment(lib,"user32.lib")

struct EnumHelper
{
	HWND hwnd = NULL;
	HWND emptyHwnd = NULL;
	QString title;
};

BOOL CALLBACK EnumWndProc(HWND hwnd, LPARAM lParam)
{
	EnumHelper *e = (EnumHelper *)lParam;
	wchar_t title[MAX_PATH];
	GetWindowText(hwnd, title, MAX_PATH);
	QString t = QString::fromWCharArray(title);
	if (t == BKE_CREATOR_TITLE)
	{
		e->emptyHwnd = hwnd;
	}
	else if (!e->title.isEmpty() && t.startsWith(BKE_CREATOR_TITLE_PROJECT_PREFIX))
	{
		t = t.mid(sizeof(BKE_CREATOR_TITLE_PROJECT_PREFIX) - 1).replace('\\', '/');
		if (t == e->title)
		{
			e->hwnd = hwnd;
		}
	}
	return !(e->emptyHwnd && (e->title.isEmpty() || e->hwnd));
}

#pragma comment(lib, "DbgHelp.lib")

LONG WINAPI ApplicationCrashHandler(EXCEPTION_POINTERS *pException){//程式异常捕获  
	/*
	***保存数据代码***
	*/
	//创建 Dump 文件  
	auto t = time(NULL);
	auto tm = localtime(&t);
	wchar_t modulename[FILENAME_MAX];
	GetModuleFileName(NULL, modulename, FILENAME_MAX);
	wstring path = modulename;
	size_t pos = path.find_last_of(L"/\\");
	if (pos != wstring::npos)
		modulename[pos + 1] = 0;
	wchar_t filename[512];
	swprintf(filename, L"%s%04d-%02d-%02d %02d-%02d-%02d.dmp", modulename, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	HANDLE hDumpFile = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hDumpFile != INVALID_HANDLE_VALUE){
		//Dump信息  
		MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
		dumpInfo.ExceptionPointers = pException;
		dumpInfo.ThreadId = GetCurrentThreadId();
		dumpInfo.ClientPointers = TRUE;
		//写入Dump文件内容  
		MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpScanMemory, &dumpInfo, NULL, NULL);
		CloseHandle(hDumpFile);
	}
	//这里弹出一个错误对话框并退出程序  
	EXCEPTION_RECORD* record = pException->ExceptionRecord;
	wchar_t msg[100];
	swprintf(msg, L"抱歉崩溃了。error code:%08X", record->ExceptionCode);
	MessageBox(NULL, msg, L"崩溃", 0);
	//保存
	if (codeedit)
		codeedit->backupAll();
	return EXCEPTION_EXECUTE_HANDLER;
}
#endif

uint32_t BKE_hash(const wchar_t *str)
{
	if (!*str)
		return 0;
	const uint32_t _FNV_offset_basis = 2166136261U;
	const uint32_t _FNV_prime = 16777619U;
	const wchar_t *c = str;
	uint32_t ret = _FNV_offset_basis;
	while (*c)
	{
		ret ^= (uint32_t)*c;
		ret *= _FNV_prime;
		c++;
	}
	return ret;
}

void CheckOpenAL32() ;
void CheckFileAssociation();

//检查上次是否正常关闭，返回false表示非正常
bool checkLastClose(QString &pro)
{
	auto userdir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/.BKE_Creator/";
	QFile p(userdir + "pro");
	if (!p.exists())
		return false;
	if (!p.isOpen() && !p.open(QFile::ReadOnly))
		return false;
	return true;
}

void clearCloseInfo()
{
	auto userdir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/.BKE_Creator/";
	QDir d(userdir);
	d.setFilter(QDir::Files);
	int i, j = d.count();
	for (i = 0; i < j; i++)
		d.remove(d[i]);
}

#include "BKS_info.h"

int main(int argc, char *argv[])
{
	QApplication::setApplicationName("BKE_Creator");
	QTextCodec *xcodec = QTextCodec::codecForLocale();

#ifdef WIN32
	SetUnhandledExceptionFilter(ApplicationCrashHandler);
	wchar_t tmp[MAX_PATH];
	GetModuleFileNameW(NULL, tmp, MAX_PATH);
	QString exeDir = QString::fromWCharArray(tmp);
#else
	QString exeDir = xcodec->toUnicode( QByteArray(argv[0]) ) ;
#endif
#ifdef Q_OS_LINUX
#if !defined(QT_DEBUG)
    QDir dir = QFileInfo( exeDir ).absoluteDir();
    dir.cdUp();
    dir.cd("plugins");
    QApplication::addLibraryPath(dir.absolutePath());
    dir.cdUp();
    dir.cd("share");
    BKE_CURRENT_DIR = dir.absolutePath();
#else
    BKE_CURRENT_DIR = QFileInfo(exeDir).absolutePath();
#endif
	//qt has a bug in 5.2.1(windows)? so I use setLibraryPaths
#else
#ifdef Q_OS_MAC
	{
		QDir d = QFileInfo( exeDir ).dir();
		d.cdUp();
		d.cd("PlugIns");
		QApplication::addLibraryPath(d.absolutePath());
	}
#else
#if !defined(Q_OS_WIN) || !defined(QT_DEBUG)
    BKE_CURRENT_DIR = QFileInfo( exeDir ).path();
    //qt has a bug in 5.2.1(windows)? so I use setLibraryPaths
	QApplication::addLibraryPath(BKE_CURRENT_DIR);
#else
	BKE_CURRENT_DIR = QDir::currentPath();
#endif
#endif
#endif

#ifdef Q_OS_MAC
    QMacOpenFileApplication a(argc, argv);
#else
    QApplication a(argc, argv);
#endif
	QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

#ifdef Q_OS_MAC
    BKE_CURRENT_DIR = QDir::homePath() + "/Documents/BKE_Creator";
    QDir dir(BKE_CURRENT_DIR);
    do{
       QDir d(qApp->applicationDirPath());
       d.cdUp();
       d.cd("Resources");

       QString dataPath = d.filePath("data.compress");
       QString macvsPath = dir.filePath("macvs.txt");
       if(dir.exists())
       {
           QFileInfo fi(dataPath);
           QFileInfo vs(macvsPath);
           if(vs.exists())
           {
               QString s;
               if(LOLI::AutoRead(s, macvsPath))
               {
                   if(!fi.exists() || s == QString::number(fi.size()))
                   {
                       break;
                   }
               }
           }
       }

       if(JlCompress::extractDir(dataPath, dir.absolutePath()).count()==0)
       {
           QMessageBox::information(0, "Error", "Cannot uncompress the resources. :( \nPlease unzip \"[.app path]/Resources/data.compress\" to \"~/Documents/BKE_Creator\" by yourself, and write the decimal size of \"data.compress\" into \"macvs.txt\", then restart the application.");
           exit(0);
       }
       LOLI::AutoWrite(macvsPath, QString::number(QFileInfo(dataPath).size()));
    }while(0);
#endif

#ifdef Q_OS_LINUX
    QIcon icon(":/icon.png");
    QApplication::setWindowIcon(icon);
#endif
	//SingleApplication a(argc, argv);

#ifndef WIN32
#else
#ifndef _DEBUG
	do
	{
		EnumHelper e;
		if (argc > 1)
			e.title = xcodec->toUnicode(QByteArray(argv[1])).replace('\\', '/');
		EnumWindows(EnumWndProc, (LPARAM)&e);
		HWND hwnd;
		if (e.hwnd != NULL)
		{
			hwnd = e.hwnd;
		}
		else if (e.emptyHwnd != NULL)
		{
			hwnd = e.emptyHwnd;
			if (!e.title.isEmpty())
			{
				//将命令行参数发送到hwnd
				QByteArray utf8 = e.title.toUtf8();
				COPYDATASTRUCT cpdata = { 0 };
				cpdata.dwData = 1;
				cpdata.lpData = (void *)utf8.constData();
				cpdata.cbData = utf8.size();
				SendMessage(hwnd, WM_COPYDATA, 0, reinterpret_cast<LPARAM>(&cpdata));
			}
		}
		else
		{
			break;
		}
		WINDOWPLACEMENT place = {0};
		place.length = sizeof(WINDOWPLACEMENT);
		GetWindowPlacement(hwnd, &place);

		switch (place.showCmd)
		{
		case SW_SHOWMAXIMIZED:
			ShowWindow(hwnd, SW_SHOWMAXIMIZED);
			break;
		case SW_SHOWMINIMIZED:
			ShowWindow(hwnd, SW_RESTORE);
			break;
		default:
			ShowWindow(hwnd, SW_NORMAL);
			break;
		}

		SetForegroundWindow(hwnd);
		return 0;
	} while (false);
#endif
#endif
	QTranslator translator;
	if( !translator.load("qt_zh_CN",BKE_CURRENT_DIR,"",".qm") ) QMessageBox::information(0,"错误","加载中文翻译失败",QMessageBox::Ok) ;
	a.installTranslator(&translator);
//启动------->>>>>>>>>>

	//创建文件夹
	QDir temp(BKE_CURRENT_DIR ) ;
	temp.mkdir("temp") ;

	temp.setPath(BKE_CURRENT_DIR+"/stencil");
	isSYSTEMP_LOWDER = temp.exists() ; //检查Stencil是否存在，确定系统是否区分大小写

	LOLI_CLEAR_TEMP(BKE_CURRENT_DIR+"/temp");  //清空临时文件夹

	// 读取api命令
	//if( !LOLI::AutoRead(BKE_API_FILE,BKE_CURRENT_DIR+"/command.api") ){
	//	QMessageBox::information(0,"初始化","读取API列表失败",QMessageBox::Ok) ;
	//}

	BkeCreator::LoadRecentProject();

	//LOLI::AutoRead(ks,BKE_CURRENT_DIR+"/files.txt") ;
	//BKE_Recently_Files = ks.split("\r\n") ;

	//读取默认方法api列表
	//BkeCreator::ReadApiList(&SYSlist,BKE_CURRENT_DIR+"/class.api",8) ;
	//BkeCreator::ReadApiList(&KEYlist,BKE_CURRENT_DIR+"/parser.api",9) ;

	//setting
	BKE_CLOSE_SETTING = new QSettings(BKE_CURRENT_DIR+"/setting.ini",QSettings::IniFormat) ;
	BKE_USER_SETTING = new QSettings(BKE_CURRENT_DIR+"/user.ini",QSettings::IniFormat) ;
	BKE_SKIN_SETTING = new QSettings(BKE_CURRENT_DIR+"/skin.ini",QSettings::IniFormat) ;
	BKE_SKIN_CURRENT = BKE_SKIN_SETTING->value("StyleName","默认").toString() ;	//颜色配置项

	//<<<<<<<---------------

	global_bke_info.init();

	MainWindow test ;

//    a.setActiveWidget(&test);
	a.setActiveWindow(&test);
#ifdef Q_OS_WIN
	if( BKE_CLOSE_SETTING->value("window/ismax").toBool() )
		test.showMaximized();
	else 
		test.show();
#else
	test.show();
#endif

	/*QString pro;
	QStringList files;

	bool notclose = checkLastClose(pro, files);

	if (notclose)
	{
		projectedit->OpenProject(pro);
		for (auto &it : files)
		{
			codeedit->addFile(it, projectedit->workpro->FileDir());
			auto edit = codeedit->getCurrentEdit();
			QFileInfo fi(it);
			if (edit)
			{
				auto userdir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/.BKE_Creator/";
				QFile f(userdir + fi.fileName());
				QString text;
				if (LOLI::AutoRead(text, &f))
				{
					edit->setText(text);
				}
			}
		}
	}
	else */if( argc > 1){
		projectedit->OpenProject(xcodec->toUnicode(QByteArray(argv[1])) );
	}

	//QObject::connect(&a,&SingleApplication::newApplication,[=](QString args)
	//    {if(args.count())
	//        projectedit->OpenProject(args);}
	//);

	if( !BKE_CLOSE_SETTING->value("update/close").toBool() ) 
		QTimer::singleShot(500,&test,&MainWindow::CheckUpdate) ;

//#ifdef QT_DEBUG
//    if( !BKE_CLOSE_SETTING->value("update/close").toBool() ) QTimer::singleShot(3000,&test,SLOT(CheckUpdate()) ) ;
//#endif

	//使用win32时，检查依赖库
#if defined(Q_OS_WIN) || defined(Q_OS_LINUX)
	CheckOpenAL32();
	CheckFileAssociation();
#endif
	
	auto res = a.exec();

	delete BKE_CLOSE_SETTING;
	delete BKE_USER_SETTING;
	delete BKE_SKIN_SETTING;

	clearCloseInfo();

	return res;
}

//检测是否安装了openal32，没有则安装
void CheckOpenAL32()
{
#ifdef Q_OS_WIN
	
#else
	QLibrary lib("openal");
	if( lib.load() )
	{
		lib.unload();
		return;
	}
	QMessageBox::information(0,"安装支持库","你的计算机没有安装libopenal1包，请先安装再运行本软件。") ;
	exit(1);
#endif
}

void CheckFileAssociation()
{
#ifdef Q_OS_WIN
	if (!QFile::exists(BKE_CURRENT_DIR + "/FileAssociation.exe"))
		return;
	
	QProcess *cmd = new QProcess();
	QObject::connect(cmd, &QProcess::readyReadStandardOutput, [cmd]() {
		if (QString::fromUtf8(cmd->readAllStandardOutput()).trimmed() != "true")
		{
			if (QMessageBox::question(0, "提示", "检测到工程文件尚未关联。是否关联工程文件？") == QMessageBox::Yes)
			{
				auto s = BKE_CURRENT_DIR.toStdWString();
				SHELLEXECUTEINFO shExInfo = { 0 };
				shExInfo.cbSize = sizeof(shExInfo);
				shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
				shExInfo.hwnd = 0;
				shExInfo.lpVerb = L"runas";                // Operation to perform
				shExInfo.lpFile = L"FileAssociation.exe";       // Application to start    
				shExInfo.lpParameters = L"-set";                  // Additional parameters
				shExInfo.lpDirectory = s.c_str();
				shExInfo.nShow = SW_HIDE;
				shExInfo.hInstApp = 0;

				ShellExecuteEx(&shExInfo);
			}
		}
		delete cmd;
	});
	cmd->setWorkingDirectory(BKE_CURRENT_DIR);
	cmd->start("FileAssociation.exe -is");
#else
	QProcess *cmd = new QProcess();
	QObject::connect(cmd, &QProcess::readyReadStandardOutput, [cmd](){
		if(QString::fromUtf8(cmd->readAllStandardOutput()).trimmed() != "bke_creator.desktop")
		{
			if(QMessageBox::question(0,"提示","检测到工程文件尚未关联。是否关联工程文件？")==QMessageBox::Yes)
			{
				class MyThread : public QThread
				{
				protected:
					void run()
					{
						system(("pkexec '"+BKE_CURRENT_DIR+"/add-menuitem.sh'").toLocal8Bit());
					}
				};
				MyThread *s = new MyThread;
				s->start();
			}
		}
		delete cmd;
	});
	cmd->start("xdg-mime query default application/x-bke-project");
#endif
}
