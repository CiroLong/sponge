#ifndef SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
#define SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH

#include "byte_stream.hh"

#include <cstdint>
#include <list>
#include <map>
#include <set>
#include <string>
#include <utility>

//! \brief A class that assembles a series of excerpts from a byte stream (possibly out of order,
//! possibly overlapping) into an in-order byte stream.
class StreamReassembler {
  public:
    // Your code here -- add private members as necessary.
    // 注意到private members always start by '_'
    struct Segement {
        std::string data;
        size_t index;
        size_t length;
        bool operator<(const Segement &s) const { return this->index < s.index; }
        Segement() : data(), index(0), length(0) {}
        Segement(std::string s, size_t a) : data(s), index(a), length(s.size()) {}
    };

    ByteStream _output;  //!< The reassembled in-order byte stream
    size_t _capacity;    //!< The maximum number of bytes

    bool _eof;                //!< 标志
    size_t _assembled_bytes;  //!< 已经重组好的字节数 _assembled_bytes or _first_unassemble_byte
    size_t _stored_bytes;     //!< 存储的字节数? = _output.size() + set中的串长

    std::set<Segement> _stored_segements;  //!< 使用set, 由于底层为红黑树首先，又重载了 < 运算符， 故Segement在set内有序

    //! \returns the bytes merged or 0 for no bytes merged
    //! merge elm2 to elm1
    size_t mergeSubstrings(Segement &elm1, const Segement &elm2);

  public:
    //! \brief Construct a `StreamReassembler` that will store up to `capacity` bytes.
    //! \note This capacity limits both the bytes that have been reassembled,
    //! and those that have not yet been reassembled.
    StreamReassembler(const size_t capacity);

    //! \brief Receive a substring and write any newly contiguous bytes into the stream.
    //!
    //! The StreamReassembler will stay within the memory limits of the `capacity`.
    //! Bytes that would exceed the capacity are silently discarded.
    //!
    //! \param data the substring
    //! \param index indicates the index (place in sequence) of the first byte in `data`
    //! \param eof the last byte of `data` will be the last byte in the entire stream
    void push_substring(const std::string &data, const uint64_t index, const bool eof);

    //! \name Access the reassembled byte stream
    //!@{
    const ByteStream &stream_out() const { return _output; }
    ByteStream &stream_out() { return _output; }
    //!@}

    //! The number of bytes in the substrings stored but not yet reassembled
    //!
    //! \note If the byte at a particular index has been pushed more than once, it
    //! should only be counted once for the purpose of this function.
    size_t unassembled_bytes() const;

    //! \brief Is the internal state empty (other than the output stream)?
    //! \returns `true` if no substrings are waiting to be assembled
    bool empty() const;

    size_t assembled_bytes() const;
    bool input_ended();
};

#endif  // SPONGE_LIBSPONGE_STREAM_REASSEMBLER_HH
