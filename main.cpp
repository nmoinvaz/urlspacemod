#include <windows.h>
#include <httpfilt.h> 
#include <shlwapi.h>
#include <wininet.h>
#include <stdio.h>

#pragma comment(lib, "shlwapi.lib")

/*********************************************************************/

#define APP_VERSION         MAKELONG(1,0)
#define APP_NAME            "url+rewrite"   
#define APP_DESCRIPTION     APP_NAME

/*********************************************************************/

BOOL Debug_PrintEvent(WORD wType, LPCSTR Format, ...)
{
    HANDLE EventSource;
    LPCTSTR Strings[1];
    char Buffer[4098];
    va_list Args;


    va_start(Args, Format);

    vsnprintf_s(Buffer, 4098, _TRUNCATE, Format, Args);

    Strings[0]  = (LPCTSTR)Buffer;
    EventSource = RegisterEventSourceA(NULL, APP_NAME);

    if (EventSource == NULL)
        return FALSE;
    
    ReportEventA(EventSource, wType, 0, 0, NULL, 1, 0, (LPCSTR*)&Strings[0], NULL);
    DeregisterEventSource(EventSource);

    va_end(Args);

    return TRUE;
}

BOOL WINAPI __stdcall GetFilterVersion(HTTP_FILTER_VERSION *Version)
{
    Version->dwFlags         = SF_NOTIFY_ORDER_DEFAULT | SF_NOTIFY_NONSECURE_PORT | SF_NOTIFY_SECURE_PORT | SF_NOTIFY_PREPROC_HEADERS;
    Version->dwFilterVersion = APP_VERSION;
    
    strncpy(Version->lpszFilterDesc, APP_DESCRIPTION, SF_MAX_FILTER_DESC_LEN);
    
    Debug_PrintEvent(EVENTLOG_INFORMATION_TYPE, "%s started", APP_NAME);
    
    return TRUE;
}

DWORD WINAPI __stdcall HttpFilterProc(HTTP_FILTER_CONTEXT *Context, DWORD NotificationType, VOID *Data)
{
    DWORD Status = SF_STATUS_REQ_NEXT_NOTIFICATION; 
    HTTP_FILTER_PREPROC_HEADERS *Headers;
    char Url[INTERNET_MAX_URL_LENGTH];
    char NewUrl[INTERNET_MAX_URL_LENGTH];
    DWORD UrlLength = INTERNET_MAX_URL_LENGTH, i, x;
    WORD Result;


    if (Context == NULL)
        return Status;

    switch (NotificationType)
        {
        case SF_NOTIFY_PREPROC_HEADERS:

                Headers = (HTTP_FILTER_PREPROC_HEADERS*)Data;
        
                if (Headers == NULL)
                    break;
                if (Headers->GetHeader(Context, "url", Url, &UrlLength) == TRUE)
                    {
                    if ((UrlLength > 1) && (memchr(Url, '+', UrlLength) != NULL))
                        {
                        memset(NewUrl, 0, INTERNET_MAX_URL_LENGTH);
 
                        for (i = 0, x = 0; i < INTERNET_MAX_URL_LENGTH; i += 1, x += 1)
                            {
                            NewUrl[i] = Url[x];

                            if (NewUrl[i] == 0)
                                break;
                            if (NewUrl[i] != '+')
                                continue;

                            NewUrl[i] = 0;
                            strncat(NewUrl, "%20", INTERNET_MAX_URL_LENGTH);
                            i += 2;
                            }
                        
                        Result = Headers->SetHeader(Context, "url", NewUrl);

//#if defined(_DEBUG)
                        Debug_PrintEvent(EVENTLOG_INFORMATION_TYPE, "Url changed from %s to %s (%d:%d)", Url, NewUrl, Result, GetLastError());
//#endif
                        }
                    }

                break;
        }
        
    return Status;
}

BOOL APIENTRY DllMain(HMODULE Module, DWORD ReasonForCall, LPVOID Reserved)
{
    switch (ReasonForCall)
        {
        case DLL_PROCESS_ATTACH:
        
                break;
      
        case DLL_PROCESS_DETACH:
                
                break;
        }
    
    return TRUE;
}

/*********************************************************************/