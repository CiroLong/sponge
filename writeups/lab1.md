Lab 1 Writeup
=============

My name: [CiroLong]

My SUNet ID:HUST

I collaborated with: [list sunetids here]

I would like to thank/reward these classmates for their help: [list sunetids here]

This lab took me about [n] hours to do. I [did/did not] attend the lab session.

Program Structure and Design of the StreamReassembler:

```
std::set<>
struct Segement {
        std::string data;
        size_t index;
        size_t length;
        bool operator<(const Segement &s) const { return this->index < s.index; }
        Segement() : data(), index(0), length(0) {}
        Segement(std::string s, size_t a) : data(s), index(a), length(s.size()) {}
    };
```



Implementation Challenges:
[]

Remaining Bugs:
[]

- Optional: I had unexpected difficulty with: [describe]

- Optional: I think you could make this lab better by: [describe]

- Optional: I was surprised by: [describe]

- Optional: I'm not sure about: [describe]

## ==~~算了，还是写中文~~==

emm算是写了很久，第一版应该就能过，但是一开始忘记make了，然后一直fail

烦

后面试了直接copy才发现问题

现在是2021/11/17 15：22

写完啦
