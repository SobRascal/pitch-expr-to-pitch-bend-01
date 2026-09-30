/* Copyright 2026 Jakub Stulir */


#include "pitch_expr_to_pitch_bend/translator.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>


namespace {


constexpr std::size_t kEventStorageSize = 64U;


struct EventStorage {


          alignas ( std::max_align_t ) std::array<std::byte, kEventStorageSize> bytes;


          };


struct InputList {


          clap_input_events                 interface;
          std::vector<clap_event_header *> events;


          };


struct OutputList {


          clap_output_events       interface;
          std::vector<EventStorage> events;
          std::uint32_t             rejectedPushes;


          };


std::uint32_t CLAP_ABI inputSize ( clap_input_events const * list ) noexcept {


          InputList const * input = static_cast<InputList const *> ( list -> ctx );


          return static_cast<std::uint32_t> ( input -> events . size ( ) );


          }


clap_event_header const * CLAP_ABI inputGet ( clap_input_events const * list,
                                              std::uint32_t             index ) noexcept {


          InputList const * input = static_cast<InputList const *> ( list -> ctx );


          if ( index >= input -> events . size ( ) ) {


                    return nullptr;


                    }


          return input -> events[index];


          }


bool CLAP_ABI outputPush ( clap_output_events const * list,
                           clap_event_header const  * event ) noexcept {


          OutputList * output = static_cast<OutputList *> ( list -> ctx );
          EventStorage storage { };


          if ( ( event == nullptr ) || ( event -> size > storage . bytes . size ( ) ) ) {


                    return false;


                    }


          if ( output -> rejectedPushes > 0U ) {


                    -- output -> rejectedPushes;


                    return false;


                    }


          std::memcpy ( storage . bytes . data ( ), event, event -> size );
          output -> events . push_back ( storage );


          return true;


          }


void initializeInput ( InputList & input ) {


          input . interface . ctx  = & input;
          input . interface . size = inputSize;
          input . interface . get  = inputGet;


          }


void initializeOutput ( OutputList & output ) {


          output . interface . ctx      = & output;
          output . interface . try_push = outputPush;


          }


clap_event_note makeNote ( std::uint16_t type,
                           std::uint32_t time,
                           std::int32_t  noteId,
                           std::int16_t  key,
                           double        velocity ) {


          clap_event_note event { };


          event . header . size     = sizeof ( event );
          event . header . time     = time;
          event . header . space_id = CLAP_CORE_EVENT_SPACE_ID;
          event . header . type     = type;
          event . note_id           = noteId;
          event . port_index        = 0;
          event . channel           = 0;
          event . key               = key;
          event . velocity          = velocity;


          return event;


          }


clap_event_note_expression makeTuning ( std::uint32_t time,
                                        std::int32_t  noteId,
                                        std::int16_t  key,
                                        double        semitones ) {


          clap_event_note_expression event { };


          event . header . size     = sizeof ( event );
          event . header . time     = time;
          event . header . space_id = CLAP_CORE_EVENT_SPACE_ID;
          event . header . type     = CLAP_EVENT_NOTE_EXPRESSION;
          event . expression_id     = CLAP_NOTE_EXPRESSION_TUNING;
          event . note_id           = noteId;
          event . port_index        = 0;
          event . channel           = 0;
          event . key               = key;
          event . value             = semitones;


          return event;


          }


clap_event_midi const & midiAt ( OutputList const & output, std::size_t index ) {


          return * reinterpret_cast<clap_event_midi const *> (
                    output . events[index] . bytes . data ( ) );


          }


clap_event_note const & noteAt ( OutputList const & output, std::size_t index ) {


          return * reinterpret_cast<clap_event_note const *> (
                    output . events[index] . bytes . data ( ) );


          }


bool expect ( bool condition, char const * message ) {


          if ( condition == false ) {


                    std::fprintf ( stderr, "FAILED: %s\n", message );


                    return false;


                    }


          return true;


          }


bool testPitchConversion ( ) {


          using pitch_expr_to_pitch_bend::EventTranslator;


          bool passed = true;


          passed = expect ( EventTranslator::pitchOffsetToBend ( -24.0 ) == 0U,
                            "-24 semitones maps to zero" ) && passed;
          passed = expect ( EventTranslator::pitchOffsetToBend ( 0.0 ) == 8192U,
                            "zero semitones maps to center" ) && passed;
          passed = expect ( EventTranslator::pitchOffsetToBend ( 12.0 ) == 12288U,
                            "+12 semitones maps to 12288" ) && passed;
          passed = expect ( EventTranslator::pitchOffsetToBend ( 24.0 ) == 16383U,
                            "+24 semitones clamps to 16383" ) && passed;
          passed = expect ( EventTranslator::pitchOffsetToBend ( 120.0 ) == 16383U,
                            "positive overflow clamps" ) && passed;


          return passed;


          }


bool testInitialOrdering ( ) {


          pitch_expr_to_pitch_bend::EventTranslator translator;
          InputList  input { };
          OutputList output { };
          clap_event_note noteOn = makeNote ( CLAP_EVENT_NOTE_ON, 77U, -1, 38, 1.0 );
          clap_event_note_expression tuning = makeTuning ( 77U, -1, 38, 1.0 );
          bool passed = true;


          initializeInput ( input );
          initializeOutput ( output );
          input . events = { & noteOn . header, & tuning . header };


          translator . process ( & input . interface, & output . interface );


          passed = expect ( output . events . size ( ) == 2U,
                            "initial translation produces bend and note on" ) && passed;


          if ( output . events . size ( ) == 2U ) {


                    clap_event_midi const & bend = midiAt ( output, 0U );
                    clap_event_note const & note = noteAt ( output, 1U );


                    passed = expect ( bend . header . time == 77U,
                                      "initial bend preserves time" ) && passed;
                    passed = expect ( bend . data[0] == 0xE1U,
                                      "first event is bend on MIDI channel 2" ) && passed;
                    passed = expect ( note . header . type == CLAP_EVENT_NOTE_ON,
                                      "second event is CLAP note on" ) && passed;
                    passed = expect ( note . channel == 1,
                                      "CLAP note on uses MIDI channel 2" ) && passed;
                    passed = expect ( note . key == 38,
                                      "note key is preserved" ) && passed;


                    }


          return passed;


          }


bool testPolyphonyReleaseAndReuse ( ) {


          pitch_expr_to_pitch_bend::EventTranslator translator;
          InputList  input { };
          OutputList output { };
          clap_event_note firstOn  = makeNote ( CLAP_EVENT_NOTE_ON, 10U, 100, 60, 0.5 );
          clap_event_note secondOn = makeNote ( CLAP_EVENT_NOTE_ON, 20U, 101, 64, 0.5 );
          clap_event_note firstOff = makeNote ( CLAP_EVENT_NOTE_OFF, 30U, 100, 60, 0.0 );
          clap_event_note thirdOn  = makeNote ( CLAP_EVENT_NOTE_ON, 40U, 102, 67, 0.5 );
          bool passed = true;


          initializeInput ( input );
          initializeOutput ( output );
          input . events = {
                    & firstOn . header,
                    & secondOn . header,
                    & firstOff . header,
                    & thirdOn . header
                    };


          translator . process ( & input . interface, & output . interface );


          passed = expect ( output . events . size ( ) == 7U,
                            "polyphony sequence emits seven translated events" ) && passed;


          if ( output . events . size ( ) == 7U ) {


                    passed = expect ( noteAt ( output, 1U ) . channel == 1,
                                      "first note uses channel 2" ) && passed;
                    passed = expect ( noteAt ( output, 3U ) . channel == 2,
                                      "second note uses channel 3" ) && passed;
                    passed = expect ( noteAt ( output, 4U ) . header . type ==
                                      CLAP_EVENT_NOTE_OFF,
                                      "release emits CLAP note off" ) && passed;
                    passed = expect ( noteAt ( output, 4U ) . channel == 1,
                                      "note off uses original channel" ) && passed;
                    passed = expect ( midiAt ( output, 5U ) . header . time == 40U,
                                      "released channel waits until reuse for its bend" ) && passed;
                    passed = expect ( midiAt ( output, 5U ) . data[0] == 0xE1U,
                                      "reused channel is prepared with pitch bend" ) && passed;
                    passed = expect ( midiAt ( output, 5U ) . data[1] == 0x00U,
                                      "reused channel is prepared at bend center LSB" ) && passed;
                    passed = expect ( midiAt ( output, 5U ) . data[2] == 0x40U,
                                      "reused channel is prepared at bend center MSB" ) && passed;
                    passed = expect ( noteAt ( output, 6U ) . channel == 1,
                                      "released channel is reused" ) && passed;


                    }


          return passed;


          }


bool testHeldExpression ( ) {


          pitch_expr_to_pitch_bend::EventTranslator translator;
          InputList  firstInput { };
          InputList  secondInput { };
          OutputList firstOutput { };
          OutputList secondOutput { };
          clap_event_note noteOn = makeNote ( CLAP_EVENT_NOTE_ON, 11U, 7, 72, 0.8 );
          clap_event_note_expression tuning = makeTuning ( 29U, 7, 72, -0.5 );
          bool passed = true;


          initializeInput ( firstInput );
          initializeInput ( secondInput );
          initializeOutput ( firstOutput );
          initializeOutput ( secondOutput );
          firstInput . events  = { & noteOn . header };
          secondInput . events = { & tuning . header };


          translator . process ( & firstInput . interface, & firstOutput . interface );
          translator . process ( & secondInput . interface, & secondOutput . interface );


          passed = expect ( secondOutput . events . size ( ) == 1U,
                            "held tuning emits one bend" ) && passed;


          if ( secondOutput . events . size ( ) == 1U ) {


                    passed = expect ( midiAt ( secondOutput, 0U ) . data[0] == 0xE1U,
                                      "held tuning stays on assigned channel" ) && passed;
                    passed = expect ( midiAt ( secondOutput, 0U ) . header . time == 29U,
                                      "held tuning preserves timestamp" ) && passed;


                    }


          return passed;


          }


bool testReleasePreservesBendForTail ( ) {


          pitch_expr_to_pitch_bend::EventTranslator translator;
          InputList  noteInput { };
          InputList  releaseInput { };
          InputList  idleExpressionInput { };
          OutputList noteOutput { };
          OutputList releaseOutput { };
          OutputList idleExpressionOutput { };
          clap_event_note noteOn = makeNote ( CLAP_EVENT_NOTE_ON, 11U, 7, 72, 0.8 );
          clap_event_note_expression initialTuning = makeTuning ( 11U, 7, 72, 0.5 );
          clap_event_note noteOff = makeNote ( CLAP_EVENT_NOTE_OFF, 29U, 7, 72, 0.0 );
          clap_event_note_expression idleTuning = makeTuning ( 41U, 7, 72, -0.5 );
          bool passed = true;


          initializeInput ( noteInput );
          initializeInput ( releaseInput );
          initializeInput ( idleExpressionInput );
          initializeOutput ( noteOutput );
          initializeOutput ( releaseOutput );
          initializeOutput ( idleExpressionOutput );
          noteInput . events = { & noteOn . header, & initialTuning . header };
          releaseInput . events = { & noteOff . header };
          idleExpressionInput . events = { & idleTuning . header };


          translator . process ( & noteInput . interface, & noteOutput . interface );
          translator . process ( & releaseInput . interface, & releaseOutput . interface );
          translator . process (
                    & idleExpressionInput . interface,
                    & idleExpressionOutput . interface );


          passed = expect ( releaseOutput . events . size ( ) == 1U,
                            "release emits only note off and preserves idle bend" ) && passed;


          if ( releaseOutput . events . size ( ) == 1U ) {


                    passed = expect ( noteAt ( releaseOutput, 0U ) . header . type ==
                                      CLAP_EVENT_NOTE_OFF,
                                      "release output is the assigned note off" ) && passed;


                    }


          passed = expect ( idleExpressionOutput . events . empty ( ),
                            "expression after release emits no pitch bend" ) && passed;


          return passed;


          }


bool testDuplicateHeldExpressionIsSuppressed ( ) {


          pitch_expr_to_pitch_bend::EventTranslator translator;
          InputList  noteInput { };
          InputList  repeatedInput { };
          InputList  changedInput { };
          OutputList noteOutput { };
          OutputList repeatedOutput { };
          OutputList changedOutput { };
          clap_event_note noteOn = makeNote ( CLAP_EVENT_NOTE_ON, 11U, 7, 72, 0.8 );
          clap_event_note_expression initialTuning = makeTuning ( 11U, 7, 72, 0.5 );
          clap_event_note_expression repeatedTuning = makeTuning ( 0U, 7, 72, 0.5 );
          clap_event_note_expression changedTuning = makeTuning ( 17U, 7, 72, 0.75 );
          bool passed = true;


          initializeInput ( noteInput );
          initializeInput ( repeatedInput );
          initializeInput ( changedInput );
          initializeOutput ( noteOutput );
          initializeOutput ( repeatedOutput );
          initializeOutput ( changedOutput );
          noteInput . events = { & noteOn . header, & initialTuning . header };
          repeatedInput . events = { & repeatedTuning . header };
          changedInput . events = { & changedTuning . header };


          translator . process ( & noteInput . interface, & noteOutput . interface );
          translator . process ( & repeatedInput . interface, & repeatedOutput . interface );
          translator . process ( & changedInput . interface, & changedOutput . interface );


          passed = expect ( repeatedOutput . events . empty ( ),
                            "unchanged held tuning does not repeat pitch bend" ) && passed;
          passed = expect ( changedOutput . events . size ( ) == 1U,
                            "changed held tuning emits one pitch bend" ) && passed;


          return passed;


          }


bool testRejectedNoteOffIsRetried ( ) {


          pitch_expr_to_pitch_bend::EventTranslator translator;
          InputList  noteInput { };
          InputList  releaseInput { };
          InputList  retryInput { };
          OutputList noteOutput { };
          OutputList releaseOutput { };
          OutputList retryOutput { };
          clap_event_note noteOn = makeNote ( CLAP_EVENT_NOTE_ON, 11U, 7, 72, 0.8 );
          clap_event_note noteOff = makeNote ( CLAP_EVENT_NOTE_OFF, 29U, 7, 72, 0.0 );
          pitch_expr_to_pitch_bend::TranslationStatistics releaseStatistics;
          bool passed = true;


          initializeInput ( noteInput );
          initializeInput ( releaseInput );
          initializeInput ( retryInput );
          initializeOutput ( noteOutput );
          initializeOutput ( releaseOutput );
          initializeOutput ( retryOutput );
          noteInput . events = { & noteOn . header };
          releaseInput . events = { & noteOff . header };
          releaseOutput . rejectedPushes = 1U;


          translator . process ( & noteInput . interface, & noteOutput . interface );
          releaseStatistics = translator . process (
                    & releaseInput . interface,
                    & releaseOutput . interface );
          translator . process ( & retryInput . interface, & retryOutput . interface );


          passed = expect ( releaseStatistics . outputFailures == 1U,
                            "rejected note off reports one output failure" ) && passed;
          passed = expect ( releaseOutput . events . empty ( ),
                            "rejected note off is not recorded as output" ) && passed;
          passed = expect ( retryOutput . events . size ( ) == 1U,
                            "pending note off is retried next block" ) && passed;


          if ( retryOutput . events . size ( ) == 1U ) {


                    passed = expect ( noteAt ( retryOutput, 0U ) . header . type ==
                                      CLAP_EVENT_NOTE_OFF,
                                      "retried event is note off" ) && passed;
                    passed = expect ( noteAt ( retryOutput, 0U ) . header . time == 0U,
                                      "pending note off retries at block start" ) && passed;
                    passed = expect ( noteAt ( retryOutput, 0U ) . channel == 1,
                                      "pending note off retains member channel" ) && passed;


                    }


          return passed;


          }


bool testWildcardChokeReleasesAllMatchingNotes ( ) {


          pitch_expr_to_pitch_bend::EventTranslator translator;
          InputList  noteInput { };
          InputList  chokeInput { };
          OutputList noteOutput { };
          OutputList chokeOutput { };
          clap_event_note firstOn = makeNote ( CLAP_EVENT_NOTE_ON, 11U, 7, 60, 0.8 );
          clap_event_note secondOn = makeNote ( CLAP_EVENT_NOTE_ON, 19U, 8, 60, 0.8 );
          clap_event_note choke = makeNote ( CLAP_EVENT_NOTE_CHOKE, 31U, -1, 60, 0.0 );
          bool passed = true;


          initializeInput ( noteInput );
          initializeInput ( chokeInput );
          initializeOutput ( noteOutput );
          initializeOutput ( chokeOutput );
          noteInput . events = { & firstOn . header, & secondOn . header };
          chokeInput . events = { & choke . header };


          translator . process ( & noteInput . interface, & noteOutput . interface );
          translator . process ( & chokeInput . interface, & chokeOutput . interface );


          passed = expect ( chokeOutput . events . size ( ) == 2U,
                            "wildcard choke releases both matching voices" ) && passed;


          if ( chokeOutput . events . size ( ) == 2U ) {


                    passed = expect ( noteAt ( chokeOutput, 0U ) . channel == 1,
                                      "wildcard choke releases member channel 2" ) && passed;
                    passed = expect ( noteAt ( chokeOutput, 1U ) . channel == 2,
                                      "wildcard choke releases member channel 3" ) && passed;


                    }


          return passed;


          }


} /* namespace */


int main ( ) {


          bool passed = true;


          passed = testPitchConversion ( ) && passed;
          passed = testInitialOrdering ( ) && passed;
          passed = testPolyphonyReleaseAndReuse ( ) && passed;
          passed = testHeldExpression ( ) && passed;
          passed = testReleasePreservesBendForTail ( ) && passed;
          passed = testDuplicateHeldExpressionIsSuppressed ( ) && passed;
          passed = testRejectedNoteOffIsRetried ( ) && passed;
          passed = testWildcardChokeReleasesAllMatchingNotes ( ) && passed;


          if ( passed == false ) {


                    return 1;


                    }


          std::printf ( "All PitchExprToPitchBend01 translator tests passed.\n" );


          return 0;


          }
