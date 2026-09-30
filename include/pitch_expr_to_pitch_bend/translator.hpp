/* Copyright 2026 Jakub Stulir */

#ifndef PITCH_EXPR_TO_PITCH_BEND_TRANSLATOR_HPP
#define PITCH_EXPR_TO_PITCH_BEND_TRANSLATOR_HPP


#include <clap/clap.h>

#include <array>
#include <cstdint>


namespace pitch_expr_to_pitch_bend {


constexpr std::uint32_t kMpeMemberChannelCount = 15U;
constexpr std::int16_t  kFirstMpeMemberChannel = 1;
constexpr std::int16_t  kLastMpeMemberChannel  = 15;
constexpr double        kPitchBendRangeSemitones = 24.0;
constexpr std::uint16_t kPitchBendCenter = 8192U;


struct TranslationStatistics {


          std::uint32_t outputFailures;
          std::uint32_t channelExhaustions;
          std::uint32_t unmatchedNoteOffs;


          };


class EventTranslator final {


public:


          EventTranslator ( ) noexcept;


          void reset ( ) noexcept;


          TranslationStatistics process ( clap_input_events const  * inputEvents,
                                          clap_output_events const * outputEvents ) noexcept;


          static std::uint16_t pitchOffsetToBend ( double pitchOffsetSemitones ) noexcept;


private:


          struct NoteIdentity {


                    std::int32_t noteId;
                    std::int16_t portIndex;
                    std::int16_t channel;
                    std::int16_t key;


                    };


          struct ActiveNote {


                    bool          active;
                    NoteIdentity  identity;
                    std::int16_t  memberChannel;
                    std::uint16_t bend;
                    bool          releasePending;
                    clap_event_note pendingNoteOff;
                    std::uint64_t allocationOrder;
                    std::uint64_t processSequence;
                    std::uint32_t startTime;


                    };


          static bool identityMatches ( NoteIdentity const & activeIdentity,
                                        std::int32_t        eventNoteId,
                                        std::int16_t        eventPortIndex,
                                        std::int16_t        eventChannel,
                                        std::int16_t        eventKey ) noexcept;


          ActiveNote * allocateNote ( clap_event_note const & noteEvent ) noexcept;


          ActiveNote * findOldestMatchingNote ( clap_event_note const & noteEvent ) noexcept;


          clap_event_note_expression const * findInitialTuning (
                    clap_input_events const & inputEvents,
                    std::uint32_t             eventCount,
                    std::uint32_t             noteOnIndex,
                    clap_event_note const   & noteEvent ) const noexcept;


          static bool emitMidi ( clap_output_events const & outputEvents,
                                 std::uint32_t              time,
                                 std::uint8_t               status,
                                 std::uint8_t               data1,
                                 std::uint8_t               data2 ) noexcept;


          static bool emitPitchBend ( clap_output_events const & outputEvents,
                                      std::uint32_t              time,
                                      std::int16_t               memberChannel,
                                      std::uint16_t              bend ) noexcept;


          static bool emitNoteOn ( clap_output_events const & outputEvents,
                                   clap_event_note const    & noteEvent,
                                   std::int16_t               memberChannel ) noexcept;


          static bool emitNoteOff ( clap_output_events const & outputEvents,
                                    clap_event_note const    & noteEvent,
                                    NoteIdentity const       & activeIdentity,
                                    std::int16_t               memberChannel ) noexcept;


          void retryPendingNoteOffs ( clap_output_events const & outputEvents,
                                      TranslationStatistics    & statistics ) noexcept;


          void releaseNote ( clap_output_events const & outputEvents,
                             clap_event_note const    & noteEvent,
                             ActiveNote               & activeNote,
                             TranslationStatistics    & statistics ) noexcept;


          void processNoteOn ( clap_input_events const  & inputEvents,
                               clap_output_events const & outputEvents,
                               std::uint32_t              eventCount,
                               std::uint32_t              eventIndex,
                               clap_event_note const    & noteEvent,
                               TranslationStatistics    & statistics ) noexcept;


          void processNoteOff ( clap_output_events const & outputEvents,
                                clap_event_note const    & noteEvent,
                                TranslationStatistics    & statistics ) noexcept;


          void processNoteChoke ( clap_output_events const & outputEvents,
                                  clap_event_note const    & noteEvent,
                                  TranslationStatistics    & statistics ) noexcept;


          void processTuningExpression (
                    clap_output_events const          & outputEvents,
                    clap_event_note_expression const & expressionEvent,
                    TranslationStatistics            & statistics ) noexcept;


          std::array<ActiveNote, kMpeMemberChannelCount> activeNotes_;
          std::uint64_t                                  nextAllocationOrder_;
          std::uint64_t                                  processSequence_;


          };


} /* namespace pitch_expr_to_pitch_bend */


#endif /* PITCH_EXPR_TO_PITCH_BEND_TRANSLATOR_HPP */
