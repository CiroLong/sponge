#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    assert(!_stream.error());
    TCPSegment syn_seg;
    // sent a SYN before sent other segment
    if (_next_seqno == 0) {  // no segment sent yet
        syn_seg.header().syn = true;
        _syn_flag = true;
        send_segment(syn_seg);
        return;
    } else if (_next_seqno == _bytes_in_flight) {
        // the SYN seg is flying
        return;
    }
    // take window_size as 1 when it equal 0
    size_t window = _window_size;
    if (window == 0) {
        _window_zero_flag = true;
        window = 1;  //<! 零窗口探测
    } else {
        _window_zero_flag = false;
    }

    size_t remain_space;  //<! 窗口的可用空间.

    // when window isn't full and never sent FIN
    while ((remain_space = window - (_next_seqno - _ackno_recv)) > 0 && !_fin_flag) {
        size_t size = min(TCPConfig::MAX_PAYLOAD_SIZE, remain_space);
        TCPSegment seg;
        string str = _stream.read(size);  // read size

        seg.payload() = Buffer(std::move(str));  // 这里有一个小bug不知道怎么回事,一定要用Buffer

        // Add fin,   (捎带)
        if (_stream.eof() && seg.length_in_sequence_space() < window) {
            seg.header().fin = true;
            _fin_flag = true;
        }
        if (seg.length_in_sequence_space() == 0) {  //空报文
            return;
        }
        send_segment(seg);
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t abs_ackno = unwrap(ackno, _isn, _ackno_recv);

    //未发送或者，已经确认过的序列号，直接返回.
    if (abs_ackno > _next_seqno) {
        return;
    }
    _window_size = window_size;  // 记录窗口大小。
    if (abs_ackno <= _ackno_recv) {
        return;
    }
    _ackno_recv = abs_ackno;

    // 确认报文
    while (!_segments_outstanding.empty()) {
        TCPSegment seg = _segments_outstanding.front();
        if (unwrap(seg.header().seqno, _isn, _next_seqno) + seg.length_in_sequence_space() <= abs_ackno) {
            _bytes_in_flight -= seg.length_in_sequence_space();
            _segments_outstanding.pop();
        } else {
            break;
        }
    }

    fill_window();

    _retransmission_timeout = _initial_retransmission_timeout;  //将RTO初始化
    _consecutive_retransmissions = 0;                           //重传计时器设置为0

    // 如果仍有报文未确认，重启计时器.
    if (!_segments_outstanding.empty()) {
        _timer_running = true;
        _timer = 0;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _timer += ms_since_last_tick;
    if (_timer >= _retransmission_timeout && !_segments_outstanding.empty()) {
        _segments_out.push(_segments_outstanding.front());  //!< 快重传
        if (!_window_zero_flag) {                           //!< 用于解决test 17 的bug
            _consecutive_retransmissions += 1;              //!< 重传计时
            _retransmission_timeout *= 2;                   //!< RTO 翻倍.
        }
        _timer_running = true;
        _timer = 0;
    }
    if (_segments_outstanding.empty()) {  //!< 所有报文被确认，停止计时器
        _timer_running = false;
        _timer = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmissions; }

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    //空报文，不导致 _next_seqno 和 _bytes_in_flight 变化
    _segments_out.push(seg);
}

void TCPSender::send_segment(TCPSegment &seg) {
    seg.header().seqno = wrap(_next_seqno, _isn);
    _next_seqno += seg.length_in_sequence_space();       // _next_seqno增加
    _bytes_in_flight += seg.length_in_sequence_space();  // _bytes_in_flight增加
    _segments_outstanding.push(seg);
    _segments_out.push(seg);  // 直接push到_segments_out中就可以
    if (!_timer_running) {    // start timer
        _timer_running = true;
        _timer = 0;
    }
}