#ifndef SPONGE_LIBSPONGE_TCP_SENDER_HH
#define SPONGE_LIBSPONGE_TCP_SENDER_HH

#include "byte_stream.hh"
#include "tcp_config.hh"
#include "tcp_segment.hh"
#include "wrapping_integers.hh"

#include <cassert>
#include <cmath>
#include <functional>
#include <queue>

//! \brief The "sender" part of a TCP implementation.

//! Accepts a ByteStream, divides it up into segments and sends the
//! segments, keeps track of which segments are still in-flight,
//! maintains the Retransmission Timer, and retransmits in-flight
//! segments if the retransmission timer expires.
class TCPSender {
  private:
    //! our initial sequence number, the number for our SYN.
    WrappingInt32 _isn;

    //! outbound queue of segments that the TCPSender wants sent
    std::queue<TCPSegment> _segments_out{};
    //! 维护已经发送的报文端，用于确认是否需要再发送.
    std::queue<TCPSegment> _segments_outstanding{};

    //! retransmission timer for the connection
    //! RTO
    //! 连接的重传定时器
    unsigned int _initial_retransmission_timeout;  //!< 初始值
    unsigned int _retransmission_timeout;          //!< 初始值和上面相同,超时后翻倍
    //! 需要的计时器
    size_t _timer{0};
    bool _timer_running{false};

    //! outgoing stream of bytes that have not yet been sent
    //! 尚未发送的传出字节流
    ByteStream _stream;

    //! the (absolute) sequence number for the next byte to be sent
    //! 要发送的下一个字节的（绝对）序列号
    uint64_t _next_seqno{0};

    //! window_size
    size_t _window_size{1};  //!< 有一个试探窗口的问题
    bool _window_zero_flag{false};

    //! SYN和FIN发送的标志
    bool _syn_flag{false};
    bool _fin_flag{false};

    //! 连续发生的重传次数
    size_t _consecutive_retransmissions{0};

    //! 发送但未被确认的字节
    size_t _bytes_in_flight{0};

    //! 收到的ackno
    size_t _ackno_recv{0};

    //! 发送segement的接口
    void send_segment(TCPSegment &seg);

  public:
    //! Initialize a TCPSender
    TCPSender(const size_t capacity = TCPConfig::DEFAULT_CAPACITY,
              const uint16_t retx_timeout = TCPConfig::TIMEOUT_DFLT,
              const std::optional<WrappingInt32> fixed_isn = {});

    //! \name "Input" interface for the writer
    //!@{
    ByteStream &stream_in() { return _stream; }
    const ByteStream &stream_in() const { return _stream; }
    //!@}

    //! \name Methods that can cause the TCPSender to send a segment
    //!@{

    //! \brief A new acknowledgment was received
    bool ack_received(const WrappingInt32 ackno, const uint16_t window_size);

    //! \brief Generate an empty-payload segment (useful for creating empty ACK segments)
    void send_empty_segment();

    //! \brief create and send segments to fill as much of the window as possible
    void fill_window();

    //! \brief Notifies the TCPSender of the passage of time
    void tick(const size_t ms_since_last_tick);
    //!@}

    //! \name Accessors
    //!@{

    //! \brief How many sequence numbers are occupied by segments sent but not yet acknowledged?
    //! \note count is in "sequence space," i.e. SYN and FIN each count for one byte
    //! (see TCPSegment::length_in_sequence_space())
    size_t bytes_in_flight() const;

    //! \brief Number of consecutive retransmissions that have occurred in a row
    unsigned int consecutive_retransmissions() const;

    //! \brief TCPSegments that the TCPSender has enqueued for transmission.
    //! \note These must be dequeued and sent by the TCPConnection,
    //! which will need to fill in the fields that are set by the TCPReceiver
    //! (ackno and window size) before sending.
    std::queue<TCPSegment> &segments_out() { return _segments_out; }
    //!@}

    //! \name What is the next sequence number? (used for testing)
    //!@{

    //! \brief absolute seqno for the next byte to be sent
    uint64_t next_seqno_absolute() const { return _next_seqno; }

    //! \brief relative seqno for the next byte to be sent
    WrappingInt32 next_seqno() const { return wrap(_next_seqno, _isn); }
    //!@}
};

#endif  // SPONGE_LIBSPONGE_TCP_SENDER_HH
