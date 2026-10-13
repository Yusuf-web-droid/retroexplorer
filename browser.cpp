/*
 * Retro Explorer
 *
 * A small Win32/OLE host for the Internet Explorer WebBrowser control.
 * It deliberately uses only interfaces present on Windows 98.
 */

#define WIN32_LEAN_AND_MEAN
#define WINVER 0x0410
#define _WIN32_WINNT 0x0410

#include <windows.h>
#include <ole2.h>
#include <exdisp.h>
#include <string.h>

#define TOOLBAR_HEIGHT 38
#define URL_BUFFER_SIZE 2048

enum ControlId {
    ID_BACK = 100,
    ID_FORWARD,
    ID_STOP,
    ID_REFRESH,
    ID_HOME,
    ID_ADDRESS,
    ID_GO
};

class BrowserSite : public IOleClientSite,
                    public IOleInPlaceSite,
                    public IOleInPlaceFrame {
public:
    BrowserSite() : m_references(1), m_window(NULL), m_object(NULL) {}

    void SetWindow(HWND window) { m_window = window; }
    void SetObject(IOleObject *object) { m_object = object; }

    void Resize() {
        RECT area;
        IOleInPlaceObject *inPlaceObject = NULL;

        if (m_window == NULL || m_object == NULL)
            return;

        GetClientRect(m_window, &area);
        area.top = TOOLBAR_HEIGHT;
        if (area.bottom < area.top)
            area.bottom = area.top;

        if (SUCCEEDED(m_object->QueryInterface(IID_IOleInPlaceObject,
                                                (void **)&inPlaceObject))) {
            inPlaceObject->SetObjectRects(&area, &area);
            inPlaceObject->Release();
        }
    }

    /* IUnknown */
    STDMETHODIMP QueryInterface(REFIID riid, void **object) {
        if (object == NULL)
            return E_POINTER;

        *object = NULL;
        if (IsEqualIID(riid, IID_IUnknown) ||
            IsEqualIID(riid, IID_IOleClientSite)) {
            *object = static_cast<IOleClientSite *>(this);
        } else if (IsEqualIID(riid, IID_IOleInPlaceSite)) {
            *object = static_cast<IOleInPlaceSite *>(this);
        } else if (IsEqualIID(riid, IID_IOleInPlaceFrame)) {
            *object = static_cast<IOleInPlaceFrame *>(this);
        } else {
            return E_NOINTERFACE;
        }

        AddRef();
        return S_OK;
    }

    STDMETHODIMP_(ULONG) AddRef() { return ++m_references; }

    /* This object lives for the lifetime of the application. */
    STDMETHODIMP_(ULONG) Release() {
        if (m_references > 1)
            --m_references;
        return m_references;
    }

    /* IOleClientSite */
    STDMETHODIMP SaveObject() { return E_NOTIMPL; }
    STDMETHODIMP GetMoniker(DWORD, DWORD, IMoniker **) { return E_NOTIMPL; }
    STDMETHODIMP GetContainer(IOleContainer **container) {
        if (container != NULL)
            *container = NULL;
        return E_NOINTERFACE;
    }
    STDMETHODIMP ShowObject() { return S_OK; }
    STDMETHODIMP OnShowWindow(BOOL) { return S_OK; }
    STDMETHODIMP RequestNewObjectLayout() { return E_NOTIMPL; }

    /* IOleWindow */
    STDMETHODIMP GetWindow(HWND *window) {
        if (window == NULL)
            return E_POINTER;
        *window = m_window;
        return S_OK;
    }
    STDMETHODIMP ContextSensitiveHelp(BOOL) { return E_NOTIMPL; }

    /* IOleInPlaceSite */
    STDMETHODIMP CanInPlaceActivate() { return S_OK; }
    STDMETHODIMP OnInPlaceActivate() { return S_OK; }
    STDMETHODIMP OnUIActivate() { return S_OK; }
    STDMETHODIMP GetWindowContext(IOleInPlaceFrame **frame,
                                  IOleInPlaceUIWindow **document,
                                  LPRECT position,
                                  LPRECT clip,
                                  LPOLEINPLACEFRAMEINFO frameInfo) {
        RECT client;

        if (frame == NULL || document == NULL || position == NULL ||
            clip == NULL || frameInfo == NULL)
            return E_POINTER;

        GetClientRect(m_window, &client);
        client.top = TOOLBAR_HEIGHT;
        if (client.bottom < client.top)
            client.bottom = client.top;

        *frame = static_cast<IOleInPlaceFrame *>(this);
        (*frame)->AddRef();
        *document = NULL;
        *position = client;
        *clip = client;

        ZeroMemory(frameInfo, sizeof(*frameInfo));
        frameInfo->cb = sizeof(*frameInfo);
        frameInfo->fMDIApp = FALSE;
        frameInfo->hwndFrame = m_window;
        return S_OK;
    }
    STDMETHODIMP Scroll(SIZE) { return E_NOTIMPL; }
    STDMETHODIMP OnUIDeactivate(BOOL) { return S_OK; }
    STDMETHODIMP OnInPlaceDeactivate() { return S_OK; }
    STDMETHODIMP DiscardUndoState() { return S_OK; }
    STDMETHODIMP DeactivateAndUndo() { return S_OK; }
    STDMETHODIMP OnPosRectChange(LPCRECT rect) {
        IOleInPlaceObject *inPlaceObject = NULL;

        if (rect == NULL || m_object == NULL)
            return E_INVALIDARG;

        if (SUCCEEDED(m_object->QueryInterface(IID_IOleInPlaceObject,
                                                (void **)&inPlaceObject))) {
            inPlaceObject->SetObjectRects(rect, rect);
            inPlaceObject->Release();
        }
        return S_OK;
    }

    /* IOleInPlaceUIWindow */
    STDMETHODIMP GetBorder(LPRECT) { return INPLACE_E_NOTOOLSPACE; }
    STDMETHODIMP RequestBorderSpace(LPCBORDERWIDTHS) {
        return INPLACE_E_NOTOOLSPACE;
    }
    STDMETHODIMP SetBorderSpace(LPCBORDERWIDTHS) { return S_OK; }
    STDMETHODIMP SetActiveObject(IOleInPlaceActiveObject *, LPCOLESTR) {
        return S_OK;
    }

    /* IOleInPlaceFrame */
    STDMETHODIMP InsertMenus(HMENU, LPOLEMENUGROUPWIDTHS) { return S_OK; }
    STDMETHODIMP SetMenu(HMENU, HOLEMENU, HWND) { return S_OK; }
    STDMETHODIMP RemoveMenus(HMENU) { return S_OK; }
    STDMETHODIMP SetStatusText(LPCOLESTR) { return S_OK; }
    STDMETHODIMP EnableModeless(BOOL) { return S_OK; }
    STDMETHODIMP TranslateAccelerator(LPMSG, WORD) { return S_FALSE; }

private:
    ULONG m_references;
    HWND m_window;
    IOleObject *m_object;
};

static HINSTANCE g_instance = NULL;
static HWND g_addressBox = NULL;
static HWND g_controls[6] = { NULL, NULL, NULL, NULL, NULL, NULL };
static WNDPROC g_oldAddressProcedure = NULL;
static BrowserSite g_site;
static IOleObject *g_oleObject = NULL;
static IWebBrowser2 *g_browser = NULL;

static void ShowComError(HWND owner, const char *action, HRESULT result) {
    char message[256];
    wsprintfA(message, "%s failed (0x%08lX).\n\nInstall Internet Explorer 4 or later, then try again.",
              action, (unsigned long)result);
    MessageBoxA(owner, message, "Retro Explorer", MB_OK | MB_ICONERROR);
}

static void SetAddressText(const char *text) {
    if (g_addressBox != NULL)
        SetWindowTextA(g_addressBox, text);
}

static void NavigateTo(const char *requestedUrl) {
    char url[URL_BUFFER_SIZE];
    WCHAR wideUrl[URL_BUFFER_SIZE];
    BSTR target;
    VARIANT empty;

    if (g_browser == NULL || requestedUrl == NULL || requestedUrl[0] == '\0')
        return;

    lstrcpynA(url, requestedUrl, sizeof(url));
    if (strchr(url, ':') == NULL) {
        char prefixed[URL_BUFFER_SIZE];
        lstrcpynA(prefixed, "http://", sizeof(prefixed));
        lstrcpynA(prefixed + 7, url, sizeof(prefixed) - 7);
        lstrcpynA(url, prefixed, sizeof(url));
    }

    if (MultiByteToWideChar(CP_ACP, 0, url, -1, wideUrl,
                            sizeof(wideUrl) / sizeof(wideUrl[0])) == 0)
        return;

    target = SysAllocString(wideUrl);
    if (target == NULL)
        return;

    VariantInit(&empty);
    g_browser->Navigate(target, &empty, &empty, &empty, &empty);
    SysFreeString(target);
}

static void NavigateFromAddressBox() {
    char url[URL_BUFFER_SIZE];
    GetWindowTextA(g_addressBox, url, sizeof(url));
    NavigateTo(url);
}

static void LayoutControls(HWND window) {
    RECT client;
    int x = 4;
    int addressWidth;
    int index;

    GetClientRect(window, &client);
    for (index = 0; index < 5; ++index) {
        MoveWindow(g_controls[index], x, 8, 51, 22, TRUE);
        x += 53;
    }

    addressWidth = client.right - x - 52;
    if (addressWidth < 24)
        addressWidth = 24;
    MoveWindow(g_addressBox, x, 8, addressWidth, 22, TRUE);
    MoveWindow(g_controls[5], x + addressWidth + 3, 8, 45, 22, TRUE);
    g_site.Resize();
}

static HRESULT CreateBrowser(HWND window) {
    ILockBytes *lockBytes = NULL;
    IStorage *storage = NULL;
    RECT client;
    HRESULT result;

    result = CreateILockBytesOnHGlobal(NULL, TRUE, &lockBytes);
    if (FAILED(result))
        return result;

    result = StgCreateDocfileOnILockBytes(lockBytes,
                                           STGM_CREATE | STGM_READWRITE |
                                               STGM_SHARE_EXCLUSIVE,
                                           0, &storage);
    if (SUCCEEDED(result)) {
        result = OleCreate(CLSID_WebBrowser, IID_IOleObject, OLERENDER_DRAW,
                           NULL, static_cast<IOleClientSite *>(&g_site),
                           storage, (void **)&g_oleObject);
    }
    if (storage != NULL)
        storage->Release();
    lockBytes->Release();

    if (FAILED(result))
        return result;

    g_site.SetObject(g_oleObject);
    OleSetContainedObject(g_oleObject, TRUE);
    g_oleObject->SetHostNames(OLESTR("Retro Explorer"), NULL);

    GetClientRect(window, &client);
    client.top = TOOLBAR_HEIGHT;
    result = g_oleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL,
                                 static_cast<IOleClientSite *>(&g_site), 0,
                                 window, &client);
    if (FAILED(result))
        return result;

    return g_oleObject->QueryInterface(IID_IWebBrowser2, (void **)&g_browser);
}

static void DestroyBrowser() {
    if (g_browser != NULL) {
        g_browser->Release();
        g_browser = NULL;
    }
    if (g_oleObject != NULL) {
        g_oleObject->Close(OLECLOSE_NOSAVE);
        g_oleObject->SetClientSite(NULL);
        g_oleObject->Release();
        g_oleObject = NULL;
    }
    g_site.SetObject(NULL);
}

static LRESULT CALLBACK AddressProcedure(HWND window, UINT message,
                                         WPARAM wParam, LPARAM lParam) {
    if (message == WM_KEYDOWN && wParam == VK_RETURN) {
        NavigateFromAddressBox();
        return 0;
    }
    return CallWindowProc(g_oldAddressProcedure, window, message, wParam,
                          lParam);
}

static HWND CreateButton(HWND parent, const char *caption, int controlId) {
    return CreateWindowExA(0, "BUTTON", caption,
                           WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
                           0, 0, 0, 0, parent, (HMENU)controlId, g_instance,
                           NULL);
}

static LRESULT CALLBACK MainProcedure(HWND window, UINT message,
                                      WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
    {
        HRESULT browserResult;
        g_controls[0] = CreateButton(window, "Back", ID_BACK);
        g_controls[1] = CreateButton(window, "Next", ID_FORWARD);
        g_controls[2] = CreateButton(window, "Stop", ID_STOP);
        g_controls[3] = CreateButton(window, "Reload", ID_REFRESH);
        g_controls[4] = CreateButton(window, "Home", ID_HOME);
        g_addressBox = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "about:blank",
                                       WS_CHILD | WS_VISIBLE | WS_TABSTOP |
                                           ES_AUTOHSCROLL,
                                       0, 0, 0, 0, window,
                                       (HMENU)ID_ADDRESS, g_instance, NULL);
#if defined(_WIN64)
        /* Used only for a modern 64-bit Wine preview build. */
        g_oldAddressProcedure = (WNDPROC)SetWindowLongPtrA(
            g_addressBox, GWLP_WNDPROC, (LONG_PTR)AddressProcedure);
#else
        /* SetWindowLongA is the Windows 98-compatible API. */
        g_oldAddressProcedure = (WNDPROC)SetWindowLongA(
            g_addressBox, GWL_WNDPROC, (LONG)AddressProcedure);
#endif
        g_controls[5] = CreateButton(window, "Go", ID_GO);

        g_site.SetWindow(window);
        browserResult = CreateBrowser(window);
        if (FAILED(browserResult)) {
            ShowComError(window, "The browser control could not start", browserResult);
            PostMessage(window, WM_CLOSE, 0, 0);
        }
        LayoutControls(window);
        return 0;
    }

    case WM_SIZE:
        LayoutControls(window);
        return 0;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case ID_BACK:
            if (g_browser != NULL)
                g_browser->GoBack();
            return 0;
        case ID_FORWARD:
            if (g_browser != NULL)
                g_browser->GoForward();
            return 0;
        case ID_STOP:
            if (g_browser != NULL)
                g_browser->Stop();
            return 0;
        case ID_REFRESH:
            if (g_browser != NULL)
                g_browser->Refresh();
            return 0;
        case ID_HOME:
            SetAddressText("http://example.com/");
            NavigateFromAddressBox();
            return 0;
        case ID_GO:
            NavigateFromAddressBox();
            return 0;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProc(window, message, wParam, lParam);
}

int APIENTRY WinMain(HINSTANCE instance, HINSTANCE, LPSTR commandLine, int show) {
    WNDCLASSA windowClass;
    HWND window;
    MSG message;
    HRESULT result;

    ZeroMemory(&message, sizeof(message));
    g_instance = instance;
    result = OleInitialize(NULL);
    if (FAILED(result)) {
        ShowComError(NULL, "OLE initialization", result);
        return 1;
    }

    ZeroMemory(&windowClass, sizeof(windowClass));
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = MainProcedure;
    windowClass.hInstance = instance;
    windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    windowClass.lpszClassName = "Win98BrowserWindow";
    RegisterClassA(&windowClass);

    window = CreateWindowExA(0, windowClass.lpszClassName, "Retro Explorer",
                             WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                             CW_USEDEFAULT, CW_USEDEFAULT, 820, 600, NULL,
                             NULL, instance, NULL);
    if (window == NULL) {
        OleUninitialize();
        return 1;
    }

    ShowWindow(window, show);
    UpdateWindow(window);

    if (commandLine != NULL && commandLine[0] != '\0') {
        SetAddressText(commandLine);
        NavigateTo(commandLine);
    } else {
        NavigateTo("about:blank");
    }

    while (GetMessage(&message, NULL, 0, 0)) {
        TranslateMessage(&message);
        DispatchMessage(&message);
    }

    DestroyBrowser();
    OleUninitialize();
    return (int)message.wParam;
}
