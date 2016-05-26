/* instructions de pre processing */
#include <winsock2.h>
#include <windows.h>

#pragma comment(lib,"ws2_32.lib")

#define IDC_EDIT_IN		101
#define IDC_EDIT_OUT		102
#define IDC_MAIN_BUTTON		103
#define WM_SOCKET		104

#define IDM_FILE_NEW 1
#define IDM_FILE_OPEN 2
#define IDM_FILE_QUIT 3

void AddMenus(HWND);
/* déclarations et initialisations des variables et constantes */
char *szServer="127.0.0.1";
int nPort=5555;

HWND hEditIn=NULL;
HWND hEditOut=NULL;
SOCKET Socket=0;
char szHistory[10000];

LRESULT CALLBACK WinProc(HWND hWnd,UINT message,WPARAM wParam,LPARAM lParam);

/* fonction principale qui ouvre la fenetre graphique */
int WINAPI WinMain(HINSTANCE hInst,HINSTANCE hPrevInst,LPSTR lpCmdLine,int nShowCmd)
{
	WNDCLASSEX wClass;
	ZeroMemory(&wClass,sizeof(WNDCLASSEX));
	wClass.cbClsExtra=0;
	wClass.cbSize=sizeof(WNDCLASSEX);
	wClass.cbWndExtra=0;
	wClass.hbrBackground=(HBRUSH)COLOR_WINDOW;
	wClass.hCursor=LoadCursor(0,IDC_ARROW);
	wClass.hIcon=0;
	wClass.hIconSm=0;
	wClass.hInstance=hInst;
	wClass.lpfnWndProc=(WNDPROC)WinProc;
	wClass.lpszClassName="Window Class";
	wClass.lpszMenuName=0;
	wClass.style=CS_HREDRAW|CS_VREDRAW;

	if(!RegisterClassEx(&wClass))
	{
//		int nResult=GetLastError();
		MessageBox(NULL,
			"Window class creation failed\r\nError code:",
			"Window Class Failed",
			MB_ICONERROR);
	}
        /* creation de la fenetre avec ses parametres  */
	HWND hWnd=CreateWindowEx(NULL,
			"Window Class",
			"Windows Async Client",
			WS_OVERLAPPEDWINDOW,
			200,
			200,
			640,
			480,
			NULL,
			NULL,
			hInst,
			0);
        /* recuperation d'un message en cas erreur */
	if(!hWnd)
	{
		int nResult=GetLastError();

		MessageBox(NULL,
			"Window creation failed\r\nError code:",
			"Window Creation Failed",
			MB_ICONERROR);
	}
    /* fonction qui ouvre la fenetre cree precedemment */
    ShowWindow(hWnd,nShowCmd);
        /* declaration des variables contenants les messages */
	MSG msg;
	ZeroMemory(&msg,sizeof(MSG));

	while(GetMessage(&msg,NULL,0,0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}
/* fonction qui gere les evenements de la fenetre graphique  ainsi que l envoi et la reception des messages vers le serveur */
LRESULT CALLBACK WinProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam)
{
	switch(msg)
	{
		case WM_CREATE:
		{
			AddMenus(hWnd);
			ZeroMemory(szHistory,sizeof(szHistory));

			// Creation de la zone d affichage des messages recu dans la fenetre
			hEditIn=CreateWindowEx(WS_EX_CLIENTEDGE,
				"EDIT",
				"",
				WS_CHILD|WS_VISIBLE|ES_MULTILINE|
				ES_AUTOVSCROLL|ES_AUTOHSCROLL,
				50,
				120,
				400,
				200,
				hWnd,
				(HMENU)IDC_EDIT_IN,
				GetModuleHandle(NULL),
				NULL);
			if(!hEditIn)
			{
				MessageBox(hWnd,
					"Could not create incoming edit box.",
					"Error",
					MB_OK|MB_ICONERROR);
			}
			HGDIOBJ hfDefault=GetStockObject(DEFAULT_GUI_FONT);
			SendMessage(hEditIn,
				WM_SETFONT,
				(WPARAM)hfDefault,
				MAKELPARAM(FALSE,0));
			SendMessage(hEditIn,
				WM_SETTEXT,
				NULL,
				(LPARAM)"Attempting to connect to server...");

			// Creation de la zone de texte editable 
			hEditOut=CreateWindowEx(WS_EX_CLIENTEDGE,
						"EDIT",
						"",
						WS_CHILD|WS_VISIBLE|ES_MULTILINE|
						ES_AUTOVSCROLL|ES_AUTOHSCROLL,
						50,
						50,
						400,
						60,
						hWnd,
						(HMENU)IDC_EDIT_IN,
						GetModuleHandle(NULL),
						NULL);
			if(!hEditOut)
			{
				MessageBox(hWnd,
					"Could not create outgoing edit box.",
					"Error",
					MB_OK|MB_ICONERROR);
			}
                        /*  initialisation du contenu de la zone editable */
			SendMessage(hEditOut,
				WM_SETFONT,(WPARAM)hfDefault,
				MAKELPARAM(FALSE,0));
			SendMessage(hEditOut,
				WM_SETTEXT,
				NULL,
				(LPARAM)"Type message here...");

			// Creation du bouton envoyer
			HWND hWndButton=CreateWindow( 
					    "BUTTON",
						"Send",
						WS_TABSTOP|WS_VISIBLE|
						WS_CHILD|BS_DEFPUSHBUTTON,
						50,	
						330,
						75,
						23,
						hWnd,
						(HMENU)IDC_MAIN_BUTTON,
						GetModuleHandle(NULL),
						NULL);
			
			SendMessage(hWndButton,
				WM_SETFONT,
				(WPARAM)hfDefault,
				MAKELPARAM(FALSE,0));

			/* parametrage de winsock */
			WSADATA WsaDat;
			int nResult=WSAStartup(MAKEWORD(2,2),&WsaDat);
			if(nResult!=0)
			{
				MessageBox(hWnd,
					"Winsock initialization failed",
					"Critical Error",
					MB_ICONERROR);
				SendMessage(hWnd,WM_DESTROY,NULL,NULL);
				break;
			}
                        /* creation de la socket */
			Socket=socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);
			if(Socket==INVALID_SOCKET)
			{
				MessageBox(hWnd,
					"Socket creation failed",
					"Critical Error",
					MB_ICONERROR);
				SendMessage(hWnd,WM_DESTROY,NULL,NULL);
				break;
			}

			nResult=WSAAsyncSelect(Socket,hWnd,WM_SOCKET,(FD_CLOSE|FD_READ));
			if(nResult)
			{
				MessageBox(hWnd,
					"WSAAsyncSelect failed",
					"Critical Error",
					MB_ICONERROR);
				SendMessage(hWnd,WM_DESTROY,NULL,NULL);
				break;
			}

			// Resolve IP address for hostname
			struct hostent *host;
			if((host=gethostbyname(szServer))==NULL)
			{
				MessageBox(hWnd,
					"Unable to resolve host name",
					"Critical Error",
					MB_ICONERROR);
				SendMessage(hWnd,WM_DESTROY,NULL,NULL);
				break;
			}

			/* configuration de la structure de l adresse de la socket */
			SOCKADDR_IN SockAddr;
			SockAddr.sin_port=htons(nPort);
			SockAddr.sin_family=AF_INET;
			SockAddr.sin_addr.s_addr=*((unsigned long*)host->h_addr);

			connect(Socket,(LPSOCKADDR)(&SockAddr),sizeof(SockAddr));
		}
		break;

		case WM_COMMAND:
			switch(LOWORD(wParam))
            {
				case IDC_MAIN_BUTTON:
				{
					char szBuffer[1024];

					int test=sizeof(szBuffer);
					ZeroMemory(szBuffer,sizeof(szBuffer));

					SendMessage(hEditOut,
						WM_GETTEXT,
						sizeof(szBuffer),
						reinterpret_cast<LPARAM>(szBuffer));
					send(Socket,szBuffer,strlen(szBuffer),0);
					SendMessage(hEditOut,WM_SETTEXT,NULL,(LPARAM)"");
				}
				break;
				
				case IDM_FILE_QUIT:
              
                  SendMessage(hWnd, WM_CLOSE, 0, 0);
                  break;
			}
			break;

		case WM_DESTROY:
		{
			PostQuitMessage(0);
			shutdown(Socket,SD_BOTH);
			closesocket(Socket);
			WSACleanup();
			return 0;
		}
		break;

		case WM_SOCKET:
		{
			if(WSAGETSELECTERROR(lParam))
			{	
				MessageBox(hWnd,
					"Connection to server failed",
					"Error",
					MB_OK|MB_ICONERROR);
				SendMessage(hWnd,WM_DESTROY,NULL,NULL);
				break;
			}
			switch(WSAGETSELECTEVENT(lParam))
			{
				case FD_READ:
				{
					char szIncoming[1024];
					ZeroMemory(szIncoming,sizeof(szIncoming));

					int inDataLength=recv(Socket,
						(char*)szIncoming,
						sizeof(szIncoming)/sizeof(szIncoming[0]),
						0);

					strncat(szHistory,szIncoming,inDataLength);
					strcat(szHistory,"\r\n");

					SendMessage(hEditIn,
						WM_SETTEXT,
						sizeof(szIncoming)-1,
						reinterpret_cast<LPARAM>(&szHistory));
				}
				break;

				case FD_CLOSE:
				{
					MessageBox(hWnd,
						"Server closed connection",
						"Connection closed!",
						MB_ICONINFORMATION|MB_OK);
					closesocket(Socket);
					SendMessage(hWnd,WM_DESTROY,NULL,NULL);
				}
				break;
			}
		} 
	}

	return DefWindowProc(hWnd,msg,wParam,lParam);
}

void AddMenus(HWND hwnd) {

    HMENU hMenubar;
    HMENU hMenu;

    hMenubar = CreateMenu();
    hMenu = CreateMenu();

    AppendMenuW(hMenu, MF_STRING, IDM_FILE_NEW, L"&New");
    AppendMenuW(hMenu, MF_STRING, IDM_FILE_OPEN, L"&Options");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);
    AppendMenuW(hMenu, MF_STRING, IDM_FILE_QUIT, L"&Quit");

    AppendMenuW(hMenubar, MF_POPUP, (UINT_PTR) hMenu, L"&Edit");
    SetMenu(hwnd, hMenubar);
}