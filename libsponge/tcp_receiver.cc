#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    uint64_t length;  //!< 报文长

    //! abs_seqno一直在动态变化，以保证计算准确

    if (seg.header().syn) {  //第一次握手
        if (_syn_flag)
            return;
        _syn_flag = true;                       //!< 设置syn标记
        _isn = seg.header().seqno.raw_value();  //!< 设置isn
        _recived_bytes = 1;                     //!< 收到字节数
        abs_seqno = 1;                          //!< 绝对序列号置为1

        length = seg.length_in_sequence_space() - 1;
        // seg.length_in_sequence_space() 这个函数返回了报文长 加上 syn 或者 fin 的长

        if (length == 0) {  //!< 只有syn号， 直接返回
            return;
        }
    } else if (!_syn_flag) {  //!< 如果之前没有syn标记，抛弃报文段.
        return;
    } else {  //!< 非syn， 计算绝对序列号， 并得到长度
        abs_seqno = unwrap(WrappingInt32(seg.header().seqno.raw_value()), WrappingInt32(_isn), abs_seqno);
        length = seg.length_in_sequence_space();
    }

    if (seg.header().fin) {
        if (_fin_flag)
            return;
        _fin_flag = true;
    }
    // not FIN and not one size's SYN, check border
    else if (seg.length_in_sequence_space() == 0 && abs_seqno == _recived_bytes) {  //!< 空
        return;
    } else if (abs_seqno >= _recived_bytes + window_size() || abs_seqno + length <= _recived_bytes) {  //!< 超出窗口
        return;
    }

    //!< abs_seqno - 1是去除syn后的序列号 stream index
    _reassembler.push_substring(seg.payload().copy(), abs_seqno - 1, seg.header().fin);
    _recived_bytes = _reassembler.assembled_bytes() + 1;  //!< syn count a byte
    if (_reassembler.input_ended())                       //!< FIN be count as one byte
        _recived_bytes++;
    return;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if (_recived_bytes > 0)
        return WrappingInt32(wrap(_recived_bytes, WrappingInt32(_isn)));
    else
        return std::nullopt;
}

size_t TCPReceiver::window_size() const { return _capacity - _reassembler.stream_out().buffer_size(); }
