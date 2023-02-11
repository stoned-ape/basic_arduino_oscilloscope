#include <windows.h>
#include <gl/gl.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>

#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"opengl32.lib")

BOOL win32_check(BOOL ret,const char *ret_str,const char *file,int line,BOOL exit){
    if(!ret){
        int error=GetLastError();
        if(!error) return 1;
        char *msg=NULL;
        FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL,error,0,msg,0,NULL
        );
        fprintf(
            stderr,"WIN32_CHECK(%s) failed: file %s line %d errno %d msg %s\n",
            ret_str,file,line,error,msg
        );
        if(exit) ExitProcess(1);
        LocalFree(msg);
    }
    return ret;
}

#define WIN32_CHECK(call) win32_check(call,#call,__FILE__,__LINE__,1)
#define WIN32_CHECK_NOEXIT(call) win32_check(call,#call,__FILE__,__LINE__,0)

LRESULT CALLBACK WndProc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam){
    switch(message){
        case WM_CLOSE: 
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProc(hwnd,message,wparam,lparam);
}

inline float map(float t,float t0,float t1,float s0,float s1){
    return s0+(s1-s0)*(t-t0)/(t1-t0);
}

int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int iCmdShow){
    AttachConsole(-1);
    freopen("CONIN$", "r",stdin);
    freopen("CONOUT$", "w",stdout);
    freopen("CONOUT$", "w",stderr);
    puts("");


    HANDLE hserial;
    hserial=CreateFile("COM5",GENERIC_READ,0,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
    if(hserial==INVALID_HANDLE_VALUE) WIN32_CHECK(0);
    
    DCB dcb_serial_params={0};
    dcb_serial_params.DCBlength=sizeof(dcb_serial_params);
    WIN32_CHECK(GetCommState(hserial,&dcb_serial_params));
    
    dcb_serial_params.BaudRate=CBR_9600;
    dcb_serial_params.ByteSize=8;
    dcb_serial_params.StopBits=ONESTOPBIT;
    dcb_serial_params.Parity=NOPARITY;
    WIN32_CHECK(SetCommState(hserial,&dcb_serial_params));
    

    COMMTIMEOUTS timeouts={0};
    timeouts.ReadIntervalTimeout=50;
    timeouts.ReadTotalTimeoutConstant=50;
    timeouts.ReadTotalTimeoutMultiplier=10;
    timeouts.WriteTotalTimeoutConstant=50;
    timeouts.WriteTotalTimeoutMultiplier=10;
    WIN32_CHECK(SetCommTimeouts(hserial,&timeouts));
    
    
    WNDCLASS wndclass={0};
    wndclass.style        =CS_OWNDC;
    wndclass.lpfnWndProc  =WndProc;
    wndclass.hInstance    =hInstance;
    wndclass.hCursor      =LoadCursor(NULL,IDC_ARROW);
    wndclass.hbrBackground=(HBRUSH)GetStockObject(WHITE_BRUSH);
    wndclass.lpszClassName="window_class";

    float width=640;
    float height=480;


    WIN32_CHECK(RegisterClass(&wndclass));
    HWND hwnd=CreateWindow(
        "window_class",
        "._____.",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        width,
        height,
        NULL,
        NULL,
        hInstance,
        NULL
    );
    assert(hwnd);
    WIN32_CHECK(ShowWindow(hwnd,iCmdShow));
    

    PIXELFORMATDESCRIPTOR pfd={0};
    pfd.nSize=sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion=1;
    pfd.dwFlags=PFD_DRAW_TO_WINDOW|PFD_SUPPORT_OPENGL|PFD_DOUBLEBUFFER;
    pfd.iPixelType=PFD_TYPE_RGBA;
    pfd.cColorBits=32;
    pfd.cDepthBits=24;
    pfd.cStencilBits=8;
    pfd.cAuxBuffers=0;
    pfd.iLayerType=PFD_MAIN_PLANE;

    HDC hdc=GetDC(hwnd);
    int pf=ChoosePixelFormat(hdc,&pfd);
    assert(pf);
    WIN32_CHECK(SetPixelFormat(hdc,pf,&pfd));
    HGLRC ctx=wglCreateContext(hdc);
    assert(ctx);
    WIN32_CHECK(wglMakeCurrent(hdc,ctx));
    puts(glGetString(GL_VERSION));
    glEnable(GL_DEPTH_TEST);

    // WIN32_CHECK(WriteConsole(GetStdHandle(STD_ERROR_HANDLE),buf,n,0,0));

    bool running=true;
    while(running){
        MSG msg;
        while(PeekMessageA(&msg,0,0,0,PM_REMOVE)){
            if(msg.message==WM_QUIT){
                running=false;
            }else{
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }
        if(!running) break;
        
        static unsigned short buf[256];
        int n=0;
        WIN32_CHECK(ReadFile(hserial,buf,sizeof(buf),&n,NULL));

        RECT rect;
        GetWindowRect(hwnd,&rect);
        width=rect.right-rect.left;
        height=rect.bottom-rect.top;

        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        glViewport(0,0,width,height);
        glPushMatrix();
        // glScalef(height/width,1,1);
        glScalef(1,width/height,1);
        glTranslatef(0,-.1,0);

        glBegin(GL_LINE_STRIP);
        glVertex3f(-1,0,0);
        glVertex3f(1,0,0);
        glEnd();

        glBegin(GL_LINE_STRIP);
        glVertex3f(0,-1,0);
        glVertex3f(0,1,0);
        glEnd();

        glBegin(GL_LINE_STRIP);
        for(int i=0;i<sizeof(buf)/2-1;i++){
            glVertex3f(map(i+0,0,sizeof(buf)/2,-1,1),map(buf[i+0],0,1024,0,1),0);
            glVertex3f(map(i+1,0,sizeof(buf)/2,-1,1),map(buf[i+1],0,1024,0,1),0);
        }
        glEnd();


        glPopMatrix();
        
        assert(glGetError()==GL_NO_ERROR);
        SwapBuffers(hdc);
    }    





    
    WIN32_CHECK(CloseHandle(hserial));

    return 0;
}
