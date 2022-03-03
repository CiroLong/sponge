#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&.../* unused */) {}

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity)
    : _output(capacity), _capacity(capacity), _eof(false), _assembled_bytes(0), _stored_bytes(0), _stored_segements() {}

//! \returns the bytes merged or  0 for no bytes merged  or -1 for can't merge
//! merge elm2 to elm1
size_t StreamReassembler::mergeSubstrings(Segement &elm1, const Segement &elm2) {
    Segement x, y;
    if (elm1.index > elm2.index) {
        x = elm2;
        y = elm1;
    } else {
        x = elm1;
        y = elm2;
    }
    if (x.index + x.length < y.index) {
        return -1;  // no intersection, couldn't merge
    } else if (x.index + x.length >= y.index + y.length) {
        elm1 = x;
        return y.length;
    } else {
        elm1.index = x.index;
        elm1.data = x.data + y.data.substr(x.index + x.length - y.index);
        elm1.length = elm1.data.length();
        return x.index + x.length - y.index;
    }
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    if (index >= _capacity + _output.bytes_read())  //!< 溢出，直接丢弃
        return;

    Segement tmp(data, index);
    if (index + tmp.length > _capacity + _output.bytes_read()) {  //!< 丢弃溢出部分
        tmp.data.resize(_capacity + _output.bytes_read() - index);
        tmp.length = tmp.data.size();
    } else
        _eof |= eof;  //!< 无溢出说明有可能是结束

    if (index + tmp.length <= _assembled_bytes) {  //!< 已经被重组，则判断是否结束读取
        if (empty())
            _output.end_input();
        return;
    } else if (index < _assembled_bytes) {
        size_t offset = _assembled_bytes - index;
        tmp.data.assign(tmp.data.begin() + offset, tmp.data.end());
        tmp.index = index + offset;
        tmp.length = tmp.data.size();
    }
    _stored_bytes += tmp.length;

    //! Calculate stored bytes (merge substrings)
    do {
        long merged_bytes = 0;
        auto iter = _stored_segements.lower_bound(tmp);
        while (iter != _stored_segements.end() && (merged_bytes = mergeSubstrings(tmp, *iter)) >= 0) {
            _stored_bytes -= merged_bytes;
            _stored_segements.erase(iter);
            iter = _stored_segements.lower_bound(tmp);
        }
        // merge prev
        if (iter == _stored_segements.begin()) {
            break;
        }
        iter--;
        while ((merged_bytes = mergeSubstrings(tmp, *iter)) >= 0) {
            _stored_bytes -= merged_bytes;
            _stored_segements.erase(iter);
            iter = _stored_segements.lower_bound(tmp);
            if (iter == _stored_segements.begin()) {
                break;
            }
            iter--;
        }
    } while (false);
    _stored_segements.insert(tmp);

    //! write to _output
    if (!_stored_segements.empty() && _stored_segements.begin()->index == _assembled_bytes) {
        const Segement head_block = *_stored_segements.begin();
        // modify _head_index and _unassembled_byte according to successful write to _output
        size_t write_bytes = _output.write(head_block.data);
        _assembled_bytes += write_bytes;
        _stored_segements.erase(_stored_segements.begin());
    }  // 由于buffer的大小也是capacity,所以不会溢出

    if (_eof && empty()) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _stored_bytes - _assembled_bytes; }

bool StreamReassembler::empty() const { return _stored_bytes == _assembled_bytes && _eof; }

size_t StreamReassembler::assembled_bytes() const { return _assembled_bytes; }

bool StreamReassembler::input_ended() { return _output.input_ended(); }