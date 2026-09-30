/* Copyright 2026 Jakub Stulir */


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "pitch_expr_to_pitch_bend/information_gui.hpp"
#include "pitch_expr_to_pitch_bend/project_info.hpp"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <new>


namespace pitch_expr_to_pitch_bend {


struct InformationGuiState {


          HWND          window;
          HINSTANCE     moduleInstance;
          ATOM          windowClass;
          HFONT         titleFont;
          HFONT         headingFont;
          HFONT         bodyFont;
          HFONT         smallFont;
          double        scale;
          std::uint32_t width;
          std::uint32_t height;


          };


namespace {


constexpr std::uint32_t kBaseWidth = 560U;
constexpr std::uint32_t kBaseHeight = 340U;
constexpr double        kMaximumScale = 16.0;
constexpr wchar_t       kWindowClassName[] =
          L"VirtueTronic.PitchExprToPitchBend01.Information.0.3.0";


ATOM          informationWindowClass = 0;
HINSTANCE     informationWindowModule = nullptr;
std::uint32_t informationWindowClassUsers = 0U;


LRESULT CALLBACK informationWindowProcedure ( HWND   window,
                                               UINT   message,
                                               WPARAM wordParameter,
                                               LPARAM longParameter );


LPCWSTR windowClassAtomName ( ATOM windowClass ) noexcept {


          return reinterpret_cast<LPCWSTR> (
                    static_cast<ULONG_PTR> ( windowClass ) );


          }


int scaledPixels ( InformationGuiState const & state, int basePixels ) noexcept {


          double const scaled = static_cast<double> ( basePixels ) * state . scale;


          return static_cast<int> ( std::lround ( scaled ) );


          }


bool scaledDimension ( std::uint32_t  baseDimension,
                       double         scale,
                       std::uint32_t & result ) noexcept {


          double scaled;


          if ( ( std::isfinite ( scale ) == false ) ||
               ( scale <= 0.0 ) ||
               ( scale > kMaximumScale ) ) {


                    return false;


                    }


          scaled = static_cast<double> ( baseDimension ) * scale;


          if ( ( scaled < 1.0 ) ||
               ( scaled > static_cast<double> ( UINT32_MAX ) ) ) {


                    return false;


                    }


          result = static_cast<std::uint32_t> ( std::lround ( scaled ) );


          return true;


          }


HINSTANCE findModuleInstance ( ) noexcept {


          HMODULE module = nullptr;
          DWORD const flags = GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                              GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT;


          if ( GetModuleHandleExW (
                     flags,
                     reinterpret_cast<LPCWSTR> ( & informationWindowProcedure ),
                     & module ) == FALSE ) {


                    return nullptr;


                    }


          return static_cast<HINSTANCE> ( module );


          }


bool retainInformationWindowClass ( InformationGuiState & state ) noexcept {


          if ( informationWindowClassUsers == 0U ) {


                    WNDCLASSEXW windowClassDefinition { };


                    informationWindowModule = findModuleInstance ( );


                    if ( informationWindowModule == nullptr ) {


                              return false;


                              }


                    windowClassDefinition . cbSize = sizeof ( windowClassDefinition );
                    windowClassDefinition . style = CS_HREDRAW | CS_VREDRAW;
                    windowClassDefinition . lpfnWndProc = informationWindowProcedure;
                    windowClassDefinition . hInstance = informationWindowModule;
                    windowClassDefinition . hCursor = LoadCursorW (
                              nullptr, MAKEINTRESOURCEW ( 32512 ) );
                    windowClassDefinition . lpszClassName = kWindowClassName;


                    informationWindowClass = RegisterClassExW ( & windowClassDefinition );


                    if ( informationWindowClass == 0 ) {


                              informationWindowModule = nullptr;


                              return false;


                              }


                    }


          ++ informationWindowClassUsers;


          state . moduleInstance = informationWindowModule;
          state . windowClass    = informationWindowClass;


          return true;


          }


void releaseInformationWindowClass ( ) noexcept {


          if ( informationWindowClassUsers == 0U ) {


                    return;


                    }


          -- informationWindowClassUsers;


          if ( informationWindowClassUsers == 0U ) {


                    if ( ( informationWindowClass != 0 ) &&
                         ( informationWindowModule != nullptr ) ) {


                              UnregisterClassW (
                                        windowClassAtomName ( informationWindowClass ),
                                        informationWindowModule );


                              }


                    informationWindowClass  = 0;
                    informationWindowModule = nullptr;


                    }


          }


HFONT createInformationFont ( InformationGuiState const & state,
                              int                         height,
                              int                         weight ) noexcept {


          return CreateFontW (
                    - scaledPixels ( state, height ),
                    0,
                    0,
                    0,
                    weight,
                    FALSE,
                    FALSE,
                    FALSE,
                    DEFAULT_CHARSET,
                    OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS,
                    CLEARTYPE_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE,
                    L"Segoe UI" );


          }


void destroyInformationFonts ( InformationGuiState & state ) noexcept {


          if ( state . titleFont != nullptr ) {


                    DeleteObject ( state . titleFont );
                    state . titleFont = nullptr;


                    }


          if ( state . headingFont != nullptr ) {


                    DeleteObject ( state . headingFont );
                    state . headingFont = nullptr;


                    }


          if ( state . bodyFont != nullptr ) {


                    DeleteObject ( state . bodyFont );
                    state . bodyFont = nullptr;


                    }


          if ( state . smallFont != nullptr ) {


                    DeleteObject ( state . smallFont );
                    state . smallFont = nullptr;


                    }


          }


void createInformationFonts ( InformationGuiState & state ) noexcept {


          destroyInformationFonts ( state );


          state . titleFont   = createInformationFont ( state, 25, FW_SEMIBOLD );
          state . headingFont = createInformationFont ( state, 15, FW_SEMIBOLD );
          state . bodyFont    = createInformationFont ( state, 15, FW_NORMAL );
          state . smallFont   = createInformationFont ( state, 12, FW_SEMIBOLD );


          }


HFONT usableFont ( HFONT requestedFont ) noexcept {


          if ( requestedFont != nullptr ) {


                    return requestedFont;


                    }


          return static_cast<HFONT> ( GetStockObject ( DEFAULT_GUI_FONT ) );


          }


void drawWideText ( HDC           deviceContext,
                    HFONT         font,
                    COLORREF      color,
                    wchar_t const * text,
                    RECT          bounds,
                    UINT          format ) noexcept {


          HFONT const selectedFont = usableFont ( font );
          HGDIOBJ const previousFont = SelectObject ( deviceContext, selectedFont );


          SetBkMode ( deviceContext, TRANSPARENT );
          SetTextColor ( deviceContext, color );
          DrawTextW ( deviceContext, text, -1, & bounds, format | DT_NOPREFIX );


          SelectObject ( deviceContext, previousFont );


          }


void drawUtf8Text ( HDC          deviceContext,
                    HFONT        font,
                    COLORREF     color,
                    char const * text,
                    RECT         bounds,
                    UINT         format ) noexcept {


          wchar_t wideText[256] { };
          int     convertedLength;


          if ( text == nullptr ) {


                    return;


                    }


          convertedLength = MultiByteToWideChar (
                    CP_UTF8,
                    MB_ERR_INVALID_CHARS,
                    text,
                    -1,
                    wideText,
                    static_cast<int> ( sizeof ( wideText ) / sizeof ( wideText[0] ) ) );


          if ( convertedLength <= 0 ) {


                    return;


                    }


          drawWideText ( deviceContext, font, color, wideText, bounds, format );


          }


void drawInformationRow ( HDC                         deviceContext,
                          InformationGuiState const & state,
                          int                         baseY,
                          wchar_t const             * label,
                          char const                * value ) noexcept {


          COLORREF const labelColor = RGB ( 147, 154, 164 );
          COLORREF const valueColor = RGB ( 234, 237, 241 );
          RECT labelBounds {
                    scaledPixels ( state, 30 ),
                    scaledPixels ( state, baseY ),
                    scaledPixels ( state, 150 ),
                    scaledPixels ( state, baseY + 23 )
                    };
          RECT valueBounds {
                    scaledPixels ( state, 160 ),
                    scaledPixels ( state, baseY ),
                    scaledPixels ( state, 530 ),
                    scaledPixels ( state, baseY + 23 )
                    };


          drawWideText ( deviceContext,
                         state . bodyFont,
                         labelColor,
                         label,
                         labelBounds,
                         DT_LEFT | DT_SINGLELINE | DT_VCENTER );
          drawUtf8Text ( deviceContext,
                         state . bodyFont,
                         valueColor,
                         value,
                         valueBounds,
                         DT_LEFT | DT_SINGLELINE | DT_VCENTER );


          }


void paintInformationWindow ( HWND window, InformationGuiState const & state ) noexcept {


          PAINTSTRUCT paint { };
          HDC         deviceContext = BeginPaint ( window, & paint );
          RECT        clientBounds { };
          COLORREF const backgroundColor = RGB ( 27, 29, 33 );
          COLORREF const panelColor = RGB ( 39, 42, 47 );
          COLORREF const orangeColor = RGB ( 236, 137, 33 );
          COLORREF const titleColor = RGB ( 245, 246, 248 );
          COLORREF const secondaryColor = RGB ( 167, 173, 182 );
          HBRUSH backgroundBrush;
          HBRUSH panelBrush;
          HBRUSH orangeBrush;
          HPEN   orangePen;
          HPEN   dividerPen;
          HGDIOBJ previousBrush;
          HGDIOBJ previousPen;


          if ( deviceContext == nullptr ) {


                    return;


                    }


          GetClientRect ( window, & clientBounds );


          backgroundBrush = CreateSolidBrush ( backgroundColor );
          panelBrush      = CreateSolidBrush ( panelColor );
          orangeBrush     = CreateSolidBrush ( orangeColor );
          orangePen       = CreatePen ( PS_SOLID, scaledPixels ( state, 1 ), orangeColor );
          dividerPen      = CreatePen ( PS_SOLID,
                                        scaledPixels ( state, 1 ),
                                        RGB ( 57, 61, 68 ) );


          FillRect ( deviceContext, & clientBounds, backgroundBrush );


          previousBrush = SelectObject ( deviceContext, orangeBrush );
          previousPen   = SelectObject ( deviceContext, orangePen );


          Ellipse ( deviceContext,
                    scaledPixels ( state, 30 ),
                    scaledPixels ( state, 27 ),
                    scaledPixels ( state, 48 ),
                    scaledPixels ( state, 45 ) );


          SelectObject ( deviceContext, previousBrush );
          SelectObject ( deviceContext, previousPen );


          {


                    RECT titleBounds {
                              scaledPixels ( state, 61 ),
                              scaledPixels ( state, 16 ),
                              scaledPixels ( state, 395 ),
                              scaledPixels ( state, 52 )
                              };
                    RECT subtitleBounds {
                              scaledPixels ( state, 30 ),
                              scaledPixels ( state, 56 ),
                              scaledPixels ( state, 520 ),
                              scaledPixels ( state, 79 )
                              };
                    RECT badgeBounds {
                              scaledPixels ( state, 414 ),
                              scaledPixels ( state, 22 ),
                              scaledPixels ( state, 530 ),
                              scaledPixels ( state, 47 )
                              };


                    drawUtf8Text ( deviceContext,
                                   state . titleFont,
                                   titleColor,
                                   kPluginName,
                                   titleBounds,
                                   DT_LEFT | DT_SINGLELINE | DT_VCENTER );
                    drawWideText ( deviceContext,
                                   state . bodyFont,
                                   secondaryColor,
                                   L"CLAP per-note tuning  ->  MPE pitch bend",
                                   subtitleBounds,
                                   DT_LEFT | DT_SINGLELINE | DT_VCENTER );


                    previousBrush = SelectObject ( deviceContext, panelBrush );
                    previousPen   = SelectObject ( deviceContext, orangePen );
                    RoundRect ( deviceContext,
                                badgeBounds . left,
                                badgeBounds . top,
                                badgeBounds . right,
                                badgeBounds . bottom,
                                scaledPixels ( state, 8 ),
                                scaledPixels ( state, 8 ) );
                    SelectObject ( deviceContext, previousBrush );
                    SelectObject ( deviceContext, previousPen );


                    drawWideText ( deviceContext,
                                   state . smallFont,
                                   orangeColor,
                                   L"CLAP NOTE EFFECT",
                                   badgeBounds,
                                   DT_CENTER | DT_SINGLELINE | DT_VCENTER );


                    }


          previousPen = SelectObject ( deviceContext, dividerPen );
          MoveToEx ( deviceContext,
                     scaledPixels ( state, 30 ),
                     scaledPixels ( state, 91 ),
                     nullptr );
          LineTo ( deviceContext,
                   scaledPixels ( state, 530 ),
                   scaledPixels ( state, 91 ) );
          SelectObject ( deviceContext, previousPen );


          drawInformationRow ( deviceContext, state, 107, L"Author", kPluginAuthor );
          drawInformationRow ( deviceContext, state, 135, L"Version", kPluginVersion );
          drawInformationRow ( deviceContext, state, 163, L"Reported latency", "0 samples" );
          drawInformationRow (
                    deviceContext,
                    state,
                    191,
                    L"MPE output",
                    "Channels 2-16; channel 1 reserved" );


          {


                    RECT panelBounds {
                              scaledPixels ( state, 30 ),
                              scaledPixels ( state, 230 ),
                              scaledPixels ( state, 530 ),
                              scaledPixels ( state, 316 )
                              };
                    RECT headingBounds {
                              scaledPixels ( state, 48 ),
                              scaledPixels ( state, 240 ),
                              scaledPixels ( state, 512 ),
                              scaledPixels ( state, 263 )
                              };
                    RECT overviewBounds {
                              scaledPixels ( state, 48 ),
                              scaledPixels ( state, 264 ),
                              scaledPixels ( state, 512 ),
                              scaledPixels ( state, 307 )
                              };


                    FillRect ( deviceContext, & panelBounds, panelBrush );
                    drawWideText ( deviceContext,
                                   state . headingFont,
                                   titleColor,
                                   L"Deterministic microtuning bridge",
                                   headingBounds,
                                   DT_LEFT | DT_SINGLELINE | DT_VCENTER );
                    drawWideText (
                              deviceContext,
                              state . bodyFont,
                              secondaryColor,
                              L"Converts CLAP-native pitch expression to 14-bit MIDI Pitch Bend. "
                              L"Fixed range: +/-24 semitones. OsTIrus Bend Down/Up: -24 / +24.",
                              overviewBounds,
                              DT_LEFT | DT_TOP | DT_WORDBREAK );


                    }


          DeleteObject ( dividerPen );
          DeleteObject ( orangePen );
          DeleteObject ( orangeBrush );
          DeleteObject ( panelBrush );
          DeleteObject ( backgroundBrush );


          EndPaint ( window, & paint );


          }


LRESULT CALLBACK informationWindowProcedure ( HWND   window,
                                               UINT   message,
                                               WPARAM wordParameter,
                                               LPARAM longParameter ) {


          InformationGuiState * state = reinterpret_cast<InformationGuiState *> (
                    GetWindowLongPtrW ( window, GWLP_USERDATA ) );


          ( void ) wordParameter;


          if ( message == WM_NCCREATE ) {


                    CREATESTRUCTW const * createInformation =
                              reinterpret_cast<CREATESTRUCTW const *> ( longParameter );


                    state = static_cast<InformationGuiState *> (
                              createInformation -> lpCreateParams );


                    if ( state == nullptr ) {


                              return FALSE;


                              }


                    state -> window = window;
                    SetWindowLongPtrW (
                              window, GWLP_USERDATA, reinterpret_cast<LONG_PTR> ( state ) );


                    return TRUE;


                    }


          if ( ( message == WM_PAINT ) && ( state != nullptr ) ) {


                    paintInformationWindow ( window, * state );


                    return 0;


                    }


          if ( message == WM_ERASEBKGND ) {


                    return 1;


                    }


          if ( message == WM_NCDESTROY ) {


                    if ( state != nullptr ) {


                              state -> window = nullptr;


                              }


                    SetWindowLongPtrW ( window, GWLP_USERDATA, 0 );


                    }


          return DefWindowProcW ( window, message, wordParameter, longParameter );


          }


} /* namespace */


InformationGui::InformationGui ( ) noexcept
          : state_ ( nullptr ) {


          }


InformationGui::~InformationGui ( ) noexcept {


          destroy ( );


          }


bool InformationGui::isAvailable ( ) noexcept {


          return true;


          }


bool InformationGui::isApiSupported ( char const * api, bool isFloating ) const noexcept {


          if ( ( api == nullptr ) || isFloating ) {


                    return false;


                    }


          return std::strcmp ( api, CLAP_WINDOW_API_WIN32 ) == 0;


          }


bool InformationGui::getPreferredApi ( char const ** api, bool * isFloating ) const noexcept {


          if ( ( api == nullptr ) || ( isFloating == nullptr ) ) {


                    return false;


                    }


          * api        = CLAP_WINDOW_API_WIN32;
          * isFloating = false;


          return true;


          }


bool InformationGui::create ( char const * api, bool isFloating ) noexcept {


          InformationGuiState * state;


          if ( ( state_ != nullptr ) ||
               ( isApiSupported ( api, isFloating ) == false ) ) {


                    return false;


                    }


          state = new ( std::nothrow ) InformationGuiState { };


          if ( state == nullptr ) {


                    return false;


                    }


          state -> scale  = 1.0;
          state -> width  = kBaseWidth;
          state -> height = kBaseHeight;


          if ( retainInformationWindowClass ( * state ) == false ) {


                    delete state;


                    return false;


                    }


          state_ = state;


          return true;


          }


void InformationGui::destroy ( ) noexcept {


          InformationGuiState * state = state_;


          if ( state == nullptr ) {


                    return;


                    }


          state_ = nullptr;


          if ( ( state -> window != nullptr ) && IsWindow ( state -> window ) ) {


                    DestroyWindow ( state -> window );


                    }


          destroyInformationFonts ( * state );
          releaseInformationWindowClass ( );


          delete state;


          }


bool InformationGui::setScale ( double scale ) noexcept {


          std::uint32_t width;
          std::uint32_t height;


          if ( ( state_ == nullptr ) || ( state_ -> window != nullptr ) ) {


                    return false;


                    }


          if ( ( scaledDimension ( kBaseWidth, scale, width ) == false ) ||
               ( scaledDimension ( kBaseHeight, scale, height ) == false ) ) {


                    return false;


                    }


          state_ -> scale  = scale;
          state_ -> width  = width;
          state_ -> height = height;


          return true;


          }


bool InformationGui::getSize ( std::uint32_t * width,
                               std::uint32_t * height ) const noexcept {


          if ( ( state_ == nullptr ) || ( width == nullptr ) || ( height == nullptr ) ) {


                    return false;


                    }


          * width  = state_ -> width;
          * height = state_ -> height;


          return true;


          }


bool InformationGui::setParent ( clap_window const * window ) noexcept {


          HWND parentWindow;


          if ( ( state_ == nullptr ) ||
               ( window == nullptr ) ||
               ( window -> api == nullptr ) ||
               ( std::strcmp ( window -> api, CLAP_WINDOW_API_WIN32 ) != 0 ) ||
               ( window -> win32 == nullptr ) ) {


                    return false;


                    }


          parentWindow = static_cast<HWND> ( window -> win32 );


          if ( IsWindow ( parentWindow ) == FALSE ) {


                    return false;


                    }


          if ( state_ -> window == nullptr ) {


                    createInformationFonts ( * state_ );


                    state_ -> window = CreateWindowExW (
                              0U,
                              windowClassAtomName ( state_ -> windowClass ),
                              L"PitchExprToPitchBend01",
                              WS_CHILD | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
                              0,
                              0,
                              static_cast<int> ( state_ -> width ),
                              static_cast<int> ( state_ -> height ),
                              parentWindow,
                              nullptr,
                              state_ -> moduleInstance,
                              state_ );


                    if ( state_ -> window == nullptr ) {


                              destroyInformationFonts ( * state_ );


                              return false;


                              }


                    }
          else {


                    SetLastError ( ERROR_SUCCESS );


                    if ( ( SetParent ( state_ -> window, parentWindow ) == nullptr ) &&
                         ( GetLastError ( ) != ERROR_SUCCESS ) ) {


                              return false;


                              }


                    SetWindowPos (
                              state_ -> window,
                              nullptr,
                              0,
                              0,
                              static_cast<int> ( state_ -> width ),
                              static_cast<int> ( state_ -> height ),
                              SWP_NOACTIVATE | SWP_NOZORDER );


                    }


          return true;


          }


bool InformationGui::show ( ) noexcept {


          if ( ( state_ == nullptr ) ||
               ( state_ -> window == nullptr ) ||
               ( IsWindow ( state_ -> window ) == FALSE ) ) {


                    return false;


                    }


          ShowWindow ( state_ -> window, SW_SHOWNA );
          InvalidateRect ( state_ -> window, nullptr, FALSE );
          UpdateWindow ( state_ -> window );


          return true;


          }


bool InformationGui::hide ( ) noexcept {


          if ( ( state_ == nullptr ) ||
               ( state_ -> window == nullptr ) ||
               ( IsWindow ( state_ -> window ) == FALSE ) ) {


                    return false;


                    }


          ShowWindow ( state_ -> window, SW_HIDE );


          return true;


          }


} /* namespace pitch_expr_to_pitch_bend */
