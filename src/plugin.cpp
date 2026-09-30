/* Copyright 2026 Jakub Stulir */


#include <clap/clap.h>
#include <clap/helpers/plugin.hh>
#include <clap/helpers/plugin.hxx>

#include "pitch_expr_to_pitch_bend/information_gui.hpp"
#include "pitch_expr_to_pitch_bend/project_info.hpp"
#include "pitch_expr_to_pitch_bend/translator.hpp"

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <new>


namespace pitch_expr_to_pitch_bend {


constexpr std::uint32_t kMaximumDebugEvents = 256U;
constexpr std::uint32_t kMaximumDebugWarnings = 32U;


char const * const kPluginFeatures[] = {


          CLAP_PLUGIN_FEATURE_NOTE_EFFECT,
          CLAP_PLUGIN_FEATURE_UTILITY,
          nullptr


          };


clap_plugin_descriptor const kPluginDescriptor = {


          CLAP_VERSION,
          kPluginId,
          kPluginName,
          kPluginVendor,
          "",
          "",
          "",
          kPluginVersion,
          kPluginDescription,
          kPluginFeatures


          };


using PluginBase = clap::helpers::Plugin<
          clap::helpers::MisbehaviourHandler::Ignore,
          clap::helpers::CheckingLevel::Minimal>;


class PitchExpressionProbe final : public PluginBase {


public:


          explicit PitchExpressionProbe ( clap_host const * host ) noexcept
                    : PluginBase ( & kPluginDescriptor, host ) {


                    }


protected:


          bool implementsLatency ( ) const noexcept override {


                    return true;


                    }


          std::uint32_t latencyGet ( ) const noexcept override {


                    return 0U;


                    }


          bool implementsGui ( ) const noexcept override {


                    return InformationGui::isAvailable ( );


                    }


          bool guiIsApiSupported ( char const * api, bool isFloating ) noexcept override {


                    return informationGui_ . isApiSupported ( api, isFloating );


                    }


          bool guiGetPreferredApi ( char const ** api,
                                    bool        * isFloating ) noexcept override {


                    return informationGui_ . getPreferredApi ( api, isFloating );


                    }


          bool guiCreate ( char const * api, bool isFloating ) noexcept override {


                    return informationGui_ . create ( api, isFloating );


                    }


          void guiDestroy ( ) noexcept override {


                    informationGui_ . destroy ( );


                    }


          bool guiSetScale ( double scale ) noexcept override {


                    return informationGui_ . setScale ( scale );


                    }


          bool guiGetSize ( std::uint32_t * width,
                            std::uint32_t * height ) noexcept override {


                    return informationGui_ . getSize ( width, height );


                    }


          bool guiCanResize ( ) const noexcept override {


                    return false;


                    }


          bool guiSetParent ( clap_window const * window ) noexcept override {


                    return informationGui_ . setParent ( window );


                    }


          bool guiShow ( ) noexcept override {


                    return informationGui_ . show ( );


                    }


          bool guiHide ( ) noexcept override {


                    return informationGui_ . hide ( );


                    }


          bool implementsNotePorts ( ) const noexcept override {


                    return true;


                    }


          std::uint32_t notePortsCount ( bool isInput ) const noexcept override {


                    ( void ) isInput;


                    return 1U;


                    }


          bool notePortsInfo ( std::uint32_t       index,
                               bool                isInput,
                               clap_note_port_info * info ) const noexcept override {


                    std::uint32_t dialects;


                    if ( ( index != 0U ) || ( info == nullptr ) ) {


                              return false;


                              }


                    std::memset ( info, 0, sizeof ( * info ) );


                    if ( isInput ) {


                              dialects = CLAP_NOTE_DIALECT_CLAP |
                                         CLAP_NOTE_DIALECT_MIDI |
                                         CLAP_NOTE_DIALECT_MIDI_MPE;


                              }
                    else {


                              dialects = CLAP_NOTE_DIALECT_CLAP |
                                         CLAP_NOTE_DIALECT_MIDI |
                                         CLAP_NOTE_DIALECT_MIDI_MPE;


                              }


                    info -> id                = 0U;
                    info -> supported_dialects = dialects;


                    if ( isInput ) {


                              info -> preferred_dialect = CLAP_NOTE_DIALECT_CLAP;


                              }
                    else {


                              info -> preferred_dialect = CLAP_NOTE_DIALECT_CLAP;


                              }


                    if ( isInput ) {


                              std::snprintf ( info -> name,
                                              sizeof ( info -> name ),
                                              "%s",
                                              "CLAP Note Input" );


                              }
                    else {


                              std::snprintf ( info -> name,
                                              sizeof ( info -> name ),
                                              "%s",
                                              "Translated Note Output" );


                              }


                    return true;


                    }


          void reset ( ) noexcept override {


                    translator_ . reset ( );


                    }


          bool implementsState ( ) const noexcept override {


                    return true;


                    }


          bool stateSave ( clap_ostream const * stream ) noexcept override {


                    char const stateTag[8] = { 'P', 'E', '2', 'P', 'B', '0', '1', '\0' };


                    if ( ( stream == nullptr ) || ( stream -> write == nullptr ) ) {


                              return false;


                              }


                    return stream -> write ( stream, stateTag, sizeof ( stateTag ) ) ==
                              static_cast<std::int64_t> ( sizeof ( stateTag ) );


                    }


          bool stateLoad ( clap_istream const * stream ) noexcept override {


                    char stateTag[8] { };


                    if ( ( stream == nullptr ) || ( stream -> read == nullptr ) ) {


                              return false;


                              }


                    if ( stream -> read ( stream, stateTag, sizeof ( stateTag ) ) !=
                         static_cast<std::int64_t> ( sizeof ( stateTag ) ) ) {


                              return false;


                              }


                    return std::memcmp ( stateTag, "PE2PB01", 7U ) == 0;


                    }


          clap_process_status process ( clap_process const * processContext ) noexcept override {


                    clap_input_events const  * inputEvents;
                    OutputTap                  outputTap;
                    std::uint32_t              eventCount;
                    std::uint32_t              eventIndex;
                    TranslationStatistics      statistics;


                    if ( processContext == nullptr ) {


                              return CLAP_PROCESS_ERROR;


                              }


                    inputEvents  = processContext -> in_events;
                    if ( ( inputEvents == nullptr ) ||
                         ( processContext -> out_events == nullptr ) ) {


                              return CLAP_PROCESS_ERROR;


                              }


                    eventCount = inputEvents -> size ( inputEvents );


                    for ( eventIndex = 0U; eventIndex < eventCount; ++ eventIndex ) {


                              clap_event_header const * eventHeader =
                                        inputEvents -> get ( inputEvents, eventIndex );


                              if ( eventHeader == nullptr ) {


                                        continue;


                                        }


                              captureDebugEvent ( eventHeader );


                              }


                    outputTap . interface . ctx      = & outputTap;
                    outputTap . interface . try_push = outputTapTryPush;
                    outputTap . destination          = processContext -> out_events;
                    outputTap . plugin               = this;


                    statistics = translator_ . process ( inputEvents, & outputTap . interface );


                    captureTranslationStatistics ( statistics );


                    return CLAP_PROCESS_CONTINUE;


                    }


private:


          struct OutputTap {


                    clap_output_events        interface;
                    clap_output_events const * destination;
                    PitchExpressionProbe     * plugin;


                    };


          static bool CLAP_ABI outputTapTryPush (
                    clap_output_events const * list,
                    clap_event_header const  * eventHeader ) noexcept {


                    OutputTap * tap;


                    if ( ( list == nullptr ) || ( list -> ctx == nullptr ) ) {


                              return false;


                              }


                    tap = static_cast<OutputTap *> ( list -> ctx );


                    if ( ( tap -> plugin == nullptr ) ||
                         ( tap -> destination == nullptr ) ||
                         ( eventHeader == nullptr ) ) {


                              return false;


                              }


                    bool const accepted = tap -> destination -> try_push (
                              tap -> destination, eventHeader );


                    if ( accepted ) {


                              tap -> plugin -> captureOutputEvent ( eventHeader );


                              }


                    return accepted;


                    }


          void captureDebugEvent ( clap_event_header const * eventHeader ) noexcept {


#if defined(PITCH_EXPR_DEBUG)
                    std::uint32_t const observationIndex =
                              debugEventCount_ . fetch_add ( 1U, std::memory_order_relaxed );


                    if ( observationIndex >= kMaximumDebugEvents ) {


                              return;


                              }


                    if ( ( eventHeader -> space_id == CLAP_CORE_EVENT_SPACE_ID ) &&
                         ( ( eventHeader -> type == CLAP_EVENT_NOTE_ON ) ||
                           ( eventHeader -> type == CLAP_EVENT_NOTE_OFF ) ||
                           ( eventHeader -> type == CLAP_EVENT_NOTE_CHOKE ) ) &&
                         ( eventHeader -> size >= sizeof ( clap_event_note ) ) ) {


                              clap_event_note const * noteEvent =
                                        reinterpret_cast<clap_event_note const *> ( eventHeader );


                              char message[256];


                              std::snprintf (
                                        message,
                                        sizeof ( message ),
                                        "PitchExprToPitchBend01 input #%u: time=%u type=%u "
                                        "note_id=%d port=%d channel=%d key=%d velocity=%.9g",
                                        observationIndex,
                                        eventHeader -> time,
                                        eventHeader -> type,
                                        noteEvent -> note_id,
                                        noteEvent -> port_index,
                                        noteEvent -> channel,
                                        noteEvent -> key,
                                        noteEvent -> velocity );


                              log ( CLAP_LOG_INFO, message );


                              }
                    else if ( ( eventHeader -> space_id == CLAP_CORE_EVENT_SPACE_ID ) &&
                         ( eventHeader -> type == CLAP_EVENT_NOTE_EXPRESSION ) &&
                         ( eventHeader -> size >= sizeof ( clap_event_note_expression ) ) ) {


                              clap_event_note_expression const * expressionEvent =
                                        reinterpret_cast<clap_event_note_expression const *> (
                                                  eventHeader );


                              char message[256];


                              std::snprintf (
                                        message,
                                        sizeof ( message ),
                                        "PitchExprToPitchBend01 input #%u: time=%u type=NOTE_EXPRESSION "
                                        "expression_id=%d note_id=%d port=%d channel=%d key=%d value=%.12g",
                                        observationIndex,
                                        eventHeader -> time,
                                        expressionEvent -> expression_id,
                                        expressionEvent -> note_id,
                                        expressionEvent -> port_index,
                                        expressionEvent -> channel,
                                        expressionEvent -> key,
                                        expressionEvent -> value );


                              log ( CLAP_LOG_INFO, message );


                              }
                    else if ( ( eventHeader -> space_id == CLAP_CORE_EVENT_SPACE_ID ) &&
                              ( eventHeader -> type == CLAP_EVENT_MIDI ) &&
                              ( eventHeader -> size >= sizeof ( clap_event_midi ) ) ) {


                              clap_event_midi const * midiEvent =
                                        reinterpret_cast<clap_event_midi const *> ( eventHeader );


                              char message[224];


                              std::snprintf (
                                        message,
                                        sizeof ( message ),
                                        "PitchExprToPitchBend01 input #%u: time=%u type=MIDI "
                                        "port=%u data=%02X %02X %02X",
                                        observationIndex,
                                        eventHeader -> time,
                                        midiEvent -> port_index,
                                        midiEvent -> data[0],
                                        midiEvent -> data[1],
                                        midiEvent -> data[2] );


                              log ( CLAP_LOG_INFO, message );


                              }
                    else {


                              char message[192];


                              std::snprintf (
                                        message,
                                        sizeof ( message ),
                                        "PitchExprToPitchBend01 input #%u: time=%u space=%u type=%u size=%u flags=%u",
                                        observationIndex,
                                        eventHeader -> time,
                                        eventHeader -> space_id,
                                        eventHeader -> type,
                                        eventHeader -> size,
                                        eventHeader -> flags );


                              log ( CLAP_LOG_INFO, message );


                              }
#else
                    ( void ) eventHeader;
#endif


                    }


          void captureOutputEvent ( clap_event_header const * eventHeader ) noexcept {


#if defined(PITCH_EXPR_DEBUG)
                    std::uint32_t const observationIndex =
                              debugOutputEventCount_ . fetch_add ( 1U, std::memory_order_relaxed );


                    if ( observationIndex >= kMaximumDebugEvents ) {


                              return;


                              }


                    if ( ( eventHeader -> space_id == CLAP_CORE_EVENT_SPACE_ID ) &&
                         ( ( eventHeader -> type == CLAP_EVENT_NOTE_ON ) ||
                           ( eventHeader -> type == CLAP_EVENT_NOTE_OFF ) ) &&
                         ( eventHeader -> size >= sizeof ( clap_event_note ) ) ) {


                              clap_event_note const * noteEvent =
                                        reinterpret_cast<clap_event_note const *> ( eventHeader );
                              char message[256];


                              std::snprintf (
                                        message,
                                        sizeof ( message ),
                                        "PitchExprToPitchBend01 output #%u: time=%u type=%u "
                                        "note_id=%d port=%d channel=%d key=%d velocity=%.9g",
                                        observationIndex,
                                        eventHeader -> time,
                                        eventHeader -> type,
                                        noteEvent -> note_id,
                                        noteEvent -> port_index,
                                        noteEvent -> channel,
                                        noteEvent -> key,
                                        noteEvent -> velocity );


                              log ( CLAP_LOG_INFO, message );


                              }
                    else if ( ( eventHeader -> space_id == CLAP_CORE_EVENT_SPACE_ID ) &&
                              ( eventHeader -> type == CLAP_EVENT_MIDI ) &&
                              ( eventHeader -> size >= sizeof ( clap_event_midi ) ) ) {


                              clap_event_midi const * midiEvent =
                                        reinterpret_cast<clap_event_midi const *> ( eventHeader );
                              char message[224];


                              std::snprintf (
                                        message,
                                        sizeof ( message ),
                                        "PitchExprToPitchBend01 output #%u: time=%u type=MIDI "
                                        "port=%u data=%02X %02X %02X",
                                        observationIndex,
                                        eventHeader -> time,
                                        midiEvent -> port_index,
                                        midiEvent -> data[0],
                                        midiEvent -> data[1],
                                        midiEvent -> data[2] );


                              log ( CLAP_LOG_INFO, message );


                              }
#else
                    ( void ) eventHeader;
#endif


                    }


          void captureTranslationStatistics (
                    TranslationStatistics const & statistics ) noexcept {


#if defined(PITCH_EXPR_DEBUG)
                    char message[192];
                    std::uint32_t warningIndex;


                    if ( ( statistics . outputFailures == 0U ) &&
                         ( statistics . channelExhaustions == 0U ) &&
                         ( statistics . unmatchedNoteOffs == 0U ) ) {


                              return;


                              }


                    warningIndex = debugWarningCount_ . fetch_add (
                              1U, std::memory_order_relaxed );


                    if ( warningIndex >= kMaximumDebugWarnings ) {


                              return;


                              }


                    std::snprintf (
                              message,
                              sizeof ( message ),
                              "PitchExprToPitchBend01 translation warning: output_failures=%u "
                              "channel_exhaustions=%u unmatched_note_offs=%u",
                              statistics . outputFailures,
                              statistics . channelExhaustions,
                              statistics . unmatchedNoteOffs );


                    log ( CLAP_LOG_WARNING, message );
#else
                    ( void ) statistics;
#endif


                    }


          std::atomic<std::uint32_t> debugEventCount_ { 0U };
          std::atomic<std::uint32_t> debugOutputEventCount_ { 0U };
          std::atomic<std::uint32_t> debugWarningCount_ { 0U };
          InformationGui            informationGui_;
          EventTranslator            translator_;


          };


clap_plugin const * createPlugin ( clap_host const * host ) noexcept {


          PitchExpressionProbe * plugin;


          if ( ( host == nullptr ) ||
               ( clap_version_is_compatible ( host -> clap_version ) == false ) ) {


                    return nullptr;


                    }


          plugin = new ( std::nothrow ) PitchExpressionProbe ( host );


          if ( plugin == nullptr ) {


                    return nullptr;


                    }


          return plugin -> clapPlugin ( );


          }


std::uint32_t CLAP_ABI pluginCount ( clap_plugin_factory const * factory ) noexcept {


          ( void ) factory;


          return 1U;


          }


clap_plugin_descriptor const * CLAP_ABI pluginDescriptor (
          clap_plugin_factory const * factory,
          std::uint32_t                index ) noexcept {


          ( void ) factory;


          if ( index != 0U ) {


                    return nullptr;


                    }


          return & kPluginDescriptor;


          }


clap_plugin const * CLAP_ABI pluginCreate ( clap_plugin_factory const * factory,
                                           clap_host const           * host,
                                           char const                * pluginId ) noexcept {


          ( void ) factory;


          if ( ( pluginId == nullptr ) || ( std::strcmp ( pluginId, kPluginId ) != 0 ) ) {


                    return nullptr;


                    }


          return createPlugin ( host );


          }


clap_plugin_factory const kPluginFactory = {


          pluginCount,
          pluginDescriptor,
          pluginCreate


          };


bool CLAP_ABI entryInit ( char const * pluginPath ) noexcept {


          ( void ) pluginPath;


          return true;


          }


void CLAP_ABI entryDeinit ( ) noexcept {


          }


void const * CLAP_ABI entryGetFactory ( char const * factoryId ) noexcept {


          if ( ( factoryId != nullptr ) &&
               ( std::strcmp ( factoryId, CLAP_PLUGIN_FACTORY_ID ) == 0 ) ) {


                    return & kPluginFactory;


                    }


          return nullptr;


          }


} /* namespace pitch_expr_to_pitch_bend */


extern "C" CLAP_EXPORT clap_plugin_entry const clap_entry = {


          CLAP_VERSION,
          pitch_expr_to_pitch_bend::entryInit,
          pitch_expr_to_pitch_bend::entryDeinit,
          pitch_expr_to_pitch_bend::entryGetFactory


          };
