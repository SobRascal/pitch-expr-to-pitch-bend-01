/* Copyright 2026 Jakub Stulir */


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>

#include "pitch_expr_to_pitch_bend/information_gui.hpp"

#include <clap/ext/gui.h>

#include <cstdint>
#include <cstdio>
#include <cstring>


namespace {


bool expect ( bool condition, char const * message ) {


          if ( condition == false ) {


                    std::fprintf ( stderr, "FAILED: %s\n", message );


                    return false;


                    }


          return true;


          }


HWND createHiddenParentWindow ( ) noexcept {


          return CreateWindowExW (
                    0U,
                    L"STATIC",
                    L"PitchExprToPitchBend01 GUI test parent",
                    WS_OVERLAPPED,
                    0,
                    0,
                    900,
                    600,
                    nullptr,
                    nullptr,
                    GetModuleHandleW ( nullptr ),
                    nullptr );


          }


bool testInformationGuiLifecycle ( ) {


          pitch_expr_to_pitch_bend::InformationGui gui;
          char const * preferredApi = nullptr;
          bool         preferredFloating = true;
          std::uint32_t width = 0U;
          std::uint32_t height = 0U;
          HWND          parentWindow = createHiddenParentWindow ( );
          HWND          childWindow;
          RECT          childBounds { };
          clap_window   clapParent { };
          bool          passed = true;


          passed = expect ( parentWindow != nullptr,
                            "hidden Win32 parent window is created" ) && passed;
          passed = expect ( pitch_expr_to_pitch_bend::InformationGui::isAvailable ( ),
                            "Win32 information GUI is available" ) && passed;
          passed = expect ( gui . isApiSupported ( CLAP_WINDOW_API_WIN32, false ),
                            "embedded Win32 API is supported" ) && passed;
          passed = expect ( gui . isApiSupported ( CLAP_WINDOW_API_WIN32, true ) == false,
                            "floating Win32 API is not supported" ) && passed;
          passed = expect ( gui . getPreferredApi ( & preferredApi, & preferredFloating ),
                            "preferred GUI API is reported" ) && passed;
          passed = expect ( ( preferredApi != nullptr ) &&
                            ( std::strcmp ( preferredApi, CLAP_WINDOW_API_WIN32 ) == 0 ),
                            "preferred API is Win32" ) && passed;
          passed = expect ( preferredFloating == false,
                            "preferred GUI is embedded" ) && passed;


          if ( parentWindow == nullptr ) {


                    return false;


                    }


          passed = expect ( gui . create ( CLAP_WINDOW_API_WIN32, false ),
                            "information GUI resources are created" ) && passed;
          passed = expect ( gui . setScale ( 1.25 ),
                            "information GUI accepts host scale before parenting" ) && passed;
          passed = expect ( gui . getSize ( & width, & height ),
                            "information GUI reports its fixed size" ) && passed;
          passed = expect ( ( width == 700U ) && ( height == 425U ),
                            "information GUI size follows physical Win32 scale" ) && passed;


          clapParent . api   = CLAP_WINDOW_API_WIN32;
          clapParent . win32 = parentWindow;


          passed = expect ( gui . setParent ( & clapParent ),
                            "information GUI embeds into the host parent" ) && passed;


          childWindow = GetWindow ( parentWindow, GW_CHILD );


          passed = expect ( childWindow != nullptr,
                            "embedded information child window exists" ) && passed;


          if ( childWindow != nullptr ) {


                    GetClientRect ( childWindow, & childBounds );


                    passed = expect (
                              ( childBounds . right == static_cast<LONG> ( width ) ) &&
                              ( childBounds . bottom == static_cast<LONG> ( height ) ),
                              "embedded child client area matches the CLAP size" ) && passed;


                    }


          passed = expect ( gui . show ( ), "information GUI shows" ) && passed;
          passed = expect ( gui . hide ( ), "information GUI hides" ) && passed;


          gui . destroy ( );


          if ( childWindow != nullptr ) {


                    passed = expect ( IsWindow ( childWindow ) == FALSE,
                                      "destroy removes only the owned child window" ) && passed;


                    }


          gui . destroy ( );
          DestroyWindow ( parentWindow );


          return passed;


          }


} /* namespace */


int main ( ) {


          if ( testInformationGuiLifecycle ( ) == false ) {


                    return 1;


                    }


          std::printf ( "All PitchExprToPitchBend01 information GUI tests passed.\n" );


          return 0;


          }
