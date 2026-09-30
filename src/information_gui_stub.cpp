/* Copyright 2026 Jakub Stulir */


#include "pitch_expr_to_pitch_bend/information_gui.hpp"


namespace pitch_expr_to_pitch_bend {


struct InformationGuiState {


          };


InformationGui::InformationGui ( ) noexcept
          : state_ ( nullptr ) {


          }


InformationGui::~InformationGui ( ) noexcept {


          destroy ( );


          }


bool InformationGui::isAvailable ( ) noexcept {


          return false;


          }


bool InformationGui::isApiSupported ( char const * api, bool isFloating ) const noexcept {


          ( void ) api;
          ( void ) isFloating;


          return false;


          }


bool InformationGui::getPreferredApi ( char const ** api, bool * isFloating ) const noexcept {


          ( void ) api;
          ( void ) isFloating;


          return false;


          }


bool InformationGui::create ( char const * api, bool isFloating ) noexcept {


          ( void ) api;
          ( void ) isFloating;


          return false;


          }


void InformationGui::destroy ( ) noexcept {


          state_ = nullptr;


          }


bool InformationGui::setScale ( double scale ) noexcept {


          ( void ) scale;


          return false;


          }


bool InformationGui::getSize ( std::uint32_t * width,
                               std::uint32_t * height ) const noexcept {


          ( void ) width;
          ( void ) height;


          return false;


          }


bool InformationGui::setParent ( clap_window const * window ) noexcept {


          ( void ) window;


          return false;


          }


bool InformationGui::show ( ) noexcept {


          return false;


          }


bool InformationGui::hide ( ) noexcept {


          return false;


          }


} /* namespace pitch_expr_to_pitch_bend */
