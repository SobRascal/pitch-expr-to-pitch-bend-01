/* Copyright 2026 Jakub Stulir */


#include "pitch_expr_to_pitch_bend/translator.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>


namespace pitch_expr_to_pitch_bend {


EventTranslator::EventTranslator ( ) noexcept
          : activeNotes_ { },
            nextAllocationOrder_ ( 1U ),
            processSequence_ ( 0U ) {


          reset ( );


          }


void EventTranslator::reset ( ) noexcept {


          std::size_t index;


          for ( index = 0U; index < activeNotes_ . size ( ); ++ index ) {


                    ActiveNote & note = activeNotes_[index];


                    note . active          = false;
                    note . identity        = { -1, -1, -1, -1 };
                    note . memberChannel   = static_cast<std::int16_t> (
                              kFirstMpeMemberChannel + static_cast<std::int16_t> ( index ) );
                    note . bend            = kPitchBendCenter;
                    note . releasePending  = false;
                    note . pendingNoteOff  = { };
                    note . allocationOrder = 0U;
                    note . processSequence = 0U;
                    note . startTime       = 0U;


                    }


          nextAllocationOrder_ = 1U;
          processSequence_     = 0U;


          }


TranslationStatistics EventTranslator::process (
          clap_input_events const  * inputEvents,
          clap_output_events const * outputEvents ) noexcept {


          TranslationStatistics statistics { 0U, 0U, 0U };
          std::uint32_t          eventCount;
          std::uint32_t          eventIndex;


          ++ processSequence_;


          if ( ( inputEvents == nullptr ) || ( outputEvents == nullptr ) ) {


                    ++ statistics . outputFailures;


                    return statistics;


                    }


          eventCount = inputEvents -> size ( inputEvents );


          retryPendingNoteOffs ( * outputEvents, statistics );


          for ( eventIndex = 0U; eventIndex < eventCount; ++ eventIndex ) {


                    clap_event_header const * eventHeader =
                              inputEvents -> get ( inputEvents, eventIndex );


                    if ( eventHeader == nullptr ) {


                              continue;


                              }


                    if ( eventHeader -> space_id != CLAP_CORE_EVENT_SPACE_ID ) {


                              if ( outputEvents -> try_push ( outputEvents, eventHeader ) == false ) {


                                        ++ statistics . outputFailures;


                                        }


                              continue;


                              }


                    if ( ( eventHeader -> type == CLAP_EVENT_NOTE_ON ) &&
                         ( eventHeader -> size >= sizeof ( clap_event_note ) ) ) {


                              clap_event_note const & noteEvent =
                                        * reinterpret_cast<clap_event_note const *> ( eventHeader );


                              processNoteOn ( * inputEvents,
                                              * outputEvents,
                                              eventCount,
                                              eventIndex,
                                              noteEvent,
                                              statistics );


                              }
                    else if ( ( eventHeader -> type == CLAP_EVENT_NOTE_OFF ) &&
                              ( eventHeader -> size >= sizeof ( clap_event_note ) ) ) {


                              clap_event_note const & noteEvent =
                                        * reinterpret_cast<clap_event_note const *> ( eventHeader );


                              processNoteOff ( * outputEvents, noteEvent, statistics );


                              }
                    else if ( ( eventHeader -> type == CLAP_EVENT_NOTE_CHOKE ) &&
                              ( eventHeader -> size >= sizeof ( clap_event_note ) ) ) {


                              clap_event_note const & noteEvent =
                                        * reinterpret_cast<clap_event_note const *> ( eventHeader );


                              processNoteChoke ( * outputEvents, noteEvent, statistics );


                              }
                    else if ( ( eventHeader -> type == CLAP_EVENT_NOTE_EXPRESSION ) &&
                              ( eventHeader -> size >= sizeof ( clap_event_note_expression ) ) ) {


                              clap_event_note_expression const & expressionEvent =
                                        * reinterpret_cast<clap_event_note_expression const *> (
                                                  eventHeader );


                              if ( expressionEvent . expression_id ==
                                   CLAP_NOTE_EXPRESSION_TUNING ) {


                                        processTuningExpression (
                                                  * outputEvents,
                                                  expressionEvent,
                                                  statistics );


                                        }
                              else if ( outputEvents -> try_push ( outputEvents, eventHeader ) ==
                                        false ) {


                                        ++ statistics . outputFailures;


                                        }


                              }
                    else if ( eventHeader -> type == CLAP_EVENT_NOTE_END ) {


                              /* NOTE_END is host/voice lifecycle information, not MIDI output. */


                              }
                    else if ( outputEvents -> try_push ( outputEvents, eventHeader ) == false ) {


                              ++ statistics . outputFailures;


                              }


                    }


          return statistics;


          }


std::uint16_t EventTranslator::pitchOffsetToBend ( double pitchOffsetSemitones ) noexcept {


          double normalized;
          double unboundedBend;
          long   roundedBend;


          if ( std::isfinite ( pitchOffsetSemitones ) == false ) {


                    return kPitchBendCenter;


                    }


          normalized = pitchOffsetSemitones / kPitchBendRangeSemitones;
          normalized = std::max ( -1.0, std::min ( 1.0, normalized ) );
          unboundedBend = 8192.0 + ( normalized * 8192.0 );
          roundedBend = std::lround ( unboundedBend );
          roundedBend = std::max ( 0L, std::min ( 16383L, roundedBend ) );


          return static_cast<std::uint16_t> ( roundedBend );


          }


bool EventTranslator::identityMatches ( NoteIdentity const & activeIdentity,
                                        std::int32_t          eventNoteId,
                                        std::int16_t          eventPortIndex,
                                        std::int16_t          eventChannel,
                                        std::int16_t          eventKey ) noexcept {


          bool matches = true;


          if ( ( eventNoteId >= 0 ) && ( activeIdentity . noteId != eventNoteId ) ) {


                    matches = false;


                    }


          if ( ( eventPortIndex >= 0 ) &&
               ( activeIdentity . portIndex != eventPortIndex ) ) {


                    matches = false;


                    }


          if ( ( eventChannel >= 0 ) && ( activeIdentity . channel != eventChannel ) ) {


                    matches = false;


                    }


          if ( ( eventKey >= 0 ) && ( activeIdentity . key != eventKey ) ) {


                    matches = false;


                    }


          return matches;


          }


EventTranslator::ActiveNote * EventTranslator::allocateNote (
          clap_event_note const & noteEvent ) noexcept {


          ActiveNote * allocatedNote = nullptr;
          std::size_t  index;


          for ( index = 0U; index < activeNotes_ . size ( ); ++ index ) {


                    ActiveNote & note = activeNotes_[index];


                    if ( note . active == false ) {


                              note . active = true;
                              note . identity = {
                                        noteEvent . note_id,
                                        noteEvent . port_index,
                                        noteEvent . channel,
                                        noteEvent . key
                                        };
                              note . bend            = kPitchBendCenter;
                              note . releasePending  = false;
                              note . pendingNoteOff  = { };
                              note . allocationOrder = nextAllocationOrder_;
                              note . processSequence = processSequence_;
                              note . startTime       = noteEvent . header . time;


                              ++ nextAllocationOrder_;
                              allocatedNote = & note;


                              break;


                              }


                    }


          return allocatedNote;


          }


EventTranslator::ActiveNote * EventTranslator::findOldestMatchingNote (
          clap_event_note const & noteEvent ) noexcept {


          ActiveNote * matchingNote = nullptr;
          std::size_t  index;


          for ( index = 0U; index < activeNotes_ . size ( ); ++ index ) {


                    ActiveNote & note = activeNotes_[index];


                    if ( ( note . active == true ) &&
                         identityMatches ( note . identity,
                                           noteEvent . note_id,
                                           noteEvent . port_index,
                                           noteEvent . channel,
                                           noteEvent . key ) ) {


                              if ( ( matchingNote == nullptr ) ||
                                   ( note . allocationOrder < matchingNote -> allocationOrder ) ) {


                                        matchingNote = & note;


                                        }


                              }


                    }


          return matchingNote;


          }


clap_event_note_expression const * EventTranslator::findInitialTuning (
          clap_input_events const & inputEvents,
          std::uint32_t             eventCount,
          std::uint32_t             noteOnIndex,
          clap_event_note const   & noteEvent ) const noexcept {


          clap_event_note_expression const * tuningEvent = nullptr;
          std::uint32_t                       eventIndex;


          for ( eventIndex = noteOnIndex + 1U;
                eventIndex < eventCount;
                ++ eventIndex ) {


                    clap_event_header const * eventHeader =
                              inputEvents . get ( & inputEvents, eventIndex );


                    if ( eventHeader == nullptr ) {


                              continue;


                              }


                    if ( eventHeader -> time < noteEvent . header . time ) {


                              continue;


                              }


                    if ( eventHeader -> time > noteEvent . header . time ) {


                              break;


                              }


                    if ( ( eventHeader -> space_id == CLAP_CORE_EVENT_SPACE_ID ) &&
                         ( eventHeader -> type == CLAP_EVENT_NOTE_EXPRESSION ) &&
                         ( eventHeader -> size >= sizeof ( clap_event_note_expression ) ) ) {


                              clap_event_note_expression const * candidate =
                                        reinterpret_cast<clap_event_note_expression const *> (
                                                  eventHeader );


                              if ( ( candidate -> expression_id ==
                                     CLAP_NOTE_EXPRESSION_TUNING ) &&
                                   identityMatches ( { noteEvent . note_id,
                                                       noteEvent . port_index,
                                                       noteEvent . channel,
                                                       noteEvent . key },
                                                     candidate -> note_id,
                                                     candidate -> port_index,
                                                     candidate -> channel,
                                                     candidate -> key ) ) {


                                        tuningEvent = candidate;


                                        }


                              }


                    }


          return tuningEvent;


          }


bool EventTranslator::emitMidi ( clap_output_events const & outputEvents,
                                 std::uint32_t              time,
                                 std::uint8_t               status,
                                 std::uint8_t               data1,
                                 std::uint8_t               data2 ) noexcept {


          clap_event_midi midiEvent { };


          midiEvent . header . size     = sizeof ( midiEvent );
          midiEvent . header . time     = time;
          midiEvent . header . space_id = CLAP_CORE_EVENT_SPACE_ID;
          midiEvent . header . type     = CLAP_EVENT_MIDI;
          midiEvent . header . flags    = 0U;
          midiEvent . port_index        = 0U;
          midiEvent . data[0]           = status;
          midiEvent . data[1]           = data1;
          midiEvent . data[2]           = data2;


          return outputEvents . try_push ( & outputEvents, & midiEvent . header );


          }


bool EventTranslator::emitPitchBend ( clap_output_events const & outputEvents,
                                      std::uint32_t              time,
                                      std::int16_t               memberChannel,
                                      std::uint16_t              bend ) noexcept {


          std::uint8_t const status = static_cast<std::uint8_t> (
                    0xE0U | static_cast<std::uint8_t> ( memberChannel ) );
          std::uint8_t const leastSignificant = static_cast<std::uint8_t> ( bend & 0x7FU );
          std::uint8_t const mostSignificant = static_cast<std::uint8_t> (
                    ( bend >> 7U ) & 0x7FU );


          return emitMidi ( outputEvents, time, status, leastSignificant, mostSignificant );


          }


bool EventTranslator::emitNoteOn ( clap_output_events const & outputEvents,
                                   clap_event_note const    & noteEvent,
                                   std::int16_t               memberChannel ) noexcept {


          clap_event_note outputNote = noteEvent;


          outputNote . header . size     = sizeof ( outputNote );
          outputNote . header . space_id = CLAP_CORE_EVENT_SPACE_ID;
          outputNote . header . type     = CLAP_EVENT_NOTE_ON;
          outputNote . port_index        = 0;
          outputNote . channel           = memberChannel;


          return outputEvents . try_push ( & outputEvents, & outputNote . header );


          }


bool EventTranslator::emitNoteOff ( clap_output_events const & outputEvents,
                                    clap_event_note const    & noteEvent,
                                    NoteIdentity const       & activeIdentity,
                                    std::int16_t               memberChannel ) noexcept {


          clap_event_note outputNote = noteEvent;


          outputNote . header . size     = sizeof ( outputNote );
          outputNote . header . space_id = CLAP_CORE_EVENT_SPACE_ID;
          outputNote . header . type     = CLAP_EVENT_NOTE_OFF;
          outputNote . note_id           = activeIdentity . noteId;
          outputNote . port_index        = 0;
          outputNote . channel           = memberChannel;
          outputNote . key               = activeIdentity . key;


          return outputEvents . try_push ( & outputEvents, & outputNote . header );


          }


void EventTranslator::retryPendingNoteOffs (
          clap_output_events const & outputEvents,
          TranslationStatistics    & statistics ) noexcept {


          std::size_t index;


          for ( index = 0U; index < activeNotes_ . size ( ); ++ index ) {


                    ActiveNote & note = activeNotes_[index];


                    if ( ( note . active == false ) ||
                         ( note . releasePending == false ) ) {


                              continue;


                              }


                    note . pendingNoteOff . header . time = 0U;


                    if ( emitNoteOff ( outputEvents,
                                       note . pendingNoteOff,
                                       note . identity,
                                       note . memberChannel ) == false ) {


                              ++ statistics . outputFailures;


                              }
                    else {


                              note . active         = false;
                              note . releasePending = false;


                              }


                    }


          }


void EventTranslator::releaseNote ( clap_output_events const & outputEvents,
                                    clap_event_note const    & noteEvent,
                                    ActiveNote               & activeNote,
                                    TranslationStatistics    & statistics ) noexcept {


          activeNote . pendingNoteOff = noteEvent;


          if ( emitNoteOff ( outputEvents,
                             noteEvent,
                             activeNote . identity,
                             activeNote . memberChannel ) == false ) {


                    ++ statistics . outputFailures;


                    activeNote . releasePending = true;


                    return;


                    }


          activeNote . active         = false;
          activeNote . releasePending = false;


          }


void EventTranslator::processNoteOn ( clap_input_events const  & inputEvents,
                                      clap_output_events const & outputEvents,
                                      std::uint32_t              eventCount,
                                      std::uint32_t              eventIndex,
                                      clap_event_note const    & noteEvent,
                                      TranslationStatistics    & statistics ) noexcept {


          ActiveNote * activeNote;
          clap_event_note_expression const * initialTuning;


          if ( ( noteEvent . port_index < 0 ) ||
               ( noteEvent . channel < 0 ) ||
               ( noteEvent . key < 0 ) ||
               ( noteEvent . key > 127 ) ) {


                    return;


                    }


          activeNote = allocateNote ( noteEvent );


          if ( activeNote == nullptr ) {


                    ++ statistics . channelExhaustions;


                    return;


                    }


          initialTuning = findInitialTuning (
                    inputEvents, eventCount, eventIndex, noteEvent );


          if ( initialTuning != nullptr ) {


                    activeNote -> bend = pitchOffsetToBend ( initialTuning -> value );


                    }


          if ( emitPitchBend ( outputEvents,
                               noteEvent . header . time,
                               activeNote -> memberChannel,
                               activeNote -> bend ) == false ) {


                    ++ statistics . outputFailures;


                    activeNote -> active = false;


                    return;


                    }


          if ( emitNoteOn ( outputEvents, noteEvent, activeNote -> memberChannel ) == false ) {


                    ++ statistics . outputFailures;


                    activeNote -> active = false;


                    }


          }


void EventTranslator::processNoteOff ( clap_output_events const & outputEvents,
                                       clap_event_note const    & noteEvent,
                                       TranslationStatistics    & statistics ) noexcept {


          ActiveNote * activeNote = findOldestMatchingNote ( noteEvent );


          if ( activeNote == nullptr ) {


                    ++ statistics . unmatchedNoteOffs;


                    return;


                    }


          releaseNote ( outputEvents, noteEvent, * activeNote, statistics );


          }


void EventTranslator::processNoteChoke (
          clap_output_events const & outputEvents,
          clap_event_note const    & noteEvent,
          TranslationStatistics    & statistics ) noexcept {


          bool        matched = false;
          std::size_t index;


          for ( index = 0U; index < activeNotes_ . size ( ); ++ index ) {


                    ActiveNote & note = activeNotes_[index];


                    if ( ( note . active == true ) &&
                         identityMatches ( note . identity,
                                           noteEvent . note_id,
                                           noteEvent . port_index,
                                           noteEvent . channel,
                                           noteEvent . key ) ) {


                              matched = true;


                              releaseNote ( outputEvents, noteEvent, note, statistics );


                              }


                    }


          if ( matched == false ) {


                    ++ statistics . unmatchedNoteOffs;


                    }


          }


void EventTranslator::processTuningExpression (
          clap_output_events const          & outputEvents,
          clap_event_note_expression const & expressionEvent,
          TranslationStatistics            & statistics ) noexcept {


          std::uint16_t const bend = pitchOffsetToBend ( expressionEvent . value );
          std::size_t         index;


          for ( index = 0U; index < activeNotes_ . size ( ); ++ index ) {


                    ActiveNote & note = activeNotes_[index];


                    if ( ( note . active == true ) &&
                         ( note . releasePending == false ) &&
                         identityMatches ( note . identity,
                                           expressionEvent . note_id,
                                           expressionEvent . port_index,
                                           expressionEvent . channel,
                                           expressionEvent . key ) ) {


                              bool const appliedBeforeNoteOn =
                                        ( note . processSequence == processSequence_ ) &&
                                        ( note . startTime == expressionEvent . header . time );


                              if ( ( appliedBeforeNoteOn == false ) &&
                                   ( note . bend != bend ) ) {


                                        if ( emitPitchBend ( outputEvents,
                                                            expressionEvent . header . time,
                                                            note . memberChannel,
                                                            bend ) == false ) {


                                                  ++ statistics . outputFailures;


                                                  }
                                        else {


                                                  note . bend = bend;


                                                  }


                                        }


                              }


                    }


          }


} /* namespace pitch_expr_to_pitch_bend */
