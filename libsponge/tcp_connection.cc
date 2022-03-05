#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight(); }

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_segment_received; }

bool TCPConnection::active() const { return _active; }

void TCPConnection::segment_received(const TCPSegment &seg) {
    if (!_active)
        return;
    _time_since_last_segment_received = 0;  //!< 重置计时器

    // 在SYN_SENT 时， 带有ack号并且有负载的报文被丢弃
    // 只有带有SYN和ackno号的不带负载的报文被接受
    if (SYN_sent() && seg.header().ack && seg.payload().size() > 0) {
        return;
    }

    bool send_empty = false;
    if (_sender.next_seqno_absolute() > 0 && seg.header().ack) {
        // unacceptable ACKs should produced a segment that existed
        if (!_sender.ack_received(seg.header().ackno, seg.header().win)) {
            send_empty = true;
        }
    }
    //
    _receiver.segment_received(seg);
    //

    if (seg.header().syn && CLOSED()) {  //当local为closed时，返回syn报文，即进行第二次握手, 并直接返回
        connect();
        return;
    }

    if (seg.header().rst) {
        // RST segements without ACKs should be ignored in SYN_SENT
        if (SYN_sent() && !seg.header().ack) {
            return;
        }
        unclean_shutdown(false);  // 接受rst
        return;
    }

    if (seg.length_in_sequence_space() > 0) {
        send_empty = true;
    }
    if (send_empty) {
        if (_receiver.ackno().has_value() && _sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
    }

    push_segement_out();
}

size_t TCPConnection::write(const string &data) {
    size_t wsize = _sender.stream_in().write(data);
    push_segement_out();
    return wsize;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    if (!_active)
        return;
    _time_since_last_segment_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS) {
        unclean_shutdown(true);
        // 由于超时，故需要发送rst
    }
    push_segement_out();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    push_segement_out();
}

void TCPConnection::connect() {
    // 链接时发送一个具有SYN标记的报文
    push_segement_out(true);
}

void TCPConnection::push_segement_out(bool send_syn) {
    if (CLOSED() && !send_syn)
        return;             // waiting for stream to begin (no SYN sent)
    _sender.fill_window();  // 第一次调用时会发送SYN
    TCPSegment seg;
    while (!_sender.segments_out().empty()) {
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {  // 在发送报文前查看_receiver中的ackno号，如果有，附带ackno和windowsize
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        if (_need_send_rst) {
            _need_send_rst = false;
            seg.header().rst = true;
        }
        _segments_out.push(seg);
    }
    clean_shutdown();
    return;
}

// about receiver's state
bool TCPConnection::in_listen() { return !_receiver.ackno().has_value(); }
bool TCPConnection::SYN_recv() { return _receiver.ackno().has_value() && !_receiver.stream_out().input_ended(); }
bool TCPConnection::FIN_recv() { return _receiver.stream_out().input_ended(); }

// about sender's state
bool TCPConnection::CLOSED() { return _sender.next_seqno_absolute() == 0; }
bool TCPConnection::SYN_sent() {
    return _sender.next_seqno_absolute() > 0 && _sender.next_seqno_absolute() == _sender.bytes_in_flight();
}
bool TCPConnection::FIN_sent() {
    return _sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
           _sender.bytes_in_flight() > 0;
}
bool TCPConnection::FIN_acked() {
    return _sender.stream_in().eof() && _sender.next_seqno_absolute() == _sender.stream_in().bytes_written() + 2 &&
           _sender.bytes_in_flight() == 0;
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";
            unclean_shutdown(true);
            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}

bool TCPConnection::clean_shutdown() {
    // 不延迟的情况
    // 对方的数据全部传输， local 的数据还没有结束输入
    if (_receiver.stream_out().input_ended() && !(_sender.stream_in().eof())) {
        _linger_after_streams_finish = false;
    }

    // 1.receiver 全部重组结束并且接收到对方的fin
    // 2.sender 的输入被应用关闭,并且全部发出，包括FIN
    // 3.sender 发送的报文全部被确认
    if (_receiver.stream_out().input_ended() && _sender.stream_in().eof() && _sender.bytes_in_flight() == 0) {
        // 4.不延迟 或者 超时10*timeout，则关闭链接
        if (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) {
            _active = false;
        }
    }
    return !_active;
}
void TCPConnection::unclean_shutdown(bool send_rst) {
    if (send_rst) {
        _need_send_rst = true;
        if (_sender.segments_out().empty()) {
            _sender.send_empty_segment();
        }
        push_segement_out();
    }
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;
}