/* Copyright 2026 Jakub Stulir */

#ifndef PITCH_EXPR_TO_PITCH_BEND_INFORMATION_GUI_HPP
#define PITCH_EXPR_TO_PITCH_BEND_INFORMATION_GUI_HPP


#include <clap/ext/gui.h>

#include <cstdint>


namespace pitch_expr_to_pitch_bend {


struct InformationGuiState;


class InformationGui final {


public:


          InformationGui ( ) noexcept;


          ~InformationGui ( ) noexcept;


          InformationGui ( InformationGui const & ) = delete;
          InformationGui & operator= ( InformationGui const & ) = delete;


          static bool isAvailable ( ) noexcept;


          bool isApiSupported ( char const * api, bool isFloating ) const noexcept;


          bool getPreferredApi ( char const ** api, bool * isFloating ) const noexcept;


          bool create ( char const * api, bool isFloating ) noexcept;


          void destroy ( ) noexcept;


          bool setScale ( double scale ) noexcept;


          bool getSize ( std::uint32_t * width, std::uint32_t * height ) const noexcept;


          bool setParent ( clap_window const * window ) noexcept;


          bool show ( ) noexcept;


          bool hide ( ) noexcept;


private:


          InformationGuiState * state_;


          };


} /* namespace pitch_expr_to_pitch_bend */


#endif /* PITCH_EXPR_TO_PITCH_BEND_INFORMATION_GUI_HPP */
